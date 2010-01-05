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

#define CLIENT_TEST_PORT 32123

static test_return_t init_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st gear;
  gearman_universal_st *gear_ptr;

  gear_ptr= gearman_universal_create(&gear, NULL);
  test_false(gear.options.allocated);
  test_false(gear_ptr->options.allocated);
  test_false(gear.options.dont_track_packets);
  test_false(gear.options.non_blocking);
  test_false(gear.options.stored_non_blocking);
  test_truth(gear_ptr == &gear);

  gearman_universal_free(&gear);
  test_false(gear.options.allocated);

  return TEST_SUCCESS;
}

static test_return_t clone_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st gear;
  gearman_universal_st *gear_ptr;

  gear_ptr= gearman_universal_create(&gear, NULL);
  test_truth(gear_ptr);
  test_truth(gear_ptr == &gear);

#if 0
    gearman_universal_st *gear_clone;

    /* For gearman_universal_st we don't allow NULL cloning creation */
    gear_clone= gearman_universal_clone(NULL, NULL);
    test_truth(gear_clone);
    gear_clone= gearman_universal_clone(NULL, &gear);
    test_truth(gear_clone);
    gear_clone= gearman_universal_clone(&gear, NULL);
    test_truth(gear_clone);
#endif

  /* Can we init from null? */
  {
    gearman_universal_st *gear_clone;
    gearman_universal_st destination;
    gear_clone= gearman_universal_clone(&destination, &gear);
    test_truth(gear_clone);
    test_false(gear_clone->options.allocated);

    { // Test all of the flags
      test_truth(gear_clone->options.dont_track_packets == gear_ptr->options.dont_track_packets);
      test_truth(gear_clone->options.non_blocking == gear_ptr->options.non_blocking);
      test_truth(gear_clone->options.stored_non_blocking == gear_ptr->options.stored_non_blocking);
    }
    test_truth(gear_clone->verbose == gear_ptr->verbose);
    test_truth(gear_clone->con_count == gear_ptr->con_count);
    test_truth(gear_clone->packet_count == gear_ptr->packet_count);
    test_truth(gear_clone->pfds_size == gear_ptr->pfds_size);
    test_truth(gear_clone->last_errno == gear_ptr->last_errno);
    test_truth(gear_clone->timeout == gear_ptr->timeout);
    test_truth(gear_clone->con_list == gear_ptr->con_list);
    test_truth(gear_clone->packet_list == gear_ptr->packet_list);
    test_truth(gear_clone->pfds == gear_ptr->pfds);
    test_truth(gear_clone->log_fn == gear_ptr->log_fn);
    test_truth(gear_clone->log_context == gear_ptr->log_context);
    test_truth(gear_clone->event_watch_fn == gear_ptr->event_watch_fn);
    test_truth(gear_clone->event_watch_context == gear_ptr->event_watch_context);
    test_truth(gear_clone->workload_malloc_fn == gear_ptr->workload_malloc_fn);
    test_truth(gear_clone->workload_malloc_context == gear_ptr->workload_malloc_context);
    test_truth(gear_clone->workload_free_fn == gear_ptr->workload_free_fn);
    test_truth(gear_clone->workload_free_context == gear_ptr->workload_free_context);

    gearman_universal_free(gear_clone);
  }

  gearman_universal_free(gear_ptr);

  return TEST_SUCCESS;
}

static test_return_t set_timout_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st gear;
  gearman_universal_st *gear_ptr;
  int time_data;

  gear_ptr= gearman_universal_create(&gear, NULL);
  test_truth(gear_ptr);
  test_truth(gear_ptr == &gear);
  test_false(gear_ptr->options.allocated);

  time_data= gearman_universal_timeout(gear_ptr);
  test_truth (time_data == -1); // Current default

  gearman_universal_set_timeout(gear_ptr, 20);
  time_data= gearman_universal_timeout(gear_ptr);
  test_truth (time_data == 20); // Current default

  gearman_universal_set_timeout(gear_ptr, 10);
  time_data= gearman_universal_timeout(gear_ptr);
  test_truth (time_data == 10); // Current default

  test_truth(gear_ptr == &gear); // Make sure noting got slipped in :)
  gearman_universal_free(gear_ptr);

  return TEST_SUCCESS;
}

static test_return_t basic_error_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st gear;
  gearman_universal_st *gear_ptr;
  const char *error;
  int error_number;

  gear_ptr= gearman_universal_create(&gear, NULL);
  test_truth(gear_ptr);
  test_truth(gear_ptr == &gear);
  test_false(gear_ptr->options.allocated);

  error= gearman_universal_error(gear_ptr);
  test_truth(error == NULL);

  error_number= gearman_universal_errno(gear_ptr);
  test_truth(error_number == 0);

  test_truth(gear_ptr == &gear); // Make sure noting got slipped in :)
  gearman_universal_free(gear_ptr);

  return TEST_SUCCESS;
}


static test_return_t state_option_test(void *object __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_truth(universal_ptr);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_false(universal_ptr->options.dont_track_packets);
    test_false(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }
  gearman_universal_free(universal_ptr);

  return TEST_SUCCESS;
}

static test_return_t state_option_on_create_test(void *object __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;
  gearman_universal_options_t options[]= { GEARMAN_NON_BLOCKING, GEARMAN_DONT_TRACK_PACKETS, GEARMAN_MAX};

  universal_ptr= gearman_universal_create(&universal, options);
  test_truth(universal_ptr);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_truth(universal_ptr->options.dont_track_packets);
    test_truth(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }
  gearman_universal_free(universal_ptr);

  return TEST_SUCCESS;
}


static test_return_t state_option_set_test(void *object __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;
  gearman_universal_options_t options[]= { GEARMAN_NON_BLOCKING, GEARMAN_DONT_TRACK_PACKETS, GEARMAN_MAX};

  universal_ptr= gearman_universal_create(&universal, options);
  test_truth(universal_ptr);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_truth(universal_ptr->options.dont_track_packets);
    test_truth(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }

  test_truth(gearman_universal_is_non_blocking(universal_ptr));

  universal_ptr= gearman_universal_create(&universal, NULL);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_false(universal_ptr->options.dont_track_packets);
    test_false(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }

  gearman_universal_add_options(universal_ptr, GEARMAN_DONT_TRACK_PACKETS);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_truth(universal_ptr->options.dont_track_packets);
    test_false(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }

  gearman_universal_remove_options(universal_ptr, GEARMAN_DONT_TRACK_PACKETS);
  { // Initial Allocated, no changes
    test_false(universal_ptr->options.allocated);
    test_false(universal_ptr->options.dont_track_packets);
    test_false(universal_ptr->options.non_blocking);
    test_false(universal_ptr->options.stored_non_blocking);
  }

  gearman_universal_free(universal_ptr);

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


static test_return_t connection_init_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  gearman_connection_st connection;
  gearman_connection_st *connection_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_false(universal.options.allocated);
  test_false(universal_ptr->options.allocated);

  connection_ptr= gearman_connection_create(universal_ptr, &connection, NULL);
  test_false(connection.options.allocated);
  test_false(connection_ptr->options.allocated);

  test_false(connection.options.ready);
  test_false(connection.options.packet_in_use);
  test_false(connection.options.external_fd);
  test_false(connection.options.ignore_lost_connection);
  test_false(connection.options.close_after_flush);

  test_truth(connection_ptr == &connection);

  gearman_connection_free(connection_ptr);
  test_false(connection.options.allocated);

  return TEST_SUCCESS;
}

static test_return_t connection_alloc_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  gearman_connection_st *connection_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_false(universal.options.allocated);
  test_false(universal_ptr->options.allocated);

  connection_ptr= gearman_connection_create(universal_ptr, NULL, NULL);
  test_truth(connection_ptr->options.allocated);

  test_false(connection_ptr->options.ready);
  test_false(connection_ptr->options.packet_in_use);
  test_false(connection_ptr->options.external_fd);
  test_false(connection_ptr->options.ignore_lost_connection);
  test_false(connection_ptr->options.close_after_flush);

  gearman_connection_free(connection_ptr);

  return TEST_SUCCESS;
}

test_st connection_st_test[] ={
  {"init", 0, connection_init_test },
  {"alloc", 0, connection_alloc_test },
  {0, 0, 0}
};

static test_return_t packet_init_test(void *not_used __attribute__((unused)))
{
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  gearman_packet_st packet;
  gearman_packet_st *packet_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_false(universal.options.allocated);
  test_false(universal_ptr->options.allocated);

  packet_ptr= gearman_packet_create(universal_ptr, &packet);
  test_false(packet.options.allocated);
  test_false(packet_ptr->options.allocated);

  test_false(packet.options.complete);
  test_false(packet.options.free_data);

  test_truth(packet_ptr == &packet);

  gearman_packet_free(packet_ptr);
  test_false(packet.options.allocated);

  return TEST_SUCCESS;
}

static test_return_t gearman_packet_give_data_test(void *not_used __attribute__((unused)))
{
  char *data = strdup("Mine!");
  size_t data_size= strlen(data);
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  gearman_packet_st packet;
  gearman_packet_st *packet_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_truth(universal_ptr);

  packet_ptr= gearman_packet_create(universal_ptr, &packet);
  test_truth(packet_ptr);

  gearman_packet_give_data(packet_ptr, data, data_size);

  test_truth(packet_ptr->data == data);
  test_truth(packet_ptr->data_size == data_size);
  test_truth(packet_ptr->options.free_data);

  gearman_packet_free(packet_ptr);
  gearman_universal_free(universal_ptr);

  return TEST_SUCCESS;
}

static test_return_t gearman_packet_take_data_test(void *not_used __attribute__((unused)))
{
  char *data = strdup("Mine!");
  char *catch;
  size_t data_size= strlen(data);
  size_t catch_size;
  gearman_universal_st universal;
  gearman_universal_st *universal_ptr;

  gearman_packet_st packet;
  gearman_packet_st *packet_ptr;

  universal_ptr= gearman_universal_create(&universal, NULL);
  test_truth(universal_ptr);

  packet_ptr= gearman_packet_create(universal_ptr, &packet);
  test_truth(packet_ptr);

  gearman_packet_give_data(packet_ptr, data, data_size);

  test_truth(packet_ptr->data == data);
  test_truth(packet_ptr->data_size == data_size);
  test_truth(packet_ptr->options.free_data);

  catch= gearman_packet_take_data(packet_ptr, &catch_size);

  test_false(packet_ptr->data);
  test_false(packet_ptr->data_size);
  test_false(packet_ptr->options.free_data);

  test_truth(catch == data);
  test_truth(data_size == catch_size);

  gearman_packet_free(packet_ptr);
  gearman_universal_free(universal_ptr);
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
