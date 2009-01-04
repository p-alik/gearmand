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
 * Queue an error packet.
 */
static gearman_return_t _server_error_packet(gearman_server_con_st *server_con,
                                             char *error_code,
                                             char *error_string);

/**
 * Process commands for a connection.
 */
static gearman_return_t _server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet);

/**
 * Send work result packets with data back to clients.
 */
static gearman_return_t 
_server_queue_work_data(gearman_server_job_st *server_job,
                        gearman_packet_st *packet, gearman_command_t command);

/** @} */

/*
 * Public definitions
 */

gearman_server_st *gearman_server_create(gearman_server_st *server)
{
  struct utsname un;

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

  if (uname(&un) == -1)
  {
    gearman_server_free(server);
    return NULL;
  }

  snprintf(server->job_handle_prefix, GEARMAN_JOB_HANDLE_SIZE, "H:%s",
           un.nodename);
  server->job_handle_count= 1;

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
  uint32_t key;

  while (server->con_list != NULL)
    gearman_server_con_free(server->con_list);

  for (key= 0; key < GEARMAN_JOB_HASH_SIZE; key++)
  {
    while (server->job_hash[key] != NULL)
      gearman_server_job_free(server->job_hash[key]);
  }

  while (server->function_list != NULL)
    gearman_server_function_free(server->function_list);

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

void gearman_server_active_add(gearman_server_con_st *server_con)
{
  if (server_con->active_next != NULL || server_con->active_prev != NULL)
    return;

  GEARMAN_LIST_ADD(server_con->server->active, server_con, active_)
}

void gearman_server_active_remove(gearman_server_con_st *server_con)
{
  GEARMAN_LIST_DEL(server_con->server->active, server_con, active_)
  server_con->active_prev= NULL;
  server_con->active_next= NULL;
}

gearman_server_con_st *gearman_server_active_next(gearman_server_st *server)
{
  gearman_server_con_st *server_con= server->active_list;

  if (server_con == NULL)
    return NULL;

  gearman_server_active_remove(server_con);

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
  while ((server_con= gearman_server_active_next(server)) != NULL)
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

static gearman_return_t _server_error_packet(gearman_server_con_st *server_con,
                                             char *error_code,
                                             char *error_string)
{
  return gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                       GEARMAN_COMMAND_ERROR, error_code,
                                       (size_t)(strlen(error_code) + 1),
                                       error_string,
                                       (size_t)strlen(error_string), NULL);
}

static gearman_return_t _server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet)
{
  gearman_return_t ret;
  gearman_server_job_st *server_job;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  gearman_server_client_st *server_client;
  char numerator_buffer[11]; /* Max string size to hold a uint32_t. */
  char denominator_buffer[11]; /* Max string size to hold a uint32_t. */

  if (packet->magic == GEARMAN_MAGIC_RESPONSE)
  {
    return _server_error_packet(server_con, "bad_magic",
                                "Request magic expected");
  }

  switch (packet->command)
  {
  /* Client/worker requests. */
  case GEARMAN_COMMAND_ECHO_REQ:
    /* Reuse the data buffer and just shove the data back. */
    ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                       GEARMAN_COMMAND_ECHO_RES, packet->data,
                                       packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    packet->options&= ~GEARMAN_PACKET_FREE_DATA;
    server_con->packet_end->packet.options|= GEARMAN_PACKET_FREE_DATA;

    break;

  /* Client requests. */
  case GEARMAN_COMMAND_SUBMIT_JOB:
  case GEARMAN_COMMAND_SUBMIT_JOB_BG:
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH:
    if (packet->command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
      server_client= NULL;
    else
    {
      server_client= gearman_server_client_add(server_con);
      if (server_client == NULL)
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    /* Create a job. */
    server_job= gearman_server_job_add(server_con->server,
                                       (char *)(packet->arg[0]),
                                       packet->arg_size[0] - 1,
                                       (char *)(packet->arg[1]),
                                       packet->arg_size[1] - 1, packet->data,
                                       packet->data_size,
                            packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH ?
                                       true : false, server_client, &ret);
    if (ret == GEARMAN_SUCCESS)
      packet->options&= ~GEARMAN_PACKET_FREE_DATA;
    else if (ret != GEARMAN_JOB_EXISTS)
      return ret;

    /* Queue the job created packet. */
    ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                            GEARMAN_COMMAND_JOB_CREATED, server_job->job_handle,
                            (size_t)strlen(server_job->job_handle), NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  case GEARMAN_COMMAND_GET_STATUS:
    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
             (uint32_t)(packet->arg_size[0]), (char *)(packet->arg[0]));

    server_job= gearman_server_job_get(server_con->server, job_handle);

    /* Queue status result packet. */
    if (server_job == NULL)
    {
      ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                         GEARMAN_COMMAND_STATUS_RES, job_handle,
                                         (size_t)(strlen(job_handle) + 1),
                                         "0", (size_t)2, "0", (size_t)2, "0",
                                         (size_t)2, "0", (size_t)1, NULL);
    }
    else
    {
      snprintf(numerator_buffer, 11, "%u", server_job->numerator);
      snprintf(denominator_buffer, 11, "%u", server_job->denominator);

      ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                         GEARMAN_COMMAND_STATUS_RES, job_handle,
                                         (size_t)(strlen(job_handle) + 1),
                                         "1", (size_t)2,
                                         server_job->worker == NULL ? "0" : "1",
                                         (size_t)2, numerator_buffer,
                                         (size_t)(strlen(numerator_buffer) + 1),
                                         denominator_buffer,
                                         (size_t)strlen(denominator_buffer),
                                         NULL);
    }

    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  /* Worker requests. */
  case GEARMAN_COMMAND_CAN_DO:
    if (gearman_server_worker_add(server_con, (char *)(packet->arg[0]),
                                  packet->arg_size[0], 0) == NULL)
    {
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    break;

  case GEARMAN_COMMAND_CAN_DO_TIMEOUT:
    if (gearman_server_worker_add(server_con, (char *)(packet->arg[0]),
                                  packet->arg_size[0] - 1,
                                  atoi((char *)(packet->arg[1]))) == NULL)
    {
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    break;

  case GEARMAN_COMMAND_CANT_DO:
    gearman_server_con_free_worker(server_con, (char *)(packet->arg[0]),
                                   packet->arg_size[0]);
    break;

  case GEARMAN_COMMAND_RESET_ABILITIES:
    gearman_server_con_free_workers(server_con);
    break;

  case GEARMAN_COMMAND_PRE_SLEEP:
    server_job= gearman_server_job_peek(server_con);
    if (server_job == NULL)
      server_con->options|= GEARMAN_SERVER_CON_SLEEPING;
    else
    {
      /* If there are jobs that could be run, queue a NOOP packet to wake the
         worker up. This could be the result of a race codition. */
      ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                              GEARMAN_COMMAND_NOOP, NULL);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    break;

  case GEARMAN_COMMAND_GRAB_JOB:
    server_con->options&= ~GEARMAN_SERVER_CON_SLEEPING;

    server_job= gearman_server_job_take(server_con);
    if (server_job == NULL)
    {
      /* No jobs found, queue no job packet. */
      ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                         GEARMAN_COMMAND_NO_JOB, NULL);
    }
    else
    {
      /* We found a runnable job, queue job assigned packet and take the job
         off the queue. */
      ret= gearman_server_con_packet_add(server_con, GEARMAN_MAGIC_RESPONSE,
                                 GEARMAN_COMMAND_JOB_ASSIGN,
                                 server_job->job_handle,
                                 (size_t)(strlen(server_job->job_handle) + 1),
                                 server_job->function->function_name,
                                 server_job->function->function_name_size + 1,
                                 server_job->data, server_job->data_size, NULL);
    }

    if (ret != GEARMAN_SUCCESS)
    {
      if (server_job != NULL)
        return gearman_server_job_queue(server_job);
      return ret;
    }

    break;

  case GEARMAN_COMMAND_WORK_DATA:
    server_job= gearman_server_job_get(server_con->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the data packet for all clients. */
    ret= _server_queue_work_data(server_job, packet, GEARMAN_COMMAND_WORK_DATA);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  case GEARMAN_COMMAND_WORK_STATUS:
    server_job= gearman_server_job_get(server_con->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Update job status. */
    server_job->numerator= atoi((char *)(packet->arg[1]));

    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(denominator_buffer, 11, "%.*s", (uint32_t)(packet->arg_size[2]),
             (char *)(packet->arg[2]));
    server_job->denominator= atoi(denominator_buffer);

    /* Queue the status packet for all clients. */
    for (server_client= server_job->client_list; server_client;
         server_client= server_client->job_next)
    {
      ret= gearman_server_con_packet_add(server_client->con,
                                         GEARMAN_MAGIC_RESPONSE,
                                         GEARMAN_COMMAND_WORK_STATUS,
                                         packet->arg[0], packet->arg_size[0],
                                         packet->arg[1], packet->arg_size[1],
                                         packet->arg[2], packet->arg_size[2],
                                         NULL);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    break;

  case GEARMAN_COMMAND_WORK_COMPLETE:
    server_job= gearman_server_job_get(server_con->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the data packet for all clients. */
    ret= _server_queue_work_data(server_job, packet,
                                 GEARMAN_COMMAND_WORK_COMPLETE);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    /* Job is done, remove it. */
    gearman_server_job_free(server_job);
    break;

  case GEARMAN_COMMAND_WORK_FAIL:
    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
             (uint32_t)(packet->arg_size[0]), (char *)(packet->arg[0]));

    server_job= gearman_server_job_get(server_con->server, job_handle);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the status packet for all clients. */
    for (server_client= server_job->client_list; server_client;
         server_client= server_client->job_next)
    {
      ret= gearman_server_con_packet_add(server_client->con,
                                         GEARMAN_MAGIC_RESPONSE,
                                         GEARMAN_COMMAND_WORK_FAIL,
                                         packet->arg[0], packet->arg_size[0],
                                         NULL);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Job is done, remove it. */
    gearman_server_job_free(server_job);
    break;

  case GEARMAN_COMMAND_SET_CLIENT_ID:
    ret= gearman_server_con_set_id(server_con, packet->arg[0],
                                   packet->arg_size[0]);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  case GEARMAN_COMMAND_NONE:
  case GEARMAN_COMMAND_UNUSED:
  case GEARMAN_COMMAND_NOOP:
  case GEARMAN_COMMAND_JOB_CREATED:
  case GEARMAN_COMMAND_NO_JOB:
  case GEARMAN_COMMAND_JOB_ASSIGN:
  case GEARMAN_COMMAND_ECHO_RES:
  case GEARMAN_COMMAND_ERROR:
  case GEARMAN_COMMAND_STATUS_RES:
  case GEARMAN_COMMAND_ALL_YOURS:
  case GEARMAN_COMMAND_SUBMIT_JOB_SCHED:
  case GEARMAN_COMMAND_SUBMIT_JOB_EPOCH:
  case GEARMAN_COMMAND_MAX:
  default:
    return _server_error_packet(server_con, "bad_command",
                                "Command not expected");
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t 
_server_queue_work_data(gearman_server_job_st *server_job,
                        gearman_packet_st *packet, gearman_command_t command)
{
  gearman_server_client_st *server_client;
  uint8_t *data;
  gearman_return_t ret;

  for (server_client= server_job->client_list; server_client;
       server_client= server_client->job_next)
  {
    if (packet->data_size > 0)
    {
      if (packet->options & GEARMAN_PACKET_FREE_DATA)
      {
        data= (uint8_t *)(packet->data);
        packet->options&= ~GEARMAN_PACKET_FREE_DATA;
      }
      else
      {
        data= malloc(packet->data_size);
        if (data == NULL)
        {
          GEARMAN_ERROR_SET(packet->gearman, "_server_run_command", "malloc")
          return GEARMAN_MEMORY_ALLOCATION_FAILURE;
        }

        memcpy(data, packet->data, packet->data_size);
      }
    }
    else
      data= NULL;

    ret= gearman_server_con_packet_add(server_client->con,
                                       GEARMAN_MAGIC_RESPONSE, command,
                                       packet->arg[0], packet->arg_size[0],
                                       data, packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    server_client->con->packet_end->packet.options|= GEARMAN_PACKET_FREE_DATA;
  }

  return GEARMAN_SUCCESS;
}
