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

/* Free a client structure. */
void gearman_client_free(gearman_client_st *client)
{
  if (client->options & GEARMAN_CLIENT_ALLOCATED)
    free(client);
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
    return GEARMAN_ERRNO;

  return GEARMAN_SUCCESS;
}

/* Run a job. */
gearman_job_st *gearman_client_do(gearman_client_st *client,
                                  gearman_job_st *job, char *function_name,
                                  uint8_t *workload, size_t workload_size,
                                  gearman_return *ret)
{
  uuid_t uuid;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_INIT:
    if (client->gearman.con_list == NULL)
    {
      *ret= GEARMAN_NO_SERVERS;
      return NULL;
    }

    client->job= gearman_job_create(&(client->gearman), job);
    if (client->job == NULL)
    {
      *ret= GEARMAN_ERRNO;
      return NULL;
    }

    uuid_generate(uuid);
    uuid_unparse(uuid, client->job->uuid);
    job->con= client->gearman.con_list;

    *ret= gearman_packet_add(&(client->gearman), &(job->packet),
                             GEARMAN_MAGIC_REQUEST,
                             GEARMAN_COMMAND_SUBMIT_JOB,
                             function_name, strlen(function_name) + 1,
                             job->uuid, (size_t)37,
                             workload, workload_size, NULL);
    if (*ret != GEARMAN_SUCCESS)
      return NULL;

  case GEARMAN_CLIENT_STATE_SUBMIT_JOB:
    *ret= gearman_con_send(job->con, &(client->job->packet));
    if (*ret != GEARMAN_SUCCESS)
    {
      if (*ret == GEARMAN_IO_WAIT)
        client->state= GEARMAN_CLIENT_STATE_SUBMIT_JOB;

      return NULL;
    }

    gearman_packet_free(&(job->packet));

  case GEARMAN_CLIENT_STATE_JOB_CREATED:
    gearman_con_recv(job->con, &(client->job->packet), ret);
    if (*ret != GEARMAN_SUCCESS)
    {
      if (*ret == GEARMAN_IO_WAIT)
        client->state= GEARMAN_CLIENT_STATE_JOB_CREATED;

      return NULL;
    }

  case GEARMAN_CLIENT_STATE_RESULT:
    gearman_con_recv(job->con, &(client->job->result), ret);
    if (*ret != GEARMAN_SUCCESS)
    {
      if (*ret == GEARMAN_IO_WAIT)
        client->state= GEARMAN_CLIENT_STATE_RESULT;

      return NULL;
    }
  }

  job= client->job;
  client->job= NULL;

  return job;
}

#if 0
char *gearman_client_do_background(gearman_client_st *client,
                                   const char *function_name,
                                   const uint8_t *workload, ssize_t workload_size,
                                   gearman_return *error)
{
  giov_st giov[1];
  gearman_server_st *server;
  gearman_result_st result;
  uuid_t uu;
  char uuid_string[37];

  if (client->list.number_of_hosts == 0)
  {
    *error= GEARMAN_FAILURE;
    return NULL;
  }

  server= &(client->list.hosts[0]);
  assert(server);

  if (gearman_result_create(&result) == NULL)
  {
    *error= GEARMAN_SUCCESS;
    return NULL;
  }

  uuid_generate(uu);
  uuid_unparse(uu, uuid_string);

  giov[0].arg= (const void *)function_name;
  giov[0].arg_length= strlen(function_name);
  giov[1].arg= uuid_string;
  giov[1].arg_length= 37;
  giov[2].arg= (const void *)workload;
  giov[2].arg_length= workload_size;
  *error= gearman_dispatch(server, GEARMAN_SUBMIT_JOB_BJ, giov, true);

  if (*error != GEARMAN_SUCCESS)
    return NULL;

  WATCHPOINT;
  *error= gearman_response(server, &result);
  WATCHPOINT;

  WATCHPOINT_STRING_LENGTH(result.value->value, result.value->length);
  gearman_result_free(&result);

  if (*error == GEARMAN_SUCCESS)
    return strdup(uuid_string);
  else
    return NULL;
}

gearman_return gearman_client_job_status(gearman_client_st *client,
                                         const char *job_id,
                                         bool *is_known,
                                         bool *is_running,
                                         long *numerator,
                                         long *denominator)
{
  gearman_return rc;
  giov_st giov[1];
  gearman_server_st *server;
  gearman_result_st result;

  if (gearman_result_create(&result) == NULL)
    return GEARMAN_FAILURE;

  if (client->list.number_of_hosts == 0)
    return GEARMAN_FAILURE;

  server= &(client->list.hosts[0]);
  assert(server);

  giov[0].arg= (const void *)job_id;
  giov[0].arg_length= strlen(job_id);
  rc= gearman_dispatch(server, GEARMAN_GET_STATUS, giov, true);
  assert(rc == GEARMAN_SUCCESS);

  rc= gearman_response(server, &result);

  assert(rc == GEARMAN_SUCCESS);
  *is_known= result.is_known;
  *is_running= result.is_running;
  *numerator= result.numerator;
  *denominator= result.denominator;

  gearman_result_free(&result);

  return rc;
}

gearman_return gearman_client_echo(gearman_client_st *client,
                                   const char *message,
                                   ssize_t message_length)
{
  gearman_return rc;
  giov_st giov[1];
  gearman_server_st *server;
  gearman_result_st result;

  if (gearman_result_create(&result) == NULL)
    return GEARMAN_FAILURE;

  if (client->list.number_of_hosts == 0)
    return GEARMAN_FAILURE;

  server= &(client->list.hosts[0]);
  assert(server);

  giov[0].arg= (const void *)message;
  giov[0].arg_length= message_length;
  rc= gearman_dispatch(server, GEARMAN_ECHO_REQ, giov, true);
  assert(rc == GEARMAN_SUCCESS);

  rc= gearman_response(server, &result);

  if (message_length == gearman_result_value_length(&result) 
      && memcmp(message, gearman_result_value(&result, NULL), gearman_result_value_length(&result)) == 0)
    rc= GEARMAN_SUCCESS;
  else
    rc= GEARMAN_FAILURE;

  gearman_result_free(&result);

  return rc;
}
#endif
