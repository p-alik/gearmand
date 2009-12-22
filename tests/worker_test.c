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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"

#define WORKER_TEST_PORT 32123

typedef struct
{
  pid_t gearmand_pid;
  gearman_worker_st worker;
} worker_test_st;

/* Prototypes */
test_return_t init_test(void *object);
test_return_t allocation_test(void *object);
test_return_t clone_test(void *object);
test_return_t echo_test(void *object);
test_return_t bug372074_test(void *object);

void *create(void *object);
void destroy(void *object);
test_return_t pre(void *object);
test_return_t post(void *object);
test_return_t flush(void);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

test_return_t init_test(void *object __attribute__((unused)))
{
  gearman_worker_st worker;

  if (gearman_worker_create(&worker) == NULL)
    return TEST_FAILURE;

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

test_return_t allocation_test(void *object __attribute__((unused)))
{
  gearman_worker_st *worker;

  worker= gearman_worker_create(NULL);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t clone_test(void *object)
{
  gearman_worker_st *from= (gearman_worker_st *)object;
  gearman_worker_st *worker;

  if (gearman_worker_clone(NULL, NULL) != NULL)
    return TEST_FAILURE;

  worker= gearman_worker_clone(NULL, from);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t echo_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_return_t rc;
  size_t value_length;
  const char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_worker_echo(worker, (uint8_t *)value, value_length);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

test_return_t bug372074_test(void *object __attribute__((unused)))
{
  gearman_state_st gearman;
  gearman_con_st con;
  gearman_packet_st packet;
  uint32_t x;
  const void *args[1];
  size_t args_size[1];

  if (gearman_state_create(&gearman) == NULL)
    return TEST_FAILURE;

  for (x= 0; x < 2; x++)
  {
    if (gearman_add_con(&gearman, &con) == NULL)
      return TEST_FAILURE;

    gearman_con_set_host(&con, NULL);
    gearman_con_set_port(&con, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_SET_CLIENT_ID,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_CAN_DO,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_CANT_DO,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    gearman_con_free(&con);

    if (gearman_add_con(&gearman, &con) == NULL)
      return TEST_FAILURE;

    gearman_con_set_host(&con, NULL);
    gearman_con_set_port(&con, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_SET_CLIENT_ID,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "digest";
    args_size[0]= strlen("digest");
    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_CAN_DO,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_CAN_DO,
                                args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    if (gearman_add_packet_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_RESET_ABILITIES,
                                NULL, NULL, 0) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_con_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    gearman_con_free(&con);
  }

  gearman_state_free(&gearman);

  return TEST_SUCCESS;
}

#ifdef NOT_DONE
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

static test_return_t simple_work_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_worker_function callback[1];

  callback[0]= simple_worker;
  gearman_server_function_register(worker, "simple", callback);
  gearman_server_work(worker);

  return TEST_SUCCESS;
}
#endif

void *create(void *object __attribute__((unused)))
{
  worker_test_st *test= (worker_test_st *)object;
  return (void *)&(test->worker);
}

void *world_create(test_return_t *error)
{
  worker_test_st *test;

  test= malloc(sizeof(worker_test_st));

  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(worker_test_st));
  if (gearman_worker_create(&(test->worker)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_worker_add_server(&(test->worker), NULL, WORKER_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, NULL, NULL, 0);

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_free(&(test->worker));
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone", 0, clone_test },
  {"echo", 0, echo_test },
  {"bug372074", 0, bug372074_test },
#ifdef NOT_DONE
  {"simple_work_test", 0, simple_work_test },
#endif
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, tests},
  {0, 0, 0, 0}
};


typedef test_return_t (*libgearman_test_callback_fn)(gearman_worker_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, worker_test_st *container)
{
  if (func)
  {
    return func(&container->worker);
  }
  else
  {
    return TEST_SUCCESS;
  }
}


static world_runner_st runner= {
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
