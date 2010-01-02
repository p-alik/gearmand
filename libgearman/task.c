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

gearman_task_st *gearman_task_create(gearman_client_st *client, gearman_task_st *task)
{
  if (task == NULL)
  {
    task= malloc(sizeof(gearman_task_st));
    if (task == NULL)
    {
      gearman_universal_set_error(&client->universal, "_task_create", "malloc");
      return NULL;
    }

    task->options.allocated= true;
  }
  else
  {
    task->options.allocated= false;
  }

  task->options.send_in_use= false;
  task->options.is_known= false;
  task->options.is_running= false;

  task->state= 0;
  task->created_id= 0;
  task->numerator= 0;
  task->denominator= 0;
  task->client= client;

  if (client->task_list != NULL)
    client->task_list->prev= task;
  task->next= client->task_list;
  task->prev= NULL;
  client->task_list= task;
  client->task_count++;

  task->context= NULL;
  task->con= NULL;
  task->recv= NULL;
  task->job_handle[0]= 0;

  return task;
}


void gearman_task_free(gearman_task_st *task)
{
  if (task->options.send_in_use)
    gearman_packet_free(&(task->send));

  if (task != &(task->client->do_task) && task->context != NULL &&
      task->client->task_context_free_fn != NULL)
  {
    task->client->task_context_free_fn(task, (void *)task->context);
  }

  if (task->client->task_list == task)
    task->client->task_list= task->next;
  if (task->prev != NULL)
    task->prev->next= task->next;
  if (task->next != NULL)
    task->next->prev= task->prev;
  task->client->task_count--;

  if (task->options.allocated)
    free(task);
}

const void *gearman_task_context(const gearman_task_st *task)
{
  return task->context;
}

void gearman_task_set_context(gearman_task_st *task, void *context)
{
  task->context= context;
}

const char *gearman_task_function_name(const gearman_task_st *task)
{
  return task->send.arg[0];
}

const char *gearman_task_unique(const gearman_task_st *task)
{
  return task->send.arg[1];
}

const char *gearman_task_job_handle(const gearman_task_st *task)
{
  return task->job_handle;
}

bool gearman_task_is_known(const gearman_task_st *task)
{
  return task->options.is_known;
}

bool gearman_task_is_running(const gearman_task_st *task)
{
  return task->options.is_running;
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
