/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server Thread Definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_server_private Private Server Functions
 * @ingroup gearman_server
 * @{
 */

/**
 * Try reading packets for a connection.
 */
static gearmand_error_t _thread_packet_read(gearman_server_con_st *con);

/**
 * Flush outgoing packets for a connection.
 */
static gearmand_error_t _thread_packet_flush(gearman_server_con_st *con);

/**
 * Start processing thread for the server.
 */
static gearmand_error_t _proc_thread_start(gearman_server_st *server);

/**
 * Kill processing thread for the server.
 */
static void _proc_thread_kill(gearman_server_st *server);

/**
 * Processing thread.
 */
static void *_proc(void *data);

/** @} */

/*
 * Public definitions
 */

bool gearman_server_thread_init(gearman_server_st *server,
                                gearman_server_thread_st *thread,
                                gearman_log_server_fn *log_function,
                                gearmand_thread_st *context,
                                gearmand_event_watch_fn *event_watch)
{
  assert(server);
  assert(thread);
  if (server->thread_count == 1)
  {
    /* The server is going to be multi-threaded, start processing thread. */
    if (_proc_thread_start(server) != GEARMAN_SUCCESS)
    {
      return false;
    }
  }

  thread->con_count= 0;
  thread->io_count= 0;
  thread->proc_count= 0;
  thread->free_con_count= 0;
  thread->free_packet_count= 0;
  thread->log_fn= log_function;
  thread->log_context= context;
  thread->run_fn= NULL;
  thread->run_fn_arg= NULL;
  thread->con_list= NULL;
  thread->io_list= NULL;
  thread->proc_list= NULL;
  thread->free_con_list= NULL;
  thread->free_packet_list= NULL;

  int error;
  if ((error= pthread_mutex_init(&(thread->lock), NULL)))
  {
    errno= error;
    gearmand_perror("pthread_mutex_init");
    return false;
  }

  GEARMAN_LIST_ADD(server->thread, thread,);

  thread->gearman= &(thread->gearmand_connection_list_static);
  gearmand_connection_list_init(thread->gearman, event_watch, NULL);

  return true;
}

void gearman_server_thread_free(gearman_server_thread_st *thread)
{
  gearman_server_con_st *con;
  gearman_server_packet_st *packet;

  _proc_thread_kill(Server);

  while (thread->con_list != NULL)
  {
    gearman_server_con_free(thread->con_list);
  }

  while (thread->free_con_list != NULL)
  {
    con= thread->free_con_list;
    thread->free_con_list= con->next;
    gearmand_crazy("free");
    free(con);
  }

  while (thread->free_packet_list != NULL)
  {
    packet= thread->free_packet_list;
    thread->free_packet_list= packet->next;
    gearmand_crazy("free");
    free(packet);
  }

  if (thread->gearman != NULL)
    gearman_connection_list_free(thread->gearman);

  pthread_mutex_destroy(&(thread->lock));

  GEARMAN_LIST_DEL(Server->thread, thread,)
}

void gearman_server_thread_set_run(gearman_server_thread_st *thread,
                                   gearman_server_thread_run_fn *run_fn,
                                   void *run_fn_arg)
{
  thread->run_fn= run_fn;
  thread->run_fn_arg= run_fn_arg;
}

gearmand_con_st *
gearman_server_thread_run(gearman_server_thread_st *thread,
                          gearmand_error_t *ret_ptr)
{
  /* If we are multi-threaded, we may have packets to flush or connections that
     should start reading again. */
  if (Server->flags.threaded)
  {
    gearman_server_con_st *server_con;

    while ((server_con= gearman_server_con_io_next(thread)) != NULL)
    {
      if (server_con->is_dead)
      {
        if (server_con->proc_removed)
        {
          gearman_server_con_free(server_con);
        }

        continue;
      }

      if (server_con->ret != GEARMAN_SUCCESS)
      {
        *ret_ptr= server_con->ret;
        return gearman_server_con_data(server_con);
      }

      /* See if any outgoing packets were queued. */
      *ret_ptr= _thread_packet_flush(server_con);
      if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
      {
        return gearman_server_con_data(server_con);
      }
    }
  }

  /* Check for new activity on connections. */
  {
    gearman_server_con_st *server_con;

    while ((server_con= gearmand_ready(thread->gearman)) != NULL)
    {
      /* Try to read new packets. */
      if (server_con->con.revents & POLLIN)
      {
        *ret_ptr= _thread_packet_read(server_con);
        if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
          return gearman_server_con_data(server_con);
      }

      /* Flush existing outgoing packets. */
      if (server_con->con.revents & POLLOUT)
      {
        *ret_ptr= _thread_packet_flush(server_con);
        if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
        {
          return gearman_server_con_data(server_con);
        }
      }
    }
  }

  /* Start flushing new outgoing packets if we are single threaded. */
  if (! (Server->flags.threaded))
  {
    gearman_server_con_st *server_con;
    while ((server_con= gearman_server_con_io_next(thread)) != NULL)
    {
      *ret_ptr= _thread_packet_flush(server_con);
      if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
      {
        return gearman_server_con_data(server_con);
      }
    }
  }

  /* Check for the two shutdown modes. */
  if (Server->shutdown)
  {
    *ret_ptr= GEARMAN_SHUTDOWN;
  }
  else if (Server->shutdown_graceful)
  {
    if (Server->job_count == 0)
    {
      *ret_ptr= GEARMAN_SHUTDOWN;
    }
    else
    {
      *ret_ptr= GEARMAN_SHUTDOWN_GRACEFUL;
    }
  }
  else
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return NULL;
}

/*
 * Private definitions
 */

static gearmand_error_t _thread_packet_read(gearman_server_con_st *con)
{
  gearmand_error_t ret;

  while (1)
  {
    if (con->packet == NULL)
    {
      con->packet= gearman_server_packet_create(con->thread, true);
      if (con->packet == NULL)
      {
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
    }

    ret= gearman_io_recv(con, true);
    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_IO_WAIT)
        break;

      gearman_server_packet_free(con->packet, con->thread, true);
      con->packet= NULL;
      return ret;
    }

    gearmand_log_debug("%15s:%5u Received  %s",
                       con->_host == NULL ? "-" : con->_host,
                       con->_port == NULL ? "-" : con->_port,
                       gearmand_command_info_list[con->packet->packet.command].name);

    /* We read a complete packet. */
    if (Server->flags.threaded)
    {
      /* Multi-threaded, queue for the processing thread to run. */
      gearman_server_proc_packet_add(con, con->packet);
      con->packet= NULL;
    }
    else
    {
      /* Single threaded, run the command here. */
      ret= gearman_server_run_command(con, &(con->packet->packet));
      gearmand_packet_free(&(con->packet->packet));
      gearman_server_packet_free(con->packet, con->thread, true);
      con->packet= NULL;
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }
  }

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _thread_packet_flush(gearman_server_con_st *con)
{
  /* Check to see if we've already tried to avoid excessive system calls. */
  if (con->con.events & POLLOUT)
    return GEARMAN_IO_WAIT;

  while (con->io_packet_list != NULL)
  {
    gearmand_error_t ret;

    ret= gearman_io_send(con, &(con->io_packet_list->packet),
                                 con->io_packet_list->next == NULL ? true : false);
    if (ret != GEARMAN_SUCCESS)
    {
      return ret;
    }

    gearmand_log_debug("%15s:%5d Sent      %s",
                       con->_host == NULL ? "-" : con->_host,
                       con->_port == NULL ? "-" : con->_port,
                       gearmand_command_info_list[con->io_packet_list->packet.command].name);

    gearman_server_io_packet_remove(con);
  }

  /* Clear the POLLOUT flag. */
  return gearmand_io_set_events(con, POLLIN);
}

static gearmand_error_t _proc_thread_start(gearman_server_st *server)
{
  pthread_attr_t attr;

  if (pthread_mutex_init(&(server->proc_lock), NULL))
  {
    gearmand_perror("pthread_mutex_init");
    return GEARMAN_PTHREAD;
  }

  if (pthread_cond_init(&(server->proc_cond), NULL))
  {
    gearmand_perror("pthread_cond_init");
    return GEARMAN_PTHREAD;
  }

  if (pthread_attr_init(&attr))
  {
    gearmand_perror("pthread_attr_init");
    return GEARMAN_PTHREAD;
  }

  if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
  {
    gearmand_perror("pthread_attr_setscope");
    return GEARMAN_PTHREAD;
  }

  if (pthread_create(&(server->proc_id), &attr, _proc, server))
  {
    gearmand_perror("pthread_create");
    return GEARMAN_PTHREAD;
  }

  (void) pthread_attr_destroy(&attr);

  server->flags.threaded= true;

  return GEARMAN_SUCCESS;
}

static void _proc_thread_kill(gearman_server_st *server)
{
  if (! (server->flags.threaded) || server->proc_shutdown)
    return;

  server->proc_shutdown= true;

  /* Signal proc thread to shutdown. */
  (void) pthread_mutex_lock(&(server->proc_lock));
  (void) pthread_cond_signal(&(server->proc_cond));
  (void) pthread_mutex_unlock(&(server->proc_lock));

  /* Wait for the proc thread to exit and then cleanup. */
  (void) pthread_join(server->proc_id, NULL);
  (void) pthread_cond_destroy(&(server->proc_cond));
  (void) pthread_mutex_destroy(&(server->proc_lock));
}

static void *_proc(void *data)
{
  gearman_server_st *server= (gearman_server_st *)data;
  gearman_server_thread_st *thread;
  gearman_server_con_st *con;
  gearman_server_packet_st *packet;

  gearmand_initialize_thread_logging("[ proc ]");

  while (1)
  {
    (void) pthread_mutex_lock(&(server->proc_lock));
    while (server->proc_wakeup == false)
    {
      if (server->proc_shutdown)
      {
        (void) pthread_mutex_unlock(&(server->proc_lock));
        return NULL;
      }

      (void) pthread_cond_wait(&(server->proc_cond), &(server->proc_lock));
    }
    server->proc_wakeup= false;
    (void) pthread_mutex_unlock(&(server->proc_lock));

    for (thread= server->thread_list; thread != NULL; thread= thread->next)
    {
      while ((con= gearman_server_con_proc_next(thread)) != NULL)
      {
        if (con->is_dead)
        {
          gearman_server_con_free_workers(con);

          while (con->client_list != NULL)
            gearman_server_client_free(con->client_list);

          con->proc_removed= true;
          gearman_server_con_io_add(con);
          continue;
        }

        while (1)
        {
          packet= gearman_server_proc_packet_remove(con);
          if (packet == NULL)
            break;

          con->ret= gearman_server_run_command(con, &(packet->packet));
          gearmand_packet_free(&(packet->packet));
          gearman_server_packet_free(packet, con->thread, false);
        }
      }
    }
  }
}
