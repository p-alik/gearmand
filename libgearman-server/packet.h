/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Packet Declarations
 */

#pragma once

#include <libgearman-server/struct/packet.h>

#include <libgearman-1.0/protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_packet Packet Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server connections. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */


/**
 * Initialize a server packet structure.
 */
GEARMAN_API
gearman_server_packet_st *
gearman_server_packet_create(gearman_server_thread_st *thread,
                             bool from_thread);

GEARMAN_LOCAL
const char *gearmand_strcommand(gearmand_packet_st *packet);

/**
 * Free a server connection structure.
 */
GEARMAN_API
void gearman_server_packet_free(gearman_server_packet_st *packet,
                                gearman_server_thread_st *thread,
                                bool from_thread);

/**
 * Add a server packet structure to io queue for a connection.
 */
GEARMAN_API
gearmand_error_t gearman_server_io_packet_add(gearman_server_con_st *con,
                                              bool take_data,
                                              enum gearman_magic_t magic,
                                              gearman_command_t command,
                                              const void *arg, ...);

/**
 * Remove the first server packet structure from io queue for a connection.
 */
GEARMAN_API
void gearman_server_io_packet_remove(gearman_server_con_st *con);

/**
 * Add a server packet structure to proc queue for a connection.
 */
GEARMAN_API
void gearman_server_proc_packet_add(gearman_server_con_st *con,
                                    gearman_server_packet_st *packet);

/**
 * Remove the first server packet structure from proc queue for a connection.
 */
GEARMAN_API
gearman_server_packet_st *
gearman_server_proc_packet_remove(gearman_server_con_st *con);


/**
 * Initialize a packet structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] packet Caller allocated structure, or NULL to allocate one.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_INTERNAL_API
void gearmand_packet_init(gearmand_packet_st *packet, enum gearman_magic_t magic, gearman_command_t command);

/**
 * Free a packet structure.
 *
 * @param[in] packet Structure previously initialized with
 *   gearmand_packet_init() or gearmand_packet_creates().
 */
GEARMAN_INTERNAL_API
void gearmand_packet_free(gearmand_packet_st *packet);

/**
 * Add an argument to a packet.
 */
GEARMAN_INTERNAL_API
  gearmand_error_t gearmand_packet_create(gearmand_packet_st *packet,
                                              const void *arg, size_t arg_size);

/**
 * Pack header.
 */
GEARMAN_INTERNAL_API
  gearmand_error_t gearmand_packet_pack_header(gearmand_packet_st *packet);

void destroy_gearman_server_packet_st(gearman_server_packet_st *packet);

/** @} */

#ifdef __cplusplus
}
#endif
