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
  bool _shutdown;
  pthread_mutex_t _shutdown_lock;
  std::string _shutdown_function;
  std::string _name;
  gearman_id_t worker_id;
  in_port_t _port;

  in_port_t port() const
  {
    return _port;
  }

  const std::string& name() const
  {
    return _name;
  }

  worker_handle_st(const char *namespace_key, const std::string& name_arg, in_port_t);
  ~worker_handle_st();

  void set_shutdown()
  {
    pthread_mutex_lock(&_shutdown_lock);
    _shutdown= true;
    pthread_mutex_unlock(&_shutdown_lock);
  }

  bool is_shutdown();
  bool shutdown();
};

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

LIBTEST_API
  struct worker_handle_st *test_worker_start(in_port_t port, 
					     const char *namespace_key,
					     const char *function_name,
					     gearman_function_t &worker_fn,
					     void *context,
					     gearman_worker_options_t options);

LIBTEST_API
bool test_worker_stop(struct worker_handle_st *);

#ifdef __cplusplus
}
#endif
