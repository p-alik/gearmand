/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection Declarations
 */

#pragma once

#include <libgearman-server/io.h>
#include <libgearman-server/packet.h>

#include <libgearman-server/struct/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_con Connection Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server connections. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

/**
 * Add a connection to a server thread. This goes into a list of connections
 * that is used later with server_thread_run, no socket I/O happens here.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_init.
 * @param fd File descriptor for a newly accepted connection.
 * @param data Application data pointer.
 * @return Gearman server connection pointer.
 */
GEARMAN_API
gearman_server_con_st *gearman_server_con_add(gearman_server_thread_st *thread, gearmand_con_st *dcon,
                                              gearmand_error_t *ret);

/**
 * Free a server connection structure.
 */
GEARMAN_API
void gearman_server_con_free(gearman_server_con_st *con);

/**
 * Get gearman connection pointer the server connection uses.
 */
GEARMAN_API
gearmand_io_st *gearman_server_con_con(gearman_server_con_st *con);

/**
 * Get application data pointer.
 */
GEARMAN_API
gearmand_con_st *gearman_server_con_data(gearman_server_con_st *con);

/**
 * Get client id.
 */
GEARMAN_API
const char *gearman_server_con_id(gearman_server_con_st *con);

/**
 * Set client id.
 */
GEARMAN_API
void gearman_server_con_set_id(gearman_server_con_st *con, char *id,
                               size_t size);

/**
 * Free server worker struction with name for a server connection.
 */
GEARMAN_API
void gearman_server_con_free_worker(gearman_server_con_st *con,
                                    char *function_name,
                                    size_t function_name_size);

/**
 * Free all server worker structures for a server connection.
 */
GEARMAN_API
void gearman_server_con_free_workers(gearman_server_con_st *con);

/**
 * Add connection to the io thread list.
 */
GEARMAN_API
void gearman_server_con_io_add(gearman_server_con_st *con);

/**
 * Remove connection from the io thread list.
 */
GEARMAN_API
void gearman_server_con_io_remove(gearman_server_con_st *con);

/**
 * Get next connection from the io thread list.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_con_io_next(gearman_server_thread_st *thread);

/**
 * Add connection to the proc thread list.
 */
GEARMAN_API
void gearman_server_con_proc_add(gearman_server_con_st *con);

/**
 * Remove connection from the proc thread list.
 */
GEARMAN_API
void gearman_server_con_proc_remove(gearman_server_con_st *con);

/**
 * Get next connection from the proc thread list.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_con_proc_next(gearman_server_thread_st *thread);

/**
 * Set protocol context pointer.
 */
GEARMAN_INTERNAL_API
void gearmand_connection_set_protocol(gearman_server_con_st *connection, 
                                      void *context,
                                      gearmand_connection_protocol_context_free_fn *free_fn,
                                      gearmand_packet_pack_fn *pack,
                                      gearmand_packet_unpack_fn *unpack);

GEARMAN_INTERNAL_API
void *gearmand_connection_protocol_context(const gearman_server_con_st *connection);

/** @} */

#ifdef __cplusplus
}
#endif
