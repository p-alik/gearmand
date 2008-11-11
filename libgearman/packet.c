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

#include "common.h"

/* Command info. */
/* Update GEARMAN_MAX_COMMAND_ARGS to the largest number in the args column. */
gearman_command_info_st gearman_command_info_list[GEARMAN_COMMAND_MAX]=
{
  { "NONE",             0 },
  { "CAN_DO",           1 },
  { "CANT_DO",          1 },
  { "RESET_ABILITIES",  0 },
  { "PRE_SLEEP",        0 },
  { "UNUSED",           0 },
  { "NOOP",             0 },
  { "SUBMIT_JOB",       3 },
  { "JOB_CREATED",      1 },
  { "GRAB_JOB",         0 },
  { "NO_JOB",           0 },
  { "JOB_ASSIGN",       3 },
  { "WORK_STATUS",      3 },
  { "WORK_COMPLETE",    2 },
  { "WORK_FAIL",        1 },
  { "GET_STATUS",       1 },
  { "ECHO_REQ",         1 },
  { "ECHO_RES",         1 },
  { "SUBMIT_JOB_BJ",    3 },
  { "ERROR",            2 },
  { "STATUS_RES",       5 },
  { "SUBMIT_JOB_HIGH",  3 },
  { "SET_CLIENT_ID",    1 },
  { "CAN_DO_TIMEOUT",   2 },
  { "ALL_YOURS",        0 },
  { "SUBMIT_JOB_SCHED", 8 },
  { "SUBMIT_JOB_EPOCH", 4 }
};

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
                                  const uint8_t *arg, ...)
{
  va_list ap;
  size_t arg_size;
  gearman_return ret;

  packet= gearman_packet_create(gearman, packet);
  if (packet == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  ret= gearman_packet_set_header(packet, magic, command);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  va_start(ap, arg);

  while (arg != NULL)
  {
    arg_size = va_arg(ap, size_t);

    ret= gearman_packet_add_arg(packet, arg, arg_size);
    if (ret != GEARMAN_SUCCESS)
    {
      va_end(ap);
      return ret;
    }

    arg = va_arg(ap, uint8_t *);
  }

  va_end(ap);

  return GEARMAN_SUCCESS;
}

/* Initialize a packet structure. */
gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet)
{
  if (packet == NULL)
  {
    packet= malloc(sizeof(gearman_packet_st) + 12);
    if (packet == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_packet_create:malloc");
      return NULL;
    }

    memset(packet, 0, sizeof(gearman_packet_st));
    packet->options|= GEARMAN_PACKET_ALLOCATED;
  }
  else
    memset(packet, 0, sizeof(gearman_packet_st));

  packet->gearman= gearman;

  if (gearman->packet_list)
    gearman->packet_list->prev= packet;
  packet->next= gearman->packet_list;
  gearman->packet_list= packet;
  gearman->packet_count++;

  return packet;
}

/* Free a packet structure. */
void gearman_packet_free(gearman_packet_st *packet)
{
  if (packet->gearman->packet_list == packet)
    packet->gearman->packet_list= packet->next;
  if (packet->prev)
    packet->prev->next= packet->next;
  if (packet->next)
    packet->next->prev= packet->prev;
  packet->gearman->packet_count--;

  if (packet->data != packet->data_buffer && packet->data != NULL)
    free(packet->data);

  if (packet->options & GEARMAN_PACKET_ALLOCATED)
    free(packet);
}

/* Set header information for a packet. */
gearman_return gearman_packet_set_header(gearman_packet_st *packet,
                                         gearman_magic magic,
                                         gearman_command command)
{
  packet->magic= magic;
  packet->command= command;

  /* Pack it now if we have all arguments. */
  if (packet->argc == gearman_command_info_list[packet->command].argc)
    return gearman_packet_pack(packet);

  return GEARMAN_SUCCESS;
}

/* Add an argument to a packet. */
gearman_return gearman_packet_add_arg(gearman_packet_st *packet,
                                      const uint8_t *arg, size_t arg_size)
{
  size_t offset;
  gearman_return ret;
  uint8_t x;

  if (packet->argc == gearman_command_info_list[packet->command].argc)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_add_arg:too many args");
    return GEARMAN_TOO_MANY_ARGS;
  }

  if (packet->data_size == 0)
    packet->data_size= 12;

  ret= gearman_packet_add_arg_data(packet, arg, arg_size);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  packet->arg_size[packet->argc]= arg_size;
  packet->argc++;

  offset= 12;
  for (x= 0; x < packet->argc; x++)
  {
    packet->arg[x]= packet->data + offset;
    offset+= packet->arg_size[x];
  }

  /* Pack it now if we have magic, command, and all arguments. */
  if (packet->magic != GEARMAN_MAGIC_NONE &&
      packet->command != GEARMAN_COMMAND_NONE &&
      packet->argc == gearman_command_info_list[packet->command].argc)
  {
    return gearman_packet_pack(packet);
  }

  return GEARMAN_SUCCESS;
}

/* Add raw argument data to a packet, call gearman_packet_unpack once all data
   has been added. */
gearman_return gearman_packet_add_arg_data(gearman_packet_st *packet,
                                           const uint8_t *data,
                                           size_t data_size)
{
  uint8_t *new_data;

  if ((packet->data_size + data_size) < GEARMAN_PACKET_BUFFER_SIZE)
    packet->data= packet->data_buffer;
  else
  {
    if (packet->data == packet->data_buffer)
      packet->data= NULL;

    new_data= realloc(packet->data, packet->data_size + data_size);
    if (new_data == NULL)
    {
      GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_add_arg_data:realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    if (packet->data_size > 0)
      memcpy(new_data, packet->data_buffer, packet->data_size);

    packet->data= new_data;
  }

  if (data != NULL)
  {
    memcpy(packet->data + packet->data_size, data, data_size);
    packet->data_size+= data_size;
  }

  return GEARMAN_SUCCESS;
}

/* Pack packet header information after all args have been added. */
gearman_return gearman_packet_pack(gearman_packet_st *packet)
{
  gearman_return ret;
  uint32_t tmp;

  if (packet->data_size == 0)
  {
    packet->data_size= 12;

    ret= gearman_packet_add_arg_data(packet, NULL, 0);
    if (ret != GEARMAN_SUCCESS)
      return ret;
  }

  switch (packet->magic)
  {
  case GEARMAN_MAGIC_REQUEST:
    memcpy(packet->data, "\0REQ", 4);
    break;

  case GEARMAN_MAGIC_RESPONSE:
    memcpy(packet->data, "\0RES", 4);
    break;

  case GEARMAN_MAGIC_NONE:
  default:
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_pack:invalid magic");
    return GEARMAN_INVALID_MAGIC;
  }

  if (packet->command == GEARMAN_COMMAND_NONE ||
      packet->command >= GEARMAN_COMMAND_MAX)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_pack:invalid command");
    return GEARMAN_INVALID_COMMAND;
  }

  tmp= packet->command;
  tmp= htonl(tmp);
  memcpy(packet->data + 4, &tmp, 4);

  tmp= packet->data_size - 12;
  tmp= htonl(tmp);
  memcpy(packet->data + 8, &tmp, 4);

  packet->options|= GEARMAN_PACKET_PACKED;

  return GEARMAN_SUCCESS;
}

/* Unpack packet information after all raw data has been added. */
gearman_return gearman_packet_unpack(gearman_packet_st *packet)
{
  uint32_t tmp;
  size_t offset;
  uint8_t x;
  uint8_t *ptr;

  if (!memcmp(packet->data, "\0REQ", 4))
    packet->magic= GEARMAN_MAGIC_REQUEST;
  else if (!memcmp(packet->data, "\0RES", 4))
    packet->magic= GEARMAN_MAGIC_RESPONSE;
  else
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack:invalid magic");
    return GEARMAN_INVALID_MAGIC;
  }

  memcpy(&tmp, packet->data + 4, 4);
  packet->command= ntohl(tmp);

  if (packet->command >= GEARMAN_COMMAND_MAX)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack:invalid command");
    return GEARMAN_INVALID_COMMAND;
  }

  memcpy(&tmp, packet->data + 8, 4);
  tmp= ntohl(tmp);

  if (tmp != (packet->data_size - 12))
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack:invalid size");
    return GEARMAN_INVALID_SIZE;
  }

  if (gearman_command_info_list[packet->command].argc != 0)
  {
    offset= 12;

    for (x= 0; x < gearman_command_info_list[packet->command].argc - 1; x++)
    {
      packet->arg[x]= packet->data + offset;
      ptr= memchr(packet->data + offset, 0, packet->data_size - offset);
      if (ptr == NULL)
      {
        GEARMAN_ERROR_SET(packet->gearman,
                          "gearman_packet_unpack:invalid args");
        return GEARMAN_INVALID_ARGS;
      }

      ptr++;
      packet->arg_size[x]= ptr - packet->arg[x];
      offset+= packet->arg_size[x];
    }

    packet->arg[x]= packet->data + offset;
    packet->arg_size[x]= packet->data_size - offset;
    packet->argc= gearman_command_info_list[packet->command].argc;
  }

  return GEARMAN_SUCCESS;
}
