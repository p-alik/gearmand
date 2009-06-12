/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Packet definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_packet_private Private Packet Functions
 * @ingroup gearman_packet
 * @{
 */

/**
 * Command info. Update GEARMAN_MAX_COMMAND_ARGS to the largest number in the
 * args column.
 */
gearman_command_info_st gearman_command_info_list[GEARMAN_COMMAND_MAX]=
{
  { "TEXT",               3, false },
  { "CAN_DO",             1, false },
  { "CANT_DO",            1, false },
  { "RESET_ABILITIES",    0, false },
  { "PRE_SLEEP",          0, false },
  { "UNUSED",             0, false },
  { "NOOP",               0, false },
  { "SUBMIT_JOB",         2, true  },
  { "JOB_CREATED",        1, false },
  { "GRAB_JOB",           0, false },
  { "NO_JOB",             0, false },
  { "JOB_ASSIGN",         2, true  },
  { "WORK_STATUS",        3, false },
  { "WORK_COMPLETE",      1, true  },
  { "WORK_FAIL",          1, false },
  { "GET_STATUS",         1, false },
  { "ECHO_REQ",           0, true  },
  { "ECHO_RES",           0, true  },
  { "SUBMIT_JOB_BG",      2, true  },
  { "ERROR",              2, false },
  { "STATUS_RES",         5, false },
  { "SUBMIT_JOB_HIGH",    2, true  },
  { "SET_CLIENT_ID",      1, false },
  { "CAN_DO_TIMEOUT",     2, false },
  { "ALL_YOURS",          0, false },
  { "WORK_EXCEPTION",     1, true  },
  { "OPTION_REQ",         1, false },
  { "OPTION_RES",         1, false },
  { "WORK_DATA",          1, true  },
  { "WORK_WARNING",       1, true  },
  { "GRAB_JOB_UNIQ",      0, false },
  { "JOB_ASSIGN_UNIQ",    3, true  },
  { "SUBMIT_JOB_HIGH_BG", 2, true  },
  { "SUBMIT_JOB_LOW",     2, true  },
  { "SUBMIT_JOB_LOW_BG",  2, true  },
  { "SUBMIT_JOB_SCHED",   7, true  },
  { "SUBMIT_JOB_EPOCH",   3, true  }
};

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearman_packet_add(gearman_st *gearman,
                                    gearman_packet_st *packet,
                                    gearman_magic_t magic,
                                    gearman_command_t command,
                                    const void *arg, ...)
{
  va_list ap;
  size_t arg_size;
  gearman_return_t ret;

  packet= gearman_packet_create(gearman, packet);
  if (packet == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  packet->magic= magic;
  packet->command= command;

  va_start(ap, arg);

  while (arg != NULL)
  {
    arg_size = va_arg(ap, size_t);

    ret= gearman_packet_add_arg(packet, arg, arg_size);
    if (ret != GEARMAN_SUCCESS)
    {
      va_end(ap);
      gearman_packet_free(packet);
      return ret;
    }

    arg = va_arg(ap, void *);
  }

  va_end(ap);

  return gearman_packet_pack_header(packet);
}

gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet)
{
  if (packet == NULL)
  {
    packet= malloc(sizeof(gearman_packet_st));
    if (packet == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_packet_create", "malloc")
      return NULL;
    }

    packet->options= GEARMAN_PACKET_ALLOCATED;
  }
  else
    packet->options= 0;

  packet->magic= 0;
  packet->command= 0;
  packet->argc= 0;
  packet->args_size= 0;
  packet->data_size= 0;
  packet->gearman= gearman;

  if (!(gearman->options & GEARMAN_DONT_TRACK_PACKETS))
    GEARMAN_LIST_ADD(gearman->packet, packet,)

  packet->args= NULL;
  packet->data= NULL;

  return packet;
}

void gearman_packet_free(gearman_packet_st *packet)
{
  if (packet->args != packet->args_buffer && packet->args != NULL)
    free(packet->args);

  if (packet->options & GEARMAN_PACKET_FREE_DATA && packet->data != NULL)
  {
    if (packet->gearman->workload_free == NULL)
      free((void *)(packet->data));
    else
    {
      packet->gearman->workload_free((void *)(packet->data),
                                  (void *)(packet->gearman->workload_free_arg));
    }
  }

  if (!(packet->gearman->options & GEARMAN_DONT_TRACK_PACKETS))
    GEARMAN_LIST_DEL(packet->gearman->packet, packet,)

  if (packet->options & GEARMAN_PACKET_ALLOCATED)
    free(packet);
}

void gearman_packet_set_options(gearman_packet_st *packet,
                                gearman_packet_options_t options,
                                uint32_t data)
{
  if (data == 0)
    packet->options &= ~options;
  else
    packet->options |= options;
}

gearman_return_t gearman_packet_add_arg(gearman_packet_st *packet,
                                        const void *arg, size_t arg_size)
{
  void *new_args;
  size_t offset;
  uint8_t x;

  if (packet->argc == gearman_command_info_list[packet->command].argc &&
      (!(gearman_command_info_list[packet->command].data) ||
       packet->data != NULL))
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_add_arg",
                      "too many arguments for command")
    return GEARMAN_TOO_MANY_ARGS;
  }

  if (packet->argc == gearman_command_info_list[packet->command].argc)
  {
    packet->data= arg;
    packet->data_size= arg_size;
    return GEARMAN_SUCCESS;
  }

  if (packet->args_size == 0 && packet->magic != GEARMAN_MAGIC_TEXT)
    packet->args_size= GEARMAN_PACKET_HEADER_SIZE;

  if ((packet->args_size + arg_size) < GEARMAN_ARGS_BUFFER_SIZE)
    packet->args= packet->args_buffer;
  else
  {
    if (packet->args == packet->args_buffer)
      packet->args= NULL;

    new_args= realloc(packet->args, packet->args_size + arg_size);
    if (new_args == NULL)
    {
      GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_add_arg", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    if (packet->args_size > 0)
      memcpy(new_args, packet->args_buffer, packet->args_size);

    packet->args= new_args;
  }

  memcpy(packet->args + packet->args_size, arg, arg_size);
  packet->args_size+= arg_size;
  packet->arg_size[packet->argc]= arg_size;
  packet->argc++;

  if (packet->magic == GEARMAN_MAGIC_TEXT)
    offset= 0;
  else
    offset= GEARMAN_PACKET_HEADER_SIZE;

  for (x= 0; x < packet->argc; x++)
  {
    packet->arg[x]= packet->args + offset;
    offset+= packet->arg_size[x];
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_packet_pack_header(gearman_packet_st *packet)
{
  uint32_t tmp;

  if (packet->magic == GEARMAN_MAGIC_TEXT)
  {
    packet->options|= GEARMAN_PACKET_COMPLETE;
    return GEARMAN_SUCCESS;
  }

  if (packet->args_size == 0)
  {
    packet->args= packet->args_buffer;
    packet->args_size= GEARMAN_PACKET_HEADER_SIZE;
  }

  switch (packet->magic)
  {
  case GEARMAN_MAGIC_TEXT:
    break;

  case GEARMAN_MAGIC_REQUEST:
    memcpy(packet->args, "\0REQ", 4);
    break;

  case GEARMAN_MAGIC_RESPONSE:
    memcpy(packet->args, "\0RES", 4);
    break;

  default:
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_pack_header",
                      "invalid magic value")
    return GEARMAN_INVALID_MAGIC;
  }

  if (packet->command == GEARMAN_COMMAND_TEXT ||
      packet->command >= GEARMAN_COMMAND_MAX)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_pack_header",
                      "invalid command value")
    return GEARMAN_INVALID_COMMAND;
  }

  tmp= packet->command;
  tmp= htonl(tmp);
  memcpy(packet->args + 4, &tmp, 4);

  tmp= (packet->args_size + packet->data_size) - GEARMAN_PACKET_HEADER_SIZE;
  tmp= htonl(tmp);
  memcpy(packet->args + 8, &tmp, 4);

  packet->options|= GEARMAN_PACKET_COMPLETE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_packet_unpack_header(gearman_packet_st *packet)
{
  uint32_t tmp;

  if (!memcmp(packet->args, "\0REQ", 4))
    packet->magic= GEARMAN_MAGIC_REQUEST;
  else if (!memcmp(packet->args, "\0RES", 4))
    packet->magic= GEARMAN_MAGIC_RESPONSE;
  else
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack_header",
                      "invalid magic value")
    return GEARMAN_INVALID_MAGIC;
  }

  memcpy(&tmp, packet->args + 4, 4);
  packet->command= ntohl(tmp);

  if (packet->command == GEARMAN_COMMAND_TEXT ||
      packet->command >= GEARMAN_COMMAND_MAX)
  {
    GEARMAN_ERROR_SET(packet->gearman, "gearman_packet_unpack_header",
                      "invalid command value")
    return GEARMAN_INVALID_COMMAND;
  }

  memcpy(&tmp, packet->args + 8, 4);
  packet->data_size= ntohl(tmp);

  return GEARMAN_SUCCESS;
}

size_t gearman_packet_pack(gearman_packet_st *packet,
                           gearman_con_st *con __attribute__ ((unused)),
                           void *data, size_t data_size,
                           gearman_return_t *ret_ptr)
{
  if (packet->args_size == 0)
  {
    *ret_ptr= GEARMAN_SUCCESS;
    return 0;
  }

  if (packet->args_size > data_size)
  {
    *ret_ptr= GEARMAN_FLUSH_DATA;
    return 0;
  }

  memcpy(data, packet->args, packet->args_size);
  *ret_ptr= GEARMAN_SUCCESS;
  return packet->args_size;
}

size_t gearman_packet_unpack(gearman_packet_st *packet,
                             gearman_con_st *con __attribute__ ((unused)),
                             const void *data, size_t data_size,
                             gearman_return_t *ret_ptr)
{
  uint8_t *ptr;
  size_t used_size;
  size_t arg_size;

  if (packet->args_size == 0)
  {
    if (data_size > 0 && ((uint8_t *)data)[0] != 0)
    {
      /* Try to parse a text-based command. */
      ptr= memchr(data, '\n', data_size);
      if (ptr == NULL)
      {
        *ret_ptr= GEARMAN_IO_WAIT;
        return 0;
      }

      packet->magic= GEARMAN_MAGIC_TEXT;
      packet->command= GEARMAN_COMMAND_TEXT;

      used_size= (ptr - ((uint8_t *)data)) + 1;
      *ptr= 0;
      if (used_size > 1 && *(ptr - 1) == '\r')
        *(ptr - 1)= 0;

      for (arg_size= used_size, ptr= (uint8_t *)data; ptr != NULL; data= ptr)
      {
        ptr= memchr(data, ' ', arg_size);
        if (ptr != NULL)
        {
          *ptr= 0;
          ptr++;
          while (*ptr == ' ')
            ptr++;

          arg_size-= (ptr - ((uint8_t *)data));
        }

        *ret_ptr= gearman_packet_add_arg(packet, data, ptr == NULL ? arg_size :
                                         (size_t)(ptr - ((uint8_t *)data)));
        if (*ret_ptr != GEARMAN_SUCCESS)
          return used_size;
      }

      return used_size;
    }
    else if (data_size < GEARMAN_PACKET_HEADER_SIZE)
    {
      *ret_ptr= GEARMAN_IO_WAIT;
      return 0;
    }

    packet->args= packet->args_buffer;
    packet->args_size= GEARMAN_PACKET_HEADER_SIZE;
    memcpy(packet->args, data, GEARMAN_PACKET_HEADER_SIZE);

    *ret_ptr= gearman_packet_unpack_header(packet);
    if (*ret_ptr != GEARMAN_SUCCESS)
      return 0;

    used_size= GEARMAN_PACKET_HEADER_SIZE;
  }
  else
    used_size= 0;

  while (packet->argc != gearman_command_info_list[packet->command].argc)
  {
    if (packet->argc != (gearman_command_info_list[packet->command].argc - 1) ||
        gearman_command_info_list[packet->command].data)
    {
      ptr= memchr(((uint8_t *)data) + used_size, 0, data_size - used_size);
      if (ptr == NULL)
      {
        *ret_ptr= GEARMAN_IO_WAIT;
        return used_size;
      }

      arg_size= (ptr - (((uint8_t *)data) + used_size)) + 1;
      *ret_ptr= gearman_packet_add_arg(packet, ((uint8_t *)data) + used_size,
                                       arg_size);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return used_size;

      packet->data_size-= arg_size;
      used_size+= arg_size;
    }
    else
    {
      if ((data_size - used_size) < packet->data_size)
      {
        *ret_ptr= GEARMAN_IO_WAIT;
        return used_size;
      }

      *ret_ptr= gearman_packet_add_arg(packet, ((uint8_t *)data) + used_size,
                                       packet->data_size);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return used_size;

      used_size+= packet->data_size;
      packet->data_size= 0;
    }
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return used_size;
}

void *gearman_packet_take_data(gearman_packet_st *packet, size_t *size)
{
  void *data= (void *)(packet->data);

  *size= packet->data_size;

  packet->data= NULL;
  packet->data_size= 0;
  packet->options&= ~GEARMAN_PACKET_FREE_DATA;

  return data;
}
