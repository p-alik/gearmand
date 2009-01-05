/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server worker declarations
 */

#ifndef __GEARMAN_SERVER_WORKER_H__
#define __GEARMAN_SERVER_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_worker Server Worker Handling
 * @ingroup gearman_server
 * This is a low level interface for gearman server workers. This is used
 * internally by the server interface, so you probably want to look there first.
 * @{
 */

/**
 * Add a new worker to a server instance.
 */
gearman_server_worker_st *
gearman_server_worker_add(gearman_server_con_st *server_con,
                          const char *function_name,
                          size_t function_name_size,
                          uint32_t timeout);

/**
 * Initialize a server worker structure.
 */
gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *server_con,
                             gearman_server_function_st *server_function,
                             gearman_server_worker_st *server_worker);

/**
 * Free a server worker structure.
 */
void gearman_server_worker_free(gearman_server_worker_st *server_worker);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_WORKER_H__ */
