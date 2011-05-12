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

#include <libgearman/protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

enum gearman_magic_t
{
  GEARMAN_MAGIC_TEXT,
  GEARMAN_MAGIC_REQUEST,
  GEARMAN_MAGIC_RESPONSE
};

/**
 * @ingroup gearman_packet
 */
struct gearmand_packet_st
{
  struct {
    bool complete;
    bool free_data;
  } options;
  enum gearman_magic_t magic;
  gearman_command_t command;
  uint8_t argc;
  size_t args_size;
  size_t data_size;
  struct gearmand_packet_st *next;
  struct gearmand_packet_st *prev;
  char *args;
  const char *data;
  char *arg[GEARMAN_MAX_COMMAND_ARGS];
  size_t arg_size[GEARMAN_MAX_COMMAND_ARGS];
  char args_buffer[GEARMAN_ARGS_BUFFER_SIZE];
};

struct gearman_command_info_st
{
  const char *name;
  const uint8_t argc; // Number of arguments to commands.
  const bool data;
};

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
  gearmand_packet_st packet;
  gearman_server_packet_st *next;
};

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
 * Command information array.
 * @ingroup gearman_constants
 */
extern GEARMAN_INTERNAL_API
struct gearman_command_info_st gearmand_command_info_list[GEARMAN_COMMAND_MAX];


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

/**
 * Pack packet into output buffer.
 */
GEARMAN_INTERNAL_API
  size_t gearmand_packet_pack(const gearmand_packet_st *packet,
                              gearman_server_con_st *con,
                              void *data, size_t data_size,
                              gearmand_error_t *ret_ptr);

/**
 * Unpack packet from input data.
 */
GEARMAN_INTERNAL_API
  size_t gearmand_packet_unpack(gearmand_packet_st *packet,
                                gearman_server_con_st *con,
                                const void *data, size_t data_size,
                                gearmand_error_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_PACKET_H__ */
