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



#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define GEARMAN_CORE

#include <libgearman/common.h>
#include <libgearman/packet.hpp>
#include <libgearman/universal.hpp>

#include <libtest/test.h>
#include <libtest/server.h>
#include <libtest/worker.h>

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


static test_return_t regression_bug_783141_test(void *)
{
  gearman_client_st *client= gearman_client_create(NULL);
  test_truth(client);

  test_truth(gearman_success(gearman_client_add_server(client, "10.0.2.253", WORKER_TEST_PORT)));

  gearman_return_t ret;
  gearman_task_st *task= gearman_client_add_task(client, NULL, NULL,
                                                 "does not exist", NULL,
                                                 gearman_literal_param("dog"),
                                                 &ret);
  test_true_got(gearman_success(ret), gearman_strerror(ret));
  test_truth(task);

  gearman_return_t local_ret= gearman_client_run_tasks(client);
  test_compare_got(GEARMAN_COULD_NOT_CONNECT, local_ret, gearman_client_error(client));

  local_ret= gearman_client_run_tasks(client);
  test_compare_got(GEARMAN_NO_ACTIVE_FDS, local_ret, gearman_client_error(client));

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t bug372074_test(void *)
{
  gearman_universal_st universal;
  const void *args[1];
  size_t args_size[1];

  gearman_universal_initialize(universal);

  for (uint32_t x= 0; x < 2; x++)
  {
    gearman_packet_st packet;
    gearman_connection_st *con_ptr;
    test_truth(con_ptr= gearman_connection_create(universal, NULL));

    con_ptr->set_host(NULL, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_SET_CLIENT_ID,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_CANT_DO,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    delete con_ptr;

    test_truth(con_ptr= gearman_connection_create(universal, NULL));

    con_ptr->set_host(NULL, WORKER_TEST_PORT);

    args[0]= "testUnregisterFunction";
    args_size[0]= strlen("testUnregisterFunction");
    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_SET_CLIENT_ID,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    args[0]= "digest";
    args_size[0]= strlen("digest");
    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_CAN_DO,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    args[0]= "reverse";
    args_size[0]= strlen("reverse");
    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_CAN_DO,
                                                          args, args_size, 1)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    test_truth(gearman_success(gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                          GEARMAN_COMMAND_RESET_ABILITIES,
                                                          NULL, NULL, 0)));

    test_truth(gearman_success(con_ptr->send(packet, true)));

    gearman_packet_free(&packet);

    delete con_ptr;
  }

  gearman_universal_free(universal);

  return TEST_SUCCESS;
}


test_st worker_tests[] ={
  {"bug372074", 0, bug372074_test },
  {0, 0, 0}
};

test_st gearman_client_run_tasks_regression[] ={
  {"lp:783141, multiple calls for bad host", 0, regression_bug_783141_test },
  {0, 0, 0}
};


collection_st collection[] ={
  {"worker_tests", 0, 0, worker_tests },
  {"gearman_client_run_tasks()", 0, 0, gearman_client_run_tasks_regression },
  {0, 0, 0, 0}
};

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static void *world_create(test_return_t *error)
{
  regression_st *test;

  test= (regression_st *)malloc(sizeof(regression_st));

  test->gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, 0, NULL);

  if (test->gearmand_pid == -1)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

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
