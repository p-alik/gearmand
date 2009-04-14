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

/*
  if (gearman_server_thread_create(&(thread->server_thread)) == NULL)
  {
    free(thread);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }
*/

  GEARMAN_LIST_ADD(gearmand->thread, thread,)

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

  thread->count= gearmand->thread_count;

  if (gearmand->threads > 0)
  {
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

    pthread_ret= pthread_create(&(thread->id), NULL, _thread, thread);
    if (pthread_ret != 0)
    {
      thread->count= 0;
      gearmand_thread_free(thread);
      GEARMAND_ERROR_SET(gearmand, "gearmand_thread_create",
                         "pthread_create:%d", pthread_ret)
      return GEARMAN_PTHREAD;
    }
  }

  GEARMAND_VERBOSE(gearmand, 1, "Thread %u created", thread->count)

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

  while (thread->dcon_list != NULL)
    gearmand_con_free(thread->dcon_list);

  for (dcon= thread->free_dcon_list; dcon != NULL; dcon= thread->free_dcon_list)
  {
    thread->free_dcon_list= dcon->next;
    free(dcon);
  }

  if (thread->gearmand->threads > 0 && thread->base != NULL)
    event_base_free(thread->base);

  //XXX gearman_server_thread_free(&(thread->server));

  GEARMAN_LIST_DEL(thread->gearmand->thread, thread,)

  if (thread->gearmand->threads > 0)
    GEARMAND_VERBOSE(thread->gearmand, 1, "Thread shutdown complete")

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

static gearman_return_t _wakeup_init(gearmand_thread_st *thread)
{
  int ret;

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

  GEARMAND_VERBOSE(thread->gearmand, 1, "Wakeup thread pipe created")

  return GEARMAN_SUCCESS;
}

static void _wakeup_close(gearmand_thread_st *thread)
{
  _wakeup_clear(thread);

  if (thread->wakeup_fd[0] >= 0)
  {
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
        /* Do nothing. */
        break;

      case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
        /* XXX Close idle connections, wait for others to finish.
               Signal server_thread here? */
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        /* XXX Signal to server_thread here? */
        _clear_events(thread);
        break;

      default:
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

/* XXX
  gearmand_con_st *dcon;
  for (dcon= thread->dcon_list; dcon != NULL; dcon= dcon->next)
    gearmand_con_clear(dcon);
*/
}
