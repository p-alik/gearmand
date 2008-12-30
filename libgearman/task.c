/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Task definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_task_st *gearman_task_create(gearman_st *gearman,
                                     gearman_task_st *task)
{
  if (task == NULL)
  {
    task= malloc(sizeof(gearman_task_st));
    if (task == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_job_create", "malloc")
      return NULL;
    }

    memset(task, 0, sizeof(gearman_task_st));
    task->options|= GEARMAN_TASK_ALLOCATED;
  }
  else
    memset(task, 0, sizeof(gearman_task_st));

  task->gearman= gearman;

  if (gearman->task_list != NULL)
    gearman->task_list->prev= task;
  task->next= gearman->task_list;
  gearman->task_list= task;
  gearman->task_count++;

  return task;
}

void gearman_task_free(gearman_task_st *task)
{
  if (task->gearman->task_list == task)
    task->gearman->task_list= task->next;
  if (task->prev != NULL)
    task->prev->next= task->next;
  if (task->next != NULL)
    task->next->prev= task->prev;
  task->gearman->task_count--;

  if (task->options & GEARMAN_TASK_SEND_IN_USE)
    gearman_packet_free(&(task->send));

  if (task->options & GEARMAN_TASK_ALLOCATED)
    free(task);
}

void *gearman_task_fn_arg(gearman_task_st *task)
{
  return (void *)task->fn_arg;
}

const char *gearman_task_function(gearman_task_st *task)
{
  return (char *)task->send.arg[0];
}

const char *gearman_task_uuid(gearman_task_st *task)
{
  return (char *)task->send.arg[1];
}

const char *gearman_task_job_handle(gearman_task_st *task)
{
  return task->job_handle;
}

bool gearman_task_is_known(gearman_task_st *task)
{
  return task->is_known;
}

bool gearman_task_is_running(gearman_task_st *task)
{
  return task->is_running;
}

uint32_t gearman_task_numerator(gearman_task_st *task)
{
  return task->numerator;
}

uint32_t gearman_task_denominator(gearman_task_st *task)
{
  return task->denominator;
}

const void *gearman_task_data(gearman_task_st *task)
{
  return task->recv->data;
}

size_t gearman_task_data_size(gearman_task_st *task)
{
  return task->recv->data_size;
}

void *gearman_task_take_data(gearman_task_st *task, size_t *size)
{
  return gearman_packet_take_data(task->recv, size);
}

size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                              size_t data_size, gearman_return_t *ret_ptr)
{
  return gearman_con_recv_data(task->con, data, data_size, ret_ptr);
}
