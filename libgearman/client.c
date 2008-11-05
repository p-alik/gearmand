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

#include <stdbool.h>

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

/* Internal Internface to run a job */
static gearman_job_st *internal_gearman_client_do(gearman_client_st *client,
                                                  gearman_job_st *job, const char *function_name,
                                                  const uint8_t *workload, ssize_t workload_size,
                                                  gearman_return *ret,
                                                  bool is_background)
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
                             is_background ? GEARMAN_CLIENT_STATE_SUBMIT_JOB_BJ : GEARMAN_COMMAND_SUBMIT_JOB,
                             function_name, strlen(function_name) + 1,
                             job->uuid, (size_t)37,
                             workload, workload_size, NULL);
    if (*ret != GEARMAN_SUCCESS)
      return NULL;

    /* TODO check background logic */
  case GEARMAN_CLIENT_STATE_SUBMIT_JOB_BJ:
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

/* Run a job. */
gearman_job_st *gearman_client_do(gearman_client_st *client,
                                  gearman_job_st *job, const char *function_name,
                                  const uint8_t *workload, ssize_t workload_size,
                                  gearman_return *ret)
{
  return internal_gearman_client_do(client, job, function_name, workload, workload_size, ret, false);
}

/* Run a job in the background. */
char * gearman_client_do_background(gearman_client_st *client,
                                    const char *function_name,
                                    const uint8_t *workload, ssize_t workload_size,
                                    gearman_return *ret)
{
  char *job_reference;
  gearman_job_st *job; /* Should be switched to static */

  assert(0);
  job= internal_gearman_client_do(client, NULL, function_name, workload, workload_size, ret, true);

  if (*ret == GEARMAN_SUCCESS && job)
    job_reference= strdup(job->uuid);
  else
    job_reference= NULL;

  if (job)
    gearman_job_free(job);

  return job_reference;
}

gearman_return gearman_client_job_status(gearman_client_st *client,
                                         const char *job_id,
                                         bool *is_known,
                                         bool *is_running,
                                         long *numerator,
                                         long *denominator)
{
  assert(0);
  (void)client;
  (void) job_id;
  (void) is_known;
  (void) is_running;
  (void) numerator;
  (void) denominator;
}

/* Make a copy of a gearman_client_st */
gearman_client_st *gearman_client_clone(gearman_client_st *client)
{
  gearman_client_st *clone;
  gearman_return rc= GEARMAN_SUCCESS;

  clone= gearman_client_create(NULL);

  if (clone == NULL)
    return NULL;

  /* If client was null, then we just return NULL */
  if (client == NULL)
    return gearman_client_create(NULL);

  /* Put code here to copy internal connection structures */
  assert(0);

  if (rc != GEARMAN_SUCCESS)
  {
    gearman_client_free(clone);

    return NULL;
  }

  return clone;
}


gearman_return gearman_client_echo(gearman_client_st *client,
                                   const char *message,
                                   ssize_t message_length)
{
  gearman_return rc;

  (void)client;
  (void)message;
  (void)message_length;

  /* 
    Send a GEARMAN_ECHO_REQ plus a message to a server (the first?) and
    see if it gets returned.
  */
  assert(0);

  return rc;
}
