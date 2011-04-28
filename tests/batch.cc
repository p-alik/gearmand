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

#include <libtest/test.h>
#include <cassert>
#include <cstring>
#include <libgearman/gearman.h>
#include <tests/batch.h>
#include <iostream>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

test_return_t gearman_client_execute_batch_basic(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_batch_t batch[]= {
    { function, gearman_workload_make(gearman_literal_param("test load")) },
    { function, gearman_workload_make(gearman_literal_param("test load2")) }
  };

  test_true_got(gearman_client_execute_batch(client, batch), gearman_client_error(client));
  gearman_client_task_free_all(client);
  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_batch_mixed_bg(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_batch_t batch[]= {
    { function, gearman_workload_make(gearman_literal_param("test load")) },
    { function, gearman_workload_make(gearman_literal_param("test load2")) },
    { function, gearman_workload_make(gearman_literal_param("test load3")) },
    { function, gearman_workload_make(gearman_literal_param("test load4")) },
  };

  gearman_workload_set_background(&batch[1].workload, true);

  test_true_got(gearman_client_execute_batch(client, batch), gearman_client_error(client));
  gearman_client_task_free_all(client);
  gearman_function_free(function);

  return TEST_SUCCESS;
}
