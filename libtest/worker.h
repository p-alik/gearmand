/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <sys/types.h>
#include <pthread.h>

#include <libgearman/gearman.h>
#include <libtest/visibility.h>

struct worker_handle_st
{
  pthread_t thread;
  volatile bool shutdown;
};

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

LIBTEST_API
  struct worker_handle_st *test_worker_start(in_port_t port, const char *function_name,
                                             gearman_worker_fn *function, void *function_arg,
                                             gearman_worker_options_t options);

LIBTEST_API
  struct worker_handle_st *test_worker_start_with_namespace(in_port_t port, const char *function_name,
							    gearman_worker_fn *function, void *function_arg,
							    const char *namespace_key,
							    gearman_worker_options_t options);

LIBTEST_API
  struct worker_handle_st *test_worker_start_with_reducer(in_port_t port,
							  const char *namespace_key,
                                                          const char *function_name,
                                                          gearman_function_fn *mapper_fn, gearman_aggregator_fn *aggregator_fn,  
                                                          void *function_arg,
                                                          gearman_worker_options_t options);

LIBTEST_API
void test_worker_stop(struct worker_handle_st *);

#ifdef __cplusplus
}
#endif
