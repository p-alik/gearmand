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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GEARMAN_CORE
#include <libgearman/common.h>
#include <libgearman/packet.hpp>

#include "libtest/test.h"
#include "libtest/server.h"
#include <libtest/worker.h>

#include "libgearman/universal.hpp"

#define CLIENT_TEST_PORT 32123

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t init_test(void *)
{
  gearman_universal_st gear;

  gearman_universal_initialize(gear);
  test_false(gear.options.dont_track_packets);
  test_false(gear.options.non_blocking);
  test_false(gear.options.stored_non_blocking);

  gearman_universal_free(gear);

  return TEST_SUCCESS;
}

static test_return_t clone_test(void *)
{
  gearman_universal_st gear;

  gearman_universal_initialize(gear);

  /* Can we init from null? */
  {
    gearman_universal_st destination;
    gearman_universal_clone(destination, gear);

    { // Test all of the flags
      test_truth(destination.options.dont_track_packets == gear.options.dont_track_packets);
      test_truth(destination.options.non_blocking == gear.options.non_blocking);
      test_truth(destination.options.stored_non_blocking == gear.options.stored_non_blocking);
    }
    test_truth(destination.verbose == gear.verbose);
    test_truth(destination.con_count == gear.con_count);
    test_truth(destination.packet_count == gear.packet_count);
    test_truth(destination.pfds_size == gear.pfds_size);
    test_truth(destination.error.last_errno == gear.error.last_errno);
    test_truth(destination.timeout == gear.timeout);
    test_truth(destination.con_list == gear.con_list);
    test_truth(destination.packet_list == gear.packet_list);
    test_truth(destination.pfds == gear.pfds);
    test_truth(destination.log_fn == gear.log_fn);
    test_truth(destination.log_context == gear.log_context);
    test_truth(destination.workload_malloc_fn == gear.workload_malloc_fn);
    test_truth(destination.workload_malloc_context == gear.workload_malloc_context);
    test_truth(destination.workload_free_fn == gear.workload_free_fn);
    test_truth(destination.workload_free_context == gear.workload_free_context);

    gearman_universal_free(gear);
  }

  gearman_universal_free(gear);

  return TEST_SUCCESS;
}

static test_return_t set_timout_test(void *)
{
  gearman_universal_st universal;

  gearman_universal_initialize(universal);

  test_compare(-1, gearman_universal_timeout(universal)); // Current default

  gearman_universal_set_timeout(universal, 20);
  test_compare(20, gearman_universal_timeout(universal)); // New value of 20

  gearman_universal_set_timeout(universal, 10);
  test_compare(10, gearman_universal_timeout(universal)); // New value of 10

  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

static test_return_t basic_error_test(void *)
{
  gearman_universal_st universal;

  gearman_universal_initialize(universal);

  const char *error= gearman_universal_error(universal);
  test_false(error);

  test_compare(0, gearman_universal_errno(universal));

  gearman_universal_free(universal);

  return TEST_SUCCESS;
}


static test_return_t state_option_test(void *)
{
  gearman_universal_st universal;

  gearman_universal_initialize(universal);
  { // Initial Allocated, no changes
    test_false(universal.options.dont_track_packets);
    test_false(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }
  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

static test_return_t state_option_on_create_test(void *)
{
  gearman_universal_st universal;
  gearman_universal_options_t options[]= { GEARMAN_NON_BLOCKING, GEARMAN_DONT_TRACK_PACKETS, GEARMAN_MAX};

  gearman_universal_initialize(universal, options);
  { // Initial Allocated, no changes
    test_truth(universal.options.dont_track_packets);
    test_truth(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }
  gearman_universal_free(universal);

  return TEST_SUCCESS;
}


static test_return_t state_option_set_test(void *)
{
  gearman_universal_st universal;
  gearman_universal_options_t options[]= { GEARMAN_NON_BLOCKING, GEARMAN_DONT_TRACK_PACKETS, GEARMAN_MAX};

  gearman_universal_initialize(universal, options);
  { // Initial Allocated, no changes
    test_truth(universal.options.dont_track_packets);
    test_truth(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }

  test_truth(gearman_universal_is_non_blocking(universal));

  gearman_universal_initialize(universal);
  { // Initial Allocated, no changes
    test_false(universal.options.dont_track_packets);
    test_false(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }

  gearman_universal_add_options(universal, GEARMAN_DONT_TRACK_PACKETS);
  { // Initial Allocated, no changes
    test_truth(universal.options.dont_track_packets);
    test_false(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }

  gearman_universal_remove_options(universal, GEARMAN_DONT_TRACK_PACKETS);
  { // Initial Allocated, no changes
    test_false(universal.options.dont_track_packets);
    test_false(universal.options.non_blocking);
    test_false(universal.options.stored_non_blocking);
  }

  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

test_st universal_st_test[] ={
  {"init", 0, init_test },
  {"clone_test", 0, clone_test },
  {"set_timeout", 0, set_timout_test },
  {"basic_error", 0, basic_error_test },
  {"state_options", 0, state_option_test },
  {"state_options_on_create", 0, state_option_on_create_test},
  {"state_options_set", 0, state_option_set_test },
  {0, 0, 0}
};


static test_return_t connection_init_test(void *)
{
  gearman_universal_st universal;

  gearman_universal_initialize(universal);

  gearman_connection_st *connection_ptr= gearman_connection_create(universal, NULL);
  test_truth(connection_ptr);

  test_false(connection_ptr->options.ready);
  test_false(connection_ptr->options.packet_in_use);
  test_false(connection_ptr->options.external_fd);
  test_false(connection_ptr->options.ignore_lost_connection);
  test_false(connection_ptr->options.close_after_flush);

  delete connection_ptr;

  return TEST_SUCCESS;
}

static test_return_t connection_alloc_test(void *)
{
  gearman_universal_st universal;

  gearman_universal_initialize(universal);

  gearman_connection_st *connection_ptr= gearman_connection_create(universal, NULL);
  test_truth(connection_ptr);

  test_false(connection_ptr->options.ready);
  test_false(connection_ptr->options.packet_in_use);
  test_false(connection_ptr->options.external_fd);
  test_false(connection_ptr->options.ignore_lost_connection);
  test_false(connection_ptr->options.close_after_flush);

  delete connection_ptr;

  return TEST_SUCCESS;
}

test_st connection_st_test[] ={
  {"init", 0, connection_init_test },
  {"alloc", 0, connection_alloc_test },
  {0, 0, 0}
};

static test_return_t packet_init_test(void *)
{
  gearman_universal_st universal;

  gearman_packet_st packet;
  gearman_packet_st *packet_ptr;

  gearman_universal_initialize(universal);

  packet_ptr= gearman_packet_create(universal, &packet);
  test_false(packet.options.allocated);
  test_false(packet_ptr->options.allocated);

  test_false(packet.options.complete);
  test_false(packet.options.free_data);

  test_truth(packet_ptr == &packet);

  gearman_packet_free(packet_ptr);
  test_false(packet.options.allocated);

  return TEST_SUCCESS;
}

static test_return_t gearman_packet_give_data_test(void *)
{
  char *data = strdup("Mine!");
  size_t data_size= strlen(data);
  gearman_universal_st universal;

  gearman_packet_st packet;

  gearman_universal_initialize(universal);

  test_truth(gearman_packet_create(universal, &packet));

  gearman_packet_give_data(packet, data, data_size);

  test_truth(packet.data == data);
  test_truth(packet.data_size == data_size);
  test_truth(packet.options.free_data);

  gearman_packet_free(&packet);
  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

static test_return_t gearman_packet_take_data_test(void *)
{
  char *data = strdup("Mine!");
  size_t data_size= strlen(data);
  gearman_universal_st universal;

  gearman_packet_st packet;

  gearman_universal_initialize(universal);

  gearman_packet_st *packet_ptr= gearman_packet_create(universal, &packet);
  test_truth(packet_ptr);

  gearman_packet_give_data(packet, data, data_size);

  test_truth(packet_ptr->data == data);
  test_compare(data_size, packet_ptr->data_size);
  test_truth(packet_ptr->options.free_data);

  size_t mine_size;
  char *mine= (char *)gearman_packet_take_data(packet, &mine_size);

  test_false(packet_ptr->data);
  test_compare(0, packet_ptr->data_size);
  test_false(packet_ptr->options.free_data);

  test_compare(mine, data);
  test_compare(data_size, mine_size);

  gearman_packet_free(packet_ptr);
  gearman_universal_free(universal);
  free(data);

  return TEST_SUCCESS;
}

test_st packet_st_test[] ={
  {"init", 0, packet_init_test },
  {"gearman_packet_give_data", 0, gearman_packet_give_data_test },
  {"gearman_packet_take_data", 0, gearman_packet_take_data_test },
  {0, 0, 0}
};


collection_st collection[] ={
  {"gearman_universal_st", 0, 0, universal_st_test},
  {"gearman_connection_st", 0, 0, connection_st_test},
  {"gearman_packet_st", 0, 0, packet_st_test},
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
}
