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

gearman_byte_array_st *gearman_byte_array_create(ssize_t initial_size)
{
  gearman_byte_array_st *array;

  array= (gearman_byte_array_st *)malloc(sizeof(gearman_byte_array_st) + initial_size);

  if (array)
    array->length= initial_size;

  return array;
}

/*
  We always add a null terminator.
*/
uint8_t *gearman_byte_array_c_copy(gearman_byte_array_st *byte_array, ssize_t *length)
{
  uint8_t *c_ptr;

  if (byte_array->length == 0)
    return NULL;

  c_ptr= (uint8_t *)malloc(byte_array->length * sizeof(uint8_t) + 1);

  if (c_ptr == NULL)
    return NULL;

  memcpy(c_ptr, byte_array->value, byte_array->length);
  *length= byte_array->length;
  c_ptr[*length]= 0;

  return c_ptr;
}

void gearman_byte_array_free(gearman_byte_array_st *ptr)
{
  if (ptr == NULL)
    return;

  free(ptr);
}

gearman_return gearman_byte_array_store(gearman_byte_array_st *byte_array,
                                        const void *value, ssize_t length)
{
  gearman_byte_array_st *new_ptr;

  if (length > byte_array->length)
  {
    new_ptr= realloc(byte_array, length);

    if (new_ptr == NULL)
      return  GEARMAN_MEMORY_ALLOCATION_FAILURE;
    byte_array= new_ptr;
  }

  byte_array->length= length;
  if (memcpy(byte_array->value, value, length) == NULL)
    return  GEARMAN_MEMORY_ALLOCATION_FAILURE;
  else 
    return GEARMAN_SUCCESS;
}
