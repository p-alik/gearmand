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

#ifndef __GEARMAN_PACKET_H__
#define __GEARMAN_PACKET_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a packet with all arguments. Variable list is alternating
   argument and argument size (size_t) pairs, terminated by a NULL argument.
   For example:
   ret= gearman_packet_add_args(packet,
                                GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_GRAB_JOB,
                                arg1, arg1_size,
                                arg2, arg2_size, NULL);
*/
gearman_return gearman_packet_add(gearman_st *gearman,
                                  gearman_packet_st *packet,
                                  gearman_magic magic, gearman_command command,
                                  const uint8_t *arg, ...);

/* Initialize a packet structure. */
gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet);

/* Free a packet structure. */
void gearman_packet_free(gearman_packet_st *packet);

/* Set header information for a packet and pack if complete. */
gearman_return gearman_packet_set_header(gearman_packet_st *packet,
                                         gearman_magic magic,
                                         gearman_command command);

/* Add an argument to a packet and pack if complete. */
gearman_return gearman_packet_add_arg(gearman_packet_st *packet,
                                      const uint8_t *arg, size_t arg_size);

/* Add raw argument data to a packet, call gearman_packet_unpack once all data
   has been added. */
gearman_return gearman_packet_add_arg_data(gearman_packet_st *packet,
                                           const uint8_t *data,
                                           size_t data_size);

/* Pack packet header information after all args have been added. */
gearman_return gearman_packet_pack(gearman_packet_st *packet);

/* Unpack packet information after all raw data has been added. */
gearman_return gearman_packet_unpack(gearman_packet_st *packet);

/* Data structures. */
struct gearman_packet_st
{
  gearman_st *gearman;
  gearman_packet_st *next;
  gearman_packet_st *prev;
  gearman_packet_options options;
  gearman_magic magic;
  gearman_command command;
  uint8_t argc;
  uint8_t *arg[GEARMAN_MAX_COMMAND_ARGS];
  size_t arg_size[GEARMAN_MAX_COMMAND_ARGS];
  uint8_t *data;
  uint8_t data_buffer[GEARMAN_PACKET_BUFFER_SIZE];
  size_t data_size;
};

struct gearman_command_info_st
{
  char *name;
  uint8_t argc;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_PACKET_H__ */
