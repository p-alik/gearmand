/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

/* Internal interface to run a job. */
static gearman_job_st *_client_do(gearman_client_st *client,
                                  gearman_job_st *job,
                                  gearman_command command,
                                  const char *function_name,
                                  const uint8_t *workload,
                                  size_t workload_size,
                                  gearman_return *ret_ptr);

/* Initialize a client structure. */
gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  if (client == NULL)
  {
    client= malloc(sizeof(gearman_client_st));
    if (client == NULL)
      return NULL;

    memset(client, 0, sizeof(gearman_client_st));
    client->options|= GEARMAN_CLIENT_ALLOCATED;
  }
  else
    memset(client, 0, sizeof(gearman_client_st));

  (void)gearman_create(&(client->gearman));

  return client;
}

/* Clone a client structure using 'from' as the source. */
gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        gearman_client_st *from)
{
  client= gearman_client_create(client);
  if (client == NULL)
    return NULL;

  client->options|= (from->options & ~GEARMAN_CLIENT_ALLOCATED);

  if (gearman_clone(&(client->gearman), &(from->gearman)) == NULL)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

/* Free a client structure. */
void gearman_client_free(gearman_client_st *client)
{
  gearman_client_reset(client);

  if (client->options & GEARMAN_CLIENT_ALLOCATED)
    free(client);
}

/* Reset state for a client structure. */
void gearman_client_reset(gearman_client_st *client)
{
  client->state= GEARMAN_CLIENT_STATE_IDLE;

  if (client->job != NULL)
  {
    gearman_job_free(client->job);
    client->job= NULL;
  }
}

/* Return an error string for last library error encountered. */
char *gearman_client_error(gearman_client_st *client)
{
  return gearman_error(&(client->gearman));
}

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_client_errno(gearman_client_st *client)
{
  return gearman_errno(&(client->gearman));
}

/* Set options for a library instance structure. */
void gearman_client_set_options(gearman_client_st *client,
                                gearman_options options,
                                uint32_t data)
{
  gearman_set_options(&(client->gearman), options, data);
}

/* Add a job server to a client. */
gearman_return gearman_client_server_add(gearman_client_st *client, char *host,
                                         in_port_t port)
{
  if (gearman_con_add(&(client->gearman), NULL, host, port) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  return GEARMAN_SUCCESS;
}

/* Run a job. */
gearman_job_st *gearman_client_do(gearman_client_st *client,
                                  gearman_job_st *job,
                                  const char *function_name,
                                  const uint8_t *workload,
                                  size_t workload_size,
                                  gearman_return *ret_ptr)
{
  return _client_do(client, job, GEARMAN_COMMAND_SUBMIT_JOB, function_name,
                    workload, workload_size, ret_ptr);
}

/* Run a high priority job. */
gearman_job_st *gearman_client_do_high(gearman_client_st *client,
                                       gearman_job_st *job,
                                       const char *function_name,
                                       const uint8_t *workload, 
                                       size_t workload_size, 
                                       gearman_return *ret_ptr)
{
  return _client_do(client, job, GEARMAN_COMMAND_SUBMIT_JOB_HIGH, function_name,
                    workload, workload_size, ret_ptr);
}

/* Run a job in the background. */
gearman_job_st *gearman_client_do_background(gearman_client_st *client,
                                             gearman_job_st *job,
                                             const char *function_name,
                                             const uint8_t *workload,
                                             size_t workload_size,
                                             gearman_return *ret_ptr)
{
  return _client_do(client, job, GEARMAN_COMMAND_SUBMIT_JOB_BG, function_name,
                    workload, workload_size, ret_ptr);
}

/* Get the job status for a job_handle. */
gearman_return gearman_client_job_status(gearman_client_st *client,
                                         const char *job_handle,  
                                         bool *is_known, 
                                         bool *is_running,
                                         long *numerator,  
                                         long *denominator)
{
  assert(0);
  (void) client;
  (void) job_handle;
  (void) is_known;
  (void) is_running;
  (void) numerator;
  (void) denominator;
}

/* Send a message to all servers and see if they return it. */
gearman_return gearman_client_echo(gearman_client_st *client,
                                   const uint8_t *message,  
                                   size_t message_size)
{
  gearman_return rc;

  (void)client;
  (void)message;
  (void)message_size;

  /* 
    Send a GEARMAN_ECHO_REQ plus a message to a server (the first?) and
    see if it gets returned.
  */
  assert(0);

  return rc;
}

/* Internal interface to run a job. */
static gearman_job_st *_client_do(gearman_client_st *client,
                                  gearman_job_st *job,
                                  gearman_command command,
                                  const char *function_name,
                                  const uint8_t *workload,
                                  size_t workload_size,
                                  gearman_return *ret_ptr)
{
  uuid_t uuid;
  char status_buffer[11]; /* Max string size to hold a uint32_t. */

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    if (client->gearman.con_list == NULL)
    {
      *ret_ptr= GEARMAN_NO_SERVERS;
      return NULL;
    }

    /* Only allow one job at a time for now. */
    if (client->job != NULL)
    {
      *ret_ptr= GEARMAN_JOB_EXISTS;
      return NULL;
    }

    client->job= gearman_job_create(&(client->gearman), job);
    if (client->job == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    uuid_generate(uuid);
    uuid_unparse(uuid, client->job->uuid);
    client->job->con= client->gearman.con_list;

    *ret_ptr= gearman_packet_add(&(client->gearman), &(client->job->packet),
                             GEARMAN_MAGIC_REQUEST, command,
                             (uint8_t *)function_name,
                             strlen(function_name) + 1,
                             (uint8_t *)client->job->uuid, (size_t)37,
                             workload, workload_size, NULL);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      gearman_client_reset(client);
      return NULL;
    }

  case GEARMAN_CLIENT_STATE_SUBMIT_JOB:
    *ret_ptr= gearman_con_send(client->job->con, &(client->job->packet));
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      if (*ret_ptr == GEARMAN_IO_WAIT)
        client->state= GEARMAN_CLIENT_STATE_SUBMIT_JOB;
      else
        gearman_client_reset(client);

      return client->job;
    }

    gearman_packet_free(&(client->job->packet));

  case GEARMAN_CLIENT_STATE_JOB_CREATED:
    gearman_con_recv(client->job->con, &(client->job->packet), ret_ptr);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      if (*ret_ptr == GEARMAN_IO_WAIT)
        client->state= GEARMAN_CLIENT_STATE_JOB_CREATED;
      else
        gearman_client_reset(client);

      return client->job;
    }

    if (job->packet.command == GEARMAN_COMMAND_ERROR)
    {
      *ret_ptr= GEARMAN_WORK_ERROR;
      break;
    }
    else if (command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
      break;

  case GEARMAN_CLIENT_STATE_RESULT:
    while (1)
    {
      gearman_con_recv(client->job->con, &(client->job->result), ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        if (*ret_ptr == GEARMAN_IO_WAIT)
          client->state= GEARMAN_CLIENT_STATE_RESULT;
        else
          gearman_client_reset(client);

        return client->job;
      }

      if (job->result.command == GEARMAN_COMMAND_WORK_COMPLETE)
        break;
      else if (job->result.command == GEARMAN_COMMAND_ERROR)
      {
        *ret_ptr= GEARMAN_WORK_ERROR;
        break;
      }
      else if (job->result.command == GEARMAN_COMMAND_WORK_FAIL)
      {
        gearman_client_reset(client);
        *ret_ptr= GEARMAN_WORK_FAIL;
        return NULL;
      }
      else if (job->result.command == GEARMAN_COMMAND_WORK_STATUS)
      {
        /* Parse out status. */
        client->job->numerator= atoi((char *)client->job->result.arg[1]);
        strncpy(status_buffer, (char *)client->job->result.arg[2], 11);
        client->job->denominator= atoi(status_buffer);
        gearman_packet_free(&(client->job->result));

        *ret_ptr= GEARMAN_WORK_STATUS;
        client->state= GEARMAN_CLIENT_STATE_RESULT;
        return client->job;
      }
    }
  }

  job= client->job;
  client->job= NULL;
  gearman_client_reset(client);

  return job;
}
