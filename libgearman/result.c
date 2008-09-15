/* 
  gearman_result_st are used to internally represent the return values from
  gearman. We use a structure so that long term as identifiers are added 
  to gearman we will be able to absorb new attributes without having 
  to addjust the entire API.
*/
#include "common.h"

gearman_result_st *gearman_result_create(gearman_st *gear_con, 
                                         gearman_result_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    memset(ptr, 0, sizeof(gearman_result_st));
    ptr->is_allocated= GEARMAN_NOT_ALLOCATED;
  }
  else
  {
    ptr= (gearman_result_st *)malloc(sizeof(gearman_result_st));

    if (ptr == NULL)
      return NULL;
    memset(ptr, 0, sizeof(gearman_result_st));
    ptr->is_allocated= GEARMAN_ALLOCATED;
  }

  ptr->root= gear_con;
  gearman_byte_array_create(gear_con, &ptr->handle, 0);
  gearman_byte_array_create(gear_con, &ptr->value, 0);
  WATCHPOINT_ASSERT(ptr->value.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->value.is_allocated == GEARMAN_NOT_ALLOCATED);
  WATCHPOINT_ASSERT(ptr->handle.byte_array == NULL);
  WATCHPOINT_ASSERT(ptr->handle.is_allocated == GEARMAN_NOT_ALLOCATED);

  return ptr;
}

void gearman_result_reset(gearman_result_st *ptr)
{
  gearman_byte_array_reset(&ptr->value);
  gearman_byte_array_reset(&ptr->handle);
  ptr->numerator= 0;
  ptr->denominator= 0;
}

/*
  NOTE turn into macro
*/
gearman_return gearman_result_set_value(gearman_result_st *ptr, char *value, size_t length)
{
  return gearman_byte_array_store(&ptr->value, value, length);
}

gearman_return gearman_result_set_handle(gearman_result_st *ptr, char *handle, size_t length)
{
  return gearman_byte_array_store(&ptr->handle, handle, length);
}

void gearman_result_free(gearman_result_st *ptr)
{
  if (ptr == NULL)
    return;

  gearman_byte_array_free(&ptr->value);
  gearman_byte_array_free(&ptr->handle);

  if (ptr->is_allocated == GEARMAN_ALLOCATED)
    free(ptr);
  else
    ptr->is_allocated= GEARMAN_USED;
}

char *gearman_result_value(gearman_result_st *ptr)
{
  gearman_byte_array_st *sptr= &ptr->value;
  return gearman_byte_array_value(sptr);
}

size_t gearman_result_length(gearman_result_st *ptr)
{
  gearman_byte_array_st *sptr= &ptr->value;
  return gearman_byte_array_length(sptr);
}

char *gearman_result_handle(gearman_result_st *ptr)
{
  gearman_byte_array_st *sptr= &ptr->handle;
  return gearman_byte_array_value(sptr);
}

size_t gearman_result_handle_length(gearman_result_st *ptr)
{
  gearman_byte_array_st *sptr= &ptr->handle;
  return gearman_byte_array_length(sptr);
}
