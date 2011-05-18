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

#include <libgearman/common.h>

#include <libgearman/add.h>
#include <libgearman/connection.h>
#include <libgearman/packet.hpp>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

gearman_task_st *add_task(gearman_client_st *client,
                          void *context,
                          gearman_command_t command,
                          const gearman_string_t &function,
                          const gearman_unique_t &unique,
                          const gearman_string_t &workload,
                          time_t when,
                          struct gearman_actions_t &actions)
{
  gearman_task_st *task= add_task(client, NULL, context, command, function, unique, workload, when);
  if (not task)
    return NULL;

  task->func= actions;

  return task;
}

gearman_task_st *add_task(gearman_client_st *client,
                          gearman_task_st *task,
                          void *context,
                          gearman_command_t command,
                          const gearman_string_t &function,
                          const gearman_unique_t &unique,
                          const gearman_string_t &workload,
                          time_t when)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[4];
  size_t args_size[4];

  if ((gearman_size(workload) && gearman_c_str(workload) == NULL) or (gearman_size(workload) == 0 && gearman_c_str(workload)))
  {
    gearman_error(&client->universal, GEARMAN_INVALID_ARGUMENT, "invalid workload");
    return NULL;
  }

  task= gearman_task_internal_create(client, task);
  if (task == NULL)
  {
    gearman_error(&client->universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "");
    return NULL;
  }

  task->context= context;

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */
  args[0]= gearman_c_str(function);
  args_size[0]= gearman_size(function) + 1;

  if (gearman_size(unique))
  {
    args[1]= gearman_c_str(unique);
    args_size[1]= gearman_size(unique) + 1;
  }
  else
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    args[1]= uuid_string;
    args_size[1]= 36 + 1; // +1 is for the needed null
  }

  gearman_return_t rc;
  if (command == GEARMAN_COMMAND_SUBMIT_JOB_EPOCH)
  {
    char time_string[30];
    int length= snprintf(time_string, sizeof(time_string), "%lld", static_cast<long long>(when));
    args[2]= time_string;
    args_size[2]= length +1;
    args[3]= gearman_c_str(workload);
    args_size[3]= gearman_size(workload);

    rc= gearman_packet_create_args(client->universal, task->send,
                                   GEARMAN_MAGIC_REQUEST, command,
                                   args, args_size,
                                   4);
  }
  else
  {
    args[2]= gearman_c_str(workload);
    args_size[2]= gearman_size(workload);

    rc= gearman_packet_create_args(client->universal, task->send,
                                   GEARMAN_MAGIC_REQUEST, command,
                                   args, args_size,
                                   3);
  }

  if (rc == GEARMAN_SUCCESS)
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }
  else
  {
    gearman_task_free(task);
    task= NULL;
    gearman_error((&client->universal), rc, "");
  }

  return task;
}

gearman_task_st *add_task(gearman_client_st *client,
                          gearman_command_t command,
                          const gearman_job_priority_t priority,
                          const gearman_string_t &mapper_function,
                          const gearman_string_t &reducer,
                          const gearman_unique_t &unique,
                          const gearman_string_t &workload,
                          struct gearman_actions_t &actions,
                          const time_t epoch,
                          void *context)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[5];
  size_t args_size[5];

  if ((gearman_size(workload) && gearman_c_str(workload) == NULL) or (gearman_size(workload) == 0 && gearman_c_str(workload)))
  {
    gearman_error(&client->universal, GEARMAN_INVALID_ARGUMENT, "invalid workload");
    return NULL;
  }

  gearman_task_st *task= gearman_task_internal_create(client, NULL);
  if (not task)
  {
    gearman_error(&client->universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "");
    return NULL;
  }

  task->context= context;
  task->func= actions;

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */
  args[0]= gearman_c_str(mapper_function);
  args_size[0]= gearman_size(mapper_function) + 1;

  if (gearman_size(unique))
  {
    args[1]= gearman_c_str(unique);
    args_size[1]= gearman_size(unique) + 1;
  }
  else
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    args[1]= uuid_string;
    args_size[1]= 36 + 1; // +1 is for the needed null
  }

  assert (command == GEARMAN_COMMAND_SUBMIT_REDUCE_JOB or command == GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND);
  args[2]= gearman_c_str(reducer);
  args_size[2]= gearman_size(reducer) +1;

  char time_string[30];
  if (epoch)
  {
    int length= snprintf(time_string, sizeof(time_string), "%lld", static_cast<long long>(epoch));
    args[3]= time_string;
    args_size[3]= length +1;
  }
  else
  {
    switch(priority)
    {
    case GEARMAN_JOB_PRIORITY_HIGH:
      args[3]= "HIGH";
      args_size[3]= 5;
    case GEARMAN_JOB_PRIORITY_NORMAL:
      args[3]= "NORMAL";
      args_size[3]= 7;
    default:
    case GEARMAN_JOB_PRIORITY_MAX:
    case GEARMAN_JOB_PRIORITY_LOW:
      args[3]= "LOW";
      args_size[3]= 4;
    }
  }

  assert(gearman_c_str(workload));
  assert(gearman_size(workload));
  args[4]= gearman_c_str(workload);
  args_size[4]= gearman_size(workload);

  gearman_return_t rc;
  rc= gearman_packet_create_args(client->universal, task->send,
                                 GEARMAN_MAGIC_REQUEST, command,
                                 args, args_size,
                                 5);

  if (gearman_success(rc))
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }
  else
  {
    gearman_task_free(task);
    task= NULL;
  }

  return task;
}
