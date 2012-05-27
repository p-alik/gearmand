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

#include <config.h>
#include <libgearman-server/common.h>
#include <libgearman-server/gearmand.h>
#include <string.h>

#include <errno.h>
#include <assert.h>
#include <iso646.h>

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_con_private Private Gearmand Connection Functions
 * @ingroup gearmand_con
 * @{
 */

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

  if (dcon->add_fn)
  {
    ret= (*dcon->add_fn)(dcon->server_con);
    if (gearmand_failed(ret))
    {
      gearman_server_con_free(dcon->server_con);

      gearmand_sockfd_close(dcon->fd);

      return ret;
    }
  }

  GEARMAN_LIST_ADD(thread->dcon, dcon,)

  return GEARMAN_SUCCESS;
}

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
    dcon= build_gearmand_con_st(); 
    if (dcon == NULL)
    {
      gearmand_perror("new build_gearmand_con_st");
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
  strncpy(dcon->host, host, NI_MAXHOST);
  dcon->host[NI_MAXHOST -1]= 0;
  strncpy(dcon->port, port, NI_MAXSERV);
  dcon->port[NI_MAXSERV -1]= 0;
  dcon->add_fn= add_fn;

  /* If we are not threaded, just add the connection now. */
  if (gearmand->threads == 0)
  {
    dcon->thread= gearmand->thread_list;
    return _con_add(gearmand->thread_list, dcon);
  }

  /* We do a simple round-robin connection queue algorithm here. */
  if (gearmand->thread_add_next == NULL)
  {
    gearmand->thread_add_next= gearmand->thread_list;
  }

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

    int error;
    if ((error= pthread_mutex_lock(&(dcon->thread->lock))) != 0)
    {
      errno= error;
      gearmand_perror("pthread_mutex_lock");
      gearmand_fatal("Lock could not be taken on dcon->thread->, shutdown to occur");
      gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN);
    }

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
    {
      gearmand_thread_wakeup(dcon->thread, GEARMAND_WAKEUP_CON);
    }

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
    gearman_server_con_attempt_free(dcon->server_con);
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
      if ((error=  pthread_mutex_lock(&(dcon->thread->lock))) == 0)
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
    destroy_gearmand_con_st(dcon);
  }
}

void gearmand_con_check_queue(gearmand_thread_st *thread)
{
  /* Dirty check is fine here, wakeup is always sent after add completes. */
  if (thread->dcon_add_count == 0)
  {
    return;
  }

  /* We want to add new connections inside the lock because other threads may
     walk the thread's dcon_list while holding the lock. */
  while (thread->dcon_add_list != NULL)
  {
    int error;
    if ((error= pthread_mutex_lock(&(thread->lock))) == 0)
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
