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

#include <config.h>

#include <libtest/test.hpp>
using namespace libtest;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>

#define GEARMAN_CORE
#include <libgearman/gearman.h>

#include <tests/start_worker.h>

#define NAMESPACE_KEY "foo123"

#define WORKER_FUNCTION_NAME "client_test"
#define WORKER_CHUNKED_FUNCTION_NAME "reverse_test"
#define WORKER_UNIQUE_FUNCTION_NAME "unique_test"
#define WORKER_SPLIT_FUNCTION_NAME "split_worker"

#include <tests/limits.h>
#include <tests/do.h>
#include <tests/server_options.h>
#include <tests/do_background.h>
#include <tests/execute.h>
#include <tests/gearman_client_do_job_handle.h>
#include <tests/gearman_execute_partition.h>
#include <tests/protocol.h>
#include <tests/regression.h>
#include <tests/task.h>
#include <tests/unique.h>
#include <tests/workers.h>
#include <tests/workers_v1.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

struct client_test_st
{
  gearman_client_st *_client;
  bool _clone;
  std::vector<worker_handle_st *> workers;
  const char *_worker_name;

  client_test_st() :
    _clone(true),
    _worker_name(WORKER_FUNCTION_NAME)
  { 
    _client= gearman_client_create(NULL);

    if (_client == NULL)
    {
      throw "gearman_client_create() failed";
    }
  }

  ~client_test_st()
  {
    for (std::vector<worker_handle_st *>::iterator iter= workers.begin(); iter != workers.end(); iter++)
    {
      delete *iter;
    }
    workers.clear();
    gearman_client_free(_client);
  }

  void push(worker_handle_st *arg)
  {
    workers.push_back(arg);
  }

  const char *worker_name() const
  {
    return _worker_name;
  }

  void set_worker_name(const char *arg)
  {
    _worker_name= arg;
  }

  void set_clone(bool arg)
  {
    _clone= arg;
  }

  bool clone() const
  {
    return _clone;
  }

  gearman_client_st *client()
  {
    return _client;
  }

  void reset_client()
  {
    gearman_client_free(_client);
    _client= gearman_client_create(NULL);
  }
};

#include <tests/runner.h>

class Client {
public:
  Client()
  {
    _client= gearman_client_create(NULL);

    if (_client == NULL)
    {
      throw "gearman_client_create() failed";
    }
  }

  gearman_client_st* operator&() const
  { 
    return _client;
  }

  gearman_client_st* operator->() const
  { 
    return _client;
  }

  ~Client()
  {
    gearman_client_free(_client);
  }

private:
  gearman_client_st *_client;

};

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/**
  @note Just here until I fix libhashkit.
*/
static uint32_t internal_generate_hash(const char *key, size_t key_length)
{
  const char *ptr= key;
  uint32_t value= 0;

  while (key_length--)
  {
    uint32_t val= (uint32_t) *ptr++;
    value += val;
    value += (value << 10);
    value ^= (value >> 6);
  }
  value += (value << 3);
  value ^= (value >> 11);
  value += (value << 15);

  return value == 0 ? 1 : (uint32_t) value;
}

/* Prototypes */
void *client_test_temp_worker(gearman_job_st *job, void *context,
                              size_t *result_size, gearman_return_t *ret_ptr);


extern "C"
{
  static void *client_thread(void *object)
  {
    volatile gearman_return_t *ret= (volatile gearman_return_t *)object;
    gearman_client_st client;
    gearman_client_st *client_ptr;
    size_t result_size;

    client_ptr= gearman_client_create(&client);

    if (client_ptr == NULL)
    {
      *ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      pthread_exit(0);
    }

    gearman_return_t rc= gearman_client_add_server(&client, NULL, libtest::default_port());
    if (gearman_failed(rc))
    {
      *ret= rc;
      pthread_exit(0);
    }

    gearman_client_set_timeout(&client, 400);
    for (size_t x= 0; x < 5; x++)
    {
      (void)gearman_client_do(&client, "client_test_temp", NULL, NULL, 0, &result_size, &rc);

      if (gearman_failed(rc))
      {
        *ret= rc;
        pthread_exit(0);
      }
    }
    gearman_client_free(client_ptr);

    pthread_exit(0);
  }
}

#if 0
static void log_2_stderr(const char *line, gearman_verbose_t verbose, void*)
{
  Error << line << " " << gearman_verbose_name(verbose);
}
#endif

static test_return_t init_test(void *)
{
  gearman_client_st client;

  test_truth(gearman_client_create(&client));

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

static test_return_t allocation_test(void *)
{
  gearman_client_st *client;

  test_truth(client= gearman_client_create(NULL));

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t clone_test(void *object)
{
  const gearman_client_st *from= (gearman_client_st *)object;

  gearman_client_st *client= gearman_client_clone(NULL, NULL);

  test_truth(client);
  test_truth(client->options.allocated);

  gearman_client_free(client);

  client= gearman_client_clone(NULL, from);
  test_truth(client);
  gearman_client_free(client);

  gearman_client_st *from_with_host= gearman_client_create(NULL);
  test_truth(from_with_host);
  gearman_client_add_server(from_with_host, "localhost", 12345);

  client= gearman_client_clone(NULL, from_with_host);
  test_truth(client);

  test_truth(client->universal.con_list);
  test_truth(gearman_client_compare(client, from_with_host));

  gearman_client_free(client);
  gearman_client_free(from_with_host);

  return TEST_SUCCESS;
}

static test_return_t option_test(void *)
{
  gearman_client_st *gear;
  gearman_client_options_t default_options;

  gear= gearman_client_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_false(gear->options.unbuffered_result);
    test_false(gear->options.no_new);
    test_false(gear->options.free_tasks);
  }

  /* Set up for default options */
  default_options= gearman_client_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_client_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_false(gear->options.unbuffered_result);
    test_false(gear->options.no_new);
    test_false(gear->options.free_tasks);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_client_remove_options(gear, GEARMAN_CLIENT_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_remove_options(gear, GEARMAN_CLIENT_NO_NEW);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
  }

  /*
    We will test modifying GEARMAN_CLIENT_NON_BLOCKING in several manners.
  */
  {
    gearman_client_remove_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_add_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_set_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_set_options(gear, GEARMAN_CLIENT_UNBUFFERED_RESULT);
    { // Everything is now set to false except GEARMAN_CLIENT_UNBUFFERED_RESULT, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_client_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_false(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_FREE_TASKS);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, (gearman_client_options_t)(GEARMAN_CLIENT_NON_BLOCKING|GEARMAN_CLIENT_UNBUFFERED_RESULT));
      { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_truth(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_client_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_false(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_FREE_TASKS);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, (gearman_client_options_t)(GEARMAN_CLIENT_FREE_TASKS|GEARMAN_CLIENT_UNBUFFERED_RESULT));
      { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
    }
  }

  gearman_client_free(gear);

  return TEST_SUCCESS;
}

static test_return_t echo_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  gearman_string_t value= { test_literal_param("This is my echo test") };

  test_compare(GEARMAN_SUCCESS, gearman_client_echo(client, gearman_string_param(value)));

  return TEST_SUCCESS;
}

static test_return_t submit_job_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  test_true(worker_function);
  gearman_string_t value= { test_literal_param("submit_job_test") };

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client, worker_function, NULL, gearman_string_param(value), &result_length, &rc);

  test_compare_got(GEARMAN_SUCCESS, 
                   rc, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));

  test_truth(job_result);
  test_compare(gearman_size(value), result_length);

  test_memcmp(gearman_c_str(value), job_result, gearman_size(value));

  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_null_job_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  test_truth(client);

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client, worker_function, NULL, NULL, 0,
                                      &result_length, &rc);
  test_compare_got(GEARMAN_SUCCESS, rc, gearman_client_error(client));
  test_zero(result_length);
  test_false(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_exception_job_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client, worker_function, NULL,
                                      test_literal_param("exception"),
                                      &result_length, &rc);
  test_compare_got(GEARMAN_SUCCESS, rc, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_memcmp("exception", job_result, result_length);
  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_warning_job_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client, worker_function, NULL,
                                      test_literal_param("warning"),
                                      &result_length, &rc);
  test_compare_got(GEARMAN_SUCCESS, rc, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_memcmp("warning", job_result, result_length);
  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_fail_job_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);


  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client, worker_function, NULL, "fail", 4,
                                      &result_length, &rc);
  test_compare_got(GEARMAN_WORK_FAIL, rc, gearman_client_error(client));
  test_false(job_result);
  test_false(result_length);

  return TEST_SUCCESS;
}

static test_return_t submit_multiple_do(void *object)
{
  for (uint32_t x= 0; x < 100 /* arbitrary */; x++)
  {
    uint32_t option= random() %3;

    switch (option)
    {
    case 0:
      test_compare(TEST_SUCCESS, submit_null_job_test(object));
      break;
    case 1:
      test_compare(TEST_SUCCESS, submit_job_test(object));
      break;
    default:
    case 2:
      test_compare(TEST_SUCCESS, submit_fail_job_test(object));
      break;
    }
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_client_job_status_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  gearman_string_t value= { test_literal_param("background_test") };

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  gearman_job_handle_t job_handle;
  test_compare_got(GEARMAN_SUCCESS,
                   gearman_client_do_background(client, worker_function, NULL, gearman_string_param(value), job_handle), 
                   gearman_client_error(client));

  gearman_return_t ret;
  bool is_known;
  do
  {
    bool is_running;
    uint32_t numerator;
    uint32_t denominator;

    test_compare_got(GEARMAN_SUCCESS,
                     ret= gearman_client_job_status(client, job_handle, &is_known, &is_running, &numerator, &denominator),
                     gearman_client_error(client));
  } while (gearman_continue(ret) and is_known);

  return TEST_SUCCESS;
}

static void* test_malloc_fn(size_t size, void *context)
{
  bool *malloc_check= (bool *)context;
  *malloc_check= true;
  return malloc(size);
}

static void test_free_fn(void *ptr, void *context)
{
  bool *free_check= (bool *)context;
  *free_check= true;
  return free(ptr);
}

static test_return_t gearman_client_set_workload_malloc_fn_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  bool malloc_check= false;
  gearman_client_set_workload_malloc_fn(client, test_malloc_fn, &malloc_check);

  test_compare(TEST_SUCCESS, submit_job_test(object));
  test_compare(true, malloc_check);

  return TEST_SUCCESS;
}

static test_return_t gearman_client_set_workload_free_fn_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  bool free_check= false;
  gearman_client_set_workload_free_fn(client, test_free_fn, &free_check);

  test_compare(TEST_SUCCESS, submit_job_test(object));
  test_compare(true, free_check);

  return TEST_SUCCESS;
}

struct _alloc_test_st {
  int64_t count;
  int64_t total;

  _alloc_test_st():
    count(0),
    total(0)
  { }

  void add() 
  {
    count++;
    total++;
  }

  void subtract() 
  {
    count--;
  }

  bool success() // count is valid as 1 only with the current test
  {
    if (total and count == 1)
      return true;

    std::cerr << __func__ << ":" << __LINE__ << " Total:" <<  total << " Count:" << count << std::endl;

    return false;
  }
};

static void* test_malloc_count_fn(size_t size, void *context)
{
  _alloc_test_st *_foo= (_alloc_test_st *)context;

  _foo->add();
  
  return malloc(size);
}

static void test_free_count_fn(void *ptr, void *context)
{
  _alloc_test_st *_foo= (_alloc_test_st *)context;

  _foo->subtract();

  return free(ptr);
}


static test_return_t gearman_client_set_workload_allocators_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  _alloc_test_st _foo;

  gearman_client_set_workload_malloc_fn(client, test_malloc_count_fn, &_foo);
  gearman_client_set_workload_free_fn(client, test_free_count_fn, &_foo);

  test_compare(TEST_SUCCESS, submit_job_test(object));
  test_true(_foo.success());

  return TEST_SUCCESS;
}

static test_return_t gearman_client_job_status_with_return(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  gearman_string_t value= { test_literal_param("background_test") };

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  gearman_job_handle_t job_handle;
  test_compare_got(GEARMAN_SUCCESS,
                   gearman_client_do_background(client, worker_function, NULL, gearman_string_param(value), job_handle), 
                   gearman_client_error(client));

  gearman_return_t ret;
  do
  {
    uint32_t numerator;
    uint32_t denominator;

    ret= gearman_client_job_status(client, job_handle, NULL, NULL, &numerator, &denominator);
  } while (gearman_continue(ret));
  test_compare(GEARMAN_SUCCESS, ret);

  return TEST_SUCCESS;
}

static test_return_t background_failure_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  gearman_job_handle_t job_handle;
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;

  gearman_return_t rc= gearman_client_do_background(client, "does_not_exist", NULL,
                                                    test_literal_param("background_failure_test"),
                                                    job_handle);
  test_compare_got(GEARMAN_SUCCESS, rc, gearman_client_error(client));

  do {
    rc= gearman_client_job_status(client, job_handle, &is_known, &is_running,
                                  &numerator, &denominator);
    test_true(is_known == true and is_running == false and numerator == 0 and denominator == 0);
  } while (gearman_continue(rc)); // We do not test for is_known since the server will keep the job around until a worker comes along
  test_compare(GEARMAN_SUCCESS, rc);

  return TEST_SUCCESS;
}

static test_return_t add_servers_test(void *)
{
  gearman_client_st *client= gearman_client_create(NULL);
  test_truth(client);

  test_compare_got(GEARMAN_SUCCESS,
                   gearman_client_add_servers(client, "localhost:4730,localhost"),
                   gearman_client_error(client));

  test_compare_got(GEARMAN_GETADDRINFO,
                   gearman_client_add_servers(client, "exist.gearman.info:7003,does_not_exist.gearman.info:12345"),
                   gearman_client_error(client));

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t hostname_resolution(void *)
{
  Client client;
  assert(&client);

  test_skip(GEARMAN_SUCCESS, gearman_client_add_servers(&client, "exist.gearman.info:12345"));

  test_compare(GEARMAN_SUCCESS, client->universal.error.rc);

  test_compare_hint(GEARMAN_COULD_NOT_CONNECT,
                    gearman_client_echo(&client, test_literal_param("foo")),
                    gearman_client_error(&client));

  return TEST_SUCCESS;
}

static test_return_t gearman_client_st_id_t_TEST(gearman_client_st*)
{
  return TEST_SKIPPED;

  // @todo, test the internal value of client, no API is available for this
  // right now.
#if 0
  gearman_client_st *client= gearman_client_create(NULL);
  test_true(client);
  test_compare(false, gearman_id_valid(client));
  gearman_client_free(client);

  return TEST_SUCCESS;
#endif
}

static test_return_t gearman_worker_st_id_t_TEST(gearman_client_st*)
{
  gearman_worker_st *worker= gearman_worker_create(NULL);
  test_true(worker);
  gearman_id_t id= gearman_worker_id(worker);
  test_compare(true, gearman_id_valid(id));
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t bug_518512_test(void *)
{
  gearman_client_st client;
  size_t result_size;

  test_truth(gearman_client_create(&client));

  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(&client, NULL, libtest::default_port()));

  gearman_client_set_timeout(&client, 0);
  gearman_return_t rc;
  void *result= gearman_client_do(&client, "client_test_temp", NULL, NULL, 0,
                                  &result_size, &rc);
  test_compare_got(GEARMAN_TIMEOUT, rc, gearman_strerror(rc));
  test_false(result);
  test_zero(result_size);

  gearman_function_t func_arg= gearman_function_create_v1(client_test_temp_worker);
  struct worker_handle_st *completion_worker= test_worker_start(libtest::default_port(), NULL, "client_test_temp",
                                                                func_arg, NULL, gearman_worker_options_t());

  test_true(completion_worker);

  gearman_client_set_timeout(&client, -1);
  result= gearman_client_do(&client, "client_test_temp", NULL, NULL, 0,
                            &result_size, &rc);
  test_true_got(rc != GEARMAN_TIMEOUT, gearman_strerror(rc));
  (void)result;

  delete completion_worker;
  gearman_client_free(&client);

  return TEST_SUCCESS;
}

#define NUMBER_OF_WORKERS 2

static test_return_t loop_test(void *)
{
  pthread_t one;
  pthread_t two;

  struct worker_handle_st *handles[NUMBER_OF_WORKERS];
  gearman_function_t func_arg= gearman_function_create_v1(client_test_temp_worker);
  for (size_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    handles[x]= test_worker_start(libtest::default_port(), NULL, "client_test_temp",
                                  func_arg, NULL, gearman_worker_options_t());
  }

  gearman_return_t one_rc= GEARMAN_SUCCESS;
  pthread_create(&one, NULL, client_thread, &one_rc);

  gearman_return_t two_rc= GEARMAN_SUCCESS;
  pthread_create(&two, NULL, client_thread, &two_rc);

  void *unused;
  pthread_join(one, &unused);
  pthread_join(two, &unused);

  for (size_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    delete handles[x];
  }

  test_compare(GEARMAN_SUCCESS, one_rc);
  test_compare(GEARMAN_SUCCESS, two_rc);

  return TEST_SUCCESS;
}

static test_return_t regression_785203_do_test(void *object)
{
  gearman_client_st *original_client= (gearman_client_st *)object;
  test_true(original_client);

  const char *worker_function= (const char *)gearman_client_context(original_client);
  test_truth(original_client);

  gearman_client_st *client;
  test_truth(client= gearman_client_create(NULL));

  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(client, NULL, libtest::default_port()));

  gearman_client_add_options(client, GEARMAN_CLIENT_FREE_TASKS);
  { // All defaults, except timeout_return
    test_truth(client->options.allocated);
    test_false(client->options.non_blocking);
    test_false(client->options.unbuffered_result);
    test_false(client->options.no_new);
    test_truth(client->options.free_tasks);
  }

  gearman_return_t rc;
  size_t result_length;
  void *result= gearman_client_do(client, worker_function, NULL, 
                                  test_literal_param("keep it rocking and sing"),
                                  &result_length, &rc);
  test_true(result);
  free(result);

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t regression_785203_do_background_test(void *object)
{
  gearman_client_st *original_client= (gearman_client_st *)object;
  test_true(original_client);

  const char *worker_function= (const char *)gearman_client_context(original_client);
  test_truth(worker_function);

  gearman_client_st *client;
  test_truth(client= gearman_client_create(NULL));

  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(client, NULL, libtest::default_port()));

  gearman_client_add_options(client, GEARMAN_CLIENT_FREE_TASKS);
  { // All defaults, except timeout_return
    test_truth(client->options.allocated);
    test_false(client->options.non_blocking);
    test_false(client->options.unbuffered_result);
    test_false(client->options.no_new);
    test_truth(client->options.free_tasks);
  }

  gearman_job_handle_t job_handle;
  test_compare_got(GEARMAN_SUCCESS,
                   gearman_client_do_background(client, worker_function, 
                                                NULL,  // No unique requested
                                                test_literal_param("keep it rocking and sing"),
                                                job_handle), 
                   gearman_client_error(client));

  gearman_return_t ret;
  do
  {
    uint32_t numerator;
    uint32_t denominator;

    ret= gearman_client_job_status(client, job_handle, NULL, NULL, &numerator, &denominator);
  } while (gearman_continue(ret));
  test_compare(GEARMAN_SUCCESS, ret);

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t regression2_TEST(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client,
                                      worker_function, // default worker function
                                      NULL,  // no unique
                                      test_literal_param("submit_log_failure"),
                                      &result_length, &rc);
  test_compare(GEARMAN_NO_SERVERS, rc);
  test_false(job_result);
  test_zero(result_length);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_timeout_TEST(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  test_truth(WORKER_DEFAULT_SLEEP);
  int timeout= WORKER_DEFAULT_SLEEP/4;
  (void)timeout;

  gearman_function_t dreaming_fn= gearman_function_create(echo_or_react_worker_v2);
  worker_handle_st* worker_handle= test_worker_start(libtest::default_port(), NULL,
                                                     __func__,
                                                     dreaming_fn, NULL,
                                                     gearman_worker_options_t(),
                                                     0);

  /*
    The client should get a timeout since the "sleeper" will sleep longer then the timeout.
  */
  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client,
                                      __func__,  // Our sleeper function
                                      NULL, // No unique 
                                      gearman_literal_param("sleep"), // We send "sleep" to tell the sleeper to sleep
                                      &result_length, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_true(job_result);
  test_compare(sizeof("slept") -1, result_length);
  test_memcmp("slept", job_result, 5);
  free(job_result);

  delete worker_handle;

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_timeout_TIMEOUT_TEST(void *object)
{
  // This test currently takes to long.
  test_skip_valgrind();

  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);

  test_truth(WORKER_DEFAULT_SLEEP);
  int timeout= WORKER_DEFAULT_SLEEP/4;

  gearman_function_t dreaming_fn= gearman_function_create(echo_or_react_worker_v2);
  worker_handle_st* worker_handle= test_worker_start(libtest::default_port(), NULL,
                                                     __func__,
                                                     dreaming_fn, NULL,
                                                     gearman_worker_options_t(),
                                                     timeout);

  /*
    The client should get a timeout since the "sleeper" will sleep longer then the timeout.
  */
  size_t result_length;
  gearman_return_t rc;
  void *job_result= gearman_client_do(client,
                                      __func__,  // Our sleeper function
                                      NULL, // No unique 
                                      gearman_literal_param("sleep"), // We send "sleep" to tell the sleeper to sleep
                                      &result_length, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_true(job_result);
  test_compare(sizeof("slept") -1, result_length);
  test_memcmp("slept", job_result, 5);
  free(job_result);

  delete worker_handle;

  return TEST_SUCCESS;
}

static test_return_t regression_975591_TEST(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_true(client);

  gearman_function_t dreaming_fn= gearman_function_create(echo_or_react_worker_v2);
  worker_handle_st* worker_handle= test_worker_start(libtest::default_port(), NULL,
                                                     __func__,
                                                     dreaming_fn, NULL,
                                                     gearman_worker_options_t(),
                                                     0);
  size_t payload_size[] = { 100, 1000, 10000, 1000000, 1000000, 0 };
  libtest::vchar_t payload;
  for (size_t *ptr= payload_size; *ptr; ptr++)
  {
    payload.reserve(*ptr);
    for (size_t x= payload.size(); x < *ptr; x++)
    {
      payload.push_back(rand());
    }

    size_t result_length;
    gearman_return_t rc;
    char *job_result= (char*)gearman_client_do(client, __func__,
                                               NULL, 
                                               &payload[0], payload.size(),
                                               &result_length, &rc);
    test_compare(GEARMAN_SUCCESS, rc);
    test_compare(payload.size(), result_length);
    test_memcmp(&payload[0], job_result, result_length);
    free(job_result);
  }

  delete worker_handle;

  return TEST_SUCCESS;
}

static test_return_t submit_log_failure_TEST(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_truth(client);
  gearman_string_t value= { test_literal_param("submit_log_failure") };

  const char *worker_function= (const char *)gearman_client_context(client);
  test_truth(worker_function);

  size_t result_length;
  gearman_return_t rc;
  test_null(client->task);
  void *job_result= gearman_client_do(client, worker_function, NULL, 
                                      gearman_string_param(value),
                                      &result_length, &rc);
  test_compare(GEARMAN_NO_SERVERS, rc);
  test_false(job_result);
  test_zero(result_length);
  test_null(client->task);

  return TEST_SUCCESS;
}

static void log_counter(const char *line, gearman_verbose_t verbose,
                        void *context)
{
  uint32_t *counter= (uint32_t *)context;

  (void)verbose;
  (void)line;

  *counter= *counter + 1;
}

static test_return_t strerror_count(void *)
{
  test_compare((int)GEARMAN_MAX_RETURN, 51);

  return TEST_SUCCESS;
}

#undef MAKE_NEW_STRERROR

static char * make_number(uint32_t expected, uint32_t got)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "Expected %uU, got %uU", expected, got);

  return strdup(buffer);
}

static test_return_t strerror_strings(void *)
{
  uint32_t values[]= {
    2723107532U, 1294272985U, 949848612U, 646434617U, 
    2273096667U, 3411376012U, 978198404U, 2644287234U, 
    1762137345U, 1727436301U, 1103093142U, 2958899803U, 
    3844590487U, 3520316764U, 3288532333U, 697573278U, 
    2328987341U, 1321921098U, 1475770122U, 4011631587U, 
    2468981698U, 2935753385U, 884320816U, 3006705975U, 
    2840498210U, 2953034368U, 501858685U, 1635925784U, 
    880765771U, 15612712U, 1489284002U, 2968621609U, 
    79936336U, 3059874010U, 3562217099U, 13337402U, 
    132823274U, 3950859856U, 237150774U, 290535510U, 
    2101976744U, 2262698284U, 3182950564U, 2391595326U, 
    1764731897U, 3485422815U, 99607280U, 2348849961U, 
    607991020U, 1597605008U, 1377573125U };

  for (int rc= GEARMAN_SUCCESS; rc < GEARMAN_MAX_RETURN; rc++)
  {
    char *make_number_str;
    uint32_t hash_val;
    const char *msg=  gearman_strerror((gearman_return_t)rc);
    hash_val= internal_generate_hash(msg, strlen(msg));
    test_compare_got(values[rc], hash_val, make_number_str= make_number(values[rc], hash_val));
    free(make_number_str);
  }

  return TEST_SUCCESS;
}

#define REGRESSION_FUNCTION_833394_54 "54_char_function_name________________________________"
#define REGRESSION_FUNCTION_833394_55 "55_char_function_name_________________________________"
#define REGRESSION_FUNCTION_833394_65 "65_char_function_name___________________________________________"

static test_return_t regression_833394_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  for (size_t x= 0; x < 100; x++)
  {
    test_true(client);
    size_t result_length;
    gearman_return_t rc;
    char *job_result= (char*)gearman_client_do(client, REGRESSION_FUNCTION_833394_54, 
                                               NULL, 
                                               test_literal_param("this should be echo'ed"),
                                               &result_length, &rc);
    test_compare_got(GEARMAN_SUCCESS, rc, gearman_strerror(rc));
    test_true(job_result);
    test_compare(test_literal_param_size("this should be echo'ed"), result_length);
    free(job_result);
  }

  for (size_t x= 0; x < 100; x++)
  {
    test_true(client);
    size_t result_length;
    gearman_return_t rc;
    char *job_result= (char*)gearman_client_do(client, REGRESSION_FUNCTION_833394_55, 
                                               NULL, 
                                               test_literal_param("this should be echo'ed"),
                                               &result_length, &rc);
    test_compare_got(GEARMAN_SUCCESS, rc, gearman_strerror(rc));
    test_true(job_result);
    test_compare(test_literal_param_size("this should be echo'ed"), result_length);
    free(job_result);
  }

  for (size_t x= 0; x < 100; x++)
  {
    test_true(client);
    size_t result_length;
    gearman_return_t rc;
    char *job_result= (char*)gearman_client_do(client, REGRESSION_FUNCTION_833394_65, 
                                               NULL, 
                                               test_literal_param("this should be echo'ed"),
                                               &result_length, &rc);
    test_compare_got(GEARMAN_SUCCESS, rc, gearman_strerror(rc));
    test_true(job_result);
    test_compare(test_literal_param_size("this should be echo'ed"), result_length);
    free(job_result);
  }

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_SUCCESS_TEST(void*)
{
  test_compare(0, int(GEARMAN_SUCCESS));

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_FAIL_COMPAT_TEST(void*)
{
  test_compare(GEARMAN_FAIL, GEARMAN_FATAL);
  test_compare(GEARMAN_FAIL, GEARMAN_WORK_FAIL);

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_ERROR_COMPAT_TEST(void*)
{
  test_compare(GEARMAN_ERROR, GEARMAN_WORK_ERROR);

  return TEST_SUCCESS;
}

static test_return_t gearman_client_set_identifier_TEST(void* object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  test_compare(GEARMAN_SUCCESS,
               gearman_client_set_identifier(client, test_literal_param(__func__)));

  return TEST_SUCCESS;
}

static test_return_t gearman_client_set_identifier_plus_work_TEST(void* object)
{
  test_compare(TEST_SUCCESS, gearman_client_set_identifier_TEST(object));
  test_compare(TEST_SUCCESS, regression_833394_test(object));

  return TEST_SUCCESS;
}

static test_return_t gearman_client_set_identifier_plus_random_TEST(void* object)
{
  for (size_t x= 0; x < 10; x++)
  {
    switch (random() %3)
    {
    case 0:
      test_compare(TEST_SUCCESS, gearman_client_set_identifier_TEST(object));
      break;

    case 1:
      test_compare(TEST_SUCCESS, regression_833394_test(object));
      break;

    default:
      test_compare(TEST_SUCCESS, regression_975591_TEST(object));
    }
  }

  return TEST_SUCCESS;
}

static uint32_t global_counter;

static test_return_t pre_chunk(void *object)
{
  client_test_st *all= (client_test_st *)object;

  all->set_worker_name(WORKER_CHUNKED_FUNCTION_NAME);

  return TEST_SUCCESS;
}

static test_return_t pre_v2(void *object)
{
  client_test_st *all= (client_test_st *)object;

  all->set_worker_name(WORKER_FUNCTION_NAME"_v2");

  return TEST_SUCCESS;
}

static test_return_t pre_chunk_v2(void *object)
{
  client_test_st *all= (client_test_st *)object;

  all->set_worker_name(WORKER_CHUNKED_FUNCTION_NAME"_v2");

  return TEST_SUCCESS;
}

static test_return_t pre_free_tasks(void *object)
{
  client_test_st *all= (client_test_st *)object;

  gearman_client_add_options(all->client(), GEARMAN_CLIENT_FREE_TASKS);

  return TEST_SUCCESS;
}

static test_return_t post_free_tasks(void *object)
{
  client_test_st *all= (client_test_st *)object;

  gearman_client_remove_options(all->client(), GEARMAN_CLIENT_FREE_TASKS);

  return TEST_SUCCESS;
}

static test_return_t pre_namespace(void *object)
{
  client_test_st *all= (client_test_st *)object;

  gearman_client_set_namespace(all->client(), NAMESPACE_KEY, strlen(NAMESPACE_KEY));
  assert(not gearman_client_has_option(all->client(), GEARMAN_CLIENT_FREE_TASKS));

  return TEST_SUCCESS;
}

static test_return_t pre_unique(void *object)
{
  client_test_st *all= (client_test_st *)object;

  all->set_worker_name(WORKER_UNIQUE_FUNCTION_NAME);

  return TEST_SUCCESS;
}

static test_return_t post_function_reset(void *object)
{
  client_test_st *all= (client_test_st *)object;

  all->set_worker_name(WORKER_FUNCTION_NAME);
  gearman_client_set_namespace(all->client(), 0, 0);
  test_false(gearman_client_has_option(all->client(), GEARMAN_CLIENT_FREE_TASKS));

  return TEST_SUCCESS;
}

static test_return_t set_defaults(void *object)
{
  client_test_st *all= (client_test_st *)object;
  test_true(all);

  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(all->client(), NULL, libtest::default_port()));

  return TEST_SUCCESS;
}

static test_return_t pre_logging(void *object)
{
  client_test_st *all= (client_test_st *)object;
  test_true(all);
  gearman_log_fn *func= log_counter;
  global_counter= 0;
  all->reset_client();
  all->set_clone(false);

  gearman_client_set_log_fn(all->client(), func, &global_counter, GEARMAN_VERBOSE_MAX);

  return TEST_SUCCESS;
}

static test_return_t post_logging(void* object)
{
  client_test_st *all= (client_test_st *)object;
  test_true(all);
  test_truth(global_counter);
  all->set_clone(true);

  return TEST_SUCCESS;
}

void *client_test_temp_worker(gearman_job_st *, void *,
                              size_t *result_size, gearman_return_t *ret_ptr)
{
  *result_size= 0;
  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (server_startup(servers, "gearmand", libtest::default_port(), 0, NULL) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }
  (void)pre_namespace;

  client_test_st *test= new client_test_st();

  // Version 1 functions
  gearman_function_t echo_react_fn_v1= gearman_function_create_v1(echo_or_react_worker);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_FUNCTION_NAME, echo_react_fn_v1, NULL, gearman_worker_options_t()));
  gearman_function_t echo_react_chunk_fn_v1= gearman_function_create_v1(echo_or_react_chunk_worker);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_CHUNKED_FUNCTION_NAME, echo_react_chunk_fn_v1, NULL, gearman_worker_options_t()));

  // Version 2 functsions
  gearman_function_t echo_react_fn_v2= gearman_function_create(echo_or_react_worker_v2);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_FUNCTION_NAME"_v2", echo_react_fn_v2, NULL, gearman_worker_options_t()));
  gearman_function_t echo_react_chunk_fn_v2= gearman_function_create(echo_or_react_chunk_worker_v2);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_CHUNKED_FUNCTION_NAME"_v2", echo_react_chunk_fn_v2, NULL, gearman_worker_options_t()));

  gearman_function_t unique_worker_arg= gearman_function_create_v1(unique_worker);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_UNIQUE_FUNCTION_NAME, unique_worker_arg, NULL, GEARMAN_WORKER_GRAB_UNIQ));

  gearman_function_t split_worker_fn= gearman_function_create_partition(split_worker, cat_aggregator_fn);
  test->push(test_worker_start(libtest::default_port(), NULL, WORKER_SPLIT_FUNCTION_NAME, split_worker_fn,  NULL, GEARMAN_WORKER_GRAB_ALL));

  // Namespace versions of the above
  test->push(test_worker_start(libtest::default_port(), NAMESPACE_KEY, WORKER_FUNCTION_NAME, echo_react_fn_v1, NULL, gearman_worker_options_t()));
  test->push(test_worker_start(libtest::default_port(), NAMESPACE_KEY, WORKER_CHUNKED_FUNCTION_NAME, echo_react_chunk_fn_v1, NULL, gearman_worker_options_t()));
  test->push(test_worker_start(libtest::default_port(), NAMESPACE_KEY, WORKER_SPLIT_FUNCTION_NAME, split_worker_fn,  NULL, GEARMAN_WORKER_GRAB_ALL));

  // Long function name
  test->push(test_worker_start(libtest::default_port(), NULL, REGRESSION_FUNCTION_833394_54, echo_react_fn_v2, NULL, gearman_worker_options_t()));
  test->push(test_worker_start(libtest::default_port(), NULL, REGRESSION_FUNCTION_833394_55, echo_react_fn_v2, NULL, gearman_worker_options_t()));
  test->push(test_worker_start(libtest::default_port(), NULL, REGRESSION_FUNCTION_833394_65, echo_react_fn_v2, NULL, gearman_worker_options_t()));

  gearman_function_t increment_reset_worker_fn= gearman_function_create_v1(increment_reset_worker);
  for (uint32_t x= 0; x < 10; x++)
  {
    test->push(test_worker_start(libtest::default_port(), 
                                 NULL,
                                 "increment_reset_worker", increment_reset_worker_fn, 
                                 NULL, gearman_worker_options_t()));
  }

  // Count worker
  gearman_function_t count_worker_fn= gearman_function_create(count_worker);
  test->push(test_worker_start(libtest::default_port(), NULL, "count", count_worker_fn, NULL, gearman_worker_options_t()));

  if (gearman_failed(gearman_client_add_server(test->client(), NULL, libtest::default_port())))
  {
    error= TEST_FAILURE;
    return NULL;
  }

  error= TEST_SUCCESS;

  return (void *)test;
}


static bool world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  delete test;

  return TEST_SUCCESS;
}

test_st gearman_client_set_identifier_TESTS[] ={
  {"gearman_client_set_identifier()", 0, gearman_client_set_identifier_TEST },
  {"gearman_client_set_identifier(with_work)", 0, gearman_client_set_identifier_plus_work_TEST },
  {"gearman_client_set_identifier(RANDOM)", 0, gearman_client_set_identifier_plus_random_TEST },
  {0, 0, 0}
};

test_st gearman_return_t_TESTS[] ={
  {"GEARMAN_SUCCESS", 0, (test_callback_fn*)GEARMAN_SUCCESS_TEST },
  {"GEARMAN_FAIL == GEARMAN_FATAL == GEARMAN_WORK_FAIL", 0, (test_callback_fn*)GEARMAN_FAIL_COMPAT_TEST },
  {"GEARMAN_ERROR == GEARMAN_WORK_ERROR", 0, (test_callback_fn*)GEARMAN_ERROR_COMPAT_TEST },
  {0, 0, 0}
};

test_st gearman_id_t_TESTS[] ={
  {"gearman_client_st", 0, (test_callback_fn*)gearman_client_st_id_t_TEST },
  {"gearman_worker_st", 0, (test_callback_fn*)gearman_worker_st_id_t_TEST },
  {0, 0, 0}
};

test_st gearman_client_st_TESTS[] ={
  {"bug_518512_test", 0, bug_518512_test },
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
  {"options", 0, option_test },
  {"submit_job", 0, submit_job_test },
  {"submit_null_job", 0, submit_null_job_test },
  {"submit_fail_job", 0, submit_fail_job_test },
  {"exception", 0, submit_exception_job_test },
  {"warning", 0, submit_warning_job_test },
  {"submit_multiple_do", 0, submit_multiple_do },
  {"gearman_client_job_status()", 0, gearman_client_job_status_test },
  {"gearman_client_job_status() with gearman_return_t", 0, gearman_client_job_status_with_return },
  {"background_failure", 0, background_failure_test },
  {"add_servers", 0, add_servers_test },
  {"gearman_client_add_servers(GEARMAN_GETADDRINFO)", 0, hostname_resolution },
  {"loop_test", 0, loop_test },
  {0, 0, 0}
};

test_st coalescence_TESTS[] ={
  {"basic coalescence", 0, coalescence_TEST },
  {0, 0, 0}
};

test_st gearman_command_t_tests[] ={
  {"gearman_command_t", 0, check_gearman_command_t },
  {0, 0, 0}
};

test_st gearman_worker_timeout_TESTS[] ={
  {"gearman_worker_timeout(0)", 0, gearman_worker_timeout_TEST },
  {"gearman_worker_timeout(DEFAULT_TIMEOUT)", 0, gearman_worker_timeout_TIMEOUT_TEST },
  {0, 0, 0}
};

test_st tests_log_TESTS[] ={
  {"submit_log_failure", 0, submit_log_failure_TEST },
  {0, 0, 0}
};

test_st regression2_TESTS[] ={
  {"stale client", 0, regression2_TEST },
  {0, 0, 0}
};


test_st gearman_strerror_tests[] ={
  {"count", 0, strerror_count },
  {"strings", 0, strerror_strings },
  {0, 0, 0}
};

test_st unique_tests[] ={
  {"compare sent unique", 0, unique_compare_test },
  {"gearman_client_unique_status()", 0, gearman_client_unique_status_TEST },
  {0, 0, 0}
};

test_st gearman_client_set_workload_malloc_fn_tests[] ={
  {"gearman_client_set_workload_malloc_fn()", 0, gearman_client_set_workload_malloc_fn_test },
  {"gearman_client_set_workload_free_fn()", 0, gearman_client_set_workload_free_fn_test },
  {"submit job and check for usage of both malloc/free", 0, gearman_client_set_workload_allocators_test },
  {0, 0, 0}
};

test_st regression_tests[] ={
#if 0
  {"lp:768317", 0, regression_768317_test },
#endif
  {"lp:785203 gearman_client_do()", 0, regression_785203_do_test },
  {"lp:785203 gearman_client_do_background()", 0, regression_785203_do_background_test },
  {"lp:833394 long function names", 0, regression_833394_test },
  {"lp:975591 Increase the work size past the standard buffer size", 0, regression_975591_TEST },
  {0, 0, 0}
};

test_st gearman_client_do_tests[] ={
  {"gearman_client_do() fail huge unique", 0, gearman_client_do_huge_unique },
  {"gearman_client_do() with active background task", 0, gearman_client_do_with_active_background_task },
  {0, 0, 0}
};

test_st gearman_execute_tests[] ={
  {"gearman_execute()", 0, gearman_execute_test },
  {"gearman_execute(GEARMAN_WORK_FAIL)", 0, gearman_execute_fail_test },
  {"gearman_execute() epoch", 0, gearman_execute_epoch_test },
  {"gearman_execute() epoch and test gearman_job_handle_t", 0, gearman_execute_epoch_check_job_handle_test },
  {"gearman_execute(GEARMAN_TIMEOUT)", 0, gearman_execute_timeout_test },
  {"gearman_execute() background", 0, gearman_execute_bg_test },
  {"gearman_execute() multiple background", 0, gearman_execute_multile_bg_test },
  {"gearman_execute() with NULL workload", 0, gearman_execute_NULL_workload_TEST },
  {"gearman_execute() with NULL workload", 0, gearman_execute_NULL_attr_NULL_workload_TEST },
  {0, 0, 0}
};

test_st gearman_client_do_background_tests[] ={
  {"gearman_client_do_background()", 0, gearman_client_do_background_basic },
  {"gearman_client_do_high_background()", 0, gearman_client_do_high_background_basic },
  {"gearman_client_do_low_background()", 0, gearman_client_do_low_background_basic },
  {0, 0, 0}
};

test_st gearman_client_do_job_handle_tests[] ={
  {"gearman_client_do_job_handle() no active tasks", 0, gearman_client_do_job_handle_no_active_task },
  {"gearman_client_do_job_handle() follow do command", 0, gearman_client_do_job_handle_follow_do },
  {0, 0, 0}
};

test_st gearman_execute_partition_tests[] ={
  {"gearman_execute_by_partition() map reduce", 0, gearman_execute_partition_basic },
  {"gearman_execute_by_partition(GEARMAN_ARGUMENT_TOO_LARGE) map reduce", 0, gearman_execute_partition_check_parameters },
  {"gearman_execute_by_partition(GEARMAN_WORK_FAIL) map reduce", 0, gearman_execute_partition_workfail },
  {"gearman_execute_by_partition() fail in reduction", 0, gearman_execute_partition_fail_in_reduction },
#if 0
  {"gearman_execute() with V2 Worker that has aggregate defined", 0, gearman_execute_partition_use_as_function },
  {"gearman_execute_by_partition() no aggregate function", 0, gearman_execute_partition_no_aggregate },
#endif
  {0, 0, 0}
};

test_st gearman_client_set_server_option_tests[] ={
  {"gearman_client_set_server_option(exceptions)", 0, gearman_client_set_server_option_exception},
  {"gearman_client_set_server_option(bad)", 0, gearman_client_set_server_option_bad},
  {0, 0, 0}
};

test_st gearman_task_tests[] ={
  {"gearman_client_add_task() ", 0, gearman_client_add_task_test},
  {"gearman_client_add_task() fail", 0, gearman_client_add_task_test_fail},
  {"gearman_client_add_task() bad workload", 0, gearman_client_add_task_test_bad_workload},
  {"gearman_client_add_task_background()", 0, gearman_client_add_task_background_test},
  {"gearman_client_add_task_low_background()", 0, gearman_client_add_task_low_background_test},
  {"gearman_client_add_task_high_background()", 0, gearman_client_add_task_high_background_test},
  {"gearman_client_add_task() exception", 0, gearman_client_add_task_exception},
  {"gearman_client_add_task() warning", 0, gearman_client_add_task_warning},
  {"gearman_client_add_task(GEARMAN_NO_SERVERS)", 0, gearman_client_add_task_no_servers},
  {"gearman_client_set_task_context_free_fn()", 0, gearman_client_set_task_context_free_fn_test},
  {"gearman_client_add_task_status_by_unique(NOT_FOUND)", 0, gearman_client_add_task_status_by_unique_NOT_FOUND_TEST },
  {"gearman_client_add_task_status_by_unique()", 0, gearman_client_add_task_status_by_unique_TEST },
  {0, 0, 0}
};

test_st gearman_task_pause_tests[] ={
  {"gearman_client_add_task(GEARMAN_PAUSE)", 0, gearman_client_add_task_pause_test},
  {0, 0, 0}
};

test_st limit_tests[] ={
  {"GEARMAN_FUNCTION_MAX_SIZE", 0, function_name_limit_test},
  {"GEARMAN_UNIQUE_MAX_SIZE", 0, unique_name_limit_test},
  {0, 0, 0}
};



collection_st collection[] ={
  {"gearman_return_t", 0, 0, gearman_return_t_TESTS},
  {"gearman_id_t", 0, 0, gearman_id_t_TESTS},
  {"gearman_client_st", 0, 0, gearman_client_st_TESTS},
  {"gearman_client_st chunky", pre_chunk, post_function_reset, gearman_client_st_TESTS}, // Test with a worker that will respond in part
  {"gearman_strerror()", 0, 0, gearman_strerror_tests },
  {"gearman_task_add_task()", 0, 0, gearman_task_tests},
  {"gearman_task_add_task() v2 workers", pre_v2, post_function_reset, gearman_task_tests},
  {"gearman_task_add_task() chunky", pre_chunk, post_function_reset, gearman_task_tests},
  {"gearman_task_add_task() chunky v2 workers", pre_chunk_v2, post_function_reset, gearman_task_tests},
  {"gearman_task_add_task() namespace", pre_namespace, post_function_reset, gearman_task_tests},
  {"gearman_task_add_task(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_task_tests},
  {"gearman_task_add_task(GEARMAN_PAUSE)", pre_chunk, post_function_reset, gearman_task_pause_tests},
  {"gearman_task_add_task(GEARMAN_PAUSE)", pre_chunk_v2, post_function_reset, gearman_task_pause_tests},
  {"unique", pre_unique, post_function_reset, unique_tests},
  {"gearman_client_set_workload_malloc_fn()", 0, 0, gearman_client_set_workload_malloc_fn_tests},
  {"gearman_client_do()", 0, 0, gearman_client_do_tests},
  {"gearman_client_do() namespace", pre_namespace, post_function_reset, gearman_client_do_tests},
  {"gearman_client_do(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_client_do_tests},
  {"gearman_client_do_job_handle()", 0, 0, gearman_client_do_job_handle_tests},
  {"gearman_client_do_job_handle() namespace", pre_namespace, post_function_reset, gearman_client_do_job_handle_tests},
  {"gearman_client_do_job_handle(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_client_do_job_handle_tests},
  {"gearman_client_do_background()", 0, 0, gearman_client_do_background_tests},
  {"gearman_client_do_background(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_client_do_background_tests},
  {"gearman_client_set_server_option", 0, 0, gearman_client_set_server_option_tests},
  {"gearman_execute()", 0, 0, gearman_execute_tests},
  {"gearman_execute(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_execute_tests},
  {"gearman_execute() chunked return", pre_chunk, post_function_reset, gearman_execute_tests},
  {"gearman_execute() chunked return", pre_chunk_v2, post_function_reset, gearman_execute_tests},
  {"gearman_execute_partition()", 0, 0, gearman_execute_partition_tests},
  {"gearman_execute_partition(GEARMAN_CLIENT_FREE_TASKS)", pre_free_tasks, post_free_tasks, gearman_execute_partition_tests},
  {"gearman_command_t", 0, 0, gearman_command_t_tests},
  {"coalescence", 0, 0, coalescence_TESTS},
  {"regression_tests", 0, 0, regression_tests},
  {"limits", 0, 0, limit_tests },
  {"client-logging", pre_logging, post_logging, tests_log_TESTS },
  {"regression", 0, 0, regression2_TESTS },
  {"gearman_worker_timeout()", set_defaults, 0, gearman_worker_timeout_TESTS },
  {"gearman_client_set_identifier()", set_defaults, 0, gearman_client_set_identifier_TESTS },
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections(collection);
  world->create(world_create);
  world->destroy(world_destroy);
  world->set_runner(new GearmandRunner);
}
