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

#include <uuid/uuid.h>
#include <libgearman/gearman_client.h>
#include "common.h"

gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  if (client == NULL)
  {
    client= (gearman_client_st *)malloc(sizeof(gearman_client_st));

    if (!client)
      return NULL; /*  GEARMAN_MEMORY_ALLOCATION_FAILURE */

    memset(client, 0, sizeof(gearman_client_st));
    client->is_allocated= true;
  }
  else
  {
    memset(client, 0, sizeof(gearman_client_st));
  }

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  /* If we have anything open, lets close it now */
  gearman_server_list_free(&client->list);

  if (client->is_allocated == true)
      free(client);
  else
    memset(client, 0, sizeof(client));
}

/*
  clone is the destination, while client is the structure to clone.
  If client is NULL the call is the same as if a gearman_client_create() was
  called.
*/
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

  rc= gearman_server_copy(&clone->list, &client->list);

  if (rc != GEARMAN_SUCCESS)
  {
    gearman_client_free(clone);

    return NULL;
  }

  return clone;
}

gearman_return gearman_client_server_add(gearman_client_st *client,
                                         const char *hostname,
                                         uint16_t port)
{
  return gearman_server_add(&client->list, hostname, port);
}

uint8_t *gearman_client_do(gearman_client_st *client __attribute__((unused)),
                           const char *function_name __attribute__((unused)), 
                           const uint8_t *workload __attribute__((unused)), ssize_t workload_size __attribute__((unused)), 
                           ssize_t *result_length,  gearman_return *error)
{
  *error= GEARMAN_SUCCESS;
  *result_length= 0;

  return NULL;
}

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

 // WATCHPOINT_STRING_LENGTH(result.handle->value, result.handle->length);
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
