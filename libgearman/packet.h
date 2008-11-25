/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief Packet declarations
 */

#ifndef __GEARMAN_PACKET_H__
#define __GEARMAN_PACKET_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_packet Packet Handling
 * This is a low level interface for gearman packet. This is used internally
 * internally by both client and worker interfaces (or more specifically, tasks
 * and jobs), so you probably want to look there first. This is usually used to
 * write lower level clients, workers, proxies, or your own server.
 * @{
 */

/**
 * Initialize a packet with all arguments. Variable list is NULL terminated
 * alternating argument and argument size (size_t) pairs. For example:
 * ret= gearman_packet_add_args(packet,
 *                              GEARMAN_MAGIC_REQUEST,
 *                              GEARMAN_COMMAND_SUBMIT_JOB,
 *                              function_name, strlen(function_name) + 1,
 *                              unique_string, strlen(unique_string) + 1,
 *                              workload, workload_size, NULL);
 */
gearman_return gearman_packet_add(gearman_st *gearman,
                                  gearman_packet_st *packet,
                                  gearman_magic magic, gearman_command command,
                                  const void *arg, ...);

/**
 * Initialize a packet structure.
 */
gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet);

/**
 * Free a packet structure.
 */
void gearman_packet_free(gearman_packet_st *packet);

/**
 * Set options for a packet structure.
 */
void gearman_packet_set_options(gearman_packet_st *packet,
                                gearman_packet_options options, uint32_t data);

/**
 * Add an argument to a packet.
 */
gearman_return gearman_packet_add_arg(gearman_packet_st *packet,
                                      const void *arg, size_t arg_size);

/**
 * Pack header.
 */
gearman_return gearman_packet_pack_header(gearman_packet_st *packet);

/**
 * Unpack header.
 */
gearman_return gearman_packet_unpack_header(gearman_packet_st *packet);

/**
 * Parse packet from input data.
 */
size_t gearman_packet_parse(gearman_packet_st *packet, const uint8_t *data,
                            size_t data_size, gearman_return *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_PACKET_H__ */
