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

gearman_worker_st *gearman_worker_create(gearman_st *gear, 
                                             gearman_worker_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    memset(ptr, 0, sizeof(gearman_worker_st));
    ptr->is_allocated= GEARMAN_NOT_ALLOCATED;
  }
  else
  {
    ptr= (gearman_worker_st *)malloc(sizeof(gearman_worker_st));

    if (ptr == NULL)
      return NULL;
    memset(ptr, 0, sizeof(gearman_worker_st));
    ptr->is_allocated= GEARMAN_ALLOCATED;
  }

  gearman_byte_array_create(gear, &ptr->client_id, 0);
  gearman_byte_array_create(gear, &ptr->function, 0);
  gearman_byte_array_create(gear, &ptr->handle, 0);

  ptr->server_key= -1;

  WATCHPOINT_ASSERT(ptr->client_id.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->client_id.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->function.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->function.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->handle.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->handle.is_allocated == GEARMAN_NOT_ALLOCATED);

  ptr->root= gearman_clone(NULL, gear);
  WATCHPOINT_ASSERT(ptr->root);
  WATCHPOINT_ASSERT(ptr->root->number_of_hosts == gear->number_of_hosts);

  return ptr;
}

gearman_return gearman_worker_answer(gearman_worker_st *ptr,
                                     gearman_result_st *result)
{
  gearman_return rc;
  gearman_server_st *server;
  giov_st giov[2];

  WATCHPOINT_ASSERT(ptr->server_key != -1);

  giov[0].arg= gearman_result_handle(result);
  giov[0].arg_length= strlen(gearman_result_handle(result));
  WATCHPOINT_ASSERT(giov[0].arg);

  giov[1].arg= gearman_byte_array_value(&(result->value));
  giov[1].arg_length= gearman_byte_array_length(&(result->value));

  server= &(ptr->root->hosts[ptr->server_key]);

  rc= gearman_dispatch(server, GEARMAN_WORK_COMPLETE, giov, 2); 
  gearman_byte_array_reset(&ptr->handle);

  return rc;
}

gearman_return gearman_worker_update(gearman_worker_st *ptr,
                                     uint32_t numerator,
                                     uint32_t denominator)
{
  gearman_return rc;
  gearman_server_st *server;
  giov_st giov[2];
  char buffer[TINY_STRING_LEN];
  int length; /* this is an int because of sprintf design */
  gearman_byte_array_st *handle;

  WATCHPOINT_ASSERT(ptr->server_key != -1);

  handle= &ptr->handle;

  giov[0].arg= gearman_byte_array_value(handle);
  giov[0].arg_length= gearman_byte_array_length(handle);

  length= sprintf(buffer, "%u", numerator);
  giov[1].arg= buffer;
  giov[1].arg_length= length;

  length= sprintf(buffer, "%u", denominator);
  giov[2].arg= buffer;
  giov[2].arg_length= length;

  server= &(ptr->root->hosts[ptr->server_key]);

  rc= gearman_dispatch(server, GEARMAN_WORK_STATUS, giov, 1); 

  return rc;
}

gearman_return gearman_worker_take(gearman_worker_st *ptr,
                                   gearman_result_st *result)
{
  gearman_return rc;
  gearman_server_st *server;

  WATCHPOINT_ASSERT(ptr->server_key != -1);

  server= &(ptr->root->hosts[ptr->server_key]);

  while (1)
  {
    rc= gearman_dispatch(server, GEARMAN_GRAB_JOB, NULL, 1); 
    WATCHPOINT_ERROR(rc);
    
    if (rc != GEARMAN_SUCCESS)
      return rc;

    rc= gearman_response(server, NULL, result);
    WATCHPOINT_ERROR(rc);
    if (rc == GEARMAN_SUCCESS)
    {
      WATCHPOINT;
      return rc;
    }
    else if (rc == GEARMAN_NOT_FOUND)
    {
      WATCHPOINT_STRING("Going to sleep");
      rc= gearman_dispatch(server, GEARMAN_PRE_SLEEP, NULL, 1); 
      if (rc != GEARMAN_SUCCESS)
        return rc;
      rc= gearman_response(server, NULL, result);
      WATCHPOINT_STRING("Waking up");
      if (rc != GEARMAN_SUCCESS)
        return rc;
    }
    else
      return rc;
  }
}

gearman_return gearman_worker_do(gearman_worker_st *ptr, 
                                 char *function)
{
  gearman_return rc;
  uint32_t server_key;
  gearman_server_st *server;
  size_t function_length;
  giov_st giov[2];

  function_length= strlen(function);
  gearman_byte_array_store(&ptr->function, function, function_length);

  server_key= find_server(ptr->root);
  ptr->server_key= server_key;
  server= &(ptr->root->hosts[ptr->server_key]);

  giov[0].arg= function;
  giov[0].arg_length= strlen(function);
  if (ptr->time_out)
  {
    char buffer[128];

    sprintf(buffer, "%" PRIu64, (uint64_t)ptr->time_out);
    giov[1].arg= buffer;
    giov[1].arg_length= strlen(buffer);

    rc= gearman_dispatch(server,  GEARMAN_CAN_DO_TIMEOUT, giov, 1);
  }
  else
    rc= gearman_dispatch(server,  GEARMAN_CAN_DO, giov, 1);

  WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);
  
  return rc;
}

/* Change to a behavior? */
void gearman_worker_set_timeout(gearman_worker_st *ptr, time_t timeout)
{
  ptr->time_out= timeout;
}

gearman_return gearman_worker_set_id(gearman_worker_st *ptr, const char *id)
{
  gearman_server_st *server;
  giov_st giov[1];

  /* We keep it around (this may go away) */
  gearman_byte_array_store(&(ptr->client_id), id, strlen(id));

  giov[0].arg= gearman_byte_array_value(&(ptr->client_id));
  giov[0].arg_length= gearman_byte_array_length(&(ptr->client_id));

  server= &(ptr->root->hosts[ptr->server_key]);

  return gearman_dispatch(server, GEARMAN_SET_CLIENT_ID, giov, 1);
}


void gearman_worker_reset(gearman_worker_st *ptr)
{
  gearman_return rc;
  gearman_server_st *server;

  WATCHPOINT_ASSERT(ptr->server_key != -1);

  server= &(ptr->root->hosts[ptr->server_key]);

  rc= gearman_dispatch(server,  GEARMAN_RESET_ABILITIES, NULL, 0);
  WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);

  gearman_byte_array_reset(&ptr->client_id);
  gearman_byte_array_reset(&ptr->function);
  gearman_byte_array_reset(&ptr->handle);
  ptr->server_key= -1;
}

void gearman_worker_free(gearman_worker_st *ptr)
{
  if (ptr == NULL)
    return;

  gearman_worker_reset(ptr);
  gearman_byte_array_free(&ptr->client_id);
  gearman_byte_array_free(&ptr->function);
  gearman_byte_array_free(&ptr->handle);

  gearman_free(ptr->root);

  if (ptr->is_allocated == GEARMAN_ALLOCATED)
    free(ptr);
  else
    ptr->is_allocated= GEARMAN_USED;
}


