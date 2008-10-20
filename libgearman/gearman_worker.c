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

#include <libgearman/gearman_worker.h>
#include "common.h"

gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    worker= (gearman_worker_st *)malloc(sizeof(gearman_worker_st));

    if (!worker)
      return NULL; /*  GEARMAN_MEMORY_ALLOCATION_FAILURE */

    memset(worker, 0, sizeof(gearman_worker_st));
    worker->is_allocated= true;
  }
  else
  {
    memset(worker, 0, sizeof(gearman_worker_st));
  }

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
  /* If we have anything open, lets close it now */
  gearman_server_list_free(&worker->list);

  if (worker->is_allocated == true)
      free(worker);
  else
    memset(worker, 0, sizeof(worker));
}

/*
  clone is the destination, while worker is the structure to clone.
  If worker is NULL the call is the same as if a gearman_worker_create() was
  called.
*/
gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker)
{
  gearman_worker_st *clone;
  gearman_return rc= GEARMAN_SUCCESS;

  clone= gearman_worker_create(NULL);

  if (clone == NULL)
    return NULL;

  /* If worker was null, then we just return NULL */
  if (worker == NULL)
    return gearman_worker_create(NULL);

  rc= gearman_server_copy(&clone->list, &worker->list);

  if (rc != GEARMAN_SUCCESS)
  {
    gearman_worker_free(clone);

    return NULL;
  }

  return clone;
}

gearman_return gearman_worker_server_add(gearman_worker_st *worker,
                                         const char *hostname,
                                         uint16_t port)
{
  return gearman_server_add(&worker->list, hostname, port);
}
