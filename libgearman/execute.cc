/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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
#include <libgearman/add.hpp>
#include <libgearman/universal.hpp>

#include <cassert>
#include <cerrno>
#include <iostream>


static inline gearman_command_t pick_command_by_priority(const gearman_job_priority_t &arg)
{
  if (arg == GEARMAN_JOB_PRIORITY_NORMAL)
    return GEARMAN_COMMAND_SUBMIT_JOB;
  else if (arg == GEARMAN_JOB_PRIORITY_HIGH)
    return GEARMAN_COMMAND_SUBMIT_JOB_HIGH;

  return GEARMAN_COMMAND_SUBMIT_JOB_LOW;
}

static inline gearman_command_t pick_command_by_priority_background(const gearman_job_priority_t &arg)
{
  if (arg == GEARMAN_JOB_PRIORITY_NORMAL)
    return GEARMAN_COMMAND_SUBMIT_JOB_BG;
  else if (arg == GEARMAN_JOB_PRIORITY_HIGH)
    return GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG;

  return GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG;
}



gearman_task_st *gearman_execute(gearman_client_st *client,
                                 const char *function_name, size_t function_length,
                                 const char *unique_str, size_t unique_length,
                                 gearman_work_t *workload,
                                 gearman_argument_t *arguments)
{
  if (not client)
  {
    errno= EINVAL;
    return NULL;
  }

  if (not function_name or not function_length)
  {
    gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "function_name was NULL");
    return NULL;
  }

  gearman_task_st *task= NULL;
  gearman_unique_t unique= gearman_unique_make(unique_str, unique_length);
  gearman_string_t function= { function_name, function_length };
  if (workload)
  {
    switch (workload->kind)
    {
    case GEARMAN_WORK_KIND_BACKGROUND:
      task= add_task(client,
                     workload->context,
                     pick_command_by_priority_background(workload->priority),
                     function,
                     unique,
                     arguments->value,
                     time_t(0),
                     gearman_actions_execute_defaults());
      break;

    case GEARMAN_WORK_KIND_EPOCH:
      task= add_task(client,
                     workload->context,
                     GEARMAN_COMMAND_SUBMIT_JOB_EPOCH,
                     function,
                     unique,
                     arguments->value,
                     gearman_workload_epoch(workload),
                     gearman_actions_execute_defaults());
      break;

    case GEARMAN_WORK_KIND_FOREGROUND:
      task= add_task(client,
                     workload->context,
                     pick_command_by_priority(workload->priority),
                     function,
                     unique,
                     arguments->value,
                     time_t(0),
                     gearman_actions_execute_defaults());
      break;
    }
  }
  else
  {
    task= add_task(client,
                   NULL,
                   GEARMAN_COMMAND_SUBMIT_JOB,
                   function,
                   unique,
                   arguments->value,
                   time_t(0),
                   gearman_actions_execute_defaults());
  }

  if (not task)
  {
    gearman_universal_error_code(client->universal);

    return NULL;
  }

  if (not workload) // We have no description, so we just run it
  {
    gearman_client_run_tasks(client);
  }
  else // Everything else, we do now.
  {
    gearman_client_run_tasks(client);
  }

  return task;
}

gearman_task_st *gearman_execute_map_reduce(gearman_client_st *client,
                                            const char *mapper_name, const size_t mapper_length,
                                            const char *reducer_name, const size_t reducer_length,
                                            const char *unique_str, const size_t unique_length,
                                            gearman_work_t *workload,
                                            gearman_argument_t *arguments)
{
  if (not client)
  {
    errno= EINVAL;
    return NULL;
  }

  if (not mapper_name or not mapper_length)
  {
    gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "mapper_name was NULL");
    return NULL;
  }

  if (not reducer_name or not reducer_length)
  {
    gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "reducer_name was NULL");
    return NULL;
  }


  gearman_task_st *task= NULL;
  gearman_unique_t unique= gearman_unique_make(unique_str, unique_length);
  gearman_string_t mapper= { mapper_name, mapper_length };
  gearman_string_t reducer= { reducer_name, reducer_length };

  if (workload)
  {
    switch (workload->kind)
    {
    case GEARMAN_WORK_KIND_BACKGROUND:
      task= add_task(client,
                     GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND,
                     workload->priority,
                     mapper,
                     reducer,
                     unique,
                     arguments->value,
                     gearman_actions_execute_defaults(),
                     time_t(0),
                     workload->context);
      break;

    case GEARMAN_WORK_KIND_EPOCH:
      gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "EPOCH is not currently supported for gearman_client_execute_reduce()");
      return NULL;
#if 0
      task= add_task(client,
                     GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND,
                     workload->priority,
                     mapper,
                     reducer,
                     unique,
                     arguments->value,
                     gearman_actions_execute_defaults(),
                     gearman_workload_epoch(workload),
                     workload->context);
#endif
      break;

    case GEARMAN_WORK_KIND_FOREGROUND:
      task= add_task(client,
                     GEARMAN_COMMAND_SUBMIT_REDUCE_JOB,
                     workload->priority,
                     mapper,
                     reducer,
                     unique,
                     arguments->value,
                     gearman_actions_execute_defaults(),
                     time_t(0),
                     workload->context);
      break;
    }
  }
  else
  {
    task= add_task(client,
                   GEARMAN_COMMAND_SUBMIT_REDUCE_JOB,
                   GEARMAN_JOB_PRIORITY_NORMAL,
                   mapper,
                   reducer,
                   unique,
                   arguments->value,
                   gearman_actions_execute_defaults(),
                   time_t(0),
                   NULL);
  }

  if (not task)
    return NULL;

  do {
    gearman_return_t rc;
    if (gearman_failed(rc= gearman_client_run_tasks(client)))
    {
      gearman_gerror(client->universal, rc);
      gearman_task_free(task);
      return NULL;
    }
  } while (gearman_continue(gearman_task_error(task)));
  std::cerr << __func__ << " " << gearman_strerror(gearman_task_error(task)) << std::endl;

  return task;
}
