/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "server_common.h"

gearman_connection_st *gearman_connection_create(gearman_connection_st *ptr)
{
  if (ptr == NULL)
  {
    ptr= (gearman_connection_st *)malloc(sizeof(gearman_connection_st));

    if (!ptr)
      return NULL; /*  GEARMAN_MEMORY_ALLOCATION_FAILURE */

    memset(ptr, 0, sizeof(gearman_connection_st));
    ptr->is_allocated= true;
  }
  else
  {
    memset(ptr, 0, sizeof(gearman_connection_st));
  }

  /* TODO Figure out how to handle punting of the NULL here */
  assert(gearman_result_create(NULL, &ptr->result));

  return ptr;
}

void gearman_connection_free(gearman_connection_st *ptr)
{
  gearman_result_free(&ptr->result);

  if (ptr->is_allocated)
      free(ptr);
}

/*
  clone is the destination, while ptr is the structure to clone.
  If ptr is NULL the call is the same as if a gearman_create() was
  called.
*/
gearman_connection_st *gearman_connection_clone(gearman_connection_st *clone, gearman_connection_st *ptr)
{
  gearman_connection_st *new_clone;

  if (ptr == NULL)
    return gearman_connection_create(clone);

  if (ptr->is_allocated)
  {
    WATCHPOINT_ASSERT(0);
    return NULL;
  }
  
  new_clone= gearman_connection_create(clone);
  
  if (new_clone == NULL)
    return NULL;

  return new_clone;
}

bool gearman_connection_add_fd(gearman_connection_st *ptr, int fd)
{
  int x;
  bool was_found;

  for (x= 0, was_found= false; x < GEARMAN_CONNECTION_MAX_FDS ; x++)
  {
    if (ptr->fds[x] == -1)
    {
      ptr->fds[x]= fd;
      was_found= true;
    }
  }

  return was_found;
}

bool gearman_connection_buffered(gearman_connection_st *ptr)
{
  /* check something in ptr? do this to suppress compiler warning for now */
  ptr= NULL;
  return false;
}
