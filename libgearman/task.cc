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

#include <config.h>
#include <libgearman/common.h>

#include <cerrno>
#include <cassert>
#include <cstring>
#include <memory>

/*
 * Public Definitions
 */

gearman_task_st *gearman_task_internal_create(gearman_client_st *client, gearman_task_st *task)
{
  assert(client);
  if (task)
  {
    task->options.allocated= false;
  }
  else
  {
    task= new (std::nothrow) gearman_task_st;
    if (task == NULL)
    {
      gearman_perror(client->universal, "gearman_task_st new");
      return NULL;
    }

    task->options.allocated= true;
  }

  task->options.send_in_use= false;
  task->options.is_known= false;
  task->options.is_running= false;
  task->options.was_reduced= false;
  task->options.is_paused= false;

  task->type= GEARMAN_TASK_KIND_ADD_TASK;

  task->state= GEARMAN_TASK_STATE_NEW;
  task->created_id= 0;
  task->numerator= 0;
  task->denominator= 0;
  task->client= client;

  task->func= client->actions;
  task->result_rc= GEARMAN_UNKNOWN_STATE;

  // Add the task to the client
  {
    if (client->task_list)
    {
      client->task_list->prev= task;
    }
    task->next= client->task_list;
    task->prev= NULL;
    client->task_list= task;
    client->task_count++;
  }

  task->context= NULL;
  task->con= NULL;
  task->recv= NULL;
  task->result_ptr= NULL;
  task->job_handle[0]= 0;

  return task;
}


void gearman_task_free(gearman_task_st *task)
{
  if (task == NULL)
  {
    return;
  }

  delete task->result_ptr;
  task->result_ptr= NULL;

  if (task->client == NULL)
  {
    return;
  }

  if (task->options.send_in_use)
  {
    gearman_packet_free(&(task->send));
  }

  if (task->type != GEARMAN_TASK_KIND_DO  and task->context and  task->client->task_context_free_fn)
  {
    task->client->task_context_free_fn(task, static_cast<void *>(task->context));
  }

  if (task->client->task_list == task)
  {
    task->client->task_list= task->next;
  }

  if (task->prev)
  {
    task->prev->next= task->next;
  }

  if (task->next)
  {
    task->next->prev= task->prev;
  }

  task->client->task_count--;

  // If the task we are removing is a current task, remove it from the client
  // structures.
  if (task->client->task == task)
  {
    task->client->task= NULL;
  }
  task->client= NULL;
  task->job_handle[0]= 0;

  if (task->options.allocated)
  {
    delete task;
  }
}

void gearman_task_free_result(gearman_task_st *task)
{
  assert(task);
  delete task->result_ptr;
  task->result_ptr= NULL;
}

bool gearman_task_is_active(const gearman_task_st *self)
{
  switch (self->state)
  {
  case GEARMAN_TASK_STATE_NEW:
  case GEARMAN_TASK_STATE_SUBMIT:
  case GEARMAN_TASK_STATE_WORKLOAD:
  case GEARMAN_TASK_STATE_WORK:
  case GEARMAN_TASK_STATE_CREATED:
  case GEARMAN_TASK_STATE_DATA:
  case GEARMAN_TASK_STATE_WARNING:
  case GEARMAN_TASK_STATE_STATUS:
    return true;

  case GEARMAN_TASK_STATE_COMPLETE:
  case GEARMAN_TASK_STATE_EXCEPTION:
  case GEARMAN_TASK_STATE_FAIL:
  case GEARMAN_TASK_STATE_FINISHED:
    break;
  }

  return false;
}

const char *gearman_task_strstate(gearman_task_st *self)
{
  switch (self->state)
  {
  case GEARMAN_TASK_STATE_NEW: return "GEARMAN_TASK_STATE_NEW";
  case GEARMAN_TASK_STATE_SUBMIT: return "GEARMAN_TASK_STATE_SUBMIT";
  case GEARMAN_TASK_STATE_WORKLOAD: return "GEARMAN_TASK_STATE_WORKLOAD";
  case GEARMAN_TASK_STATE_WORK: return "GEARMAN_TASK_STATE_WORK";
  case GEARMAN_TASK_STATE_CREATED: return "GEARMAN_TASK_STATE_CREATED";
  case GEARMAN_TASK_STATE_DATA: return "GEARMAN_TASK_STATE_DATA";
  case GEARMAN_TASK_STATE_WARNING: return "GEARMAN_TASK_STATE_WARNING";
  case GEARMAN_TASK_STATE_STATUS: return "GEARMAN_TASK_STATE_STATUS";
  case GEARMAN_TASK_STATE_COMPLETE: return "GEARMAN_TASK_STATE_COMPLETE";
  case GEARMAN_TASK_STATE_EXCEPTION: return "GEARMAN_TASK_STATE_EXCEPTION";
  case GEARMAN_TASK_STATE_FAIL: return "GEARMAN_TASK_STATE_FAIL";
  case GEARMAN_TASK_STATE_FINISHED: return "GEARMAN_TASK_STATE_FINISHED";
  }

  return "";
}

void gearman_task_clear_fn(gearman_task_st *task)
{
  task->func= gearman_actions_default();
}

void *gearman_task_context(const gearman_task_st *task)
{
  if (task == NULL)
    return NULL;

  return const_cast<void *>(task->context);
}

void gearman_task_set_context(gearman_task_st *task, void *context)
{
  if (task == NULL)
    return;

  task->context= context;
}

const char *gearman_task_function_name(const gearman_task_st *task)
{
  if (task == NULL)
    return 0;

  return task->send.arg[0];
}

const char *gearman_task_unique(const gearman_task_st *task)
{
  if (task == NULL)
    return 0;

  return task->send.arg[1];
}

const char *gearman_task_job_handle(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return 0;
  }

  return task->job_handle;
}

bool gearman_task_is_known(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return false;
  }

  return task->options.is_known;
}

bool gearman_task_is_running(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return false;
  }

  return task->options.is_running;
}

uint32_t gearman_task_numerator(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return 0;
  }

  return task->numerator;
}

uint32_t gearman_task_denominator(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return 0;
  }

  return task->denominator;
}

void gearman_task_give_workload(gearman_task_st *task, const void *workload,
                                size_t workload_size)
{
  if (task == NULL)
  {
    return;
  }

  gearman_packet_give_data(task->send, workload, workload_size);
}

size_t gearman_task_send_workload(gearman_task_st *task, const void *workload,
                                  size_t workload_size,
                                  gearman_return_t *ret_ptr)
{
  if (task == NULL)
    return 0;

  return task->con->send_and_flush(workload, workload_size, ret_ptr);
}

gearman_result_st *gearman_task_result(gearman_task_st *task)
{
  if (task == NULL)
    return NULL;

  return task->result_ptr;
}

gearman_result_st *gearman_task_mutable_result(gearman_task_st *task)
{
  assert(task); // Programmer error
  if (not task->result_ptr)
    task->result_ptr= new gearman_result_st();

  return task->result_ptr;
}

const void *gearman_task_data(const gearman_task_st *task)
{
  if (task == NULL)
    return NULL;

  if (task->recv and task->recv->data)
    return task->recv->data;

  return 0;
}

size_t gearman_task_data_size(const gearman_task_st *task)
{
  if (task == NULL)
    return 0;

  if (task->recv and task->recv->data_size)
    return task->recv->data_size;

  return 0;
}

void *gearman_task_take_data(gearman_task_st *task, size_t *data_size)
{
  if (task == NULL)
    return 0;

  return gearman_packet_take_data(*task->recv, data_size);
}

size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                                  size_t data_size,
                                  gearman_return_t *ret_ptr)
{
  if (task == NULL)
  {
    return 0;
  }

  if (not ret_ptr)
  {
    gearman_return_t unused;
    return task->con->receiving(data, data_size, unused);
  }

  return task->con->receiving(data, data_size, *ret_ptr);
}

const char *gearman_task_error(const gearman_task_st *task)
{
  if (task == NULL)
  {
    return NULL;
  }

  if (task->result_rc == GEARMAN_UNKNOWN_STATE or task->result_rc == GEARMAN_SUCCESS)
  {
    return NULL;
  }

  return gearman_strerror(task->result_rc);
}

gearman_return_t gearman_task_return(const gearman_task_st *task)
{
  assert(task); // Only used internally.
  if (task == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return task->result_rc;
}
