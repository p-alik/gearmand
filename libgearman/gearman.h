/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman core declarations
 */

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/uio.h>

#include <libgearman/constants.h>
#include <libgearman/structs.h>
#include <libgearman/con.h>
#include <libgearman/packet.h>
#include <libgearman/task.h>
#include <libgearman/job.h>
#include <libgearman/client.h>
#include <libgearman/worker.h>
#include <libgearman/server_con.h>
#include <libgearman/server_function.h>
#include <libgearman/server_client.h>
#include <libgearman/server_worker.h>
#include <libgearman/server_job.h>
#include <libgearman/server.h>
#include <libgearman/gearmand.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman Gearman Core Interface
 * This is a low level interface gearman library instances. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 *
 * There is no locking within a single gearman_st structure, so for threaded
 * applications you must either ensure isolation in the application or use
 * multiple gearman_st structures (for example, one for each thread).
 * @{
 */


/**
 * Return gearman version
 */
const char *gearman_version();

/**
 * Return gearman bug report URL
 */
const char *gearman_bugreport();

/**
 * Initialize a gearman structure.
 */
gearman_st *gearman_create(gearman_st *gearman);

/**
 * Clone a gearman structure.
 */
gearman_st *gearman_clone(gearman_st *gearman, gearman_st *from);

/**
 * Free a gearman structure.
 */
void gearman_free(gearman_st *gearman);

/**
 * Return an error string for last library error encountered.
 */
const char *gearman_error(gearman_st *gearman);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 */
int gearman_errno(gearman_st *gearman);

/**
 * Set options for a gearman structure.
 */
void gearman_set_options(gearman_st *gearman, gearman_options_t options,
                         uint32_t data);

/**
 * Set custom I/O event callbacks for a gearman structure.
 */
void gearman_set_event_watch(gearman_st *gearman,
                             gearman_event_watch_fn *event_watch,
                             void *event_watch_arg);

/**
 * Set custom memory allocation function for workloads. Normally gearman uses
 * the standard system malloc to allocate memory used with workloads. This
 * function is used instead.
 */
void gearman_set_workload_malloc(gearman_st *gearman,
                                 gearman_malloc_fn *workload_malloc,
                                 const void *workload_malloc_arg);

/**
 * Set custom memory free function for workloads. Normally gearman uses the
 * standard system free to free memory used with workloads. This function
 * is used instead.
 */
void gearman_set_workload_free(gearman_st *gearman,
                               gearman_free_fn *workload_free,
                               const void *workload_free_arg);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
