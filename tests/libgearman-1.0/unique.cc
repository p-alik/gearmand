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

#include "gear_config.h"
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cstring>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libgearman/gearman.h>
#include <tests/unique.h>

#include <tests/start_worker.h>

#include "libgearman/client.hpp"
#include "libgearman/worker.hpp"
using namespace org::gearmand;

#include "tests/libgearman-1.0/client_test.h"
#include "tests/workers/v1/unique.h"
#include "tests/workers/v2/sleep_return_random.h"
#include "tests/workers/v2/echo_or_react.h"

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#define WORKER_UNIQUE_FUNCTION_NAME "unique_test"

test_return_t unique_SETUP(void *object)
{
  client_test_st *test= (client_test_st *)object;

  test->set_worker_name(WORKER_UNIQUE_FUNCTION_NAME);

  gearman_function_t unique_worker_arg= gearman_function_create_v1(unique_worker);
  test->push(test_worker_start(libtest::default_port(), NULL,
                               test->worker_name(),
                               unique_worker_arg, NULL, GEARMAN_WORKER_GRAB_UNIQ));

  return TEST_SUCCESS;
}


test_return_t coalescence_TEST(void *object)
{
  gearman_client_st *client_one= (gearman_client_st *)object;
  ASSERT_TRUE(client_one);

  libgearman::Client client_two(client_one);

  const char* unique_handle= "local_handle";

  gearman_function_t sleep_return_random_worker_FN= gearman_function_create(sleep_return_random_worker);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                             NULL,
                                                             __func__,
                                                             sleep_return_random_worker_FN,
                                                             NULL,
                                                             gearman_worker_options_t(),
                                                             0)); // timeout

  // First task
  gearman_return_t ret;
  gearman_task_st *first_task= gearman_client_add_task(client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       NULL, 0, // workload
                                                       &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(first_task);
  ASSERT_TRUE(gearman_task_unique(first_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(first_task)));
 
  // Second task
  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        NULL, 0, // workload
                                                        &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(second_task);
  ASSERT_TRUE(gearman_task_unique(second_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(second_task)));
  
  test_strcmp(gearman_task_unique(first_task), gearman_task_unique(second_task));

  do {
    ret= gearman_client_run_tasks(client_one);
    gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  gearman_result_st* first_result= gearman_task_result(first_task);
  gearman_result_st* second_result= gearman_task_result(second_task);

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(first_task));
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(second_task));

  ASSERT_EQ(gearman_result_value(first_result), gearman_result_value(second_result));

  gearman_task_free(first_task);
  gearman_task_free(second_task);

  return TEST_SUCCESS;
}

test_return_t coalescence_by_data_hash_TEST(void *object)
{
  gearman_client_st *client_one= (gearman_client_st *)object;
  ASSERT_TRUE(client_one);

  libgearman::Client client_two(client_one);

  const char* unique_handle= "#";

  gearman_function_t sleep_return_random_worker_FN= gearman_function_create(sleep_return_random_worker);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                             NULL,
                                                             __func__,
                                                             sleep_return_random_worker_FN,
                                                             NULL,
                                                             gearman_worker_options_t(),
                                                             0)); // timeout

  // First task
  gearman_return_t ret;
  gearman_task_st *first_task= gearman_client_add_task(client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       test_literal_param(__func__), // workload
                                                       &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(first_task);
  ASSERT_TRUE(gearman_task_unique(first_task));
  ASSERT_EQ(strlen("2285535048"), strlen(gearman_task_unique(first_task)));
 
  // Second task
  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        test_literal_param(__func__), // workload
                                                        &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(second_task);
  ASSERT_TRUE(gearman_task_unique(second_task));
  ASSERT_EQ(strlen("2285535048"), strlen(gearman_task_unique(second_task)));
  
  test_strcmp(gearman_task_unique(first_task), gearman_task_unique(second_task));
  test_strcmp("2285535048", gearman_task_unique(second_task));

  do {
    ret= gearman_client_run_tasks(client_one);
    gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  gearman_result_st* first_result= gearman_task_result(first_task);
  gearman_result_st* second_result= gearman_task_result(second_task);

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(first_task));
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(second_task));

  ASSERT_EQ(gearman_result_value(first_result), gearman_result_value(second_result));

  gearman_task_free(first_task);
  gearman_task_free(second_task);

  return TEST_SUCCESS;
}

test_return_t coalescence_by_data_TEST(void *object)
{
  gearman_client_st *client_one= (gearman_client_st *)object;
  ASSERT_TRUE(client_one);

  libgearman::Client client_two(client_one);

  const char* unique_handle= "-";

  gearman_function_t sleep_return_random_worker_FN= gearman_function_create(sleep_return_random_worker);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                             NULL,
                                                             __func__,
                                                             sleep_return_random_worker_FN,
                                                             NULL,
                                                             gearman_worker_options_t(),
                                                             0)); // timeout

  // First task
  gearman_return_t ret;
  gearman_task_st *first_task= gearman_client_add_task(client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       NULL, 0, // workload
                                                       &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(first_task);
  ASSERT_TRUE(gearman_task_unique(first_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(first_task)));
 
  // Second task
  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        NULL, 0, // workload
                                                        &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(second_task);
  ASSERT_TRUE(gearman_task_unique(second_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(second_task)));
  
  test_strcmp(gearman_task_unique(first_task), gearman_task_unique(second_task));

  do {
    ret= gearman_client_run_tasks(client_one);
    gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  gearman_result_st* first_result= gearman_task_result(first_task);
  gearman_result_st* second_result= gearman_task_result(second_task);

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(first_task));
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(second_task));

  ASSERT_EQ(gearman_result_value(first_result), gearman_result_value(second_result));

  gearman_task_free(first_task);
  gearman_task_free(second_task);

  return TEST_SUCCESS;
}

test_return_t coalescence_by_data_FAIL_TEST(void *object)
{
  gearman_client_st *client_one= (gearman_client_st *)object;
  ASSERT_TRUE(client_one);

  libgearman::Client client_two(client_one);

  const char* unique_handle= "-";

  gearman_function_t sleep_return_random_worker_FN= gearman_function_create(sleep_return_random_worker);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                             NULL,
                                                             __func__,
                                                             sleep_return_random_worker_FN,
                                                             NULL,
                                                             gearman_worker_options_t(),
                                                             0)); // timeout

  // First task
  gearman_return_t ret;
  gearman_task_st *first_task= gearman_client_add_task(client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       NULL, 0, // workload
                                                       &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(first_task);
  ASSERT_TRUE(gearman_task_unique(first_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(first_task)));
 
  // Second task
  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        test_literal_param("mine"), // workload
                                                        &ret);
  ASSERT_EQ(GEARMAN_SUCCESS, ret);
  ASSERT_TRUE(second_task);
  ASSERT_TRUE(gearman_task_unique(second_task));
  ASSERT_EQ(strlen(unique_handle), strlen(gearman_task_unique(second_task)));
  
  test_strcmp(gearman_task_unique(first_task), gearman_task_unique(second_task));

  do {
    ret= gearman_client_run_tasks(client_one);
    gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  gearman_result_st* first_result= gearman_task_result(first_task);
  gearman_result_st* second_result= gearman_task_result(second_task);

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(first_task));
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_task_return(second_task));

  ASSERT_EQ(gearman_result_value(first_result), gearman_result_value(second_result));

  gearman_task_free(first_task);
  gearman_task_free(second_task);

  return TEST_SUCCESS;
}

test_return_t unique_compare_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  size_t job_length;

  gearman_string_t unique= { test_literal_param("my little unique") };

  void *job_result= gearman_client_do(client,
                                      worker_function, // function
                                      gearman_c_str(unique),  // unique
                                      gearman_string_param(unique), //workload
                                      &job_length, // result size
                                      &rc);

  ASSERT_EQ(rc, GEARMAN_SUCCESS);
  ASSERT_EQ(gearman_size(unique), job_length);
  test_memcmp(gearman_c_str(unique), job_result, job_length);

  free(job_result);

  return TEST_SUCCESS;
}

test_return_t gearman_client_unique_status_TEST(void *object)
{
  gearman_client_st *original_client= (gearman_client_st *)object;

  libgearman::Client status_client(original_client);

  libgearman::Client client_one(original_client);
  libgearman::Client client_two(original_client);
  libgearman::Client client_three(original_client);

  const char* unique_handle= "local_handle4";

  gearman_return_t ret;
  // First task
  gearman_task_st *first_task= gearman_client_add_task_background(&client_one,
                                                                  NULL, // preallocated task
                                                                  NULL, // context 
                                                                  __func__, // function
                                                                  unique_handle, // unique
                                                                  test_literal_param("first_task"), // workload
                                                                  &ret);
  ASSERT_TRUE(first_task);

  gearman_task_st *second_task= gearman_client_add_task_background(&client_two,
                                                                   NULL, // preallocated task
                                                                   NULL, // context 
                                                                   __func__, // function
                                                                   unique_handle, // unique
                                                                   test_literal_param("second_task"), // workload
                                                                   &ret);
  ASSERT_TRUE(second_task);

  gearman_task_st *third_task= gearman_client_add_task_background(&client_three,
                                                                  NULL, // preallocated task
                                                                  NULL, // context 
                                                                  __func__, // function
                                                                  unique_handle, // unique
                                                                  test_literal_param("third_task"), // workload
                                                                  &ret);
  ASSERT_TRUE(third_task);

  {
    ASSERT_EQ(gearman_client_run_tasks(&client_one), GEARMAN_SUCCESS);
    ASSERT_EQ(gearman_client_run_tasks(&client_two), GEARMAN_SUCCESS);
    ASSERT_EQ(gearman_client_run_tasks(&client_three), GEARMAN_SUCCESS);
  }

  ASSERT_EQ(gearman_client_job_status(&status_client,
                                         gearman_task_job_handle(third_task), // job handle
                                         NULL, // is_known
                                         NULL, // is_running
                                         NULL, // numerator
                                         NULL // denominator
                                         ), GEARMAN_JOB_EXISTS);

  {
    libgearman::Client unique_client(original_client);
    gearman_status_t status= gearman_client_unique_status(&unique_client,
                                                          unique_handle, strlen(unique_handle));
    ASSERT_EQ(GEARMAN_SUCCESS, gearman_status_return(status));
  }

  {
    gearman_status_t status= gearman_client_unique_status(&client_one,
                                                          unique_handle, strlen(unique_handle));
    ASSERT_EQ(GEARMAN_SUCCESS, gearman_status_return(status));
    ASSERT_EQ(true, gearman_status_is_known(status));
    ASSERT_EQ(false, gearman_status_is_running(status));
    test_zero(gearman_status_numerator(status));
    test_zero(gearman_status_denominator(status));
  }

  gearman_function_t func= gearman_function_create_v2(echo_or_react_worker_v2);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(), NULL,
                                                             __func__,
                                                             func, NULL, gearman_worker_options_t()));

  {
    ASSERT_EQ(gearman_client_run_tasks(&client_one), GEARMAN_SUCCESS);
    ASSERT_EQ(gearman_client_run_tasks(&client_two), GEARMAN_SUCCESS);
    ASSERT_EQ(gearman_client_run_tasks(&client_three), GEARMAN_SUCCESS);
  }

  gearman_task_free(first_task);
  gearman_task_free(second_task);
  gearman_task_free(third_task);

  return TEST_SUCCESS;
}

/*
  Regression test for #520: a GET_STATUS_UNIQUE request must not consume a
  created_id slot, otherwise the JOB_CREATED of a following background submit
  on the same connection is never matched and the client hangs.
*/
test_return_t gearman_client_unique_status_then_do_background_TEST(void *object)
{
  gearman_client_st *original_client= (gearman_client_st *)object;

  libgearman::Client client(original_client);
  gearman_client_set_timeout(&client, 2000);

  const char* unique_handle= YATL_UNIQUE;

  {
    gearman_status_t status= gearman_client_unique_status(&client,
                                                          unique_handle, strlen(unique_handle));
    ASSERT_EQ(GEARMAN_SUCCESS, gearman_status_return(status));
    ASSERT_EQ(false, gearman_status_is_known(status));
  }

  gearman_job_handle_t job_handle;
  ASSERT_EQ(GEARMAN_SUCCESS,
            gearman_client_do_background(&client,
                                         __func__, // function
                                         unique_handle, // unique
                                         test_literal_param("do_background"), // workload
                                         job_handle));
  ASSERT_TRUE(job_handle[0]);

  return TEST_SUCCESS;
}

namespace {

/*
  Minimal single-connection stand-in for a server that does not implement
  GET_STATUS_UNIQUE: it answers that request with ERROR and answers
  SUBMIT_JOB_BG with JOB_CREATED.
*/
struct error_on_status_unique_server_st
{
  int listen_fd;
  in_port_t port;
  pthread_t thread;
};

bool read_exact(int fd, void* buffer, size_t length)
{
  char* ptr= static_cast<char*>(buffer);
  while (length)
  {
    ssize_t read_length= recv(fd, ptr, length, 0);
    if (read_length <= 0)
    {
      return false;
    }
    ptr+= read_length;
    length-= size_t(read_length);
  }

  return true;
}

bool send_exact(int fd, const void* buffer, size_t length)
{
  const char* ptr= static_cast<const char*>(buffer);
  while (length)
  {
    ssize_t sent_length= send(fd, ptr, length, MSG_NOSIGNAL);
    if (sent_length <= 0)
    {
      return false;
    }
    ptr+= sent_length;
    length-= size_t(sent_length);
  }

  return true;
}

bool send_response(int fd, gearman_command_t command, const char* data, uint32_t data_size)
{
  char header[12]= { '\0', 'R', 'E', 'S' };
  uint32_t value= htonl(uint32_t(command));
  memcpy(header +4, &value, sizeof(value));
  value= htonl(data_size);
  memcpy(header +8, &value, sizeof(value));

  return send_exact(fd, header, sizeof(header)) and
         send_exact(fd, data, data_size);
}

void* error_on_status_unique_server_run(void* object)
{
  error_on_status_unique_server_st* server= static_cast<error_on_status_unique_server_st*>(object);

  int fd= accept(server->listen_fd, NULL, NULL);
  if (fd == -1)
  {
    return NULL;
  }

  char header[12];
  while (read_exact(fd, header, sizeof(header)))
  {
    uint32_t command;
    uint32_t data_size;
    memcpy(&command, header +4, sizeof(command));
    memcpy(&data_size, header +8, sizeof(data_size));
    command= ntohl(command);
    data_size= ntohl(data_size);

    std::vector<char> data(data_size);
    if (data_size and read_exact(fd, &data[0], data_size) == false)
    {
      break;
    }

    if (command == GEARMAN_COMMAND_GET_STATUS_UNIQUE)
    {
      static const char error[]= "ERR_UNKNOWN_COMMAND\0Unknown+server+command";
      send_response(fd, GEARMAN_COMMAND_ERROR, error, sizeof(error) -1);
    }
    else if (command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
    {
      static const char job_handle[]= "H:stub:1";
      send_response(fd, GEARMAN_COMMAND_JOB_CREATED, job_handle, sizeof(job_handle) -1);
    }
  }

  close(fd);
  return NULL;
}

bool error_on_status_unique_server_start(error_on_status_unique_server_st& server)
{
  server.listen_fd= socket(AF_INET, SOCK_STREAM, 0);
  if (server.listen_fd == -1)
  {
    return false;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family= AF_INET;
  addr.sin_addr.s_addr= htonl(INADDR_LOOPBACK);
  socklen_t addr_length= sizeof(addr);

  if (bind(server.listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1 or
      listen(server.listen_fd, 1) == -1 or
      getsockname(server.listen_fd, (struct sockaddr*)&addr, &addr_length) == -1)
  {
    close(server.listen_fd);
    return false;
  }
  server.port= ntohs(addr.sin_port);

  if (pthread_create(&server.thread, NULL, error_on_status_unique_server_run, &server) != 0)
  {
    close(server.listen_fd);
    return false;
  }

  return true;
}

} // namespace

/*
  An ERROR in reply to GET_STATUS_UNIQUE (a server without GET_STATUS_UNIQUE
  support) has no created_id slot to release, so it must not advance created_id:
  otherwise the following background submit's JOB_CREATED is never matched.
*/
test_return_t gearman_client_unique_status_ERROR_then_do_background_TEST(void *)
{
  error_on_status_unique_server_st server;
  ASSERT_TRUE(error_on_status_unique_server_start(server));

  {
    libgearman::Client client;
    // The stub speaks plain TCP only.
    gearman_client_remove_options(&client, GEARMAN_CLIENT_SSL);
    gearman_client_set_timeout(&client, 2000);
    ASSERT_EQ(GEARMAN_SUCCESS, gearman_client_add_server(&client, "127.0.0.1", server.port));

    const char* unique_handle= YATL_UNIQUE;

    gearman_status_t status= gearman_client_unique_status(&client,
                                                          unique_handle, strlen(unique_handle));
    ASSERT_TRUE(gearman_failed(gearman_status_return(status)));

    gearman_job_handle_t job_handle;
    ASSERT_EQ(GEARMAN_SUCCESS,
              gearman_client_do_background(&client,
                                           __func__, // function
                                           unique_handle, // unique
                                           test_literal_param("do_background"), // workload
                                           job_handle));
    ASSERT_STREQ("H:stub:1", job_handle);
  }

  // The client has disconnected, so the server thread's read loop ends.
  pthread_join(server.thread, NULL);
  close(server.listen_fd);

  return TEST_SUCCESS;
}

test_return_t gearman_client_unique_status_NOT_FOUND_TEST(void *object)
{
  gearman_client_st *original_client= (gearman_client_st *)object;

  libgearman::Client status_client(original_client);
  const char* unique_handle= YATL_UNIQUE;

  gearman_function_t func= gearman_function_create_v2(echo_or_react_worker_v2);
  std::unique_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(), NULL,
                                                             __func__,
                                                             func, NULL, gearman_worker_options_t()));


  {
    gearman_status_t status= gearman_client_unique_status(&status_client,
                                                          unique_handle, strlen(unique_handle));
    ASSERT_EQ(GEARMAN_SUCCESS, gearman_status_return(status));
    ASSERT_EQ(false, gearman_status_is_known(status));
    ASSERT_EQ(false, gearman_status_is_running(status));
    test_zero(gearman_status_numerator(status));
    test_zero(gearman_status_denominator(status));
  }

  return TEST_SUCCESS;
}
