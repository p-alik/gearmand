/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Thread Definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_thread private Private Gearmand Thread Functions
 * @ingroup gearmand_thread
 * @{
 */

static void *_thread(void *data);
static void _run(gearman_server_thread_st *thread __attribute__ ((unused)),
                 void *fn_arg);

static gearman_return_t _wakeup_init(gearmand_thread_st *thread);
static void _wakeup_close(gearmand_thread_st *thread);
static void _wakeup_clear(gearmand_thread_st *thread);
static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg);
static void _clear_events(gearmand_thread_st *thread);

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearmand_thread_create(gearmand_st *gearmand)
{
  gearmand_thread_st *thread;
  gearman_return_t ret;
  int pthread_ret;

  thread= malloc(sizeof(gearmand_thread_st));
  if (thread == NULL)
  {
    GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(thread, 0, sizeof(gearmand_thread_st));

  thread->gearmand= gearmand;

  if (gearman_server_thread_create(&(gearmand->server),
                                   &(thread->server_thread)) == NULL)
  {
    free(thread);
    GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create",
                       "gearman_server_thread_create:NULL")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearman_server_thread_set_event_watch(&(thread->server_thread),
                                        gearmand_con_watch, NULL);

  GEARMAN_LIST_ADD(gearmand->thread, thread,)

  /* If we have no threads, we still create a fake thread that uses the main
     libevent instance. Otherwise create a libevent instance for each thread. */
  if (gearmand->threads == 0)
    thread->base= gearmand->base;
  else
  {
    GEARMAND_VERBOSE(gearmand, 1, "Initializing libevent for IO thread")

    thread->base= event_base_new();
    if (thread->base == NULL)
    {
      gearmand_thread_free(thread);
      GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create",
                         "event_base_new:NULL")
      return GEARMAN_EVENT;
    }
  }

  ret= _wakeup_init(thread);
  if (ret != GEARMAN_SUCCESS)
  {
    gearmand_thread_free(thread);
    return ret;
  }

  /* If we don't have any threads, just return the thread context. */
  if (gearmand->threads == 0)
    return GEARMAN_SUCCESS;

  thread->count= gearmand->thread_count;

  pthread_ret= pthread_mutex_init(&(thread->lock), NULL);
  if (pthread_ret != 0)
  {
    thread->count= 0;
    gearmand_thread_free(thread);
    GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create",
                       "pthread_mutex_init:%d", pthread_ret)
    return GEARMAN_PTHREAD;
  }

  thread->options|= GEARMAND_THREAD_LOCK;

  gearman_server_thread_set_run(&(thread->server_thread), _run, thread);

  pthread_ret= pthread_create(&(thread->id), NULL, _thread, thread);
  if (pthread_ret != 0)
  {
    thread->count= 0;
    gearmand_thread_free(thread);
    GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create", "pthread_create:%d",
                       pthread_ret)
    return GEARMAN_PTHREAD;
  }

  GEARMAND_VERBOSE(gearmand, 0, "Thread %u created", thread->count)

  return GEARMAN_SUCCESS;
}

void gearmand_thread_free(gearmand_thread_st *thread)
{
  gearmand_con_st *dcon;

  if (thread->gearmand->threads && thread->count > 0)
  {
    GEARMAND_VERBOSE(thread->gearmand, 1, "Shutting down thread %u",
                     thread->count)

    gearmand_thread_wakeup(thread, GEARMAND_WAKEUP_SHUTDOWN);
    (void) pthread_join(thread->id, NULL);
  }

  if (thread->options & GEARMAND_THREAD_LOCK)
    (void) pthread_mutex_destroy(&(thread->lock));

  _wakeup_close(thread);

  assert(thread->dcon_list == NULL);

  while (thread->free_dcon_list != NULL)
  {
    dcon= thread->free_dcon_list;
    thread->free_dcon_list= dcon->next;
    free(dcon);
  }

  gearman_server_thread_free(&(thread->server_thread));

  GEARMAN_LIST_DEL(thread->gearmand->thread, thread,)

  if (thread->gearmand->threads > 0)
  {
    if (thread->base != NULL)
      event_base_free(thread->base);

    GEARMAND_VERBOSE(thread->gearmand, 0, "Thread %u shutdown complete",
                     thread->count)
  }

  free(thread);
}

void gearmand_thread_wakeup(gearmand_thread_st *thread,
                            gearmand_wakeup_t wakeup)
{
  uint8_t buffer= wakeup;

  /* If this fails, there is not much we can really do. This should never fail
     though if the thread is still active. */
  (void) write(thread->wakeup_fd[1], &buffer, 1);
}

void gearmand_thread_run(gearmand_thread_st *thread)
{
  gearman_server_con_st *server_con;
  gearman_return_t ret;
  gearmand_con_st *dcon;

  while (1)
  { 
    server_con= gearman_server_thread_run(&(thread->server_thread), &ret);
    if (ret == GEARMAN_SUCCESS || ret == GEARMAN_IO_WAIT ||
        ret == GEARMAN_SHUTDOWN_GRACEFUL)
    { 
      return;
    }

    if (server_con == NULL)
    { 
      if (ret != GEARMAN_SHUTDOWN_GRACEFUL)
      { 
        GEARMAND_ERROR_SET(thread->gearmand, "_con_ready", "%s",
                          gearman_server_thread_error(&(thread->server_thread)))
        thread->gearmand->last_errno=
                          gearman_server_thread_errno(&(thread->server_thread));
      }

      /* We either got a GEARMAN_SHUTDOWN or some other fatal internal error.
         Either way, we want to shut the server down. */
      gearmand_wakeup(thread->gearmand, GEARMAND_WAKEUP_SHUTDOWN);

      return;
    }

    dcon= (gearmand_con_st *)gearman_server_con_data(server_con);

    if (ret != GEARMAN_LOST_CONNECTION)
    { 
      GEARMAND_VERBOSE(thread->gearmand, 0, "[%4u] %15s:%5s %s", thread->count,
                       dcon->host, dcon->port,
                       gearman_server_thread_error(&(thread->server_thread)))
    }

    GEARMAND_VERBOSE(thread->gearmand, 0, "[%4u] %15s:%5s Disconnected",
                     thread->count, dcon->host, dcon->port)

    gearmand_con_free(dcon);
  }
}

/*
 * Private definitions
 */

static void *_thread(void *data)
{
  gearmand_thread_st *thread= (gearmand_thread_st *)data;

  GEARMAND_VERBOSE(thread->gearmand, 1, "[%4u] Entering thread event loop",
                   thread->count)

  if (event_base_loop(thread->base, 0) == -1)
  {
    GEARMAND_ERROR_SET(thread->gearmand, "_io_thread", "event_base_loop:-1")
    thread->gearmand->ret= GEARMAN_EVENT;
  }

  GEARMAND_VERBOSE(thread->gearmand, 1, "[%4u] Exiting thread event loop",
                   thread->count)

  return NULL;
}

static void _run(gearman_server_thread_st *thread __attribute__ ((unused)),
                 void *fn_arg)
{
  gearmand_thread_st *dthread= (gearmand_thread_st *)fn_arg;
  gearmand_thread_wakeup(dthread, GEARMAND_WAKEUP_RUN);
}

static gearman_return_t _wakeup_init(gearmand_thread_st *thread)
{
  int ret;

  GEARMAND_VERBOSE(thread->gearmand, 1, "Creating IO thread wakeup pipe")

  ret= pipe(thread->wakeup_fd);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_init", "pipe:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(thread->wakeup_fd[0], F_GETFL, 0);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_init", "fcntl:F_GETFL:%d",
                       errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(thread->wakeup_fd[0], F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_init", "fcntl:F_SETFL:%d",
                       errno)
    return GEARMAN_ERRNO;
  }

  event_set(&(thread->wakeup_event), thread->wakeup_fd[0], EV_READ | EV_PERSIST,
            _wakeup_event, thread);
  event_base_set(thread->base, &(thread->wakeup_event));

  if (event_add(&(thread->wakeup_event), NULL) == -1)
  {
    GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_init", "event_add:-1")
    return GEARMAN_EVENT;
  }

  thread->options|= GEARMAND_THREAD_WAKEUP_EVENT;

  return GEARMAN_SUCCESS;
}

static void _wakeup_close(gearmand_thread_st *thread)
{
  _wakeup_clear(thread);

  if (thread->wakeup_fd[0] >= 0)
  {
    GEARMAND_VERBOSE(thread->gearmand, 1, "Closing IO thread wakeup pipe")
    close(thread->wakeup_fd[0]);
    thread->wakeup_fd[0]= -1;
    close(thread->wakeup_fd[1]);
    thread->wakeup_fd[1]= -1;
  }
}

static void _wakeup_clear(gearmand_thread_st *thread)
{
  if (thread->options & GEARMAND_THREAD_WAKEUP_EVENT)
  {
    GEARMAND_VERBOSE(thread->gearmand, 1,
                     "[%4u] Clearing event for IO thread wakeup pipe",
                     thread->count)
    assert(event_del(&(thread->wakeup_event)) == 0);
    thread->options&= ~GEARMAND_THREAD_WAKEUP_EVENT;
  }
}

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_thread_st *thread= (gearmand_thread_st *)arg;
  uint8_t buffer[GEARMAN_PIPE_BUFFER_SIZE];
  ssize_t ret;
  ssize_t x;

  while (1)
  {
    ret= read(fd, buffer, GEARMAN_PIPE_BUFFER_SIZE);
    if (ret == 0)
    {
      _clear_events(thread);
      GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_event", "read:EOF");
      thread->gearmand->ret= GEARMAN_PIPE_EOF;
      return;
    }
    else if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      if (errno == EAGAIN)
        break;

      _clear_events(thread);
      GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_event", "read:%d", errno);
      thread->gearmand->ret= GEARMAN_ERRNO;
      return;
    }

    for (x= 0; x < ret; x++)
    {
      switch ((gearmand_wakeup_t)buffer[x])
      {
      case GEARMAND_WAKEUP_PAUSE:
        GEARMAND_VERBOSE(thread->gearmand, 1,
                         "[%4u] Received PAUSE wakeup event", thread->count)
        break;

      case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
        GEARMAND_VERBOSE(thread->gearmand, 1,
                         "[%4u] Received SHUTDOWN_GRACEFUL wakeup event",
                         thread->count)
        if (gearman_server_shutdown_graceful(&(thread->gearmand->server)) ==
            GEARMAN_SHUTDOWN)
        {
          gearmand_wakeup(thread->gearmand, GEARMAND_WAKEUP_SHUTDOWN);
        }
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        GEARMAND_VERBOSE(thread->gearmand, 1,
                         "[%4u] Received SHUTDOWN wakeup event", thread->count)
        _clear_events(thread);
        break;

      case GEARMAND_WAKEUP_CON:
        GEARMAND_VERBOSE(thread->gearmand, 1, "[%4u] Received CON wakeup event",
                         thread->count)
        gearmand_con_check_queue(thread);
        break;

      case GEARMAND_WAKEUP_RUN:
        GEARMAND_VERBOSE(thread->gearmand, 1, "[%4u] Received RUN wakeup event",
                         thread->count)
        gearmand_thread_run(thread);
        break;

      default:
        GEARMAND_VERBOSE(thread->gearmand, 1,
                         "[%4u] Received unknown wakeup event (%u)",
                         thread->count, buffer[x])
        _clear_events(thread);
        GEARMAND_ERROR_SET(thread->gearmand, "_wakeup_event", "unknown state");
        thread->gearmand->ret= GEARMAN_UNKNOWN_STATE;
        break;
      }
    }
  }
}

static void _clear_events(gearmand_thread_st *thread)
{
  _wakeup_clear(thread);

  while (thread->dcon_list != NULL)
    gearmand_con_free(thread->dcon_list);
}
