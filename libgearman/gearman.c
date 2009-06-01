/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman core definitions
 */

#include "common.h"

/*
 * Public definitions
 */

const char *gearman_version(void)
{
    return PACKAGE_VERSION;
}

const char *gearman_bugreport(void)
{
    return PACKAGE_BUGREPORT;
}

gearman_st *gearman_create(gearman_st *gearman)
{
  if (gearman == NULL)
  {
    gearman= malloc(sizeof(gearman_st));
    if (gearman == NULL)
      return NULL;

    memset(gearman, 0, sizeof(gearman_st));
    gearman->options|= GEARMAN_ALLOCATED;
  }
  else
    memset(gearman, 0, sizeof(gearman_st));

  return gearman;
}

gearman_st *gearman_clone(gearman_st *gearman, gearman_st *from)
{
  gearman_con_st *con;

  gearman= gearman_create(gearman);
  if (gearman == NULL)
    return NULL;

  gearman->options|= (from->options & ~GEARMAN_ALLOCATED);

  for (con= from->con_list; con != NULL; con= con->next)
  {
    if (gearman_con_clone(gearman, NULL, con) == NULL)
    {
      gearman_free(gearman);
      return NULL;
    }
  }

  /* Don't clone job or packet information, this is state information for
     old and active jobs/connections. */

  return gearman;
}

void gearman_free(gearman_st *gearman)
{
  gearman_con_st *con;
  gearman_job_st *job;
  gearman_task_st *task;
  gearman_packet_st *packet;

  for (con= gearman->con_list; con != NULL; con= gearman->con_list)
    gearman_con_free(con);

  for (job= gearman->job_list; job != NULL; job= gearman->job_list)
    gearman_job_free(job);

  for (task= gearman->task_list; task != NULL; task= gearman->task_list)
    gearman_task_free(task);

  for (packet= gearman->packet_list; packet != NULL;
       packet= gearman->packet_list)
  {
    gearman_packet_free(packet);
  }

  if (gearman->pfds != NULL)
    free(gearman->pfds);

  if (gearman->options & GEARMAN_ALLOCATED)
    free(gearman);
}

const char *gearman_error(gearman_st *gearman)
{
  return (const char *)(gearman->last_error);
}

int gearman_errno(gearman_st *gearman)
{
  return gearman->last_errno;
}

void gearman_set_options(gearman_st *gearman, gearman_options_t options,
                         uint32_t data)
{
  if (data)
    gearman->options |= options;
  else
    gearman->options &= ~options;
}

void gearman_set_log(gearman_st *gearman, gearman_log_fn log_fn,
                     void *log_fn_arg, gearman_verbose_t verbose)
{
  gearman->log_fn= log_fn;
  gearman->log_fn_arg= log_fn_arg;
  gearman->verbose= verbose;
}

void gearman_set_event_watch(gearman_st *gearman,
                             gearman_event_watch_fn *event_watch,
                             void *event_watch_arg)
{
  gearman->event_watch= event_watch;
  gearman->event_watch_arg= event_watch_arg;
}

void gearman_set_workload_malloc(gearman_st *gearman,
                                 gearman_malloc_fn *workload_malloc,
                                 const void *workload_malloc_arg)
{
  gearman->workload_malloc= workload_malloc;
  gearman->workload_malloc_arg= workload_malloc_arg;
}

void gearman_set_workload_free(gearman_st *gearman,
                               gearman_free_fn *workload_free,
                               const void *workload_free_arg)
{
  gearman->workload_free= workload_free;
  gearman->workload_free_arg= workload_free_arg;
}

void gearman_set_task_fn_arg_free(gearman_st *gearman, 
                                  gearman_task_fn_arg_free_fn *free_fn)
{
  gearman->task_fn_arg_free_fn= free_fn;
}

void *gearman_queue_fn_arg(gearman_st *gearman)
{
  return (void *)gearman->queue_fn_arg;
}

void gearman_set_queue_fn_arg(gearman_st *gearman, const void *fn_arg)
{
  gearman->queue_fn_arg= fn_arg;
}

void gearman_set_queue_add(gearman_st *gearman, gearman_queue_add_fn *add_fn)
{
  gearman->queue_add_fn= add_fn;
}

void gearman_set_queue_flush(gearman_st *gearman,
                             gearman_queue_flush_fn *flush_fn)
{
  gearman->queue_flush_fn= flush_fn;
}

void gearman_set_queue_done(gearman_st *gearman,
                            gearman_queue_done_fn *done_fn)
{
  gearman->queue_done_fn= done_fn;
}

void gearman_set_queue_replay(gearman_st *gearman,
                              gearman_queue_replay_fn *replay_fn)
{
  gearman->queue_replay_fn= replay_fn;
}

gearman_return_t gearman_parse_servers(const char *servers, void *data,
                                       gearman_parse_server_fn *server_fn)
{ 
  const char *ptr= servers;
  size_t x;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  gearman_return_t ret;

  while (1)
  { 
    x= 0;

    while (*ptr != 0 && *ptr != ',' && *ptr != ':')
    { 
      if (x < (NI_MAXHOST - 1))
        host[x++]= *ptr;

      ptr++;
    }

    host[x]= 0;

    if (*ptr == ':')
    { 
      ptr++;
      x= 0;

      while (*ptr != 0 && *ptr != ',')
      { 
        if (x < (NI_MAXSERV - 1))
          port[x++]= *ptr;

        ptr++;
      }

      port[x]= 0;
    }
    else
      port[0]= 0;

    ret= (*server_fn)(host, atoi(port), data);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    if (*ptr == 0)
      break;

    ptr++;
  }

  return GEARMAN_SUCCESS;
}
