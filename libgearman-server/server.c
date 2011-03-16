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
static gearmand_error_t _queue_replay_add(gearman_server_st *server, void *context,
                                          const char *unique, size_t unique_size,
                                          const char *function_name, size_t function_name_size,
                                          const void *data, size_t data_size,
                                          gearmand_job_priority_t priority);

/**
 * Queue an error packet.
 */
static gearmand_error_t _server_error_packet(gearman_server_con_st *server_con,
                                             const char *error_code,
                                             const char *error_string);

/**
 * Process text commands for a connection.
 */
static gearmand_error_t _server_run_text(gearman_server_con_st *server_con,
                                         gearmand_packet_st *packet);

/**
 * Send work result packets with data back to clients.
 */
static gearmand_error_t
_server_queue_work_data(gearman_server_job_st *server_job,
                        gearmand_packet_st *packet, gearman_command_t command);

/** @} */

/*
 * Public definitions
 */

gearmand_error_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearmand_packet_st *packet)
{
  gearmand_error_t ret;
  gearman_server_job_st *server_job;
  char job_handle[GEARMAND_JOB_HANDLE_SIZE];
  char option[GEARMAN_OPTION_SIZE];
  gearman_server_client_st *server_client;
  char numerator_buffer[11]; /* Max string size to hold a uint32_t. */
  char denominator_buffer[11]; /* Max string size to hold a uint32_t. */
  gearmand_job_priority_t priority;

  int checked_length;

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
    ret= gearman_server_io_packet_add(server_con, true, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_ECHO_RES, packet->data,
                                      packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("gearman_server_io_packet_add", ret);
      return ret;
    }

    packet->options.free_data= false;

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
      priority= GEARMAND_JOB_PRIORITY_NORMAL;
    }
    else if (packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH ||
             packet->command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG)
    {
      priority= GEARMAND_JOB_PRIORITY_HIGH;
    }
    else
      priority= GEARMAND_JOB_PRIORITY_LOW;

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
      {
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
    }

    /* Create a job. */
    server_job= gearman_server_job_add(Server,
                                       (char *)(packet->arg[0]),
                                       packet->arg_size[0] - 1,
                                       (char *)(packet->arg[1]),
                                       packet->arg_size[1] - 1, packet->data,
                                       packet->data_size, priority,
                                       server_client, &ret);
    if (ret == GEARMAN_SUCCESS)
    {
      packet->options.free_data= false;
    }
    else if (ret == GEARMAN_JOB_QUEUE_FULL)
    {
      return _server_error_packet(server_con, "queue_full",
                                  "Job queue is full");
    }
    else if (ret != GEARMAN_JOB_EXISTS)
    {
      gearmand_gerror("gearman_server_job_add", ret);
      return ret;
    }

    /* Queue the job created packet. */
    ret= gearman_server_io_packet_add(server_con, false, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_JOB_CREATED,
                                      server_job->job_handle,
                                      (size_t)strlen(server_job->job_handle),
                                      NULL);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("gearman_server_io_packet_add", ret);
      return ret;
    }

    break;

  case GEARMAN_COMMAND_GET_STATUS:
    {
      /* This may not be NULL terminated, so copy to make sure it is. */
      checked_length= snprintf(job_handle, GEARMAND_JOB_HANDLE_SIZE, "%.*s",
                               (int)(packet->arg_size[0]), (char *)(packet->arg[0]));

      if (checked_length >= GEARMAND_JOB_HANDLE_SIZE || checked_length < 0)
      {
        gearmand_error("snprintf");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }

      server_job= gearman_server_job_get(Server, job_handle, NULL);

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
        checked_length= snprintf(numerator_buffer, sizeof(numerator_buffer), "%u", server_job->numerator);
        if ((size_t)checked_length >= sizeof(numerator_buffer) || checked_length < 0)
        {
          gearmand_log_error("_server_command_get_status", "snprintf");
          return GEARMAN_MEMORY_ALLOCATION_FAILURE;
        }

        checked_length= snprintf(denominator_buffer, sizeof(denominator_buffer), "%u", server_job->denominator);
        if ((size_t)checked_length >= sizeof(denominator_buffer) || checked_length < 0)
        {
          gearmand_log_error("_server_command_get_status", "snprintf");
          return GEARMAN_MEMORY_ALLOCATION_FAILURE;
        }

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
      {
        gearmand_gerror("gearman_server_io_packet_add", ret);
        return ret;
      }
    }

    break;

  case GEARMAN_COMMAND_OPTION_REQ:
    /* This may not be NULL terminated, so copy to make sure it is. */
    checked_length= snprintf(option, GEARMAN_OPTION_SIZE, "%.*s",
                                 (int)(packet->arg_size[0]), (char *)(packet->arg[0]));

    if (checked_length >= GEARMAN_OPTION_SIZE || checked_length < 0)
    {
      gearmand_log_error("_server_command_option_request", "snprintf");
      return _server_error_packet(server_con, "unknown_option",
                                  "Server does not recognize given option");
    }

    if (!strcasecmp(option, "exceptions"))
    {
      server_con->is_exceptions= true;
    }
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
    {
      gearmand_gerror("gearman_server_io_packet_add", ret);
      return ret;
    }

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
                                  (in_port_t)atoi((char *)(packet->arg[1])))
         == NULL)
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
    {
      server_con->is_sleeping= true;
    }
    else
    {
      /* If there are jobs that could be run, queue a NOOP packet to wake the
         worker up. This could be the result of a race codition. */
      ret= gearman_server_io_packet_add(server_con, false,
                                        GEARMAN_MAGIC_RESPONSE,
                                        GEARMAN_COMMAND_NOOP, NULL);
      if (ret != GEARMAN_SUCCESS)
      {
        gearmand_gerror("gearman_server_io_packet_add", ret);
        return ret;
      }
    }

    break;

  case GEARMAN_COMMAND_GRAB_JOB:
  case GEARMAN_COMMAND_GRAB_JOB_UNIQ:
    server_con->is_sleeping= false;
    server_con->is_noop_sent= false;

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
      gearmand_gerror("gearman_server_io_packet_add", ret);

      if (server_job != NULL)
        return gearman_server_job_queue(server_job);

      return ret;
    }

    break;

  case GEARMAN_COMMAND_WORK_DATA:
  case GEARMAN_COMMAND_WORK_WARNING:
    server_job= gearman_server_job_get(Server,
                                       (char *)(packet->arg[0]),
                                       server_con);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the data/warning packet for all clients. */
    ret= _server_queue_work_data(server_job, packet, packet->command);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("_server_queue_work_data", ret);
      return ret;
    }

    break;

  case GEARMAN_COMMAND_WORK_STATUS:
    server_job= gearman_server_job_get(Server,
                                       (char *)(packet->arg[0]),
                                       server_con);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Update job status. */
    server_job->numerator= (uint32_t)atoi((char *)(packet->arg[1]));

    /* This may not be NULL terminated, so copy to make sure it is. */
    checked_length= snprintf(denominator_buffer, sizeof(denominator_buffer), "%.*s", (int)(packet->arg_size[2]),
                             (char *)(packet->arg[2]));

    if ((size_t)checked_length > sizeof(denominator_buffer) || checked_length < 0)
    {
      gearmand_error("snprintf");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }


    server_job->denominator= (uint32_t)atoi(denominator_buffer);

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
      {
        gearmand_gerror("gearman_server_io_packet_add", ret);
        return ret;
      }
    }

    break;

  case GEARMAN_COMMAND_WORK_COMPLETE:
    server_job= gearman_server_job_get(Server,
                                       (char *)(packet->arg[0]),
                                       server_con);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the complete packet for all clients. */
    ret= _server_queue_work_data(server_job, packet,
                                 GEARMAN_COMMAND_WORK_COMPLETE);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("_server_queue_work_data", ret);
      return ret;
    }

    /* Remove from persistent queue if one exists. */
    if (server_job->job_queued && Server->queue._done_fn != NULL)
    {
      ret= (*(Server->queue._done_fn))(Server, (void *)Server->queue._context,
                                       server_job->unique,
                                       (size_t)strlen(server_job->unique),
                                       server_job->function->function_name,
                                       server_job->function->function_name_size);
      if (ret != GEARMAN_SUCCESS)
      {
        gearmand_gerror("Remove from persistent queue", ret);
        return ret;
      }
    }

    /* Job is done, remove it. */
    gearman_server_job_free(server_job);
    break;

  case GEARMAN_COMMAND_WORK_EXCEPTION:
    server_job= gearman_server_job_get(Server,
                                       (char *)(packet->arg[0]),
                                       server_con);
    if (server_job == NULL)
    {
      return _server_error_packet(server_con, "job_not_found",
                                  "Job given in work result not found");
    }

    /* Queue the exception packet for all clients. */
    ret= _server_queue_work_data(server_job, packet,
                                 GEARMAN_COMMAND_WORK_EXCEPTION);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("_server_queue_work_data", ret);
      return ret;
    }
    break;

  case GEARMAN_COMMAND_WORK_FAIL:
    /* This may not be NULL terminated, so copy to make sure it is. */
    checked_length= snprintf(job_handle, GEARMAND_JOB_HANDLE_SIZE, "%.*s",
                             (int)(packet->arg_size[0]), (char *)(packet->arg[0]));

    if (checked_length >= GEARMAND_JOB_HANDLE_SIZE || checked_length < 0)
    {
      return _server_error_packet(server_con, "job_name_too_large",
                                  "Error occured due to GEARMAND_JOB_HANDLE_SIZE being too small from snprintf");
    }

    server_job= gearman_server_job_get(Server, job_handle,
                                       server_con);
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
      {
        gearmand_gerror("gearman_server_io_packet_add", ret);
        return ret;
      }
    }

    /* Remove from persistent queue if one exists. */
    if (server_job->job_queued && Server->queue._done_fn != NULL)
    {
      ret= (*(Server->queue._done_fn))(Server, (void *)Server->queue._context,
                                       server_job->unique,
                                       (size_t)strlen(server_job->unique),
                                       server_job->function->function_name,
                                       server_job->function->function_name_size);
      if (ret != GEARMAN_SUCCESS)
      {
        gearmand_gerror("Remove from persistent queue", ret);
        return ret;
      }
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

gearmand_error_t gearman_server_shutdown_graceful(gearman_server_st *server)
{
  server->shutdown_graceful= true;

  if (server->job_count == 0)
    return GEARMAN_SHUTDOWN;

  return GEARMAN_SHUTDOWN_GRACEFUL;
}

gearmand_error_t gearman_server_queue_replay(gearman_server_st *server)
{
  gearmand_error_t ret;

  if (server->queue._replay_fn == NULL)
    return GEARMAN_SUCCESS;

  server->state.queue_startup= true;

  ret= (*(server->queue._replay_fn))(server, (void *)server->queue._context,
                                     _queue_replay_add, server);

  server->state.queue_startup= false;

  return ret;
}

void *gearman_server_queue_context(const gearman_server_st *server)
{
  return (void *)server->queue._context;
}

void gearman_server_set_queue(gearman_server_st *server,
                              void *context,
                              gearman_queue_add_fn *add,
                              gearman_queue_flush_fn *flush,
                              gearman_queue_done_fn *done,
                              gearman_queue_replay_fn *replay)
{
  server->queue._context= context;
  server->queue._add_fn= add;
  server->queue._flush_fn= flush;
  server->queue._done_fn= done;
  server->queue._replay_fn= replay;
}

/*
 * Private definitions
 */

gearmand_error_t _queue_replay_add(gearman_server_st *server,
                                   void *context __attribute__ ((unused)),
                                   const char *unique, size_t unique_size,
                                   const char *function_name, size_t function_name_size,
                                   const void *data, size_t data_size,
                                   gearmand_job_priority_t priority)
{
  gearmand_error_t ret= GEARMAN_SUCCESS;

  (void)gearman_server_job_add(server,
                               function_name, function_name_size,
                               unique, unique_size,
                               data, data_size, priority, NULL, &ret);

  if (ret != GEARMAN_SUCCESS)
    gearmand_gerror("gearman_server_job_add", ret);

  return ret;
}

static gearmand_error_t _server_error_packet(gearman_server_con_st *server_con,
                                             const char *error_code,
                                             const char *error_string)
{
  return gearman_server_io_packet_add(server_con, false, GEARMAN_MAGIC_RESPONSE,
                                      GEARMAN_COMMAND_ERROR, error_code,
                                      (size_t)(strlen(error_code) + 1),
                                      error_string,
                                      (size_t)strlen(error_string), NULL);
}

static gearmand_error_t _server_run_text(gearman_server_con_st *server_con,
                                         gearmand_packet_st *packet)
{
  char *data;
  char *new_data;
  size_t size;
  size_t total;
  int priority;
  uint32_t max_queue_size[GEARMAND_JOB_PRIORITY_MAX];
  gearman_server_thread_st *thread;
  gearman_server_con_st *con;
  gearman_server_worker_st *worker;
  gearman_server_function_st *function;
  gearman_server_packet_st *server_packet;
  int checked_length;

  data= (char *)malloc(GEARMAN_TEXT_RESPONSE_SIZE);
  if (data == NULL)
  {
    gearmand_perror("malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }
  total= GEARMAN_TEXT_RESPONSE_SIZE;

  if (packet->argc == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
             "ERR unknown_command Unknown+server+command\n");
  }
  else if (!strcasecmp("workers", (char *)(packet->arg[0])))
  {
    size= 0;

    for (thread= Server->thread_list; thread != NULL;
         thread= thread->next)
    {
      int error;
      if (! (error= pthread_mutex_lock(&thread->lock)))
      {
        for (con= thread->con_list; con != NULL; con= con->next)
        {
          if (con->_host == NULL)
            continue;

          if (size > total)
            size= total;

          /* Make sure we have at least GEARMAN_TEXT_RESPONSE_SIZE bytes. */
          if (size + GEARMAN_TEXT_RESPONSE_SIZE > total)
          {
            new_data= (char *)realloc(data, total + GEARMAN_TEXT_RESPONSE_SIZE);
            if (new_data == NULL)
            {
              (void) pthread_mutex_unlock(&thread->lock);
              gearmand_perror("realloc");
              gearmand_crazy("free");
              free(data);
              return GEARMAN_MEMORY_ALLOCATION_FAILURE;
            }

            data= new_data;
            total+= GEARMAN_TEXT_RESPONSE_SIZE;
          }

          checked_length= snprintf(data + size, total - size, "%d %s %s :",
                                   con->con.fd, con->_host, con->id);

          if ((size_t)checked_length > total - size || checked_length < 0)
          {
            (void) pthread_mutex_unlock(&thread->lock);
            gearmand_crazy("free");
            free(data);
            gearmand_error("snprintf");
            return GEARMAN_MEMORY_ALLOCATION_FAILURE;
          }

          size+= (size_t)checked_length;
          if (size > total)
            continue;

          for (worker= con->worker_list; worker != NULL; worker= worker->con_next)
          {
            checked_length= snprintf(data + size, total - size, " %.*s",
                                     (int)(worker->function->function_name_size),
                                     worker->function->function_name);

            if ((size_t)checked_length > total - size || checked_length < 0)
            {
              (void) pthread_mutex_unlock(&thread->lock);
              gearmand_crazy("free");
              free(data);
              gearmand_error("snprintf");
              return GEARMAN_MEMORY_ALLOCATION_FAILURE;
            }

            size+= (size_t)checked_length;
            if (size > total)
              break;
          }

          if (size > total)
            continue;

          checked_length= snprintf(data + size, total - size, "\n");
          if ((size_t)checked_length > total - size || checked_length < 0)
          {
            (void) pthread_mutex_unlock(&thread->lock);
            gearmand_crazy("free");
            free(data);
            gearmand_perror("snprintf");
            return GEARMAN_MEMORY_ALLOCATION_FAILURE;
          }
          size+= (size_t)checked_length;
        }

        (void) pthread_mutex_unlock(&thread->lock);
      }
      else
      {
        errno= error;
        gearmand_error("pthread_mutex_lock");
        assert(! "pthread_mutex_lock");
      }
    }

    if (size < total)
    {
      checked_length= snprintf(data + size, total - size, ".\n");
      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        gearmand_error("snprintf");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
    }
  }
  else if (!strcasecmp("status", (char *)(packet->arg[0])))
  {
    size= 0;

    for (function= Server->function_list; function != NULL;
         function= function->next)
    {
      if (size + GEARMAN_TEXT_RESPONSE_SIZE > total)
      {
        new_data= (char *)realloc(data, total + GEARMAN_TEXT_RESPONSE_SIZE);
        if (new_data == NULL)
        {
          gearmand_perror("realloc");
          gearmand_crazy("free");
          free(data);
          return GEARMAN_MEMORY_ALLOCATION_FAILURE;
        }

        data= new_data;
        total+= GEARMAN_TEXT_RESPONSE_SIZE;
      }

      checked_length= snprintf(data + size, total - size, "%.*s\t%u\t%u\t%u\n",
                               (int)(function->function_name_size),
                               function->function_name, function->job_total,
                               function->job_running, function->worker_count);

      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        gearmand_perror("snprintf");
        gearmand_crazy("free");
        free(data);
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }

      size+= (size_t)checked_length;
      if (size > total)
        size= total;
    }

    if (size < total)
    {
      checked_length= snprintf(data + size, total - size, ".\n");
      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        gearmand_perror("snprintf");
        gearmand_crazy("free");
        free(data);
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
    }
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
      for (priority= 0; priority < GEARMAND_JOB_PRIORITY_MAX; ++priority)
      {
        const int argc= priority+2;
        if (packet->argc > argc)
        {
          const int parameter = atoi((char *)(packet->arg[argc]));
          if (parameter < 0)
            max_queue_size[priority]= 0;
          else
            max_queue_size[priority]= (uint32_t)parameter;
        }
        else
        {
          max_queue_size[priority]= GEARMAN_DEFAULT_MAX_QUEUE_SIZE;
        }
      }
      /* To preserve the existing behavior of maxqueue, ensure that the
         one-parameter invocation is applied to all priorities. */
      if (packet->argc <= 3)
        for (priority= 1; priority < GEARMAND_JOB_PRIORITY_MAX; ++priority)
          max_queue_size[priority]= max_queue_size[0];
       
      for (function= Server->function_list;
           function != NULL; function= function->next)
      {
        if (strlen((char *)(packet->arg[1])) == function->function_name_size &&
            !memcmp(packet->arg[1], function->function_name,
                    function->function_name_size))
        {
          gearmand_log_debug("Applying queue limits to %s",
                             function->function_name);
          memcpy(function->max_queue_size, max_queue_size,
                 sizeof(uint32_t) * GEARMAND_JOB_PRIORITY_MAX);
        }
      }

      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
  }
  else if (!strcasecmp("shutdown", (char *)(packet->arg[0])))
  {
    if (packet->argc == 1)
    {
      Server->shutdown= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
    else if (packet->argc == 2 &&
             !strcasecmp("graceful", (char *)(packet->arg[1])))
    {
      Server->shutdown_graceful= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK\n");
    }
    else
    {
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
               "ERR unknown_args Unknown+arguments+to+server+command\n");
    }
  }
  else if (!strcasecmp("verbose", (char *)(packet->arg[0])))
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "%s\n", gearmand_verbose_name(Gearmand()->verbose));
  }
  else if (!strcasecmp("version", (char *)(packet->arg[0])))
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "%s\n", PACKAGE_VERSION);
  }
  else
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE,
             "ERR unknown_command Unknown+server+command\n");
  }

  server_packet= gearman_server_packet_create(server_con->thread, false);
  if (server_packet == NULL)
  {
    gearmand_crazy("free");
    free(data);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearmand_packet_init(&(server_packet->packet), GEARMAN_MAGIC_TEXT, GEARMAN_COMMAND_TEXT);

  server_packet->packet.magic= GEARMAN_MAGIC_TEXT;
  server_packet->packet.command= GEARMAN_COMMAND_TEXT;
  server_packet->packet.options.complete= true;
  server_packet->packet.options.free_data= true;

  server_packet->packet.data= data;
  server_packet->packet.data_size= strlen(data);

  int error;
  if (! (error= pthread_mutex_lock(&server_con->thread->lock)))
  {
    GEARMAN_FIFO_ADD(server_con->io_packet, server_packet,);
    (void) pthread_mutex_unlock(&server_con->thread->lock);
  }
  else
  {
    errno= error;
    gearmand_perror("pthread_mutex_lock");
    assert(!"pthread_mutex_lock");
  }

  gearman_server_con_io_add(server_con);

  return GEARMAN_SUCCESS;
}

static gearmand_error_t
_server_queue_work_data(gearman_server_job_st *server_job,
                        gearmand_packet_st *packet, gearman_command_t command)
{
  gearman_server_client_st *server_client;
  uint8_t *data;
  gearmand_error_t ret;

  for (server_client= server_job->client_list; server_client;
       server_client= server_client->job_next)
  {
    if (command == GEARMAN_COMMAND_WORK_EXCEPTION && !(server_client->con->is_exceptions))
    {
      continue;
    }

    if (packet->data_size > 0)
    {
      if (packet->options.free_data &&
          server_client->job_next == NULL)
      {
        data= (uint8_t *)(packet->data);
        packet->options.free_data= false;
      }
      else
      {
        data= (uint8_t *)malloc(packet->data_size);
        if (data == NULL)
        {
          gearmand_perror("malloc");
          return GEARMAN_MEMORY_ALLOCATION_FAILURE;
        }

        memcpy(data, packet->data, packet->data_size);
      }
    }
    else
    {
      data= NULL;
    }

    ret= gearman_server_io_packet_add(server_client->con, true,
                                      GEARMAN_MAGIC_RESPONSE, command,
                                      packet->arg[0], packet->arg_size[0],
                                      data, packet->data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("gearman_server_io_packet_add", ret);
      return ret;
    }
  }

  return GEARMAN_SUCCESS;
}
