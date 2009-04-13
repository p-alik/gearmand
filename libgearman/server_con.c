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

gearman_server_con_st *
gearman_server_con_create(gearman_server_st *server,
                          gearman_server_con_st *server_con)
{
  if (server_con == NULL)
  {
    if (server->free_con_count > 0)
    {
      server_con= server->free_con_list;
      GEARMAN_LIST_DEL(server->free_con, server_con,)
    }
    else
    {
      server_con= malloc(sizeof(gearman_server_con_st));
      if (server_con == NULL)
      {
        GEARMAN_ERROR_SET(server->gearman, "gearman_server_con_create",
                          "malloc")
        return NULL;
      }
    }

    memset(server_con, 0, sizeof(gearman_server_con_st));
    server_con->options|= GEARMAN_SERVER_CON_ALLOCATED;
  }
  else
    memset(server_con, 0, sizeof(gearman_server_con_st));

  if (gearman_con_create(server->gearman, &(server_con->con)) == NULL)
  {
    if (server_con->options & GEARMAN_SERVER_CON_ALLOCATED)
      free(server_con);

    return NULL;
  }

  server_con->server= server;
  strcpy(server_con->addr, "-");
  strcpy(server_con->id, "-");

  GEARMAN_LIST_ADD(server->con, server_con,)

  return server_con;
}

void gearman_server_con_free(gearman_server_con_st *server_con)
{
  gearman_con_free(&(server_con->con));

  if (server_con->active_next != NULL || server_con->active_prev != NULL)
    gearman_server_active_remove(server_con);

  while (server_con->packet_list != NULL)
    gearman_server_con_packet_remove(server_con);

  gearman_server_con_free_workers(server_con);

  while (server_con->client_list != NULL)
    gearman_server_client_free(server_con->client_list);

  GEARMAN_LIST_DEL(server_con->server->con, server_con,)

  if (server_con->options & GEARMAN_SERVER_CON_ALLOCATED)
  {
    if (server_con->server->free_con_count < GEARMAN_MAX_FREE_SERVER_CON)
      GEARMAN_LIST_ADD(server_con->server->free_con, server_con,)
    else
      free(server_con);
  }
}

void *gearman_server_con_data(gearman_server_con_st *server_con)
{
  return gearman_con_data(&(server_con->con));
}
  
void gearman_server_con_set_data(gearman_server_con_st *server_con, void *data)
{
  gearman_con_set_data(&(server_con->con), data);
}

const char *gearman_server_con_addr(gearman_server_con_st *server_con)
{
  return server_con->addr;
}

void gearman_server_con_set_addr(gearman_server_con_st *server_con,
                                 const char *addr)
{
  strncpy(server_con->addr, addr, GEARMAN_SERVER_CON_ADDR_SIZE);
  server_con->addr[GEARMAN_SERVER_CON_ADDR_SIZE - 1]= 0;
}

const char *gearman_server_con_id(gearman_server_con_st *server_con)
{
  return server_con->id;
}

void gearman_server_con_set_id(gearman_server_con_st *server_con, char *id,
                               size_t size)
{
  if (size >= GEARMAN_SERVER_CON_ID_SIZE)
    size= GEARMAN_SERVER_CON_ID_SIZE - 1;

  memcpy(server_con->id, id, size);
  server_con->id[size]= 0;
}

gearman_return_t
gearman_server_con_packet_add(gearman_server_con_st *server_con,
                              gearman_magic_t magic, gearman_command_t command,
                              const void *arg, ...)
{
  gearman_server_packet_st *server_packet;
  va_list ap;
  size_t arg_size;
  gearman_return_t ret;

  if (server_con->server->free_packet_count > 0)
  {
    server_packet= server_con->server->free_packet_list;
    server_con->server->free_packet_list= server_packet->next;
    server_con->server->free_packet_count--;
  }
  else
  {
    server_packet= malloc(sizeof(gearman_server_packet_st));
    if (server_packet == NULL)
    {
      GEARMAN_ERROR_SET(server_con->server->gearman,
                        "gearman_server_con_packet_add", "malloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }
  }

  memset(server_packet, 0, sizeof(gearman_server_packet_st));

  if (gearman_packet_create(server_con->server->gearman,
                            &(server_packet->packet)) == NULL)
  {
    free(server_packet);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  server_packet->packet.magic= magic;
  server_packet->packet.command= command;

  va_start(ap, arg);

  while (arg != NULL)
  {
    arg_size = va_arg(ap, size_t);

    ret= gearman_packet_add_arg(&(server_packet->packet), arg, arg_size);
    if (ret != GEARMAN_SUCCESS)
    {
      va_end(ap);
      gearman_packet_free(&(server_packet->packet));
      free(server_packet);
      return ret;
    }

    arg = va_arg(ap, void *);
  }

  va_end(ap);

  ret= gearman_packet_pack_header(&(server_packet->packet));
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_packet_free(&(server_packet->packet));
    free(server_packet);
    return ret;
  }

  if (server_con->packet_end == NULL)
    server_con->packet_list= server_packet;
  else
    server_con->packet_end->next= server_packet;
  server_con->packet_end= server_packet;
  server_con->packet_count++;

  gearman_server_active_add(server_con);

  return GEARMAN_SUCCESS;
}

void gearman_server_con_packet_remove(gearman_server_con_st *server_con)
{
  gearman_server_packet_st *server_packet= server_con->packet_list;

  gearman_packet_free(&(server_packet->packet));

  server_con->packet_list= server_packet->next;
  if (server_con->packet_list == NULL)
    server_con->packet_end= NULL;
  server_con->packet_count--;

  if (server_con->server->free_packet_count < GEARMAN_MAX_FREE_SERVER_PACKET)
  {
    server_packet->next= server_con->server->free_packet_list;
    server_con->server->free_packet_list= server_packet;
    server_con->server->free_packet_count++;
  }
  else
    free(server_packet);
}

void gearman_server_con_free_worker(gearman_server_con_st *server_con,
                                    char *function_name,
                                    size_t function_name_size)
{
  gearman_server_worker_st *server_worker;

  for (server_worker= server_con->worker_list; server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->function_name_size == function_name_size &&
        !memcmp(server_worker->function->function_name, function_name,
                function_name_size))
    {
      gearman_server_worker_free(server_worker);
    }
  }
}

void gearman_server_con_free_workers(gearman_server_con_st *server_con)
{
  while (server_con->worker_list != NULL)
    gearman_server_worker_free(server_con->worker_list);
}
