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

#include <libgearman-server/common.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static gearman_server_con_st * _server_con_create(gearman_server_thread_st *thread, gearmand_con_st *dcon,
                                                  gearmand_error_t *ret);

/*
 * Public definitions
 */

gearman_server_con_st *gearman_server_con_add(gearman_server_thread_st *thread, gearmand_con_st *dcon, gearmand_error_t *ret)
{
  gearman_server_con_st *con;

  con= _server_con_create(thread, dcon, ret);
  if (con == NULL)
  {
    return NULL;
  }

  if ((*ret= gearman_io_set_fd(&(con->con), dcon->fd)) != GEARMAN_SUCCESS)
  {
    gearman_server_con_free(con);
    return NULL;
  }

  *ret= gearmand_io_set_events(con, POLLIN);
  if (*ret != GEARMAN_SUCCESS)
  {
    gearmand_gerror("gearmand_io_set_events", *ret);
    gearman_server_con_free(con);
    return NULL;
  }

  return con;
}

static gearman_server_con_st * _server_con_create(gearman_server_thread_st *thread, gearmand_con_st *dcon,
                                                  gearmand_error_t *ret)
{
  gearman_server_con_st *con;

  if (thread->free_con_count > 0)
  {
    con= thread->free_con_list;
    GEARMAN_LIST_DEL(thread->free_con, con,)
  }
  else
  {
    con= (gearman_server_con_st *)malloc(sizeof(gearman_server_con_st));
    if (con == NULL)
    {
      gearmand_perror("malloc");
      *ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }
  }
  assert(con);
  if (!con)
  {
    gearmand_error("Neigther an allocated gearman_server_con_st() or free listed could be found");
    *ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  gearmand_connection_options_t options[] = { GEARMAND_CON_MAX };
  gearmand_connection_init(thread->gearman, &(con->con), dcon, options);

  con->con.root= con;

  con->is_sleeping= false;
  con->is_exceptions= false;
  con->is_dead= false;
  con->is_noop_sent= false;

  con->ret= 0;
  con->io_list= false;
  con->proc_list= false;
  con->proc_removed= false;
  con->io_packet_count= 0;
  con->proc_packet_count= 0;
  con->worker_count= 0;
  con->client_count= 0;
  con->thread= thread;
  con->packet= NULL;
  con->io_packet_list= NULL;
  con->io_packet_end= NULL;
  con->proc_packet_list= NULL;
  con->proc_packet_end= NULL;
  con->io_next= NULL;
  con->io_prev= NULL;
  con->proc_next= NULL;
  con->proc_prev= NULL;
  con->worker_list= NULL;
  con->client_list= NULL;
  con->_host= dcon->host;
  con->_port= dcon->port;
  strcpy(con->id, "-");

  con->protocol.context= NULL;
  con->protocol.context_free_fn= NULL;
  con->protocol.packet_pack_fn= gearmand_packet_pack;
  con->protocol.packet_unpack_fn= gearmand_packet_unpack;

  int error;
  if (! (error= pthread_mutex_lock(&thread->lock)))
  {
    GEARMAN_LIST_ADD(thread->con, con,);
    (void) pthread_mutex_unlock(&thread->lock);
  }
  else
  {
    errno= error;
    gearmand_perror("pthread_mutex_lock");
    gearman_server_con_free(con);

    *ret= GEARMAN_ERRNO;
    return NULL;
  }


  return con;
}

void gearman_server_con_free(gearman_server_con_st *con)
{
  gearman_server_thread_st *thread= con->thread;
  gearman_server_packet_st *packet;

  con->_host= NULL;
  con->_port= NULL;

  if (Server->flags.threaded &&
      !(con->proc_removed) && !(Server->proc_shutdown))
  {
    con->is_dead= true;
    con->is_sleeping= false;
    con->is_exceptions= false;
    con->is_noop_sent= false;
    gearman_server_con_proc_add(con);
    return;
  }

  gearmand_io_free(&(con->con));

  if (con->protocol.context != NULL && con->protocol.context_free_fn != NULL)
  {
    con->protocol.context_free_fn(con, (void *)con->protocol.context);
  }


  if (con->proc_list)
  {
    gearman_server_con_proc_remove(con);
  }

  if (con->io_list)
  {
    gearman_server_con_io_remove(con);
  }

  if (con->packet != NULL)
  {
    if (&(con->packet->packet) != con->con.recv_packet)
    {
      gearmand_packet_free(&(con->packet->packet));
    }

    gearman_server_packet_free(con->packet, con->thread, true);
  }

  while (con->io_packet_list != NULL)
  {
    gearman_server_io_packet_remove(con);
  }

  while (con->proc_packet_list != NULL)
  {
    packet= gearman_server_proc_packet_remove(con);
    gearmand_packet_free(&(packet->packet));
    gearman_server_packet_free(packet, con->thread, true);
  }

  gearman_server_con_free_workers(con);

  while (con->client_list != NULL)
  {
    gearman_server_client_free(con->client_list);
  }

  (void) pthread_mutex_lock(&thread->lock);
  GEARMAN_LIST_DEL(con->thread->con, con,)
  (void) pthread_mutex_unlock(&thread->lock);

  if (thread->free_con_count < GEARMAN_MAX_FREE_SERVER_CON)
  {
    GEARMAN_LIST_ADD(thread->free_con, con,)
  }
  else
  {
    gearmand_crazy("free");
    free(con);
  }
}

gearmand_io_st *gearman_server_con_con(gearman_server_con_st *con)
{
  return &con->con;
}

gearmand_con_st *gearman_server_con_data(gearman_server_con_st *con)
{
  return gearman_io_context(&(con->con));
}

const char *gearman_server_con_id(gearman_server_con_st *con)
{
  return con->id;
}

void gearman_server_con_set_id(gearman_server_con_st *con, char *id,
                               size_t size)
{
  if (size >= GEARMAN_SERVER_CON_ID_SIZE)
    size= GEARMAN_SERVER_CON_ID_SIZE - 1;

  memcpy(con->id, id, size);
  con->id[size]= 0;
}

void gearman_server_con_free_worker(gearman_server_con_st *con,
                                    char *function_name,
                                    size_t function_name_size)
{
  gearman_server_worker_st *worker= con->worker_list;
  gearman_server_worker_st *prev_worker= NULL;

  while (worker != NULL)
  {
    if (worker->function->function_name_size == function_name_size &&
        !memcmp(worker->function->function_name, function_name,
                function_name_size))
    {
      gearman_server_worker_free(worker);

      /* Set worker to the last kept worker, or the beginning of the list. */
      if (prev_worker == NULL)
        worker= con->worker_list;
      else
        worker= prev_worker;
    }
    else
    {
      /* Save this so we don't need to scan the list again if one is removed. */
      prev_worker= worker;
      worker= worker->con_next;
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

  (void) pthread_mutex_lock(&con->thread->lock);

  GEARMAN_LIST_ADD(con->thread->io, con, io_)
  con->io_list= true;

  /* Looks funny, but need to check io_count locked, but call run unlocked. */
  if (con->thread->io_count == 1 && con->thread->run_fn)
  {
    (void) pthread_mutex_unlock(&con->thread->lock);
    (*con->thread->run_fn)(con->thread, con->thread->run_fn_arg);
  }
  else
  {
    (void) pthread_mutex_unlock(&con->thread->lock);
  }
}

void gearman_server_con_io_remove(gearman_server_con_st *con)
{
  (void) pthread_mutex_lock(&con->thread->lock);
  if (con->io_list)
  {
    GEARMAN_LIST_DEL(con->thread->io, con, io_)
    con->io_list= false;
  }
  (void) pthread_mutex_unlock(&con->thread->lock);
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

  (void) pthread_mutex_lock(&con->thread->lock);
  GEARMAN_LIST_ADD(con->thread->proc, con, proc_)
  con->proc_list= true;
  (void) pthread_mutex_unlock(&con->thread->lock);

  if (! (Server->proc_shutdown) &&
      !(Server->proc_wakeup))
  {
    (void) pthread_mutex_lock(&(Server->proc_lock));
    Server->proc_wakeup= true;
    (void) pthread_cond_signal(&(Server->proc_cond));
    (void) pthread_mutex_unlock(&(Server->proc_lock));
  }
}

void gearman_server_con_proc_remove(gearman_server_con_st *con)
{
  (void) pthread_mutex_lock(&con->thread->lock);

  if (con->proc_list)
  {
    GEARMAN_LIST_DEL(con->thread->proc, con, proc_)
    con->proc_list= false;
  }
  (void) pthread_mutex_unlock(&con->thread->lock);
}

gearman_server_con_st *
gearman_server_con_proc_next(gearman_server_thread_st *thread)
{
  gearman_server_con_st *con;

  if (thread->proc_list == NULL)
    return NULL;

  (void) pthread_mutex_lock(&thread->lock);

  con= thread->proc_list;
  while (con != NULL)
  {
    GEARMAN_LIST_DEL(thread->proc, con, proc_)
    con->proc_list= false;
    if (!(con->proc_removed))
      break;
    con= thread->proc_list;
  }

  (void) pthread_mutex_unlock(&thread->lock);

  return con;
}

void gearmand_connection_set_protocol(gearman_server_con_st *connection, void *context,
                                      gearmand_connection_protocol_context_free_fn *free_fn,
                                      gearmand_packet_pack_fn *pack,
                                      gearmand_packet_unpack_fn *unpack)
{
  connection->protocol.context= context;
  connection->protocol.context_free_fn= free_fn;
  connection->protocol.packet_pack_fn= pack;
  connection->protocol.packet_unpack_fn= unpack;
}

void *gearmand_connection_protocol_context(const gearman_server_con_st *connection)
{
  return connection->protocol.context;
}



