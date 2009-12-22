/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Task Definitions
 */

#include "common.h"

/*
 * Public Definitions
 */

void *gearman_task_context(const gearman_task_st *task)
{
  return (void *)task->context;
}

void gearman_task_set_context(gearman_task_st *task, const void *context)
{
  task->context= context;
}

const char *gearman_task_function_name(const gearman_task_st *task)
{
  return (char *)task->send.arg[0];
}

const char *gearman_task_unique(const gearman_task_st *task)
{
  return (char *)task->send.arg[1];
}

const char *gearman_task_job_handle(const gearman_task_st *task)
{
  return task->job_handle;
}

bool gearman_task_is_known(const gearman_task_st *task)
{
  return task->is_known;
}

bool gearman_task_is_running(const gearman_task_st *task)
{
  return task->is_running;
}

uint32_t gearman_task_numerator(const gearman_task_st *task)
{
  return task->numerator;
}

uint32_t gearman_task_denominator(const gearman_task_st *task)
{
  return task->denominator;
}

void gearman_task_give_workload(gearman_task_st *task, const void *workload,
                                size_t workload_size)
{
  gearman_packet_give_data(&(task->send), workload, workload_size);
}

size_t gearman_task_send_workload(gearman_task_st *task, const void *workload,
                                  size_t workload_size,
                                  gearman_return_t *ret_ptr)
{
  return gearman_connection_send_data(task->con, workload, workload_size, ret_ptr);
}

const void *gearman_task_data(const gearman_task_st *task)
{
  return task->recv->data;
}

size_t gearman_task_data_size(const gearman_task_st *task)
{
  return task->recv->data_size;
}

void *gearman_task_take_data(gearman_task_st *task, size_t *data_size)
{
  return gearman_packet_take_data(task->recv, data_size);
}

size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                                  size_t data_size,
                                  gearman_return_t *ret_ptr)
{
  return gearman_connection_recv_data(task->con, data, data_size, ret_ptr);
}
