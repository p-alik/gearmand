/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define GEARMAN_CORE
#include <libgearman/gearman.h>

#include <libtest/server.h>
#include <libtest/test.h>
#include <libtest/worker.h>

#define CLIENT_TEST_PORT 32123

#define WORKER_FUNCTION_NAME "client_test"
#define WORKER_CHUNKED_FUNCTION_NAME "reverse_test"
#define WORKER_UNIQUE_FUNCTION_NAME "unique_test"
#define WORKER_SPLIT_FUNCTION_NAME "split_worker"

#include <tests/do.h>
#include <tests/server_options.h>
#include <tests/do_background.h>
#include <tests/execute.h>
#include <tests/gearman_client_do_job_handle.h>
#include <tests/gearman_client_execute_reduce.h>
#include <tests/task.h>
#include <tests/unique.h>
#include <tests/workers.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

struct client_test_st
{
  gearman_client_st *_client;
  bool _clone;
  pid_t gearmand_pid;
  struct worker_handle_st *completion_worker;
  struct worker_handle_st *chunky_worker;
  struct worker_handle_st *unique_check;
  struct worker_handle_st *split_worker;
  const char *_worker_name;

  client_test_st() :
    _clone(true),
    gearmand_pid(-1),
    completion_worker(NULL),
    chunky_worker(NULL),
    unique_check(NULL),
    split_worker(NULL),
    _worker_name(WORKER_FUNCTION_NAME)
  { 
    if (not (_client= gearman_client_create(NULL)))
    {
      abort(); // This would only happen from a programming error
    }

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

  ~client_test_st()
  {
    gearman_client_free(_client);
  }

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
void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);


static void *client_thread(void *object)
{
  (void)object;
  gearman_return_t rc;
  gearman_client_st client;
  gearman_client_st *client_ptr;
  size_t result_size;

  client_ptr= gearman_client_create(&client);

  if (client_ptr == NULL)
    abort(); // This would be pretty bad.

  rc= gearman_client_add_server(&client, NULL, CLIENT_TEST_PORT);
  if (gearman_failed(rc))
  {
    pthread_exit(0);
  }

  gearman_client_set_timeout(&client, 400);
  for (size_t x= 0; x < 5; x++)
  {
    (void) gearman_client_do(&client, "client_test_temp", NULL, NULL, 0,
                             &result_size, &rc);

  }
  gearman_client_free(client_ptr);

  pthread_exit(0);
}

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
  gearman_client_st *from_with_host;
  gearman_client_st *client;

  client= gearman_client_clone(NULL, NULL);

  test_truth(client);
  test_truth(client->options.allocated);

  gearman_client_free(client);

  client= gearman_client_clone(NULL, from);
  test_truth(client);
  gearman_client_free(client);

  from_with_host= gearman_client_create(NULL);
  test_truth(from_with_host);
  gearman_client_add_server(from_with_host, "127.0.0.1", 12345);

  client= gearman_client_clone(NULL, from_with_host);
  test_truth(client);

  test_truth(client->universal.con_list);
  test_truth(gearman_client_compare(client, from_with_host));

  gearman_client_free(client);
  gearman_client_free(from_with_host);

  return TEST_SUCCESS;
}

static test_return_t option_test(void *object __attribute__((unused)))
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
  gearman_return_t rc;
  size_t value_length;
  const char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_client_echo(client, (uint8_t *)value, value_length);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client));

  return TEST_SUCCESS;
}

static test_return_t submit_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  void *job_result;
  size_t job_length;
  uint8_t *value= (uint8_t *)"submit_job_test";
  size_t value_length= strlen("submit_job_test");

  job_result= gearman_client_do(client, worker_function, NULL, value,
				value_length, &job_length, &rc);

  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_truth(job_result);
  test_compare(value_length, job_length);

  test_memcmp(value, job_result, value_length);

  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_null_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  void *job_result;
  size_t job_length;

  const char *worker_function= (const char *)gearman_client_context(client);
  job_result= gearman_client_do(client, worker_function, NULL, NULL, 0,
                                &job_length, &rc);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client));
  test_compare(0, job_length);
  test_truth(not job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_exception_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  void *job_result;
  size_t job_length;

  const char *worker_function= (const char *)gearman_client_context(client);
  job_result= gearman_client_do(client, worker_function, NULL,
                                gearman_literal_param("exception"),
                                &job_length, &rc);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_memcmp("exception", job_result, job_length);
  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_warning_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  void *job_result;
  size_t job_length;

  const char *worker_function= (const char *)gearman_client_context(client);
  job_result= gearman_client_do(client, worker_function, NULL,
                                gearman_literal_param("warning"),
                                &job_length, &rc);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_memcmp("warning", job_result, job_length);
  free(job_result);

  return TEST_SUCCESS;
}

static test_return_t submit_fail_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  void *job_result;
  size_t job_length;

  const char *worker_function= (const char *)gearman_client_context(client);
  job_result= gearman_client_do(client, worker_function, NULL, "fail", 4,
                                &job_length, &rc);
  test_true_got(rc == GEARMAN_WORK_FAIL, gearman_client_error(client));
  test_false(job_result);
  test_false(job_length);

  return TEST_SUCCESS;
}

static test_return_t submit_multiple_do(void *object)
{
  for (uint32_t x= 0; x < 100 /* arbitrary */; x++)
  {
    uint32_t option= random() %3;
    test_return_t rc;

    switch (option)
    {
    case 0:
      rc= submit_null_job_test(object);
      test_truth(rc == TEST_SUCCESS);
      break;
    case 1:
      rc= submit_job_test(object);
      test_truth(rc == TEST_SUCCESS);
      break;
    default:
    case 2:
      rc= submit_fail_job_test(object);
      test_truth(rc == TEST_SUCCESS);
      break;
    }
  }

  return TEST_SUCCESS;
}

static test_return_t background_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");

  const char *worker_function= (const char *)gearman_client_context(client);
  rc= gearman_client_do_background(client, worker_function, NULL, value,
                                   value_length, job_handle);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client));

  while (1)
  {
    bool is_known;
    bool is_running;
    uint32_t numerator;
    uint32_t denominator;

    rc= gearman_client_job_status(client, job_handle, &is_known, &is_running,
                                  &numerator, &denominator);
    if (gearman_failed(rc))
    {
      printf("background_test:%s\n", gearman_client_error(client));
      return TEST_FAILURE;
    }

    if (is_known == false)
      break;
  }

  return TEST_SUCCESS;
}

static test_return_t background_failure_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_failure_test";
  size_t value_length= strlen("background_failure_test");

  gearman_return_t rc;
  rc= gearman_client_do_background(client, "does_not_exist", NULL, value,
                                   value_length, job_handle);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_client_error(client));

  rc= gearman_client_job_status(client, job_handle, &is_known, &is_running,
                                &numerator, &denominator);
  if (rc != GEARMAN_SUCCESS || is_known != true || is_running != false ||
      numerator != 0 || denominator != 0)
  {
    printf("background_failure_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

static test_return_t add_servers_test(void *)
{
  gearman_client_st client, *client_ptr;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  gearman_return_t rc;
  rc= gearman_client_add_servers(&client, "127.0.0.1:4730,localhost");
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  rc= gearman_client_add_servers(&client, "old_jobserver:7003,broken:12345");
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

static test_return_t bug_518512_test(void *)
{
  gearman_client_st client;
  size_t result_size;

  test_truth(gearman_client_create(&client));

  gearman_return_t rc;
  rc= gearman_client_add_server(&client, NULL, CLIENT_TEST_PORT);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  gearman_client_set_timeout(&client, 100);
  (void) gearman_client_do(&client, "client_test_temp", NULL, NULL, 0,
                           &result_size, &rc);
  test_true_got(rc == GEARMAN_TIMEOUT, gearman_strerror(rc));

  struct worker_handle_st *completion_worker= test_worker_start(CLIENT_TEST_PORT, "client_test_temp",
                                                                client_test_temp_worker, NULL, gearman_worker_options_t());

  gearman_client_set_timeout(&client, -1);
  (void) gearman_client_do(&client, "client_test_temp", NULL, NULL, 0,
                           &result_size, &rc);
  test_true_got(rc != GEARMAN_TIMEOUT, gearman_strerror(rc));

  test_worker_stop(completion_worker);
  gearman_client_free(&client);

  return TEST_SUCCESS;
}

#define NUMBER_OF_WORKERS 2

static test_return_t loop_test(void *)
{
  void *unused;
  pthread_attr_t attr;

  pthread_t one;
  pthread_t two;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  struct worker_handle_st *handles[NUMBER_OF_WORKERS];
  for (size_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    handles[x]= test_worker_start(CLIENT_TEST_PORT, "client_test_temp",
                                  client_test_temp_worker, NULL, gearman_worker_options_t());
  }

  pthread_create(&one, &attr, client_thread, NULL);
  pthread_create(&two, &attr, client_thread, NULL);

  pthread_join(one, &unused);
  pthread_join(two, &unused);

  for (size_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    test_worker_stop(handles[x]);
  }

  pthread_attr_destroy(&attr);

  return TEST_SUCCESS;
}

static test_return_t submit_log_failure(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  void *job_result;
  size_t job_length;
  uint8_t *value= (uint8_t *)"submit_log_failure";
  size_t value_length= strlen("submit_log_failure");

  const char *worker_function= (const char *)gearman_client_context(client);
  job_result= gearman_client_do(client, worker_function, NULL, value,
                                value_length, &job_length, &rc);
  test_true_got(rc == GEARMAN_NO_SERVERS, gearman_strerror(rc));

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
  test_compare((int)GEARMAN_MAX_RETURN, 50);

  return TEST_SUCCESS;
}

#undef MAKE_NEW_STRERROR

static char * make_number(uint32_t expected, uint32_t got)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "Expected %uU, got %uU", expected, got);

  return strdup(buffer);
}

static test_return_t strerror_strings(void *object  __attribute__((unused)))
{
  uint32_t values[] = { 324335284U, 1940666259U, 4156775927U, 18028287U,
			1834995715U, 1009419836U, 1038124396U, 3050095617U,
			4004269877U, 2913489720U, 1389266665U, 1374361090U,
			3775104989U, 1158738795U, 2490507301U, 426780991U,
			2421852085U, 426121997U, 3669711613U, 2620567638U,
			48094985U, 4052600452U, 2697110207U, 4260329382U,
			3706494438U, 1765339649U, 1176029865U, 2899482444U,
			2255507756U, 1844534215U, 1685626311U, 3134591697U,
			1469920452U, 2236059486U, 1693700353U, 1173962212U,
			2491943732U, 1864825729U, 523632457U, 1342225548U,
			245155833U, 3999913926U, 2789053153U, 2576033598U,
			463490826U, 1983660343U, 2268979717U, 1656388188U,
                        1558344702U, 3577742799U};

  for (int rc= GEARMAN_SUCCESS; rc < GEARMAN_MAX_RETURN; rc++)
  {
    uint32_t hash_val;
    const char *msg=  gearman_strerror((gearman_return_t)rc);
    hash_val= internal_generate_hash(msg, strlen(msg));
    test_true_got(values[rc] == hash_val, make_number(values[rc], hash_val));
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

  return TEST_SUCCESS;
}

static test_return_t pre_logging(void *object)
{
  client_test_st *all= (client_test_st *)object;
  gearman_log_fn *func= log_counter;
  global_counter= 0;
  all->reset_client();
  all->set_clone(false);

  gearman_client_set_log_fn(all->client(), func, &global_counter, GEARMAN_VERBOSE_MAX);

  return TEST_SUCCESS;
}

static test_return_t post_logging(void *)
{
  test_truth(global_counter);

  return TEST_SUCCESS;
}

void *client_test_temp_worker(gearman_job_st *, void *,
                              size_t *result_size, gearman_return_t *ret_ptr)
{
  *result_size= 0;
  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

void *world_create(test_return_t *error)
{
  client_test_st *test= new client_test_st();
  pid_t gearmand_pid;

  /**
   *  @TODO We cast this to char ** below, which is evil. We need to do the
   *  right thing
   */
  const char *argv[1]= { "client_gearmand" };

  if (not test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  /**
    We start up everything before we allocate so that we don't have to track memory in the forked process.
  */
  gearmand_pid= test_gearmand_start(CLIENT_TEST_PORT, 1, argv);
  
  if (gearmand_pid == -1)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->completion_worker= test_worker_start(CLIENT_TEST_PORT, WORKER_FUNCTION_NAME, echo_or_react_worker, NULL, gearman_worker_options_t());
  test->chunky_worker= test_worker_start(CLIENT_TEST_PORT, WORKER_CHUNKED_FUNCTION_NAME, echo_or_react_chunk_worker, NULL, gearman_worker_options_t());
  test->unique_check= test_worker_start(CLIENT_TEST_PORT, WORKER_UNIQUE_FUNCTION_NAME, unique_worker, NULL, GEARMAN_WORKER_GRAB_UNIQ);
  test->split_worker= test_worker_start_with_reducer(CLIENT_TEST_PORT, WORKER_SPLIT_FUNCTION_NAME, split_worker, NULL, GEARMAN_WORKER_GRAB_ALL, cat_aggregator_fn);

  test->gearmand_pid= gearmand_pid;

  if (gearman_failed(gearman_client_add_server(test->client(), NULL, CLIENT_TEST_PORT)))
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return (void *)test;
}


test_return_t world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  test_gearmand_stop(test->gearmand_pid);
  test_worker_stop(test->completion_worker);
  test_worker_stop(test->chunky_worker);
  test_worker_stop(test->unique_check);
  test_worker_stop(test->split_worker);
  delete test;

  return TEST_SUCCESS;
}


test_st tests[] ={
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
  {"background", 0, background_test },
  {"background_failure", 0, background_failure_test },
  {"add_servers", 0, add_servers_test },
  {"bug_518512_test", 0, bug_518512_test },
  {"loop_test", 0, loop_test },
  {0, 0, 0}
};


test_st tests_log[] ={
  {"submit_log_failure", 0, submit_log_failure },
  {0, 0, 0}
};

test_st gearman_strerror_tests[] ={
  {"count", 0, strerror_count },
  {"strings", 0, strerror_strings },
  {0, 0, 0}
};

test_st unique_tests[] ={
  {"compare sent unique", 0, unique_compare_test },
  {0, 0, 0}
};

test_st gearman_client_do_tests[] ={
  {"gearman_client_do() fail huge unique", 0, gearman_client_do_huge_unique },
  {0, 0, 0}
};

test_st gearman_client_execute_tests[] ={
  {"gearman_client_execute()", 0, gearman_client_execute_test },
  {"gearman_client_execute() epoch", 0, gearman_client_execute_epoch_test },
  {"gearman_client_execute() timeout", 0, gearman_client_execute_timeout_test },
  {"gearman_client_execute() background", 0, gearman_client_execute_bg_test },
  {"gearman_client_execute() multiple background", 0, gearman_client_execute_multile_bg_test },
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

test_st gearman_worker_set_reducer_tests[] ={
  {"gearman_worker_set_reducer()", 0, gearman_worker_set_reducer_test },
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
  {0, 0, 0}
};


collection_st collection[] ={
  {"gearman_worker_set_reducer()", 0, 0, gearman_worker_set_reducer_tests},
  {"gearman_client_st", 0, 0, tests},
  {"gearman_client_st chunky", pre_chunk, post_function_reset, tests}, // Test with a worker that will respond in part
  {"gearman_strerror", 0, 0, gearman_strerror_tests},
  {"gearman_task", 0, 0, gearman_task_tests},
  {"gearman_task chunky", pre_chunk, post_function_reset, gearman_task_tests},
  {"unique", pre_unique, post_function_reset, unique_tests},
  {"gearman_client_do()", 0, 0, gearman_client_do_tests},
  {"gearman_client_execute chunky", pre_chunk, post_function_reset, gearman_client_execute_tests},
  {"gearman_client_do_job_handle", 0, 0, gearman_client_do_job_handle_tests},
  {"gearman_client_do_background", 0, 0, gearman_client_do_background_tests},
  {"gearman_client_set_server_option", 0, 0, gearman_client_set_server_option_tests},
  {"gearman_client_execute", 0, 0, gearman_client_execute_tests},
  {"client-logging", pre_logging, post_logging, tests_log},
  {0, 0, 0, 0}
};

typedef test_return_t (*libgearman_test_prepost_callback_fn)(client_test_st *);
typedef test_return_t (*libgearman_test_callback_fn)(gearman_client_st *);
static test_return_t _runner_prepost_default(libgearman_test_prepost_callback_fn func, client_test_st *container)
{
  if (func)
  {
    return func(container);
  }

  return TEST_SUCCESS;
}

static test_return_t _runner_default(libgearman_test_callback_fn func, client_test_st *container)
{
  if (func)
  {
    test_return_t rc;

    if (container->clone())
    {
      gearman_client_st *client= gearman_client_clone(NULL, container->client());
      test_truth(client);
      gearman_client_set_context(client, (void *)container->worker_name());
      rc= func(client);
      if (rc == TEST_SUCCESS)
        test_truth(not client->task_list);
      gearman_client_free(client);
    }
    else
    {
      gearman_client_set_context(container->client(), (void *)container->worker_name());
      rc= func(container->client());
      assert(not container->client()->task_list);
    }

    return rc;
  }

  return TEST_SUCCESS;
}

static world_runner_st runner= {
  (test_callback_runner_fn)_runner_prepost_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_prepost_default
};


void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
