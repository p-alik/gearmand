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

#include "common.h"

/*
 * Public definitions
 */

gearman_server_packet_st *
gearman_server_packet_create(gearman_server_thread_st *thread,
                             bool from_thread)
{
  gearman_server_packet_st *server_packet= NULL;

  if (from_thread && thread->server->flags.threaded)
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
    if (thread->server->free_packet_count > 0)
    {
      server_packet= thread->server->free_packet_list;
      thread->server->free_packet_list= server_packet->next;
      thread->server->free_packet_count--;
    }
  }

  if (server_packet == NULL)
  {
    server_packet= (gearman_server_packet_st *)malloc(sizeof(gearman_server_packet_st));
    if (server_packet == NULL)
    {
      gearman_log_error(thread->gearman, "gearman_server_packet_create", "malloc");
      return NULL;
    }
  }

  server_packet->next= NULL;

  return server_packet;
}

void gearman_server_packet_free(gearman_server_packet_st *packet,
                                gearman_server_thread_st *thread,
                                bool from_thread)
{
  if (from_thread && thread->server->flags.threaded)
  {
    if (thread->free_packet_count < GEARMAN_MAX_FREE_SERVER_PACKET)
    {
      packet->next= thread->free_packet_list;
      thread->free_packet_list= packet;
      thread->free_packet_count++;
    }
    else
      free(packet);
  }
  else
  {
    if (thread->server->free_packet_count < GEARMAN_MAX_FREE_SERVER_PACKET)
    {
      packet->next= thread->server->free_packet_list;
      thread->server->free_packet_list= packet;
      thread->server->free_packet_count++;
    }
    else
      free(packet);
  }
}

gearman_return_t gearman_server_io_packet_add(gearman_server_con_st *con,
                                              bool take_data,
                                              enum gearman_magic_t magic,
                                              gearman_command_t command,
                                              const void *arg, ...)
{
  gearman_server_packet_st *server_packet;
  va_list ap;
  size_t arg_size;
  gearman_return_t ret;

  server_packet= gearman_server_packet_create(con->thread, false);
  if (server_packet == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  if (gearman_packet_create(con->thread->gearman,
                            &(server_packet->packet)) == NULL)
  {
    gearman_server_packet_free(server_packet, con->thread, false);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  server_packet->packet.magic= magic;
  server_packet->packet.command= command;

  va_start(ap, arg);

  while (arg != NULL)
  {
    arg_size = va_arg(ap, size_t);

    ret= gearman_packet_create_arg(&(server_packet->packet), arg, arg_size);
    if (ret != GEARMAN_SUCCESS)
    {
      va_end(ap);
      gearman_packet_free(&(server_packet->packet));
      gearman_server_packet_free(server_packet, con->thread, false);
      return ret;
    }

    arg = va_arg(ap, void *);
  }

  va_end(ap);

  ret= gearman_packet_pack_header(&(server_packet->packet));
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_packet_free(&(server_packet->packet));
    gearman_server_packet_free(server_packet, con->thread, false);
    return ret;
  }

  if (take_data)
    server_packet->packet.options.free_data= true;

  (void) pthread_mutex_lock(&con->thread->lock);
  GEARMAN_FIFO_ADD(con->io_packet, server_packet,);
  (void) pthread_mutex_unlock(&con->thread->lock);

  gearman_server_con_io_add(con);

  return GEARMAN_SUCCESS;
}

void gearman_server_io_packet_remove(gearman_server_con_st *con)
{
  gearman_server_packet_st *server_packet= con->io_packet_list;

  gearman_packet_free(&(server_packet->packet));

  (void) pthread_mutex_lock(&con->thread->lock);
  GEARMAN_FIFO_DEL(con->io_packet, server_packet,);
  (void) pthread_mutex_unlock(&con->thread->lock);

  gearman_server_packet_free(server_packet, con->thread, true);
}

void gearman_server_proc_packet_add(gearman_server_con_st *con,
                                    gearman_server_packet_st *packet)
{
  (void) pthread_mutex_lock(&con->thread->lock);
  GEARMAN_FIFO_ADD(con->proc_packet, packet,);
  (void) pthread_mutex_unlock(&con->thread->lock);

  gearman_server_con_proc_add(con);
}

gearman_server_packet_st *
gearman_server_proc_packet_remove(gearman_server_con_st *con)
{
  gearman_server_packet_st *server_packet= con->proc_packet_list;

  if (server_packet == NULL)
    return NULL;

  (void) pthread_mutex_lock(&con->thread->lock);
  GEARMAN_FIFO_DEL(con->proc_packet, server_packet,);
  (void) pthread_mutex_unlock(&con->thread->lock);

  return server_packet;
}
