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

#define GEARMAN_CORE

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"
#include "test_worker.h"

#define WORKER_TEST_PORT 32123

typedef struct
{
  pid_t gearmand_pid;
} regression_st;

void *create(void *object);
void destroy(void *object);
test_return_t pre(void *object);
test_return_t post(void *object);
test_return_t flush(void);


static test_return_t bug372074_test(void *object __attribute__((unused)))
{
  gearman_universal_st gearman;
  gearman_connection_st con;
  gearman_packet_st packet;
  uint32_t x;
  const void *args[1];
  size_t args_size[1];

  if (gearman_universal_create(&gearman, NULL) == NULL)
    return TEST_FAILURE;

  for (x= 0; x < 2; x++)
  {
    if (gearman_connection_create(&gearman, &con, NULL) == NULL)
      return TEST_FAILURE;

    gearman_connection_set_host(&con, NULL, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_SET_CLIENT_ID,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_CAN_DO,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_CANT_DO,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    gearman_connection_free(&con);

    if (gearman_connection_create(&gearman, &con, NULL) == NULL)
      return TEST_FAILURE;

    gearman_connection_set_host(&con, NULL, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_SET_CLIENT_ID,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "digest";
    args_size[0]= strlen("digest");
    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_CAN_DO,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_CAN_DO,
                                   args, args_size, 1) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                   GEARMAN_COMMAND_RESET_ABILITIES,
                                   NULL, NULL, 0) != GEARMAN_SUCCESS)
    {
      return TEST_FAILURE;
    }

    if (gearman_connection_send(&con, &packet, true) != GEARMAN_SUCCESS)
      return TEST_FAILURE;

    gearman_packet_free(&packet);

    gearman_connection_free(&con);
  }

  gearman_universal_free(&gearman);

  return TEST_SUCCESS;
}


test_st worker_tests[] ={
  {"bug372074", 0, bug372074_test },
  {0, 0, 0}
};


collection_st collection[] ={
  {"worker_tests", 0, 0, worker_tests },
  {0, 0, 0, 0}
};


static void *world_create(test_return_t *error)
{
  regression_st *test;

  test= malloc(sizeof(regression_st));

  test->gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, NULL, NULL, 0);

  *error= TEST_SUCCESS;

  return (void *)test;
}


static test_return_t world_destroy(void *object)
{
  regression_st *test= (regression_st *)object;
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  return TEST_SUCCESS;
}


void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
