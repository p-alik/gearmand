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

#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <poll.h>

#include <event.h>

#include <libgearman/visibility.h>
#include <libgearman/protocol.h>

#include <libgearman-server/error.h>

#include <libgearman-server/constants.h>
#include <libgearman-server/wakeup.h>
#include <libgearman-server/connection_list.h>
#include <libgearman-server/byteorder.h>
#include <libgearman-server/log.h>
#include <libgearman-server/packet.h>
#include <libgearman-server/connection.h>
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

struct gearmand_st
{
  gearmand_verbose_t verbose;
  gearmand_error_t ret;
  int backlog; // Set socket backlog for listening connection
  bool is_listen_event;
  bool is_wakeup_event;
  int timeout;
  uint32_t port_count;
  uint32_t threads;
  uint32_t thread_count;
  uint32_t free_dcon_count;
  uint32_t max_thread_free_dcon_count;
  int wakeup_fd[2];
  const char *host;
  gearmand_log_fn *log_fn;
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
 * @addtogroup gearmand Gearmand Declarations
 *
 * This is a server implementation using the gearman_server interface.
 *
 * @{
 */

GEARMAN_API
gearmand_st *Gearmand(void);

#define Server (&(Gearmand()->server))

/**
 * Create a server instance.
 * @param host Host for the server to listen on.
 * @param port Port for the server to listen on.
 * @return Pointer to an allocated gearmand structure.
 */
GEARMAN_API
gearmand_st *gearmand_create(const char *host,
                             const char *port,
                             uint32_t threads,
                             int backlog,
                             uint8_t job_retries,
                             uint8_t worker_wakeup,
                             gearmand_log_fn *function, void *log_context, const gearmand_verbose_t verbose,
                             bool round_robin);

/**
 * Free resources used by a server instace.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 */
GEARMAN_API
void gearmand_free(gearmand_st *gearmand);


GEARMAN_API
gearman_server_st *gearmand_server(gearmand_st *gearmand);

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
gearmand_error_t gearmand_port_add(gearmand_st *gearmand,
                                   const char *port,
                                   gearmand_connection_add_fn *function);

/**
 * Run the server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearmand_error_t gearmand_run(gearmand_st *gearmand);

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

GEARMAN_API
const char *gearmand_version(void);

GEARMAN_API
const char *gearmand_bugreport(void);

GEARMAN_API
const char *gearmand_verbose_name(gearmand_verbose_t verbose);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_H__ */
