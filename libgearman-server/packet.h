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

#ifndef __GEARMAN_SERVER_PACKET_H__
#define __GEARMAN_SERVER_PACKET_H__

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


struct gearman_server_packet_st
{
  gearman_packet_st packet;
  gearman_server_packet_st *next;
};


/**
 * Initialize a server packet structure.
 */
GEARMAN_API
gearman_server_packet_st *
gearman_server_packet_create(gearman_server_thread_st *thread,
                             bool from_thread);

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
gearman_return_t gearman_server_io_packet_add(gearman_server_con_st *con,
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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_PACKET_H__ */
