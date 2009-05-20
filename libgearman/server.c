/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server Definitions
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
 * Add job to queue wihle replaying queue during startup.
 */
gearman_return_t _queue_replay_add(gearman_st *gearman __attribute__ ((unused)),
                                   void *fn_arg, const void *unique,
                                   size_t unique_size,
                                   const void *function_name,
                                   size_t function_name_size, const void *data,
                                   size_t data_size,
                                   gearman_job_priority_t priority);

/**
 * Queue an error packet.
 */
static gearman_return_t _server_error_packet(gearman_server_con_st *server_con,
                                             char *error_code,
                                             char *error_string);

/**
 * Process text commands for a connection.
 */
static gearman_return_t _server_run_text(gearman_server_con_st *server_con,
                                         gearman_packet_st *packet);

/**
 * Send work result packets with data back to clients.
 */
static gearman_return_t
_server_queue_work_data(gearman_server_job_st *server_job,
                        gearman_packet_st *packet, gearman_command_t command);

/**
 * Wrapper for log handling.
 */
static void _log(gearman_st *gearman __attribute__ ((unused)),
                 gearman_verbose_t verbose, const char *line, void *fn_arg);

/** @} */

/*
 * Public definitions
 */

gearman_server_st *gearman_server_create(gearman_server_st *server)
{
  struct utsname un;

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

  server->gearman= gearman_create(&(server->gearman_static));
  if (server->gearman == NULL)
  {
    gearman_server_free(server);
    return NULL;
  }

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

void gearman_server_free(gearman_server_st *server)
{
  uint32_t key;
  gearman_server_packet_st *packet;
  gearman_server_job_st *job;
  gearman_server_client_st *client;
  gearman_server_worker_st *worker;

  /* All threads should be cleaned up before calling this. */
  assert(server->thread_list == NULL);

  for (key= 0; key < GEARMAN_JOB_HASH_SIZE; key++)
  {
    while (server->job_hash[key] != NULL)
      gearman_server_job_free(server->job_hash[key]);
  }

  while (server->function_list != NULL)
    gearman_server_function_free(server->function_list);

  while (server->free_packet_list != NULL)
  {
    packet= server->free_packet_list;
    server->free_packet_list= packet->next;
    free(packet);
  }

  while (server->free_job_list != NULL)
  {
    job= server->free_job_list;
    server->free_job_list= job->next;
    free(job);
  }

  while (server->free_client_list != NULL)
  {
    client= server->free_client_list;
    server->free_client_list= client->con_next;
    free(client);
  }

  while (server->free_worker_list != NULL)
  {
    worker= server->free_worker_list;
    server->free_worker_list= worker->con_next;
    free(worker);
  }

  if (server->gearman != NULL)
    gearman_free(server->gearman);

  if (server->options & GEARMAN_SERVER_ALLOCATED)
    free(server);
}

void gearman_server_set_log(gearman_server_st *server,
                            gearman_server_log_fn log_fn, void *log_fn_arg,
                            gearman_verbose_t verbose)
{
  server->log_fn= log_fn;
  server->log_fn_arg= log_fn_arg;
  gearman_set_log(server->gearman, _log, server, verbose);
}

gearman_return_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet)
{
  gearman_return_t ret;
  gearman_server_job_st *server_job;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  char option[GEARMAN_OPTION_SIZE];
  gearman_server_client_st *server_client;
  char numerator_buffer[11]; /* Max string size to hold a uint32_t. */
  char denominator_buffer[11]; /* Max string size to hold a uint32_t. */
  gearman_job_priority_t priority;
  gearman_st *gearman= gearman= server_con->thread->server->gearman;

  if (packet->magic == GEARMAN_MAGIC_RESPONSE)
  {
    return _server_error_packet(server_con, "bad_magic",
                                "Request magic expected");
  }

  GEARMAN_DEBUG(gearman, "%s Received %s",
              server_con->host == NULL ? "-" : server_con->host,
              gearman_command_info_list[packet->command].name)

  switch (packet->command)
  {
  /* Client/worker requests. */
  case GEARMAN_COMMAND_ECHO_REQ:
    /* Reuse the data buffer and just shove the data back. */
    ret= gearman_server_io_packet_add(server_con, true, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_ECHO_RES, packet->data,
                                      packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    packet->options&= ~GEARMAN_PACKET_FREE_DATA;

    break;

  /* Client requests. */
  case GEARMAN_COMMAND_SUBMIT_JOB:
  case GEARMAN_COMMAND_SUBMIT_JOB_BG:
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH:
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG:
  case GEARMAN_COMMAND_SUBMIT_JOB_LOW:
  case GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG:

    if (packet->command == GEARMAN_COMMAND_SUBMIT_JOB ||
        packet->command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
    {
      priority= GEARMAN_JOB_PRIORITY_NORMAL;
    }
    else if (packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH ||
             packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG)
    {
      priority= GEARMAN_JOB_PRIORITY_HIGH;
    }
    else
      priority= GEARMAN_JOB_PRIORITY_LOW;

    if (packet->command == GEARMAN_COMMAND_SUBMIT_JOB_BG ||
        packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG ||
        packet->command == GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG)
    {
      server_client= NULL;
    }
    else
    {
      server_client= gearman_server_client_add(server_con);
      if (server_client == NULL)
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    /* Create a job. */
    server_job= gearman_server_job_add(server_con->thread->server,
                                       (char *)(packet->arg[0]),
                                       packet->arg_size[0] - 1,
                                       (char *)(packet->arg[1]),
                                       packet->arg_size[1] - 1, packet->data,
                                       packet->data_size, priority,
                                       server_client, &ret);
    if (ret == GEARMAN_SUCCESS)
      packet->options&= ~GEARMAN_PACKET_FREE_DATA;
    else if (ret == GEARMAN_JOB_QUEUE_FULL)
    {
      return _server_error_packet(server_con, "queue_full",
                                  "Job queue is full");
    }
    else if (ret != GEARMAN_JOB_EXISTS)
      return ret;

    /* Queue the job created packet. */
    ret= gearman_server_io_packet_add(server_con, false, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_JOB_CREATED,
                                      server_job->job_handle,
                                      (size_t)strlen(server_job->job_handle),
                                      NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  case GEARMAN_COMMAND_GET_STATUS:
    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
             (uint32_t)(packet->arg_size[0]), (char *)(packet->arg[0]));

    server_job= gearman_server_job_get(server_con->thread->server, job_handle);

    /* Queue status result packet. */
    if (server_job == NULL)
    {
      ret= gearman_server_io_packet_add(server_con, false,
                                        GEARMAN_MAGIC_RESPONSE,
                                        GEARMAN_COMMAND_STATUS_RES, job_handle,
                                        (size_t)(strlen(job_handle) + 1),
                                        "0", (size_t)2, "0", (size_t)2, "0",
                                        (size_t)2, "0", (size_t)1, NULL);
    }
    else
    {
      snprintf(numerator_buffer, 11, "%u", server_job->numerator);
      snprintf(denominator_buffer, 11, "%u", server_job->denominator);

      ret= gearman_server_io_packet_add(server_con, false,
                                        GEARMAN_MAGIC_RESPONSE,
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

  case GEARMAN_COMMAND_OPTION_REQ:
    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(option, GEARMAN_OPTION_SIZE, "%.*s",
             (uint32_t)(packet->arg_size[0]), (char *)(packet->arg[0]));
    if (!strcasecmp(option, "exceptions"))
      server_con->options|= GEARMAN_SERVER_CON_EXCEPTIONS;
    else
    {
      return _server_error_packet(server_con, "unknown_option",
                                  "Server does not recognize given option");
    }

    ret= gearman_server_io_packet_add(server_con, false, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_OPTION_RES,
                                      packet->arg[0], packet->arg_size[0],
                                      NULL);
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
      ret= gearman_server_io_packet_add(server_con, false,
                                        GEARMAN_MAGIC_RESPONSE,
                                        GEARMAN_COMMAND_NOOP, NULL);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    break;

  case GEARMAN_COMMAND_GRAB_JOB:
  case GEARMAN_COMMAND_GRAB_JOB_UNIQ:
    server_con->options&= ~GEARMAN_SERVER_CON_SLEEPING;

    server_job= gearman_server_job_take(server_con);
    if (server_job == NULL)
    {
      /* No jobs found, queue no job packet. */
      ret= gearman_server_io_packet_add(server_con, false,
                                        GEARMAN_MAGIC_RESPONSE,
                                        GEARMAN_COMMAND_NO_JOB, NULL);
    }
    else if (packet->command == GEARMAN_COMMAND_GRAB_JOB_UNIQ)
    {
      /* We found a runnable job, queue job assigned packet and take the job
         off the queue. */
      ret= gearman_server_io_packet_add(server_con, false,
                                   GEARMAN_MAGIC_RESPONSE,
                                   GEARMAN_COMMAND_JOB_ASSIGN_UNIQ,
                                   server_job->job_handle,
                                   (size_t)(strlen(server_job->job_handle) + 1),
                                   server_job->function->function_name,
                                   server_job->function->function_name_size + 1,
                                   server_job->unique,
                                   (size_t)(strlen(server_job->unique) + 1),
                                   server_job->data, server_job->data_size,
                                   NULL);
    }
    else
    {
      /* Same, but without unique ID. */
      ret= gearman_server_io_packet_add(server_con, false,
                                   GEARMAN_MAGIC_RESPONSE,
                                   GEARMAN_COMMAND_JOB_ASSIGN,
                                   server_job->job_handle,
                                   (size_t)(strlen(server_job->job_handle) + 1),
                                   server_job->function->function_name,
                                   server_job->function->function_name_size + 1,
                                   server_job->data, server_job->data_size,
                                   NULL);
    }

    if (ret != GEARMAN_SUCCESS)
    {
      if (server_job != NULL)
        return gearman_server_job_queue(server_job);
      return ret;
    }

    break;

  case GEARMAN_COMMAND_WORK_DATA:
  case GEARMAN_COMMAND_WORK_WARNING:
    server_job= gearman_server_job_get(server_con->thread->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the data/warning packet for all clients. */
    ret= _server_queue_work_data(server_job, packet, packet->command);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    break;

  case GEARMAN_COMMAND_WORK_STATUS:
    server_job= gearman_server_job_get(server_con->thread->server,
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
      ret= gearman_server_io_packet_add(server_client->con, false,
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
    server_job= gearman_server_job_get(server_con->thread->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the complete packet for all clients. */
    ret= _server_queue_work_data(server_job, packet,
                                 GEARMAN_COMMAND_WORK_COMPLETE);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    /* Remove from persistent queue if one exists. */
    if (server_job->options & GEARMAN_SERVER_JOB_QUEUED &&
        gearman->queue_done_fn != NULL)
    {
      ret= (*(gearman->queue_done_fn))(gearman, (void *)gearman->queue_fn_arg,
                                       server_job->unique,
                                       (size_t)strlen(server_job->unique),
                                       server_job->function->function_name,
                                       (size_t)strlen(server_job->function->function_name));
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Job is done, remove it. */
    gearman_server_job_free(server_job);
    break;

  case GEARMAN_COMMAND_WORK_EXCEPTION:
    server_job= gearman_server_job_get(server_con->thread->server,
                                       (char *)(packet->arg[0]));
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the exception packet for all clients. */
    ret= _server_queue_work_data(server_job, packet,
                                 GEARMAN_COMMAND_WORK_EXCEPTION);
    if (ret != GEARMAN_SUCCESS)
      return ret;
    break;

  case GEARMAN_COMMAND_WORK_FAIL:
    /* This may not be NULL terminated, so copy to make sure it is. */
    snprintf(job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
             (uint32_t)(packet->arg_size[0]), (char *)(packet->arg[0]));

    server_job= gearman_server_job_get(server_con->thread->server, job_handle);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the fail packet for all clients. */
    for (server_client= server_job->client_list; server_client;
         server_client= server_client->job_next)
    {
      ret= gearman_server_io_packet_add(server_client->con, false,
                                        GEARMAN_MAGIC_RESPONSE,
                                        GEARMAN_COMMAND_WORK_FAIL,
                                        packet->arg[0], packet->arg_size[0],
                                        NULL);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Remove from persistent queue if one exists. */
    if (server_job->options & GEARMAN_SERVER_JOB_QUEUED &&
        gearman->queue_done_fn != NULL)
    {
      ret= (*(gearman->queue_done_fn))(gearman, (void *)gearman->queue_fn_arg,
                                       server_job->unique,
                                       (size_t)strlen(server_job->unique),
                                       server_job->function->function_name,
                                       (size_t)strlen(server_job->function->function_name));
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Job is done, remove it. */
    gearman_server_job_free(server_job);
    break;

  case GEARMAN_COMMAND_SET_CLIENT_ID:
    gearman_server_con_set_id(server_con, (char *)(packet->arg[0]),
                              packet->arg_size[0]);
    break;

  case GEARMAN_COMMAND_TEXT:
    return _server_run_text(server_con, packet);

  case GEARMAN_COMMAND_UNUSED:
  case GEARMAN_COMMAND_NOOP:
  case GEARMAN_COMMAND_JOB_CREATED:
  case GEARMAN_COMMAND_NO_JOB:
  case GEARMAN_COMMAND_JOB_ASSIGN:
  case GEARMAN_COMMAND_ECHO_RES:
  case GEARMAN_COMMAND_ERROR:
  case GEARMAN_COMMAND_STATUS_RES:
  case GEARMAN_COMMAND_ALL_YOURS:
  case GEARMAN_COMMAND_OPTION_RES:
  case GEARMAN_COMMAND_SUBMIT_JOB_SCHED:
  case GEARMAN_COMMAND_SUBMIT_JOB_EPOCH:
  case GEARMAN_COMMAND_JOB_ASSIGN_UNIQ:
  case GEARMAN_COMMAND_MAX:
  default:
    return _server_error_packet(server_con, "bad_command",
                                "Command not expected");
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_server_shutdown_graceful(gearman_server_st *server)
{
  server->shutdown_graceful= true;

  if (server->job_count == 0)
    return GEARMAN_SHUTDOWN;

  return GEARMAN_SHUTDOWN_GRACEFUL;
}

gearman_return_t gearman_server_queue_replay(gearman_server_st *server)
{
  gearman_return_t ret;

  if (server->gearman->queue_replay_fn == NULL)
    return GEARMAN_SUCCESS;

  server->options|= GEARMAN_SERVER_QUEUE_REPLAY;

  ret= (*(server->gearman->queue_replay_fn))(server->gearman,
                                          (void *)server->gearman->queue_fn_arg,
                                          _queue_replay_add, server);

  server->options&= ~GEARMAN_SERVER_QUEUE_REPLAY;

  return ret;
}

/*
 * Private definitions
 */

gearman_return_t _queue_replay_add(gearman_st *gearman __attribute__ ((unused)),
                                   void *fn_arg, const void *unique,
                                   size_t unique_size,
                                   const void *function_name,
                                   size_t function_name_size, const void *data,
                                   size_t data_size,
                                   gearman_job_priority_t priority)
{
  gearman_server_st *server= (gearman_server_st *)fn_arg;
  gearman_return_t ret;

  (void)gearman_server_job_add(server, (char *)function_name,
                               function_name_size, (char *)unique, unique_size,
                               data, data_size, priority, NULL, &ret);
  return ret;
}

static gearman_return_t _server_error_packet(gearman_server_con_st *server_con,
                                             char *error_code,
                                             char *error_string)
{
  return gearman_server_io_packet_add(server_con, false, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_ERROR, error_code,
                                      (size_t)(strlen(error_code) + 1),
                                      error_string,
                                      (size_t)strlen(error_string), NULL);
}

static gearman_return_t _server_run_text(gearman_server_con_st *server_con,
                                         gearman_packet_st *packet)
{
  char data[GEARMAN_TEXT_RESPONSE_SIZE];
  size_t size;
  gearman_return_t ret;
  int max_queue_size;
  gearman_server_thread_st *thread;
  gearman_server_con_st *con;
  gearman_server_worker_st *worker;
  gearman_server_function_st *function;

  if (packet->argc == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
             "ERR unknown_command Unknown+server+command\n");
  }
  else if (!strcasecmp("workers", (char *)(packet->arg[0])))
  {
    size= 0;

    for (thread= server_con->thread->server->thread_list; thread != NULL;
         thread= thread->next)
    {
      GEARMAN_SERVER_THREAD_LOCK(thread)

      for (con= thread->con_list; con != NULL; con= con->next)
      {
        if (con->host == NULL)
          continue;

        size+= snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size,
                        "%d %s %s :", con->con.fd, con->host, con->id);
        if (size > GEARMAN_TEXT_RESPONSE_SIZE)
          break;

        for (worker= con->worker_list; worker != NULL; worker= worker->con_next)
        {
          size+= snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size,
                          " %.*s", (int)(worker->function->function_name_size),
                          worker->function->function_name);
          if (size > GEARMAN_TEXT_RESPONSE_SIZE)
            break;
        }

        if (size > GEARMAN_TEXT_RESPONSE_SIZE)
          break;

        size+= snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size, "\n");
      }

      GEARMAN_SERVER_THREAD_UNLOCK(thread)
    }

    if (size < GEARMAN_TEXT_RESPONSE_SIZE)
      snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size, ".\n");
  }
  else if (!strcasecmp("status", (char *)(packet->arg[0])))
  {
    size= 0;

    for (function= server_con->thread->server->function_list; function != NULL;
         function= function->next)
    {
      size+= snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size,
                      "%.*s\t%u\t%u\t%u\n", (int)(function->function_name_size),
                      function->function_name, function->job_total,
                      function->job_running, function->worker_count);
      if (size > GEARMAN_TEXT_RESPONSE_SIZE)
        break;
    }

    if (size < GEARMAN_TEXT_RESPONSE_SIZE)
      snprintf(data + size, GEARMAN_TEXT_RESPONSE_SIZE - size, ".\n");
  }
  else if (!strcasecmp("maxqueue", (char *)(packet->arg[0])))
  {
    if (packet->argc == 1)
    {
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "ERR incomplete_args "
               "An+incomplete+set+of+arguments+was+sent+to+this+command\n");
    }
    else
    {
      if (packet->argc == 2)
        max_queue_size= GEARMAN_DEFAULT_MAX_QUEUE_SIZE;
      else
      {
        max_queue_size= atoi((char *)(packet->arg[2]));
        if (max_queue_size < 0)
          max_queue_size= 0;
      }

      for (function= server_con->thread->server->function_list;
           function != NULL; function= function->next)
      {
        if (strlen((char *)(packet->arg[1])) == function->function_name_size &&
            !memcmp(packet->arg[1], function->function_name,
                    function->function_name_size))
        {
          function->max_queue_size= max_queue_size;
        }
      }

      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
  }
  else if (!strcasecmp("shutdown", (char *)(packet->arg[0])))
  {
    if (packet->argc == 1)
    {
      server_con->thread->server->shutdown= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
    else if (packet->argc == 2 &&
             !strcasecmp("graceful", (char *)(packet->arg[1])))
    {
      server_con->thread->server->shutdown_graceful= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
    else
    {
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
               "ERR unknown_args Unknown+arguments+to+server+command\n");
    }
  }
  else if (!strcasecmp("version", (char *)(packet->arg[0])))
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "%s\n", PACKAGE_VERSION);
  else
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
             "ERR unknown_command Unknown+server+command\n");
  }

  ret= gearman_server_io_packet_add(server_con, GEARMAN_MAGIC_TEXT, false,
                                    GEARMAN_COMMAND_TEXT, data, strlen(data),
                                    NULL);
  if (ret != GEARMAN_SUCCESS)
    return ret;

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
    if (command == GEARMAN_COMMAND_WORK_EXCEPTION &&
        !(server_client->con->options & GEARMAN_SERVER_CON_EXCEPTIONS))
    {
      continue;
    }

    if (packet->data_size > 0)
    {
      if (packet->options & GEARMAN_PACKET_FREE_DATA &&
          server_client->job_next == NULL)
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

    ret= gearman_server_io_packet_add(server_client->con, true,
                                      GEARMAN_MAGIC_RESPONSE, command,
                                      packet->arg[0], packet->arg_size[0],
                                      data, packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

static void _log(gearman_st *gearman __attribute__ ((unused)),
                 gearman_verbose_t verbose, const char *line, void *fn_arg)
{
  gearman_server_st *server= (gearman_server_st *)fn_arg;
  (*(server->log_fn))(server, verbose, line, server->log_fn_arg);
}
