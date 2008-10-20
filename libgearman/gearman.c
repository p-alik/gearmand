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

gearman_st *gearman_create(gearman_st *ptr)
{
  gearman_result_st *result_ptr;

  if (ptr == NULL)
  {
    ptr= (gearman_st *)malloc(sizeof(gearman_st));

    if (!ptr)
      return NULL; /*  GEARMAN_MEMORY_ALLOCATION_FAILURE */

    memset(ptr, 0, sizeof(gearman_st));
    ptr->is_allocated= GEARMAN_ALLOCATED;
  }
  else
  {
    memset(ptr, 0, sizeof(gearman_st));
  }
  result_ptr= gearman_result_create(ptr, &ptr->result);
  WATCHPOINT_ASSERT(result_ptr);
  ptr->poll_timeout= GEARMAN_DEFAULT_TIMEOUT;
  ptr->connect_timeout= GEARMAN_DEFAULT_TIMEOUT;
  ptr->retry_timeout= 0;
  ptr->distribution= GEARMAN_DISTRIBUTION_MODULA;

  return ptr;
}

void gearman_free(gearman_st *ptr)
{
  /* If we have anything open, lets close it now */
  gearman_quit(ptr);
  server_list_free(ptr, ptr->hosts);
  gearman_result_free(&ptr->result);

  if (ptr->is_allocated == GEARMAN_ALLOCATED)
      free(ptr);
  else
    ptr->is_allocated= GEARMAN_USED;
}

/*
  clone is the destination, while ptr is the structure to clone.
  If ptr is NULL the call is the same as if a gearman_create() was
  called.
*/
gearman_st *gearman_clone(gearman_st *clone, gearman_st *ptr)
{
  gearman_return rc= GEARMAN_SUCCESS;
  gearman_st *new_clone;

  if (ptr == NULL)
    return gearman_create(clone);

  if (ptr->is_allocated == GEARMAN_USED)
  {
    WATCHPOINT_ASSERT(0);
    return NULL;
  }
  
  new_clone= gearman_create(clone);
  
  if (new_clone == NULL)
    return NULL;

  if (ptr->hosts)
    rc= gearman_server_push(new_clone, ptr);

  if (rc != GEARMAN_SUCCESS)
  {
    gearman_free(new_clone);

    return NULL;
  }


  new_clone->flags= ptr->flags;
  new_clone->send_size= ptr->send_size;
  new_clone->recv_size= ptr->recv_size;
  new_clone->poll_timeout= ptr->poll_timeout;
  new_clone->connect_timeout= ptr->connect_timeout;
  new_clone->retry_timeout= ptr->retry_timeout;
  new_clone->distribution= ptr->distribution;
  new_clone->hash= ptr->hash;
  new_clone->user_data= ptr->user_data;

  return new_clone;
}
