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

gearman_result_st *gearman_result_create(gearman_result_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    memset(ptr, 0, sizeof(gearman_result_st));
    ptr->is_allocated= false;
  }
  else
  {
    ptr= (gearman_result_st *)malloc(sizeof(gearman_result_st));

    if (ptr == NULL)
      return NULL;
    memset(ptr, 0, sizeof(gearman_result_st));
    ptr->is_allocated= true;
  }

  return ptr;
}

/*
  NOTE turn into macro
*/
gearman_return gearman_result_value_store(gearman_result_st *ptr, const char *value, ssize_t length)
{
  if (ptr->value == NULL)
    ptr->value= gearman_byte_array_create(length);

  return gearman_byte_array_store(ptr->value, value, length);
}

gearman_return gearman_result_handle_store(gearman_result_st *ptr, const char *handle, ssize_t length)
{
  if (ptr->handle == NULL)
    ptr->handle= gearman_byte_array_create(length);

  return gearman_byte_array_store(ptr->handle, handle, length);
}

void gearman_result_free(gearman_result_st *ptr)
{
  if (ptr == NULL)
    return;

  gearman_byte_array_free(ptr->value);
  gearman_byte_array_free(ptr->handle);

  if (ptr->is_allocated == true)
    free(ptr);
}

uint8_t *gearman_result_value(gearman_result_st *result, ssize_t *length)
{
  if (length)
    *length= result->value->length;
  return result->value->value;
}

ssize_t gearman_result_value_length(gearman_result_st *result)
{
  return result->value->length;
}

uint8_t *gearman_result_handle(gearman_result_st *result, ssize_t *length)
{
  if (length)
    *length= result->handle->length;
  return result->handle->value;
}

ssize_t gearman_result_handle_length(gearman_result_st *result)
{
  return result->handle->length;
}
