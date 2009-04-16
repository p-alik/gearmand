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
                                              int fd, const char *host,
                                              void *data)
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

  gearman_server_con_set_addr(server_con, host);
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

  if (gearman_con_create(thread->gearman, &(server_con->con)) == NULL)
  {
    free(server_con);
    return NULL;
  }

  server_con->thread= thread;
  strcpy(server_con->addr, "-");
  strcpy(server_con->id, "-");

  GEARMAN_SERVER_THREAD_LOCK(thread)
  GEARMAN_LIST_ADD(thread->con, server_con,)
  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  return server_con;
}

void gearman_server_con_free(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;

/* XXX check race cond with proc_shutdown */
  if (thread->server->thread_count > 1 &&
      !(server_con->options & GEARMAN_SERVER_CON_FREE) &&
      !(thread->server->proc_shutdown))
  {
    server_con->options= GEARMAN_SERVER_CON_DEAD;
    gearman_server_con_proc_add(server_con);
    return;
  }

  if (server_con->options & GEARMAN_SERVER_CON_PACKET)
    gearman_packet_free(&(server_con->con.packet));

  gearman_con_free(&(server_con->con));

  if (server_con->proc_next != NULL || server_con->proc_prev != NULL)
    gearman_server_con_proc_remove(server_con);

  if (server_con->io_next != NULL || server_con->io_prev != NULL)
    gearman_server_con_io_remove(server_con);

  while (server_con->io_packet_list != NULL)
    gearman_server_io_packet_remove(server_con);

  while (server_con->proc_packet_list != NULL)
    gearman_server_proc_packet_remove(server_con);

  if (thread->server->thread_count == 1)
  {
    gearman_server_con_free_workers(server_con);

    while (server_con->client_list != NULL)
      gearman_server_client_free(server_con->client_list);
  }

  GEARMAN_SERVER_THREAD_LOCK(thread)
  GEARMAN_LIST_DEL(server_con->thread->con, server_con,)
  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  if (thread->free_con_count < GEARMAN_MAX_FREE_SERVER_CON)
    GEARMAN_LIST_ADD(thread->free_con, server_con,)
  else
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

void gearman_server_con_io_add(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;

  if (server_con->io_next != NULL || server_con->io_prev != NULL)
    return;

  GEARMAN_SERVER_THREAD_LOCK(thread)

  GEARMAN_LIST_ADD(thread->io, server_con, io_)

  if (thread->io_count == 1 && thread->run_fn)
    (*thread->run_fn)(thread, thread->run_arg);

  GEARMAN_SERVER_THREAD_UNLOCK(thread)
}

void gearman_server_con_io_remove(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;

  GEARMAN_SERVER_THREAD_LOCK(thread)

  GEARMAN_LIST_DEL(thread->io, server_con, io_)
  server_con->io_prev= NULL;
  server_con->io_next= NULL;

  GEARMAN_SERVER_THREAD_UNLOCK(thread)
}

gearman_server_con_st *
gearman_server_con_io_next(gearman_server_thread_st *thread)
{
  gearman_server_con_st *server_con= thread->io_list;

  if (server_con == NULL)
    return NULL;

  gearman_server_con_io_remove(server_con);

  return server_con;
}

void gearman_server_con_proc_add(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;

  if (server_con->proc_next != NULL || server_con->proc_prev != NULL)
    return;

  GEARMAN_SERVER_THREAD_LOCK(thread)
  GEARMAN_LIST_ADD(thread->proc, server_con, proc_)
  GEARMAN_SERVER_THREAD_UNLOCK(thread)

  if (thread->server->thread_count > 1 && !(thread->server->proc_wakeup))
  {
    (void) pthread_mutex_lock(&(thread->server->proc_lock));
    thread->server->proc_wakeup= true;
    (void) pthread_cond_signal(&(thread->server->proc_cond));
    (void) pthread_mutex_unlock(&(thread->server->proc_lock));
  }
}

void gearman_server_con_proc_remove(gearman_server_con_st *server_con)
{
  gearman_server_thread_st *thread= server_con->thread;

  GEARMAN_SERVER_THREAD_LOCK(thread)

  GEARMAN_LIST_DEL(thread->proc, server_con, proc_)
  server_con->proc_prev= NULL;
  server_con->proc_next= NULL;

  GEARMAN_SERVER_THREAD_UNLOCK(thread)
}

gearman_server_con_st *
gearman_server_con_proc_next(gearman_server_thread_st *thread)
{
  gearman_server_con_st *server_con= thread->proc_list;

  if (server_con == NULL)
    return NULL;

  gearman_server_con_proc_remove(server_con);

  return server_con;
}
