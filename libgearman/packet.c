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

/* Initialize a packet structure. */
gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet)
{
  if (packet == NULL)
  {
    packet= malloc(sizeof(gearman_packet_st) + 12);
    if (packet == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_packet_create:malloc:%d", errno);
      gearman->last_errno= errno;
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

  if (packet->data != NULL)
    free(packet->data);

  if (packet->options & GEARMAN_PACKET_ALLOCATED)
    free(packet);
}

/* Set header information for a packet. */
void gearman_packet_set_header(gearman_packet_st *packet, gearman_magic magic,
                               gearman_command command)
{
  packet->magic= magic;
  packet->command= command;
}

/* Add an argument to a packet. */
gearman_return gearman_packet_arg_add(gearman_packet_st *packet, char *arg,
                                      size_t arg_len)
{
  size_t offset;
  gearman_return ret;
  uint8_t x;

  if (packet->argc == gearman_command_info_list[packet->command].argc)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_arg_add:too many args");
    return GEARMAN_TOO_MANY_ARGS;
  }

  if (packet->length == 0)
    packet->length= 12;

  ret= gearman_packet_arg_add_data(packet, arg, arg_len);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  packet->arg_len[packet->argc]= arg_len;
  packet->argc++;

  offset= 12;
  for (x= 0; x < packet->argc; x++)
  {
    packet->arg[x]= packet->data + offset;
    offset+= packet->arg_len[x];
  }

  return GEARMAN_SUCCESS;
}

/* Pack packet header information after all args have been added. */
gearman_return gearman_packet_pack(gearman_packet_st *packet)
{
  gearman_return ret;
  uint32_t tmp;

  if (packet->length == 0)
  {
    packet->length= 12;

    ret= gearman_packet_arg_add_data(packet, NULL, 0);
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

  default:
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_pack:invalid magic");
    return GEARMAN_INVALID_MAGIC;
  }

  tmp= packet->command;
  tmp= htonl(tmp);
  memcpy(packet->data + 4, &tmp, 4);

  tmp= packet->length - 12;
  tmp= htonl(tmp);
  memcpy(packet->data + 8, &tmp, 4);

  return GEARMAN_SUCCESS;
}

/* Add raw argument data to a packet, call gearman_packet_unpack once all data
   has been added. */
gearman_return gearman_packet_arg_add_data(gearman_packet_st *packet,
                                           char *data, size_t data_len)
{
  char *new_data;

  new_data= realloc(packet->data, packet->length + data_len);
  if (new_data == NULL)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_arg_add_data:realloc:%d",
                      errno);
    packet->gearman->last_errno= errno;
    return GEARMAN_ERRNO;
  }

  packet->data= new_data;
  if (data != NULL)
  {
    memcpy(packet->data + packet->length, data, data_len);
    packet->length+= data_len;
  }

  return GEARMAN_SUCCESS;
}

/* Unpack packet information after all raw data has been added. */
gearman_return gearman_packet_unpack(gearman_packet_st *packet)
{
  uint32_t tmp;
  size_t offset;
  uint8_t x;
  char *ptr;

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

  if (tmp != (packet->length - 12))
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack:invalid length");
    return GEARMAN_INVALID_LENGTH;
  }

  if (gearman_command_info_list[packet->command].argc != 0)
  {
    offset= 12;

    for (x= 0; x < gearman_command_info_list[packet->command].argc - 1; x++)
    {
      packet->arg[x]= packet->data + offset;
      ptr= memchr(packet->data + offset, 0, packet->length - offset);
      if (ptr == NULL)
      {
        GEARMAN_ERROR_SET(packet->gearman,
                          "gearman_packet_unpack:invalid args");
        return GEARMAN_INVALID_ARGS;
      }

      ptr++;
      packet->arg_len[x]= ptr - packet->arg[x];
      offset+= packet->arg_len[x];
    }

    packet->arg[x]= packet->data + offset;
    packet->arg_len[x]= packet->length - offset;
  }

  return GEARMAN_SUCCESS;
}
