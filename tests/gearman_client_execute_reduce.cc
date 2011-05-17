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
#include <tests/gearman_client_execute_reduce.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

test_return_t gearman_worker_set_reducer_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  test_true_got(gearman_success(gearman_client_echo(client, gearman_literal_param("this is mine"))), gearman_client_error(client));

  gearman_client_set_server_option(client, gearman_literal_param("should fail"));
  gearman_argument_t work_args= gearman_argument_make(gearman_literal_param("this dog does not hunt"));

  gearman_string_t function= { gearman_literal_param("split_worker") };
  gearman_string_t reducer= { gearman_literal_param("client_test") };
  gearman_task_st *task;
  test_true_got(task= gearman_client_execute_reduce(client,
                                                    gearman_string_param(function),
                                                    gearman_string_param(reducer),
                                                    NULL, 0,  // unique
                                                    NULL,
                                                    &work_args), gearman_client_error(client));

  gearman_return_t rc;
  test_true_got(gearman_success(rc= gearman_task_error(task)), gearman_strerror(rc));
  gearman_result_st *result= gearman_task_result(task);
  test_truth(result);
  const char *value= gearman_result_value(result);
  test_truth(value);
  test_compare(18, gearman_result_size(result));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}
