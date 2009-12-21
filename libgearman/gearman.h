/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman Declarations
 */

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <inttypes.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/uio.h>
#include <stdarg.h>

#include <libgearman/visibility.h>
#include <libgearman/constants.h>

// Everything above this line must be in the order specified.

#include <libgearman/command.h>
#include <libgearman/packet.h>
#include <libgearman/conn.h>
#include <libgearman/task.h>
#include <libgearman/job.h>

/**
  @note we define gearman_st before we include worker.h/client.h since they have static declarations internally of this structure.
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman
 */
struct gearman_st
{
  gearman_options_t options;
  gearman_verbose_t verbose;
  uint32_t con_count;
  uint32_t packet_count;
  uint32_t pfds_size;
  uint32_t sending;
  int last_errno;
  int timeout;
  gearman_con_st *con_list;
  gearman_packet_st *packet_list;
  struct pollfd *pfds;
  gearman_log_fn *log_fn;
  const void *log_context;
  gearman_event_watch_fn *event_watch_fn;
  const void *event_watch_context;
  gearman_malloc_fn *workload_malloc_fn;
  const void *workload_malloc_context;
  gearman_free_fn *workload_free_fn;
  const void *workload_free_context;
  char last_error[GEARMAN_MAX_ERROR_SIZE];
};

#ifdef __cplusplus
}
#endif

#include <libgearman/worker.h>
#include <libgearman/client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman Gearman Declarations
 *
 * This is a low level interface for gearman library instances. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 *
 * There is no locking within a single gearman_st structure, so for threaded
 * applications you must either ensure isolation in the application or use
 * multiple gearman_st structures (for example, one for each thread).
 *
 * @{
 */

/**
 * Get Gearman library version.
 *
 * @return Version string of library.
 */
GEARMAN_API
const char *gearman_version(void);

/**
 * Get bug report URL.
 *
 * @return Bug report URL string.
 */
GEARMAN_API
const char *gearman_bugreport(void);

/**
 * Get string with the name of the given verbose level.
 *
 * @param[in] verbose Verbose logging level.
 * @return String form of verbose level.
 */
GEARMAN_API
const char *gearman_verbose_name(gearman_verbose_t verbose);

/**
 * Initialize a gearman structure. Always check the return value even if passing
 * in a pre-allocated structure. Some other initialization may have failed. It
 * is not required to memset() a structure before providing it.
 *
 * @param[in] gearman Caller allocated structure, or NULL to allocate one.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_API
gearman_st *gearman_create(gearman_st *gearman);

/**
 * Clone a gearman structure.
 *
 * @param[in] gearman Caller allocated structure, or NULL to allocate one.
 * @param[in] from Structure to use as a source to clone from.
 * @return Same return as gearman_create().
 */
GEARMAN_API
gearman_st *gearman_clone(gearman_st *gearman, const gearman_st *from);

/**
 * Free a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 */
GEARMAN_API
void gearman_free(gearman_st *gearman);

/**
 * Return an error string for last error encountered.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Pointer to a buffer in the structure that holds an error string.
 */
GEARMAN_API
const char *gearman_error(const gearman_st *gearman);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return An errno value as defined in your system errno.h file.
 */
GEARMAN_API
int gearman_errno(const gearman_st *gearman);

/**
 * Get options for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Options set for the gearman structure.
 */
GEARMAN_API
gearman_options_t gearman_options(const gearman_st *gearman);

/**
 * Set options for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] options Available options for gearman structures.
 */
GEARMAN_API
void gearman_set_options(gearman_st *gearman, gearman_options_t options);

/**
 * Add options for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] options Available options for gearman structures.
 */
GEARMAN_API
void gearman_add_options(gearman_st *gearman, gearman_options_t options);

/**
 * Remove options for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] options Available options for gearman structures.
 */
GEARMAN_API
void gearman_remove_options(gearman_st *gearman, gearman_options_t options);

/**
 * Get current socket I/O activity timeout value.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Timeout in milliseconds to wait for I/O activity. A negative value
 *  means an infinite timeout.
 */
GEARMAN_API
int gearman_timeout(gearman_st *gearman);

/**
 * Set socket I/O activity timeout for connections in a Gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] timeout Milliseconds to wait for I/O activity. A negative value
 *  means an infinite timeout.
 */
GEARMAN_API
void gearman_set_timeout(gearman_st *gearman, int timeout);

/**
 * Set logging function for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Function to call when there is a logging message.
 * @param[in] context Argument to pass into the callback function.
 * @param[in] verbose Verbosity level threshold. Only call function when the
 *  logging message is equal to or less verbose that this.
 */
GEARMAN_API
void gearman_set_log_fn(gearman_st *gearman, gearman_log_fn *function,
                        const void *context, gearman_verbose_t verbose);

/**
 * Set custom I/O event callback function for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Function to call when there is an I/O event.
 * @param[in] context Argument to pass into the callback function.
 */
GEARMAN_API
void gearman_set_event_watch_fn(gearman_st *gearman,
                                gearman_event_watch_fn *function,
                                const void *context);

/**
 * Set custom memory allocation function for workloads. Normally gearman uses
 * the standard system malloc to allocate memory used with workloads. The
 * provided function will be used instead.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Memory allocation function to use instead of malloc().
 * @param[in] context Argument to pass into the callback function.
 */
GEARMAN_API
void gearman_set_workload_malloc_fn(gearman_st *gearman,
                                    gearman_malloc_fn *function,
                                    const void *context);

/**
 * Set custom memory free function for workloads. Normally gearman uses the
 * standard system free to free memory used with workloads. The provided
 * function will be used instead.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Memory free function to use instead of free().
 * @param[in] context Argument to pass into the callback function.
 */
GEARMAN_API
void gearman_set_workload_free_fn(gearman_st *gearman,
                                  gearman_free_fn *function,
                                  const void *context);

/*
 * Connection related functions.
 */

/**
 * Initialize a connection structure. Always check the return value even if
 * passing in a pre-allocated structure. Some other initialization may have
 * failed.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] con Caller allocated structure, or NULL to allocate one.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_API
gearman_con_st *gearman_add_con(gearman_st *gearman, gearman_con_st *con);

/**
 * Create a connection structure with the given host and port.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] con Caller allocated structure, or NULL to allocate one.
 * @param[in] host Host or IP address to connect to.
 * @param[in] port Port to connect to.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_API
gearman_con_st *gearman_add_con_args(gearman_st *gearman, gearman_con_st *con,
                                     const char *host, in_port_t port);

/**
 * Clone a connection structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] con Caller allocated structure, or NULL to allocate one.
 * @param[in] from Structure to use as a source to clone from.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_API
gearman_con_st *gearman_clone_con(gearman_st *gearman, gearman_con_st *con,
                                  const gearman_con_st *from);

/**
 * Free a connection structure.
 *
 * @param[in] con Structure previously initialized with gearman_add_con(),
 *  gearman_add_con_args(), or gearman_clone_con().
 */
GEARMAN_API
void gearman_con_free(gearman_con_st *con);

/**
 * Free all connections for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 */
GEARMAN_API
void gearman_free_all_cons(gearman_st *gearman);

/**
 * Flush the send buffer for all connections.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_flush_all(gearman_st *gearman);

/**
 * Send packet to all connections.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] packet Initialized packet to send to all connections.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_send_all(gearman_st *gearman,
                                  const gearman_packet_st *packet);

/**
 * Wait for I/O on connections.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_wait(gearman_st *gearman);

/**
 * Get next connection that is ready for I/O.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @return Connection that is ready for I/O, or NULL if there are none.
 */
GEARMAN_API
gearman_con_st *gearman_ready(gearman_st *gearman);

/**
 * Test echo with all connections.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] workload Data to send in echo packet.
 * @param[in] workload_size Size of workload.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_echo(gearman_st *gearman, const void *workload,
                              size_t workload_size);

/*
 * Packet related functions.
 */

/**
 * Initialize a packet structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] packet Caller allocated structure, or NULL to allocate one.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_API
gearman_packet_st *gearman_add_packet(gearman_st *gearman,
                                      gearman_packet_st *packet);

/**
 * Initialize a packet with all arguments. For example:
 *
 * @code
 * void *args[3];
 * size_t args_suze[3];
 *
 * args[0]= function_name;
 * args_size[0]= strlen(function_name) + 1;
 * args[1]= unique_string;
 * args_size[1]= strlen(unique_string,) + 1;
 * args[2]= workload;
 * args_size[2]= workload_size;
 *
 * ret= gearman_add_packet_args(gearman, packet,
 *                              GEARMAN_MAGIC_REQUEST,
 *                              GEARMAN_COMMAND_SUBMIT_JOB,
 *                              args, args_size, 3);
 * @endcode
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] packet Pre-allocated packet to initialize with arguments.
 * @param[in] magic Magic type for packet header.
 * @param[in] command Command type for packet.
 * @param[in] args Array of arguments to add.
 * @param[in] args_size Array of sizes of each byte array in the args array.
 * @param[in] args_count Number of elements in args/args_sizes arrays.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_add_packet_args(gearman_st *gearman,
                                         gearman_packet_st *packet,
                                         gearman_magic_t magic,
                                         gearman_command_t command,
                                         const void *args[],
                                         const size_t args_size[],
                                         size_t args_count);

/**
 * Free all packets for a gearman structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 */
GEARMAN_API
void gearman_free_all_packets(gearman_st *gearman);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
