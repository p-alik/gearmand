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
#include <libgearman/gearman.h>
#include <libgearman/watchpoint.h>

#include "test.h"

/* Prototypes */
test_return init_test(void *not_used);
test_return allocation_test(void *not_used);
test_return clone_test(void *object);
void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used __attribute__((unused)))
{
  gearman_worker_st object;

  (void)gearman_worker_create(NULL, &object);
  gearman_worker_free(&object);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used __attribute__((unused)))
{
  gearman_worker_st *object;
  object= gearman_worker_create(NULL, NULL);
  assert(object);
  gearman_worker_free(object);

  return TEST_SUCCESS;
}

test_return clone_test(void *object)
{
  gearman_worker_st *param= (gearman_worker_st *)object;

  /* All null? */
  {
    gearman_worker_st *clone;
    clone= gearman_worker_clone(NULL);
    assert(clone);
    gearman_worker_free(clone);
  }

  /* Can we init from null? */
  {
    gearman_worker_st *clone;
    clone= gearman_worker_clone(param);
    assert(clone);
    gearman_worker_free(clone);
  }

  return TEST_SUCCESS;
}

/* Prototype */
uint8_t* simple_worker(gearman_worker_st *job,
                       uint8_t *value,  
                       ssize_t value_length,  
                       ssize_t *result_length,  
                       gearman_return *error);

uint8_t* simple_worker(gearman_worker_st *job,
                       uint8_t *value,  
                       ssize_t value_length,  
                       ssize_t *result_length,  
                       gearman_return *error)
{
  fprintf(stderr, "%.*s\n", value_length, value);

  (void)job;

  *error= GEARMAN_SUCCESS;
  *result_length= strlen("successful");

  return (uint8_t *)strdup("successful");
}

static test_return simple_work_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_worker_function callback[1];

  callback[0]= simple_worker;
  gearman_server_function_register(worker, "simple", callback);
  gearman_server_work(worker);

  return TEST_SUCCESS;
}

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *not_used __attribute__((unused)))
{
  gearman_worker_st *worker;
  gearman_return rc;

  worker= gearman_worker_create(NULL);

  assert(worker);

  rc= gearman_worker_server_add(worker, "localhost", 0);

  assert(rc == GEARMAN_SUCCESS);


  return (void *)worker;
}

void destroy(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  gearman_worker_free(worker);
}

test_return pre(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  return TEST_SUCCESS;
}

test_return post(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"simple_work_test", 0, simple_work_test },
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
