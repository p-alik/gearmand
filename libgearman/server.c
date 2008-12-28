/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_server_private Private Server Functions
 * @ingroup gearman_server
 * @{
 */

/**
 * Allocate a server structure.
 */
static gearman_server_st *_server_allocate(gearman_server_st *server);

/**
 * Flush outgoing packets for a connection.
 */
static gearman_return_t _server_packet_flush(gearman_server_con_st *server_con);

/**
 * Process commands for a connection.
 */
static gearman_return_t _server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet);

/** @} */

/*
 * Public definitions
 */

gearman_server_st *gearman_server_create(gearman_server_st *server)
{
  server= _server_allocate(server);
  if (server == NULL)
    return NULL;

  server->gearman= gearman_create(&(server->gearman_static));
  if (server->gearman == NULL)
  {
    gearman_server_free(server);
    return NULL;
  }

  gearman_set_options(server->gearman, GEARMAN_NON_BLOCKING, 1);

  return server;
}

gearman_server_st *gearman_server_clone(gearman_server_st *server,
                                        gearman_server_st *from)
{
  if (from == NULL)
    return NULL;

  server= _server_allocate(server);
  if (server == NULL)
    return NULL;

  server->options|= (from->options & ~GEARMAN_SERVER_ALLOCATED);

  server->gearman= gearman_clone(&(server->gearman_static), from->gearman);
  if (server->gearman == NULL)
  {
    gearman_server_free(server);
    return NULL;
  }

  return server;
}

void gearman_server_free(gearman_server_st *server)
{
  while (server->server_con_list != NULL)
    gearman_server_con_free(server->server_con_list);

  gearman_free(server->gearman);

  if (server->options & GEARMAN_SERVER_ALLOCATED)
    free(server);
}

const char *gearman_server_error(gearman_server_st *server)
{
  return gearman_error(server->gearman);
}

int gearman_server_errno(gearman_server_st *server)
{
  return gearman_errno(server->gearman);
}

void gearman_server_set_options(gearman_server_st *server,
                                gearman_server_options_t options,
                                uint32_t data)
{
  if (data)
    server->options |= options;
  else
    server->options &= ~options;
}

void gearman_server_set_event_watch(gearman_server_st *server,
                                    gearman_event_watch_fn *event_watch,
                                    void *event_watch_arg)
{
  gearman_set_event_watch(server->gearman, event_watch, event_watch_arg);
}

gearman_server_con_st *gearman_server_add_con(gearman_server_st *server,
                                              gearman_server_con_st *server_con,
                                              int fd, void *data)
{
  gearman_return_t ret;

  server_con= gearman_server_con_create(server, server_con);
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

void gearman_server_active_list_add(gearman_server_con_st *server_con)
{
  if (server_con->server->active_list != NULL)
    server_con->server->active_list->active_prev= server_con;
  server_con->active_next= server_con->server->active_list;
  server_con->server->active_list= server_con;
  server_con->server->active_count++;
}

void gearman_server_active_list_remove(gearman_server_con_st *server_con)
{
  if (server_con->server->active_list == server_con)
    server_con->server->active_list= server_con->active_next;
  if (server_con->active_prev != NULL)
  {
    server_con->active_prev->active_next= server_con->active_next;
    server_con->active_prev= NULL;
  }
  if (server_con->active_next != NULL)
  {
    server_con->active_next->active_prev= server_con->active_prev;
    server_con->active_next= NULL;
  }
  server_con->server->active_count--;
}

gearman_server_con_st *gearman_server_active_list_next(gearman_server_st *server)
{
  gearman_server_con_st *server_con= server->active_list;

  if (server_con == NULL)
    return NULL;

  gearman_server_active_list_remove(server_con);

  return server_con;
}

gearman_server_con_st *gearman_server_run(gearman_server_st *server,
                                          gearman_return_t *ret_ptr)
{
  gearman_con_st *con;
  gearman_server_con_st *server_con;

  while ((con= gearman_con_ready(server->gearman)) != NULL)
  {
    /* Inherited classes anyone? Some people would call this a hack, I call
       it clean (avoids extra ptrs). Brian, I'll give you your C99 0-byte
       arrays at the ends of structs for this. :) */
    server_con= (gearman_server_con_st *)con;

    /* Try to read new packets. */
    if (con->revents & POLLIN)
    {
      while (1)
      {
        (void)gearman_con_recv(con, &(con->packet), ret_ptr, true);
        if (*ret_ptr != GEARMAN_SUCCESS)
        {
          if (*ret_ptr == GEARMAN_IO_WAIT)
            break;
          return server_con;
        }

        /* We read a complete packet, run the command. */
        *ret_ptr= _server_run_command(server_con, &(con->packet));

        gearman_packet_free(&(con->packet));

        if (*ret_ptr != GEARMAN_SUCCESS)
          return server_con;
      }
    }

    /* Flush existing outgoing packets. */
    if (con->revents & POLLOUT)
    {
      *ret_ptr= _server_packet_flush(server_con);
      if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
        return server_con;
    }
  }

  /* Start flushing new outgoing packets. */
  while ((server_con= gearman_server_active_list_next(server)) != NULL)
  {
    *ret_ptr= _server_packet_flush(server_con);
    if (*ret_ptr != GEARMAN_SUCCESS && *ret_ptr != GEARMAN_IO_WAIT)
      return server_con;
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

/*
 * Private definitions
 */

static gearman_server_st *_server_allocate(gearman_server_st *server)
{
  if (server == NULL)
  {
    server= malloc(sizeof(gearman_server_st));
    if (server == NULL)
      return NULL;

    memset(server, 0, sizeof(gearman_server_st));
    server->options|= GEARMAN_SERVER_ALLOCATED;
  }
  else
    memset(server, 0, sizeof(gearman_server_st));

  return server;
}

static gearman_return_t _server_packet_flush(gearman_server_con_st *server_con)
{
  gearman_return_t ret;

  /* Check to see if we've already tried to avoid excessive system calls. */
  if (server_con->con.events & POLLOUT)
    return GEARMAN_IO_WAIT;

  while (server_con->packet_list != NULL)
  {
    ret= gearman_con_send(&(server_con->con),
                          &(server_con->packet_list->packet),
                          server_con->packet_list->next == NULL ? true : false);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    gearman_server_con_packet_remove(server_con);
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet)
{
  gearman_return_t ret;
  gearman_server_packet_st *server_packet;

  if (packet->magic == GEARMAN_MAGIC_RESPONSE)
  {
    /* queue error packet due to bad magic */
  }

  switch (packet->command)
  {
  /* Client/worker requests. */
  case GEARMAN_COMMAND_ECHO_REQ:
    server_packet= gearman_server_con_packet_add(server_con);
    if (server_packet == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    ret= gearman_packet_add(server_con->server->gearman,
                            &(server_packet->packet), GEARMAN_MAGIC_RESPONSE,
                            GEARMAN_COMMAND_ECHO_RES, packet->data,
                            packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    server_packet->options|= GEARMAN_SERVER_PACKET_IN_USE;
    packet->options&= ~GEARMAN_PACKET_FREE_DATA;
    server_packet->packet.options|= GEARMAN_PACKET_FREE_DATA;
    gearman_server_active_list_add(server_con);

    break;

  /* Client requests. */
  case GEARMAN_COMMAND_SUBMIT_JOB:
  case GEARMAN_COMMAND_SUBMIT_JOB_BG:
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH:
#if 0
    /* Create new job. */
    server_job= gearman_server_job_add(server);
    if (server_job == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    if (con->packet.command != GEARMAN_COMMAND_SUBMIT_JOB_BG)
      server_job->watcher= server_con;

    create function entry if it doesnt exist
    add job to function entry (code above)
    queue job created packet
    queue NOOP to all matching idle workers
#endif
    break;

  case GEARMAN_COMMAND_GET_STATUS:
    break;

  /* Worker requests. */
  case GEARMAN_COMMAND_CAN_DO:
  case GEARMAN_COMMAND_CAN_DO_TIMEOUT:
#if 0
    create function entry if it doesnt exist
    add worker to function entry
#endif
    break;

  case GEARMAN_COMMAND_CANT_DO:
  case GEARMAN_COMMAND_RESET_ABILITIES:
#if 0
    remove worker from function entry
#endif
    break;

  case GEARMAN_COMMAND_PRE_SLEEP:
#if 0
    mark worker as idle
    check function entry for jobs, if so, queue NOOP
#endif
    break;

  case GEARMAN_COMMAND_GRAB_JOB:
#if 0
    find a job matching the function name
    queue job assigned packet
    make job as running
#endif
    break;

  case GEARMAN_COMMAND_WORK_DATA:
  case GEARMAN_COMMAND_WORK_STATUS:
  case GEARMAN_COMMAND_WORK_COMPLETE:
  case GEARMAN_COMMAND_WORK_FAIL:
    /* forward response packets */
  case GEARMAN_COMMAND_SET_CLIENT_ID:
  default:
    /* queue error packet due to bad comand */
    break;
  }

  return GEARMAN_SUCCESS;
}
