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
    server_con= malloc(sizeof(gearman_server_con_st));
    if (server_con == NULL)
    {
      GEARMAN_ERROR_SET(server->gearman, "gearman_server_con_create", "malloc")
      return NULL;
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

  if (server->con_list)
    server->con_list->prev= server_con;
  server_con->next= server->con_list;
  server->con_list= server_con;
  server->con_count++;

  return server_con;
}

void gearman_server_con_free(gearman_server_con_st *server_con)
{
  gearman_server_job_st *job;

  gearman_con_free(&(server_con->con));

  if (server_con->active_next != NULL || server_con->active_prev != NULL)
    gearman_server_active_remove(server_con);

  while (server_con->packet_list != NULL)
    gearman_server_con_packet_remove(server_con);

  gearman_server_con_free_workers(server_con);

  if (server_con->job_count > 0)
  {
    for (job= server_con->server->job_list; job != NULL; job= job->next)
    {
      if (job->client != server_con)
        continue;

      job->client= NULL;
      server_con->job_count--;
      if (server_con->job_count == 0)
        break;
    }
  }

  if (server_con->server->con_list == server_con)
    server_con->server->con_list= server_con->next;
  if (server_con->prev)
    server_con->prev->next= server_con->next;
  if (server_con->next)
    server_con->next->prev= server_con->prev;
  server_con->server->con_count--;

  if (server_con->options & GEARMAN_SERVER_CON_ALLOCATED)
    free(server_con);
}

void *gearman_server_con_data(gearman_server_con_st *server_con)
{
  return gearman_con_data(&(server_con->con));
}
  
void gearman_server_con_set_data(gearman_server_con_st *server_con, void *data)
{
  gearman_con_set_data(&(server_con->con), data);
}

uint8_t *gearman_server_con_id(gearman_server_con_st *server_con, size_t *size)
{
  *size= server_con->id_size;
  return server_con->id;
}

gearman_return_t gearman_server_con_set_id(gearman_server_con_st *server_con,
                                           uint8_t *id, size_t size)
{
  uint8_t *new_id;

  if (size == 0)
  {
    if (server_con->id != NULL)
    {
      free(server_con->id);
      server_con->id= NULL;
      server_con->id_size= size;
    }

    return GEARMAN_SUCCESS;
  }

  if (size > server_con->id_size)
  {
    new_id= realloc(server_con->id, size + 1);
    if (new_id == NULL)
    {
      GEARMAN_ERROR_SET(server_con->server->gearman,
                        "gearman_server_con_set_id", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    server_con->id= new_id;
  }

  memcpy(server_con->id, id, size);
  server_con->id_size= size;
  server_con->id[size]= 0;

  return GEARMAN_SUCCESS;
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

  server_packet= malloc(sizeof(gearman_server_packet_st));
  if (server_packet == NULL)
  {
    GEARMAN_ERROR_SET(server_con->server->gearman,
                      "gearman_server_con_packet_add", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
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
