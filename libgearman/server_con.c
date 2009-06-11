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

gearman_server_con_st *gearman_server_con_add(gearman_server_thread_st *thread,
                                              int fd, void *data)
{
  gearman_server_con_st *server_con;
  gearman_return_t ret;

  server_con= gearman_server_con_create(thread);
  if (server_con == NULL)
    return NULL;

  if (gearman_con_set_fd(&(server_con->con), fd) != GEARMAN_SUCCESS)
  {
    gearman_server_con_free(server_con);
    return NULL;
  }

  server_con->con.data= data;

  ret= gearman_con_set_events(&(server_con->con), POLLIN);
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_server_con_free(server_con);
    return NULL;
  }

  return server_con;
}

gearman_server_con_st *
gearman_server_con_create(gearman_server_thread_st *thread)
{
  gearman_server_con_st *server_con;

  if (thread->free_con_count > 0)
  {
    server_con= thread->free_con_list;
    GEARMAN_LIST_DEL(thread->free_con, server_con,)
  }
  else
  {
    server_con= malloc(sizeof(gearman_server_con_st));
    if (server_con == NULL)
    {
      GEARMAN_ERROR_SET(thread->gearman, "gearman_server_con_create",
                        "malloc")
      return NULL;
    }
  }

  memset(server_con, 0, sizeof(gearman_server_con_st));

  server_con->thread= thread;
  strcpy(server_con->id, "-");

  if (gearman_con_create(thread->gearman, &(server_con->con)) == NULL)
  {
    free(server_con);
    return NULL;
  }

  GEARMAN_SERVER_THREAD_LOCK(thread)
  GEARMAN_LIST_ADD(thread->con, server_con,)
  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  return server_con;
}

void gearman_server_con_free(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;
  gearman_server_packet_st *packet;

  server_con->host= NULL;
  server_con->port= NULL;

  if (thread->server->options & GEARMAN_SERVER_PROC_THREAD &&
      !(server_con->proc_removed) && !(thread->server->proc_shutdown))
  {
    server_con->options= GEARMAN_SERVER_CON_DEAD;
    gearman_server_con_proc_add(server_con);
    return;
  }

  gearman_con_free(&(server_con->con));

  if (server_con->proc_list)
    gearman_server_con_proc_remove(server_con);

  if (server_con->io_list)
    gearman_server_con_io_remove(server_con);

  if (server_con->packet != NULL)
  {
    if (&(server_con->packet->packet) != server_con->con.recv_packet)
      gearman_packet_free(&(server_con->packet->packet));
    gearman_server_packet_free(server_con->packet, server_con->thread, true); 
  }

  while (server_con->io_packet_list != NULL)
    gearman_server_io_packet_remove(server_con);

  while (server_con->proc_packet_list != NULL)
  {
    packet= gearman_server_proc_packet_remove(server_con);
    gearman_packet_free(&(packet->packet));
    gearman_server_packet_free(packet, server_con->thread, true); 
  }

  gearman_server_con_free_workers(server_con);

  while (server_con->client_list != NULL)
    gearman_server_client_free(server_con->client_list);

  GEARMAN_SERVER_THREAD_LOCK(thread)
  GEARMAN_LIST_DEL(server_con->thread->con, server_con,)
  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  if (thread->free_con_count < GEARMAN_MAX_FREE_SERVER_CON)
    GEARMAN_LIST_ADD(thread->free_con, server_con,)
  else
    free(server_con);
}

gearman_con_st *gearman_server_con_con(gearman_server_con_st *server_con)
{
  return &server_con->con;
}

void *gearman_server_con_data(gearman_server_con_st *server_con)
{
  return gearman_con_data(&(server_con->con));
}
  
void gearman_server_con_set_data(gearman_server_con_st *server_con, void *data)
{
  gearman_con_set_data(&(server_con->con), data);
}

const char *gearman_server_con_host(gearman_server_con_st *server_con)
{
  return server_con->host;
}

void gearman_server_con_set_host(gearman_server_con_st *server_con,
                                 const char *host)
{
  server_con->host= host;
}

const char *gearman_server_con_port(gearman_server_con_st *server_con)
{
  return server_con->port;
}

void gearman_server_con_set_port(gearman_server_con_st *server_con,
                                 const char *port)
{
  server_con->port= port;
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

void gearman_server_con_free_worker(gearman_server_con_st *con,
                                    char *function_name,
                                    size_t function_name_size)
{
  gearman_server_worker_st *worker;

  for (worker= con->worker_list; worker != NULL; worker= worker->con_next)
  {
    if (worker->function->function_name_size == function_name_size &&
        !memcmp(worker->function->function_name, function_name,
                function_name_size))
    {
      gearman_server_worker_free(worker);
    }
  }
}

void gearman_server_con_free_workers(gearman_server_con_st *con)
{
  while (con->worker_list != NULL)
    gearman_server_worker_free(con->worker_list);
}

void gearman_server_con_io_add(gearman_server_con_st *con)
{
  if (con->io_list)
    return;

  GEARMAN_SERVER_THREAD_LOCK(con->thread)

  GEARMAN_LIST_ADD(con->thread->io, con, io_)
  con->io_list= true;

  /* Looks funny, but need to check io_count locked, but call run unlocked. */
  if (con->thread->io_count == 1 && con->thread->run_fn)
  {
    GEARMAN_SERVER_THREAD_UNLOCK(con->thread)
    (*con->thread->run_fn)(con->thread, con->thread->run_arg);
  }
  else
    GEARMAN_SERVER_THREAD_UNLOCK(con->thread)
}

void gearman_server_con_io_remove(gearman_server_con_st *con)
{
  GEARMAN_SERVER_THREAD_LOCK(con->thread)
  if (con->io_list)
  {
    GEARMAN_LIST_DEL(con->thread->io, con, io_)
    con->io_list= false;
  }
  GEARMAN_SERVER_THREAD_UNLOCK(con->thread)
}

gearman_server_con_st *
gearman_server_con_io_next(gearman_server_thread_st *thread)
{
  gearman_server_con_st *con= thread->io_list;

  if (con == NULL)
    return NULL;

  gearman_server_con_io_remove(con);

  return con;
}

void gearman_server_con_proc_add(gearman_server_con_st *con)
{
  if (con->proc_list)
    return;

  GEARMAN_SERVER_THREAD_LOCK(con->thread)
  GEARMAN_LIST_ADD(con->thread->proc, con, proc_)
  con->proc_list= true;
  GEARMAN_SERVER_THREAD_UNLOCK(con->thread)

  if (!(con->thread->server->proc_shutdown) &&
      !(con->thread->server->proc_wakeup))
  {
    (void) pthread_mutex_lock(&(con->thread->server->proc_lock));
    con->thread->server->proc_wakeup= true;
    (void) pthread_cond_signal(&(con->thread->server->proc_cond));
    (void) pthread_mutex_unlock(&(con->thread->server->proc_lock));
  }
}

void gearman_server_con_proc_remove(gearman_server_con_st *con)
{
  GEARMAN_SERVER_THREAD_LOCK(con->thread)
  if (con->proc_list)
  {
    GEARMAN_LIST_DEL(con->thread->proc, con, proc_)
    con->proc_list= false;
  }
  GEARMAN_SERVER_THREAD_UNLOCK(con->thread)
}

gearman_server_con_st *
gearman_server_con_proc_next(gearman_server_thread_st *thread)
{
  gearman_server_con_st *con;

  if (thread->proc_list == NULL)
    return NULL;

  GEARMAN_SERVER_THREAD_LOCK(thread)

  con= thread->proc_list;
  while (con != NULL)
  {
    GEARMAN_LIST_DEL(thread->proc, con, proc_)
    con->proc_list= false;
    if (!(con->proc_removed))
      break;
    con= thread->proc_list;
  }

  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  return con;
}
