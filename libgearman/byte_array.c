#include "common.h"

gearman_return gearman_byte_array_check(gearman_byte_array_st *byte_array, size_t need)
{
  if (need && need > (size_t)(byte_array->current_size - (size_t)(byte_array->end - byte_array->byte_array)))
  {
    size_t current_offset= byte_array->end - byte_array->byte_array;
    char *new_value;
    size_t adjust;
    size_t new_size;

    /* This is the block multiplier. To keep it larger and surive division errors we must round it up */
    adjust= (need - (size_t)(byte_array->current_size - (size_t)(byte_array->end - byte_array->byte_array))) / byte_array->block_size;
    adjust++;

    new_size= sizeof(char) * (size_t)((adjust * byte_array->block_size) + byte_array->current_size);
    /* Test for overflow */
    if (new_size < need)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    new_value= (char *)realloc(byte_array->byte_array, new_size);

    if (new_value == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    byte_array->byte_array= new_value;
    byte_array->end= byte_array->byte_array + current_offset;

    byte_array->current_size+= (byte_array->block_size * adjust);
  }

  return GEARMAN_SUCCESS;
}

gearman_byte_array_st *gearman_byte_array_create(gearman_st *ptr, gearman_byte_array_st *byte_array, size_t initial_size)
{
  gearman_return rc;

  /* Saving malloc calls :) */
  if (byte_array)
  {
    memset(byte_array, 0, sizeof(gearman_byte_array_st));
    byte_array->is_allocated= GEARMAN_NOT_ALLOCATED;
  }
  else
  {
    byte_array= (gearman_byte_array_st *)malloc(sizeof(gearman_byte_array_st));

    if (byte_array == NULL)
      return NULL;
    memset(byte_array, 0, sizeof(gearman_byte_array_st));
    byte_array->is_allocated= GEARMAN_ALLOCATED;
  }
  byte_array->block_size= GEARMAN_BLOCK_SIZE;
  byte_array->root= ptr;

  rc=  gearman_byte_array_check(byte_array, initial_size);
  if (rc != GEARMAN_SUCCESS)
  {
    free(byte_array);

    return NULL;
  }

  WATCHPOINT_ASSERT(byte_array->byte_array == byte_array->end);

  return byte_array;
}

gearman_return gearman_byte_array_append_character(gearman_byte_array_st *byte_array, 
                                                   char character)
{
  gearman_return rc;

  WATCHPOINT_ASSERT(byte_array->is_allocated != GEARMAN_USED);

  rc=  gearman_byte_array_check(byte_array, 1);

  if (rc != GEARMAN_SUCCESS)
    return rc;

  *byte_array->end= ' ';
  byte_array->end++;

  return GEARMAN_SUCCESS;
}

gearman_return gearman_byte_array_append(gearman_byte_array_st *byte_array,
                                         const char *value, size_t length)
{
  gearman_return rc;

  if (length == 0)
    return GEARMAN_SUCCESS;
  WATCHPOINT_ASSERT(byte_array->is_allocated != GEARMAN_USED);

  rc= gearman_byte_array_check(byte_array, length);

  if (rc != GEARMAN_SUCCESS)
    return rc;

  WATCHPOINT_ASSERT(length <= byte_array->current_size);
  WATCHPOINT_ASSERT(byte_array->byte_array);
  WATCHPOINT_ASSERT(byte_array->end >= byte_array->byte_array);

  memcpy(byte_array->end, value, length);
  byte_array->end+= length;

  return GEARMAN_SUCCESS;
}

size_t gearman_byte_array_backspace(gearman_byte_array_st *byte_array, size_t remove)
{
  WATCHPOINT_ASSERT(byte_array->is_allocated != GEARMAN_USED);

  if (byte_array->end - byte_array->byte_array  > remove)
  {
    size_t difference;

    difference= byte_array->end - byte_array->byte_array;
    byte_array->end= byte_array->byte_array;

    return difference;
  }
  byte_array->end-= remove;

  return remove;
}

char *gearman_byte_array_c_copy(gearman_byte_array_st *byte_array)
{
  char *c_ptr;

  WATCHPOINT_ASSERT(byte_array->is_allocated != GEARMAN_USED);

  if (gearman_byte_array_length(byte_array) == 0)
    return NULL;

  c_ptr= (char *)malloc((gearman_byte_array_length(byte_array)+1) * sizeof(char));

  if (c_ptr == NULL)
    return NULL;

  memcpy(c_ptr, gearman_byte_array_value(byte_array), gearman_byte_array_length(byte_array));
  c_ptr[gearman_byte_array_length(byte_array)]= 0;

  return c_ptr;
}

gearman_return gearman_byte_array_reset(gearman_byte_array_st *byte_array)
{
  WATCHPOINT_ASSERT(byte_array->is_allocated != GEARMAN_USED);
  byte_array->end= byte_array->byte_array;

  return GEARMAN_SUCCESS;
}

void gearman_byte_array_free(gearman_byte_array_st *ptr)
{
  if (ptr == NULL)
    return;

  if (ptr->byte_array)
    free(ptr->byte_array);

  if (ptr->is_allocated == GEARMAN_ALLOCATED)
    free(ptr);
  else
    ptr->is_allocated= GEARMAN_USED;
}


gearman_return gearman_byte_array_store(gearman_byte_array_st *byte_array,
                                         const char *value, size_t length)
{
  gearman_return rc;

  (void)gearman_byte_array_reset(byte_array);
  rc= gearman_byte_array_append(byte_array, value, length);

  return rc;
}
