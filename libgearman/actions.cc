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
#include <libgearman/unique.h>
#include <cassert>

static gearman_return_t _client_fail(gearman_task_st *task)
{
  task->result_rc= GEARMAN_WORK_FAIL;
  return GEARMAN_SUCCESS;
}


gearman_actions_t gearman_actions_default(void)
{
  static gearman_actions_t default_actions= { 0, 0, 0, 0, 0, 0, 0, _client_fail };

  return default_actions;
}

static gearman_return_t _client_do_data(gearman_task_st *task)
{
  if (task->result_rc != GEARMAN_SUCCESS)
  {
    task->result_rc= GEARMAN_SUCCESS;
    return GEARMAN_SUCCESS;
  }

  if (not task->result_ptr)
    task->result_ptr= gearman_string_create(NULL, gearman_task_data_size(task) +1);
  assert(task->result_ptr);

  gearman_string_append(task->result_ptr, static_cast<const char*>(gearman_task_data(task)), gearman_task_data_size(task));

  if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
  {
    task->result_rc= GEARMAN_WORK_DATA;
  }
  else if (task->recv->command == GEARMAN_COMMAND_WORK_WARNING)
  {
    task->result_rc= GEARMAN_WORK_WARNING;
  }
  else if (task->recv->command == GEARMAN_COMMAND_WORK_EXCEPTION)
  {
    task->result_rc= GEARMAN_WORK_EXCEPTION;
  }
  else
  {
    return GEARMAN_SUCCESS;
  }

  return GEARMAN_PAUSE;
}

static gearman_return_t _client_do_status(gearman_task_st *task)
{
  if (task->result_rc != GEARMAN_SUCCESS)
  {
    task->result_rc= GEARMAN_SUCCESS;
    return GEARMAN_SUCCESS;
  }

  task->result_rc= GEARMAN_WORK_STATUS;
  return GEARMAN_PAUSE;
}

gearman_actions_t gearman_actions_do_default(void)
{
  static gearman_actions_t default_actions= { 0, 0, _client_do_data, _client_do_data, _client_do_status, _client_do_data, _client_do_data, _client_fail };

  return default_actions;
}
