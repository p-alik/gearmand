/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
 * @code
 * ret= gearman_packet_add_args(gearman, packet,
 *                              GEARMAN_MAGIC_REQUEST,
 *                              GEARMAN_COMMAND_SUBMIT_JOB,
 *                              function_name, strlen(function_name) + 1,
 *                              unique_string, strlen(unique_string) + 1,
 *                              workload, workload_size, NULL);
 * @endcode
 */
gearman_return_t gearman_packet_add(gearman_st *gearman,
                                    gearman_packet_st *packet,
                                    gearman_magic_t magic,
                                    gearman_command_t command,
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
                                gearman_packet_options_t options,
                                uint32_t data);

/**
 * Add an argument to a packet.
 */
gearman_return_t gearman_packet_add_arg(gearman_packet_st *packet,
                                        const void *arg, size_t arg_size);

/**
 * Pack header.
 */
gearman_return_t gearman_packet_pack_header(gearman_packet_st *packet);

/**
 * Unpack header.
 */
gearman_return_t gearman_packet_unpack_header(gearman_packet_st *packet);

/**
 * Parse packet from input data.
 */
size_t gearman_packet_parse(gearman_packet_st *packet, const uint8_t *data,
                            size_t data_size, gearman_return_t *ret_ptr);

/**
 * Take allocated data from packet. After this, the caller is responsible for
 * free()ing the memory.
 */
void *gearman_packet_take_data(gearman_packet_st *packet, size_t *size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_PACKET_H__ */
