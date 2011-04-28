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
			  gearman_task_st *task,
			  void *context,
			  gearman_command_t command,
			  const char *function_name,
			  size_t function_name_length,
			  const char *unique,
			  size_t unique_length,
			  const void *workload,
			  size_t workload_size,
			  time_t when)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[4];
  size_t args_size[4];

  if ((workload_size && workload == NULL) or (workload_size == 0 && workload))
  {
    gearman_error(&client->universal, GEARMAN_INVALID_ARGUMENT, "invalid workload");
    return NULL;
  }

  task= gearman_task_create(client, task);
  if (task == NULL)
  {
    gearman_error(&client->universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "");
    return NULL;
  }

  task->context= context;

  if (unique == NULL)
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    unique= uuid_string;
    unique_length= 36; // @note This comes from uuid/uuid.h (which does not define a number)
  }

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */
  args[0]= function_name;
  args_size[0]= function_name_length + 1;
  args[1]= unique;
  args_size[1]= unique_length + 1;

  gearman_return_t rc;
  if (command == GEARMAN_COMMAND_SUBMIT_JOB_EPOCH)
  {
    char time_string[30];
    int length= snprintf(time_string, sizeof(time_string), "%lld", static_cast<long long>(when));
    args[2]= time_string;
    args_size[2]= length +1;
    args[3]= workload;
    args_size[3]= workload_size;

    rc= gearman_packet_create_args(&client->universal, &(task->send),
                                   GEARMAN_MAGIC_REQUEST, command,
                                   args, args_size,
                                   4);
  }
  else
  {
    args[2]= workload;
    args_size[2]= workload_size;

    rc= gearman_packet_create_args(&client->universal, &(task->send),
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
