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

  gearman_reset(gearman);

  if (gearman->options & GEARMAN_ALLOCATED)
    free(gearman);
}

void gearman_reset(gearman_st *gearman)
{
  gearman->con_ready= NULL;
  gearman->sending= 0;
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

void gearman_set_event_cb(gearman_st *gearman, 
                          gearman_event_watch_fn *event_watch,
                          gearman_event_close_fn *event_close, void *arg)
{
  gearman->event_watch= event_watch;
  gearman->event_close= event_close;
  gearman->event_cb_arg= arg;
}
