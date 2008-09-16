/* 
  gearman_job_st are used to represent jobs we are sending to the server.
*/
#include "common.h"

gearman_job_st *gearman_job_create(gearman_st *gear, 
                                             gearman_job_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    memset(ptr, 0, sizeof(gearman_job_st));
    ptr->is_allocated= GEARMAN_NOT_ALLOCATED;
  }
  else
  {
    ptr= (gearman_job_st *)malloc(sizeof(gearman_job_st));

    if (ptr == NULL)
      return NULL;
    memset(ptr, 0, sizeof(gearman_job_st));
    ptr->is_allocated= GEARMAN_ALLOCATED;
  }

  ptr->root= gearman_clone(NULL, gear);
  WATCHPOINT_ASSERT(ptr->root);
  WATCHPOINT_ASSERT(ptr->root->number_of_hosts == gear->number_of_hosts);

  gearman_byte_array_create(gear, &ptr->value, 0);
  gearman_byte_array_create(gear, &ptr->unique, 0);
  gearman_byte_array_create(gear, &ptr->function, 0);
  gearman_byte_array_create(gear, &ptr->handle, 0);

  WATCHPOINT_ASSERT(ptr->value.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->value.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->unique.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->unique.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->function.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->function.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->handle.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->handle.is_allocated == GEARMAN_NOT_ALLOCATED);

  return ptr;
}

void gearman_job_reset(gearman_job_st *ptr)
{
  gearman_byte_array_reset(&ptr->value);
  gearman_byte_array_reset(&ptr->function);
  gearman_byte_array_reset(&ptr->handle);
  gearman_byte_array_reset(&ptr->unique);
}

gearman_return gearman_job_result(gearman_job_st *ptr, 
                                  gearman_result_st *result)
{
  gearman_return rc;
  gearman_server_st *server;
  gearman_byte_array_st *handle;
  giov_st giov[1];

  handle= &ptr->handle;

  WATCHPOINT_STRING("Active Cursor");
  WATCHPOINT_NUMBER(ptr->cursor);
  server= &(ptr->root->hosts[ptr->cursor]);

  giov[0].arg= gearman_byte_array_value(handle);
  giov[0].arg_length= gearman_byte_array_length(handle);
  rc= gearman_dispatch(server, GEARMAN_GET_STATUS, giov, 1);

  if (rc != GEARMAN_SUCCESS)
  {
    /* Possibly shutdown network connection? */
    return GEARMAN_FAILURE;
  }

  return gearman_response(server, handle, result);
}

gearman_return gearman_job_submit(gearman_job_st *ptr)
{
  gearman_return rc;
  gearman_server_st *server;
  giov_st giov[3];

  giov[0].arg= gearman_byte_array_value(&(ptr->function));
  giov[0].arg_length= gearman_byte_array_length(&(ptr->function));

  giov[1].arg= NULL;
  giov[1].arg_length= 0;

  giov[2].arg= gearman_byte_array_value(&(ptr->value));
  giov[2].arg_length= gearman_byte_array_length(&(ptr->value));

  ptr->cursor= 0;
  while (1)
  {
    if (ptr->cursor >= ptr->root->number_of_hosts)
    {
      ptr->cursor= 0;
      break;
    }

    WATCHPOINT_NUMBER(ptr->cursor);
    server= &(ptr->root->hosts[ptr->cursor]);

    rc= gearman_dispatch(server,  
                         ptr->flags & GEARMAN_BEHAVIOR_JOB_BACKGROUND ? GEARMAN_SUBMIT_JOB_BJ : GEARMAN_SUBMIT_JOB, 
                         giov,
                         1); 
    switch (rc)
    {
    case GEARMAN_SUCCESS:
      WATCHPOINT_STRING("Good Cursor");
      WATCHPOINT_NUMBER(ptr->cursor);
      rc= gearman_response(server, &(ptr->handle), NULL);
      return rc;
    case GEARMAN_ERRNO:
      ptr->cursor++;
      continue;
    default:
      return rc;
    }
  }

  return GEARMAN_FAILURE;
}

/*
  NOTE turn into macro
*/
gearman_return gearman_job_set_value(gearman_job_st *ptr, char *value, size_t length)
{
  gearman_byte_array_reset(&ptr->value);
  return gearman_byte_array_append(&ptr->value, value, length);
}

gearman_return gearman_job_set_function(gearman_job_st *ptr, char *function)
{
  size_t function_length= strlen(function);

  gearman_byte_array_reset(&ptr->function);

  return gearman_byte_array_append(&ptr->function, function, function_length);
}

/*
  Behavior modification.
*/
static void set_behavior_flag(gearman_job_st *ptr, gearman_job_behavior flag, uint64_t data)
{
  if (data)
    ptr->flags|= flag;
  else
    ptr->flags&= ~flag;
}

gearman_return gearman_job_set_behavior(gearman_job_st *ptr, 
                                        gearman_job_behavior flag, 
                                        uint64_t data)
{
  switch (flag)
  {
  case GEARMAN_BEHAVIOR_JOB_BACKGROUND:
    {
      set_behavior_flag(ptr, flag, data);
      break;
    }
  default:
    return GEARMAN_FAILURE;
  };

  return GEARMAN_SUCCESS;
}

uint64_t gearman_job_get_behavior(gearman_st *ptr, 
                                  gearman_job_behavior flag)
{
  /* This needs to be expanded with a switch at some point */
  if (ptr->flags & flag)
    return 1;
  else
    return 0;

  return GEARMAN_SUCCESS;

}

void gearman_job_free(gearman_job_st *ptr)
{
  if (ptr == NULL)
    return;

  gearman_byte_array_free(&ptr->value);
  gearman_byte_array_free(&ptr->unique);
  gearman_byte_array_free(&ptr->function);
  gearman_byte_array_free(&ptr->handle);
  gearman_free(ptr->root);

  if (ptr->is_allocated == GEARMAN_ALLOCATED)
    free(ptr);
  else
    ptr->is_allocated= GEARMAN_USED;
}
