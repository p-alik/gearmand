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

#include <config.h>

#include <libgearman/common.h>
#include <libgearman/unique.h>
#include <cassert>
#include <memory>

struct gearman_result_st;

static gearman_return_t _client_do_data(gearman_task_st *task)
{
  if (gearman_task_data_size(task))
  {
    if (not task->result_ptr)
      task->result_ptr= new (std::nothrow) gearman_result_st(gearman_task_data_size(task));
    assert(task->result_ptr);

    gearman_string_append(gearman_task_mutable_result(task)->string(), static_cast<const char*>(gearman_task_data(task)), gearman_task_data_size(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_do_complete(gearman_task_st *task)
{
  if (gearman_task_data_size(task))
  {
    if (not task->result_ptr)
      task->result_ptr= new (std::nothrow) gearman_result_st(gearman_task_data_size(task));
    assert(task->result_ptr);

    gearman_string_append(gearman_task_mutable_result(task)->string(), static_cast<const char*>(gearman_task_data(task)), gearman_task_data_size(task));
  }

  task->result_rc= GEARMAN_SUCCESS;

  return GEARMAN_SUCCESS;
}

gearman_actions_t &gearman_actions_default(void)
{
  static gearman_actions_t default_actions= { 0, 0, 0, 0, 0, 0, 0, 0 };

  return default_actions;
}


gearman_actions_t &gearman_actions_do_default(void)
{
  static gearman_actions_t default_actions= { 0, 0, _client_do_data, 0, 0, _client_do_complete, 0, 0 };

  return default_actions;
}

gearman_actions_t &gearman_actions_execute_defaults(void)
{
  static gearman_actions_t default_actions= { 0, 0, _client_do_data, 0, 0, _client_do_complete, 0, 0 };

  return default_actions;
}
