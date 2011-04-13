/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief Task Definitions
 */

#include <libgearman/common.h>
#include <libgearman/connection.h>
#include <libgearman/packet.h>

#include <assert.h>
#include <memory>

/*
 * Public Definitions
 */

gearman_task_st *gearman_task_create(gearman_client_st *client, gearman_task_st *task)
{
  if (task == NULL)
  {
    task= new (std::nothrow) gearman_task_st;
    if (task == NULL)
    {
      gearman_perror(&client->universal, "gearman_task_st new");
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

  task->state= GEARMAN_TASK_STATE_NEW;
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
  if (not task)
    return;

  if (not task->client)
    return;

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
    delete task;
}

const void *gearman_task_context(const gearman_task_st *task)
{
  if (not task)
    return NULL;

  return task->context;
}

void gearman_task_set_context(gearman_task_st *task, void *context)
{
  if (not task)
    return;

  task->context= context;
}

const char *gearman_task_function_name(const gearman_task_st *task)
{
  if (not task)
    return 0;

  return task->send.arg[0];
}

const char *gearman_task_unique(const gearman_task_st *task)
{
  if (not task)
    return 0;

  return task->send.arg[1];
}

const char *gearman_task_job_handle(const gearman_task_st *task)
{
  if (not task)
    return 0;

  return task->job_handle;
}

bool gearman_task_is_known(const gearman_task_st *task)
{
  if (not task)
    return false;

  return task->options.is_known;
}

bool gearman_task_is_running(const gearman_task_st *task)
{
  if (not task)
    return false;

  return task->options.is_running;
}

uint32_t gearman_task_numerator(const gearman_task_st *task)
{
  if (not task)
    return 0;

  return task->numerator;
}

uint32_t gearman_task_denominator(const gearman_task_st *task)
{
  if (not task)
    return 0;

  return task->denominator;
}

void gearman_task_give_workload(gearman_task_st *task, const void *workload,
                                size_t workload_size)
{
  if (not task)
    return;

  gearman_packet_give_data(&(task->send), workload, workload_size);
}

size_t gearman_task_send_workload(gearman_task_st *task, const void *workload,
                                  size_t workload_size,
                                  gearman_return_t *ret_ptr)
{
  if (not task)
    return 0;

  return gearman_connection_send_data(task->con, workload, workload_size, ret_ptr);
}

const void *gearman_task_data(const gearman_task_st *task)
{
  if (not task)
    return NULL;

  if (not task->recv)
    return NULL;

  return task->recv->data;
}

size_t gearman_task_data_size(const gearman_task_st *task)
{
  if (not task)
    return 0;

  if (not task->recv)
    return 0;

  return task->recv->data_size;
}

void *gearman_task_take_data(gearman_task_st *task, size_t *data_size)
{
  if (not task)
    return 0;

  return gearman_packet_take_data(task->recv, data_size);
}

size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                                  size_t data_size,
                                  gearman_return_t *ret_ptr)
{
  if (not task)
    return 0;

  return gearman_connection_recv_data(task->con, data, data_size, ret_ptr);
}
