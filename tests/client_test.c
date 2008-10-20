/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define GEARMAN_INTERNAL 1
#include <libgearman/gearman_client.h>

#include "test.h"

/* Prototypes */
test_return init_test(void *not_used);
test_return allocation_test(void *not_used);
test_return clone_test(void *object);
test_return echo_test(void *object);
test_return submit_job_test(void *object);
test_return background_test(void *object);
test_return error_test(void *object);
void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used __attribute__((unused)))
{
  gearman_client_st object;

  (void)gearman_client_create(&object);
  gearman_client_free(&object);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used __attribute__((unused)))
{
  gearman_client_st *object;
  object= gearman_client_create(NULL);
  assert(object);
  gearman_client_free(object);

  return TEST_SUCCESS;
}

test_return clone_test(void *object)
{
  gearman_client_st *param= (gearman_client_st *)object;

  /* All null? */
  {
    gearman_client_st *clone;
    clone= gearman_client_clone(NULL);
    assert(clone);
    gearman_client_free(clone);
  }

  /* Can we init from null? */
  {
    gearman_client_st *clone;
    clone= gearman_client_clone(param);
    assert(clone);
    gearman_client_free(clone);
  }

  return TEST_SUCCESS;
}

test_return echo_test(void *object)
{
  gearman_return rc;
  gearman_client_st *client= (gearman_client_st *)object;
  size_t value_length;
  char *value= "This is my echo test";

  value_length= strlen(value);

  assert (client->list.number_of_hosts);
  rc= gearman_client_echo(client, value, value_length);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}


#ifdef NOT_DONE

test_return submit_job_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_result_st *result;
  gearman_job_st *job;
  char *value= "submit_job_test";
  size_t value_length= strlen("submit_job_test");

  result= gearman_result_create(param, NULL);
  job= gearman_job_create(param, NULL);

  assert(result);
  assert(job);


  gearman_job_set_function(job, "echo");
  gearman_job_set_value(job, value, value_length);

  rc= gearman_job_submit(job);

  assert(rc == GEARMAN_SUCCESS);
  WATCHPOINT;

  rc= gearman_job_result(job, result);

  assert(rc == GEARMAN_SUCCESS);
  assert(result->action == GEARMAN_WORK_COMPLETE);
  assert(gearman_result_length(result) == value_length);
  assert(memcmp(gearman_result_value(result), value, value_length) == 0);

  return TEST_SUCCESS;
}

test_return background_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_result_st *result;
  gearman_job_st *job;
  char *value= "background_test";
  size_t value_length= strlen("background_test");

  result= gearman_result_create(param, NULL);
  job= gearman_job_create(param, NULL);

  assert(result);
  assert(job);

  gearman_job_set_function(job, "echo");
  gearman_job_set_value(job, value, value_length);
  rc= gearman_job_set_behavior(job, GEARMAN_BEHAVIOR_JOB_BACKGROUND, 1);
  assert(GEARMAN_SUCCESS == rc);

  rc= gearman_job_submit(job);

  assert(rc == GEARMAN_SUCCESS);

  do {
    rc= gearman_job_result(job, result);

    assert(rc == GEARMAN_SUCCESS ||  rc == GEARMAN_STILL_RUNNING);

    if (rc == GEARMAN_SUCCESS)
      break;
  } while (1);

  gearman_result_free(result);
  gearman_job_free(job);

  return TEST_SUCCESS;
}

test_return error_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;

  for (rc= GEARMAN_SUCCESS; rc < GEARMAN_MAXIMUM_RETURN; rc++)
  {
    char *string;
    string= gearman_strerror(param, rc);
  }

  return TEST_SUCCESS;
}

#endif /* NOT_DONE */

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *not_used __attribute__((unused)))
{
  gearman_client_st *client;
  gearman_return rc;

  client= gearman_client_create(NULL);

  assert(client);

  rc= gearman_client_server_add(client, "localhost", 0);
  assert (client->list.number_of_hosts == 1);

  assert(rc == GEARMAN_SUCCESS);


  return (void *)client;
}

void destroy(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  gearman_client_free(client);
}

test_return pre(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  return TEST_SUCCESS;
}

test_return post(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
#ifdef NOT_DONE
  {"error", 0, error_test },
  {"submit_job", 0, submit_job_test },
  {"submit_job2", 0, submit_job_test },
  {"background", 0, background_test },
#endif
  {0, 0, 0}
};

collection_st collection[] ={
  {"norm", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= NULL;
  world->destroy= NULL;
}
