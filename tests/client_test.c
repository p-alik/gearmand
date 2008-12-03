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
#include <stdbool.h>

#define GEARMAN_INTERNAL 1
#include <libgearman/gearman.h>
#include <libgearman/watchpoint.h>

#include "test.h"

/* Prototypes */
test_return init_test(void *not_used);
test_return allocation_test(void *not_used);
test_return clone_test(void *object);
test_return echo_test(void *object);
test_return submit_job_test(void *object);
test_return background_failure_test(void *object);
test_return background_test(void *object);
test_return error_test(void *object);
void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used __attribute__((unused)))
{
  gearman_client_st client;

  (void)gearman_client_create(NULL, &client);
  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used __attribute__((unused)))
{
  gearman_client_st *object;
  object= gearman_client_create(NULL, NULL);
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
    clone= gearman_client_clone(NULL, NULL);
    assert(clone);
    gearman_client_free(clone);
  }

  /* Can we init from null? */
  {
    gearman_client_st *clone;
    clone= gearman_client_clone(NULL, param);
    assert(clone);
    gearman_client_free(clone);
  }

  return TEST_SUCCESS;
}

#ifdef NOT_DONE
test_return echo_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  size_t value_length;
  char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_client_echo(client, (uint8_t *)value, value_length);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}
#endif

test_return background_failure_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_id[1024];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_failure_test";
  ssize_t value_length= strlen("background_failure_test");

  rc= gearman_client_do_background(client, "does_not_exist",
                                   value, value_length, job_id);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);

  rc= gearman_client_task_status(client, job_id, &is_known, &is_running, &numerator, &denominator);
  assert(rc == GEARMAN_SUCCESS);
  assert(is_known == false);
  assert(is_running == false);
  assert(numerator == 0);
  assert(denominator == 0);

  return TEST_SUCCESS;
}

test_return background_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_id[1024];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");

  rc= gearman_client_do_background(client, "frog",
                                   value, value_length, job_id);
  //WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);
  //sleep(1); /* Yes, this could fail on an overloaded system to give the
  //server enough time to assign */

  rc= gearman_client_task_status(client, job_id, &is_known, &is_running, &numerator, &denominator);
  assert(rc == GEARMAN_SUCCESS);
  assert(is_known == true);

  while (1)
  {
    rc= gearman_client_task_status(client, job_id, &is_known, &is_running, &numerator, &denominator);
    assert(rc == GEARMAN_SUCCESS);
    if (is_running == true)
    {
      WATCHPOINT_NUMBER(numerator);
      WATCHPOINT_NUMBER(denominator);
      continue;
    }
    else
      break;
  }

  return TEST_SUCCESS;
}

test_return submit_job_test(void *object)
{
(void)object;
#if 0
  gearman_return rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  ssize_t job_length;
  uint8_t *value= (uint8_t *)"submit_job_test";
  size_t value_length= strlen("submit_job_test");

  job_result= gearman_client_do(client, "frog",
                                value, value_length, &job_length, &rc);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);
  assert(job_result);
  WATCHPOINT_STRING_LENGTH((char *)job_result, job_length);

  if (job_result)
    free(job_result);

#endif
  return TEST_SUCCESS;
}



#ifdef NOT_DONE

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
  gearman_return_t rc;

  client= gearman_client_create(NULL, NULL);

  assert(client);

  rc= gearman_client_add_server(client, "localhost", 0);

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
  {"submit_job", 0, submit_job_test },
  {"background", 0, background_test },
  {"background_failure", 0, background_failure_test },
#ifdef NOT_DONE
  {"echo", 0, echo_test },
  {"error", 0, error_test },
  {"submit_job2", 0, submit_job_test },
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
