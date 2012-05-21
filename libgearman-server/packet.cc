/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server connection definitions
 */

#include <config.h>
#include <libgearman-server/common.h>

#include <libgearman/command.h>

#include <libgearman-server/fifo.h>
#include <cassert>
#include <cstring>
#include <memory>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/*
 * Public definitions
 */

gearman_server_packet_st *
gearman_server_packet_create(gearman_server_thread_st *thread,
                             bool from_thread)
{
  gearman_server_packet_st *server_packet= NULL;

  if (from_thread && Server->flags.threaded)
  {
    if (thread->free_packet_count > 0)
    {
      server_packet= thread->free_packet_list;
      thread->free_packet_list= server_packet->next;
      thread->free_packet_count--;
    }
  }
  else
  {
    if (Server->free_packet_count > 0)
    {
      server_packet= Server->free_packet_list;
      Server->free_packet_list= server_packet->next;
      Server->free_packet_count--;
    }
  }

  if (server_packet == NULL)
  {
    server_packet= new (std::nothrow) gearman_server_packet_st;
    if (server_packet == NULL)
    {
      gearmand_perror("new() gearman_server_packet_st");
      return NULL;
    }
  }

  server_packet->next= NULL;

  return server_packet;
}

void destroy_gearman_server_packet_st(gearman_server_packet_st *packet)
{
  delete packet;
}

void gearman_server_packet_free(gearman_server_packet_st *packet,
                                gearman_server_thread_st *thread,
                                bool from_thread)
{
  if (from_thread and Server->flags.threaded)
  {
    if (thread->free_packet_count < GEARMAN_MAX_FREE_SERVER_PACKET)
    {
      packet->next= thread->free_packet_list;
      thread->free_packet_list= packet;
      thread->free_packet_count++;
    }
    else
    {
      gearmand_debug("delete() gearman_server_packet_st");
      delete packet;
    }
  }
  else
  {
    if (Server->free_packet_count < GEARMAN_MAX_FREE_SERVER_PACKET)
    {
      packet->next= Server->free_packet_list;
      Server->free_packet_list= packet;
      Server->free_packet_count++;
    }
    else
    {
      gearmand_debug("delete() gearman_server_packet_st");
      delete packet;
    }
  }
}

gearmand_error_t gearman_server_io_packet_add(gearman_server_con_st *con,
                                              bool take_data,
                                              enum gearman_magic_t magic,
                                              gearman_command_t command,
                                              const void *arg, ...)
{
  gearman_server_packet_st *server_packet;
  va_list ap;

  server_packet= gearman_server_packet_create(con->thread, false);
  if (server_packet == NULL)
  {
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearmand_packet_init(&(server_packet->packet), magic, command);

  va_start(ap, arg);

  while (arg)
  {
    size_t arg_size= va_arg(ap, size_t);

    gearmand_error_t ret= gearmand_packet_create(&(server_packet->packet), arg, arg_size);
    if (gearmand_failed(ret))
    {
      va_end(ap);
      gearmand_packet_free(&(server_packet->packet));
      gearman_server_packet_free(server_packet, con->thread, false);
      return ret;
    }

    arg= va_arg(ap, void *);
  }

  va_end(ap);

  gearmand_error_t ret= gearmand_packet_pack_header(&(server_packet->packet));
  if (gearmand_failed(ret))
  {
    gearmand_packet_free(&(server_packet->packet));
    gearman_server_packet_free(server_packet, con->thread, false);
    return ret;
  }

  if (take_data)
  {
    server_packet->packet.options.free_data= true;
  }

  if (pthread_mutex_lock(&con->thread->lock) == 0)
  {
    gearmand_server_con_fifo_add(con, server_packet);
    if (pthread_mutex_unlock(&con->thread->lock) != 0)
    {
      gearmand_fatal("pthread_mutex_unlock()");
    }
  }
  else
  {
    gearmand_fatal("pthread_mutex_lock()");
  }

  gearman_server_con_io_add(con);

  return GEARMAN_SUCCESS;
}

void gearman_server_io_packet_remove(gearman_server_con_st *con)
{
  gearman_server_packet_st *server_packet= con->io_packet_list;

  gearmand_packet_free(&(server_packet->packet));

  if (pthread_mutex_lock(&con->thread->lock) == 0)
  {
    gearmand_server_con_fifo_free(con, server_packet);
    if (pthread_mutex_unlock(&con->thread->lock) != 0)
    {
      gearmand_fatal("pthread_mutex_unlock()");
    }
  }
  else
  {
    gearmand_fatal("pthread_mutex_lock()");
  }

  gearman_server_packet_free(server_packet, con->thread, true);
}

void gearman_server_proc_packet_add(gearman_server_con_st *con,
                                    gearman_server_packet_st *packet)
{
  if (pthread_mutex_lock(&con->thread->lock) == 0)
  {
    gearmand_server_con_fifo_proc_add(con, packet);
    if (pthread_mutex_unlock(&con->thread->lock) != 0)
    {
      gearmand_fatal("pthread_mutex_unlock()");
    }
  }
  else
  {
    gearmand_fatal("pthread_mutex_lock()");
  }

  gearman_server_con_proc_add(con);
}

gearman_server_packet_st *
gearman_server_proc_packet_remove(gearman_server_con_st *con)
{
  gearman_server_packet_st *server_packet= con->proc_packet_list;

  if (server_packet == NULL)
  {
    return NULL;
  }

  if (pthread_mutex_lock(&con->thread->lock) == 0)
  {
    gearmand_server_con_fifo_proc_free(con, server_packet);
    if (pthread_mutex_unlock(&con->thread->lock) != 0)
    {
      gearmand_fatal("pthread_mutex_unlock()");
    }
  }
  else
  {
    gearmand_fatal("pthread_mutex_lock()");
  }

  return server_packet;
}

const char *gearmand_strcommand(gearmand_packet_st *packet)
{
  assert(packet);
  return gearman_command_info(packet->command)->name;
}

inline static gearmand_error_t packet_create_arg(gearmand_packet_st *packet,
                                                 const void *arg, size_t arg_size)
{
  if (packet->argc == gearman_command_info(packet->command)->argc and
      (not (gearman_command_info(packet->command)->data) or
       packet->data))
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "too many arguments for command(%s)", gearman_command_info(packet->command)->name);
    return GEARMAN_TOO_MANY_ARGS;
  }

  if (packet->argc == gearman_command_info(packet->command)->argc)
  {
    packet->data= static_cast<const char *>(arg);
    packet->data_size= arg_size;
    return GEARMAN_SUCCESS;
  }

  if (packet->args_size == 0 and packet->magic != GEARMAN_MAGIC_TEXT)
  {
    packet->args_size= GEARMAN_PACKET_HEADER_SIZE;
  }

  if ((packet->args_size + arg_size) < GEARMAN_ARGS_BUFFER_SIZE)
  {
    packet->args= packet->args_buffer;
  }
  else
  {
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "resizing packet buffer");
    if (packet->args == packet->args_buffer)
    {
      packet->args= (char *)malloc(packet->args_size + arg_size);
      memcpy(packet->args, packet->args_buffer, packet->args_size);
    }
    else
    {
      char *new_args= (char *)realloc(packet->args, packet->args_size + arg_size);
      if (new_args == NULL)
      {
        gearmand_perror("realloc");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
      packet->args= new_args;
    }
  }

  memcpy(packet->args + packet->args_size, arg, arg_size);
  packet->args_size+= arg_size;
  packet->arg_size[packet->argc]= arg_size;
  packet->argc++;

  size_t offset;
  if (packet->magic == GEARMAN_MAGIC_TEXT)
  {
    offset= 0;
  }
  else
  {
    offset= GEARMAN_PACKET_HEADER_SIZE;
  }

  for (uint8_t x= 0; x < packet->argc; x++)
  {
    packet->arg[x]= packet->args + offset;
    offset+= packet->arg_size[x];
  }

  return GEARMAN_SUCCESS;
}

/** @} */

/*
 * Public Definitions
 */


void gearmand_packet_init(gearmand_packet_st *packet, enum gearman_magic_t magic, gearman_command_t command)
{
  assert(packet);

  packet->options.complete= false;
  packet->options.free_data= false;

  packet->magic= magic;
  packet->command= command;
  packet->argc= 0;
  packet->args_size= 0;
  packet->data_size= 0;

  packet->args= NULL;
  packet->data= NULL;
}

gearmand_error_t gearmand_packet_create(gearmand_packet_st *packet,
                                          const void *arg, size_t arg_size)
{
  return packet_create_arg(packet, arg, arg_size);
}

void gearmand_packet_free(gearmand_packet_st *packet)
{
  if (packet->args != packet->args_buffer && packet->args != NULL)
  {
    gearmand_debug("free packet's args");
    free(packet->args);
    packet->args= NULL;
  }

  if (packet->options.free_data && packet->data != NULL)
  {
    gearmand_debug("free() packet's data");
    free((void *)packet->data); //@todo fix the need for the casting.
    packet->data= NULL;
  }
}

gearmand_error_t gearmand_packet_pack_header(gearmand_packet_st *packet)
{
  if (packet->magic == GEARMAN_MAGIC_TEXT)
  {
    packet->options.complete= true;
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
    gearmand_error("invalid magic value");
    return GEARMAN_INVALID_MAGIC;
  }

  if (packet->command == GEARMAN_COMMAND_TEXT ||
      packet->command >= GEARMAN_COMMAND_MAX)
  {
    gearmand_error("invalid command value");
    return GEARMAN_INVALID_COMMAND;
  }

  uint32_t tmp= packet->command;
  tmp= htonl(tmp);
  memcpy(packet->args + 4, &tmp, 4);

  uint64_t length_64= packet->args_size + packet->data_size - GEARMAN_PACKET_HEADER_SIZE;

  // Check for overflow on 32bit(portable?).
  if (length_64 >= UINT32_MAX || length_64 < packet->data_size)
  {
    gearmand_error("data size too too long");
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  tmp= (uint32_t)length_64;
  tmp= htonl(tmp);
  memcpy(packet->args + 8, &tmp, 4);

  packet->options.complete= true;

  return GEARMAN_SUCCESS;
}
