/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Connection Definitions
 */

#include "common.h"
#include "gearmand.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_con_private Private Gearmand Connection Functions
 * @ingroup gearmand_con
 * @{
 */

static void _con_ready(int fd, short events, void *arg);

static gearmand_error_t _con_add(gearmand_thread_st *thread,
                                 gearmand_con_st *con);

/** @} */

/*
 * Public definitions
 */

gearmand_error_t gearmand_con_create(gearmand_st *gearmand, int fd,
                                     const char *host, const char *port,
                                     gearmand_connection_add_fn *add_fn)
{
  gearmand_con_st *dcon;

  if (gearmand->free_dcon_count > 0)
  {
    dcon= gearmand->free_dcon_list;
    GEARMAN_LIST_DEL(gearmand->free_dcon, dcon,)
  }
  else
  {
    dcon= (gearmand_con_st *)malloc(sizeof(gearmand_con_st));
    if (dcon == NULL)
    {
      gearmand_perror("malloc");
      gearmand_sockfd_close(fd);

      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    memset(&dcon->event, 0, sizeof(struct event));
  }

  dcon->last_events= 0;
  dcon->fd= fd;
  dcon->next= NULL;
  dcon->prev= NULL;
  dcon->server_con= NULL;
  dcon->add_fn= NULL;
  strncpy(dcon->host, host, NI_MAXHOST - 1);
  strncpy(dcon->port, port, NI_MAXSERV - 1);
  dcon->add_fn= add_fn;

  /* If we are not threaded, just add the connection now. */
  if (gearmand->threads == 0)
  {
    dcon->thread= gearmand->thread_list;
    return _con_add(gearmand->thread_list, dcon);
  }

  /* We do a simple round-robin connection queue algorithm here. */
  if (gearmand->thread_add_next == NULL)
    gearmand->thread_add_next= gearmand->thread_list;

  dcon->thread= gearmand->thread_add_next;

  /* We don't need to lock if the list is empty. */
  if (dcon->thread->dcon_add_count == 0 &&
      dcon->thread->free_dcon_count < gearmand->max_thread_free_dcon_count)
  {
    GEARMAN_LIST_ADD(dcon->thread->dcon_add, dcon,)
    gearmand_thread_wakeup(dcon->thread, GEARMAND_WAKEUP_CON);
  }
  else
  {
    uint32_t free_dcon_count;
    gearmand_con_st *free_dcon_list;

    (void ) pthread_mutex_lock(&(dcon->thread->lock));

    GEARMAN_LIST_ADD(dcon->thread->dcon_add, dcon,)

    /* Take the free connection structures back to reuse. */
    free_dcon_list= dcon->thread->free_dcon_list;
    free_dcon_count= dcon->thread->free_dcon_count;
    dcon->thread->free_dcon_list= NULL;
    dcon->thread->free_dcon_count= 0;

    (void ) pthread_mutex_unlock(&(dcon->thread->lock));

    /* Only wakeup the thread if this is the first in the queue. We don't need
       to lock around the count check, worst case it was already picked up and
       we send an extra byte. */
    if (dcon->thread->dcon_add_count == 1)
      gearmand_thread_wakeup(dcon->thread, GEARMAND_WAKEUP_CON);

    /* Put the free connection structures we grabbed on the main list. */
    while (free_dcon_list != NULL)
    {
      dcon= free_dcon_list;
      GEARMAN_LIST_DEL(free_dcon, dcon,)
      GEARMAN_LIST_ADD(gearmand->free_dcon, dcon,)
    }
  }

  gearmand->thread_add_next= gearmand->thread_add_next->next;

  return GEARMAN_SUCCESS;
}

void gearmand_con_free(gearmand_con_st *dcon)
{
  if (event_initialized(&(dcon->event)))
  {
    if (event_del(&(dcon->event)) < 0)
    {
      gearmand_perror("event_del");
    }
    else
    {
      /* This gets around a libevent bug when both POLLIN and POLLOUT are set. */
      event_set(&(dcon->event), dcon->fd, EV_READ, _con_ready, dcon);
      event_base_set(dcon->thread->base, &(dcon->event));

      if (event_add(&(dcon->event), NULL) < 0)
      {
        gearmand_perror("event_add");
      }
      else
      {
        if (event_del(&(dcon->event)) < 0)
        {
          gearmand_perror("event_del");
        }
      }
    }
  }

  // @note server_con could be null if we failed to complete the initial
  // connection.
  if (dcon->server_con)
  {
    gearman_server_con_free(dcon->server_con);
  }

  GEARMAN_LIST_DEL(dcon->thread->dcon, dcon,)

  gearmand_sockfd_close(dcon->fd);

  if (Gearmand()->free_dcon_count < GEARMAN_MAX_FREE_SERVER_CON)
  {
    if (Gearmand()->threads == 0)
    {
      GEARMAN_LIST_ADD(Gearmand()->free_dcon, dcon,)
    }
    else
    {
      /* Lock here because the main thread may be emptying this. */
      int error;
      if (! (error=  pthread_mutex_lock(&(dcon->thread->lock))))
      {
        GEARMAN_LIST_ADD(dcon->thread->free_dcon, dcon,);
        (void ) pthread_mutex_unlock(&(dcon->thread->lock));
      }
      else
      {
        errno= error;
        gearmand_perror("pthread_mutex_lock");
      }
    }
  }
  else
  {
    gearmand_crazy("free");
    free(dcon);
  }
}

void gearmand_con_check_queue(gearmand_thread_st *thread)
{
  /* Dirty check is fine here, wakeup is always sent after add completes. */
  if (thread->dcon_add_count == 0)
    return;

  /* We want to add new connections inside the lock because other threads may
     walk the thread's dcon_list while holding the lock. */
  while (thread->dcon_add_list != NULL)
  {
    int error;
    if (! (error= pthread_mutex_lock(&(thread->lock))))
    {
      gearmand_con_st *dcon= thread->dcon_add_list;
      GEARMAN_LIST_DEL(thread->dcon_add, dcon,);

      if ((error= pthread_mutex_unlock(&(thread->lock))) != 0)
      {
        errno= error;
        gearmand_perror("pthread_mutex_unlock");
        gearmand_fatal("Error in locking forcing a shutdown");
        gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN);
      }

      gearmand_error_t rc;
      if ((rc= _con_add(thread, dcon)) != GEARMAN_SUCCESS)
      {
	gearmand_gerror("_con_add() has failed, please report any crashes that occur immediatly after this.", rc);
        gearmand_con_free(dcon);
      }
    }
    else
    {
      errno= error;
      gearmand_perror("pthread_mutex_lock");
      gearmand_fatal("Lock could not be taken on thread->, shutdown to occur");
      gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN);
    }
  }
}

gearmand_error_t gearmand_connection_watch(gearmand_io_st *con, short events,
                                           void *context __attribute__ ((unused)))
{
  (void) context;
  gearmand_con_st *dcon;
  short set_events= 0;

  dcon= (gearmand_con_st *)gearman_io_context(con);

  if (events & POLLIN)
    set_events|= EV_READ;
  if (events & POLLOUT)
    set_events|= EV_WRITE;

  if (dcon->last_events != set_events)
  {
    if (dcon->last_events != 0)
    {
      if (event_del(&(dcon->event)) < 0)
      {
        gearmand_perror("event_del");
        assert(! "event_del");
      }
    }
    event_set(&(dcon->event), dcon->fd, set_events | EV_PERSIST, _con_ready, dcon);
    event_base_set(dcon->thread->base, &(dcon->event));

    if (event_add(&(dcon->event), NULL) < 0)
    {
      gearmand_perror("event_add");
      return GEARMAN_EVENT;
    }

    dcon->last_events= set_events;
  }

  gearmand_log_crazy("%15s:%5s Watching  %6s %s",
                     dcon->host, dcon->port,
                     events & POLLIN ? "POLLIN" : "",
                     events & POLLOUT ? "POLLOUT" : "");

  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static void _con_ready(int fd __attribute__ ((unused)), short events,
                       void *arg)
{
  gearmand_con_st *dcon= (gearmand_con_st *)arg;
  short revents= 0;

  if (events & EV_READ)
    revents|= POLLIN;
  if (events & EV_WRITE)
    revents|= POLLOUT;

  gearmand_error_t ret;
  ret= gearmand_io_set_revents(dcon->server_con, revents);
  if (ret != GEARMAN_SUCCESS)
  {
    gearmand_gerror("gearmand_io_set_revents", ret);
    gearmand_con_free(dcon);
    return;
  }

  gearmand_log_crazy("%15s:%5s Ready     %6s %s",
                     dcon->host, dcon->port,
                     revents & POLLIN ? "POLLIN" : "",
                     revents & POLLOUT ? "POLLOUT" : "");

  gearmand_thread_run(dcon->thread);
}

static gearmand_error_t _con_add(gearmand_thread_st *thread,
                                 gearmand_con_st *dcon)
{
  gearmand_error_t ret= GEARMAN_SUCCESS;
  dcon->server_con= gearman_server_con_add(&(thread->server_thread), dcon, &ret);

  assert(dcon->server_con || ret != GEARMAN_SUCCESS);
  assert(! dcon->server_con || ret == GEARMAN_SUCCESS);

  if (dcon->server_con == NULL)
  {
    gearmand_sockfd_close(dcon->fd);

    return ret;
  }

  if (dcon->add_fn != NULL)
  {
    ret= (*dcon->add_fn)(dcon->server_con);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_server_con_free(dcon->server_con);

      gearmand_sockfd_close(dcon->fd);

      return ret;
    }
  }

  gearmand_log_info("%15s:%5s Connected", dcon->host, dcon->port);

  GEARMAN_LIST_ADD(thread->dcon, dcon,)

  return GEARMAN_SUCCESS;
}
