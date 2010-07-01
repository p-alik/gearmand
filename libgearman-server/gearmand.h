/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Declarations
 */

#ifndef __GEARMAND_H__
#define __GEARMAND_H__

#include <libgearman/gearman.h>
#include <event.h>

#include <libgearman-server/constants.h>
#include <libgearman-server/log.h>
#include <libgearman-server/conf.h>
#include <libgearman-server/conf_module.h>
#include <libgearman-server/connection.h>
#include <libgearman-server/packet.h>
#include <libgearman-server/function.h>
#include <libgearman-server/client.h>
#include <libgearman-server/worker.h>
#include <libgearman-server/job.h>
#include <libgearman-server/thread.h>
#include <libgearman-server/server.h>
#include <libgearman-server/gearmand_thread.h>
#include <libgearman-server/gearmand_con.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand Gearmand Declarations
 *
 * This is a server implementation using the gearman_server interface.
 *
 * @{
 */

struct gearmand_port_st
{
  in_port_t port;
  uint32_t listen_count;
  gearmand_st *gearmand;
  gearman_connection_add_fn *add_fn;
  int *listen_fd;
  struct event *listen_event;
};

struct gearmand_st
{
  gearman_verbose_t verbose;
  gearman_return_t ret;
  int backlog;
  bool is_listen_event;
  bool is_wakeup_event;
  uint32_t port_count;
  uint32_t threads;
  uint32_t thread_count;
  uint32_t free_dcon_count;
  uint32_t max_thread_free_dcon_count;
  int wakeup_fd[2];
  const char *host;
  gearman_log_fn *log_fn;
  void *log_context;
  struct event_base *base;
  gearmand_port_st *port_list;
  gearmand_thread_st *thread_list;
  gearmand_thread_st *thread_add_next;
  gearmand_con_st *free_dcon_list;
  gearman_server_st server;
  struct event wakeup_event;
};

/**
 * Create a server instance.
 * @param host Host for the server to listen on.
 * @param port Port for the server to listen on.
 * @return Pointer to an allocated gearmand structure.
 */
GEARMAN_API
gearmand_st *gearmand_create(const char *host, in_port_t port);

/**
 * Free resources used by a server instace.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 */
GEARMAN_API
void gearmand_free(gearmand_st *gearmand);

/**
 * Set socket backlog for listening connection.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param backlog Number of backlog connections to set during listen.
 */
GEARMAN_API
void gearmand_set_backlog(gearmand_st *gearmand, int backlog);

/**
 * Set maximum job retry count.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param job_retries Number of job attempts.
 */
GEARMAN_API
void gearmand_set_job_retries(gearmand_st *gearmand, uint8_t job_retries);

/**
 * Set maximum number of workers to wake up per job.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param worker_wakeup Number of workers to wake up.
 */
GEARMAN_API
void gearmand_set_worker_wakeup(gearmand_st *gearmand, uint8_t worker_wakeup);

/**
 * Set number of I/O threads for server to use.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param threads Number of threads.
 */
GEARMAN_API
void gearmand_set_threads(gearmand_st *gearmand, uint32_t threads);

/**
 * Set logging callback for server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param function Function to call when there is a logging message.
 * @param context Argument to pass into the log callback function.
 * @param verbose Verbosity level.
 */
GEARMAN_API
void gearmand_set_log_fn(gearmand_st *gearmand, gearman_log_fn *function,
                         void *context, gearman_verbose_t verbose);

/**
 * Add a port to listen on when starting server with optional callback.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param port Port for the server to listen on.
 * @param function Optional callback function that is called when a connection
           has been accepted on the given port.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearmand_port_add(gearmand_st *gearmand, in_port_t port,
                                   gearman_connection_add_fn *function);

/**
 * Run the server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearmand_run(gearmand_st *gearmand);

/**
 * Interrupt a running gearmand server from another thread. You should only
 * call this when another thread is currently running gearmand_run() and you
 * want to wakeup the server with the given event.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param wakeup Wakeup event to send to running gearmand.
 */
GEARMAN_API
void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup);

/**
 * Sets the round-robin mode on the server object. RR will distribute work
 * fairly among every function assigned to a worker, instead of draining
 * each function before moving on to the next.
 * @param gearmand Server instance previously initialized
 * @param bool true=round robin is used, false=round robin is not used
 */
GEARMAN_API
void gearmand_set_round_robin(gearmand_st *gearmand, bool round_robin);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_H__ */
