/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman daemon thread definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_thread private Private Gearman Daemon Thread Functions
 * @ingroup gearmand_thread
 * @{
 */

#if 0
static void *_thread(void *data);
#endif

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearmand_thread_create(gearmand_st *gearmand)
{
#if 0
  gearmand_thread_st *thread;

  thread= malloc(sizeof(gearmand_thread_st));
  if (thread == NULL)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "gearmand_thread_create",
                      "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(thread, 0, sizeof(gearmand_thread_st));

  thread->gearmand= gearmand;

  if (gearman_server_thread_create(&(thread->thread)) == NULL)
  {
    free(thread);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  GEARMAN_LIST_ADD(gearmand->thread, thread,)

  if (gearmand->threads == 0)
    thread->base= base;
  else
  {
    thread->base= event_base_new();
    if (thread->base == NULL)
    {
      gearmand_thread_free(thread);
      GEARMAN_ERROR_SET(gearmand->server.gearman, "gearmand_thread_create",
                        "event_base_new:NULL")
      return GEARMAN_EVENT;
    }

    ret= pthread_create(&(thread->id), NULL, _thread, thread);
    if (ret != 0)
    {
      free(thread);
      GEARMAN_ERROR_SET(gearmand->server.gearman, "gearmand_thread_create",
                        "pthread_create:%d", ret)
      return GEARMAN_PTHREAD;
    }

    thread->options|= GEARMAND_THREAD_RUNNING;
  }

#endif
  (void) gearmand;
  return GEARMAN_SUCCESS;
}

void gearmand_thread_free(gearmand_thread_st *thread)
{
#if 0
  gearmand_con_st *dcon;

  while (thread->con_list != NULL)
    gearmand_con_free(thread->con_list);

  if (gearmand->listen_fd >= 0)
  {
    close(gearmand->listen_fd);
    gearmand->listen_fd= -1;
  }

  if (gearmand->base != NULL)
    event_base_free(gearmand->base);

  gearman_server_free(&(gearmand->server));
  free(gearmand);
#endif
  (void) thread;
}

/*
  for (dcon= gearmand->dcon_list; dcon != NULL; dcon= gearmand->dcon_list)
  {
    gearmand->dcon_list= dcon->next;
    gearman_server_con_free(&(dcon->server_con));
    close(dcon->fd);
    free(dcon);
  }

  for (dcon= gearmand->free_dcon_list; dcon != NULL;
       dcon= gearmand->free_dcon_list)
  {
    gearmand->free_dcon_list= dcon->next;
    free(dcon);
  }
*/

/*
 * Private definitions
 */

#if 0
static void *_thread(void *data)
{
  gearmand_thread_st *thread= (gearmand_thread_st *)data;

  if (event_base_loop(thread->base, 0) == -1)
  {
    GEARMAN_ERROR_SET(thread->gearmand->server.gearman, "_io_thread",
                      "event_base_loop:-1")
    thread->gearmand->ret= GEARMAN_EVENT;
  }

  return NULL;
}
#endif
