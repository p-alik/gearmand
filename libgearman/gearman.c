/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
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

/* Initialize a library instance structure. */
gearman_st *gearman_create(gearman_st *gearman)
{
  if (gearman == NULL)
  {
    gearman= malloc(sizeof(gearman_st));
    if (gearman == NULL)
      return NULL;

    memset(gearman, 0, sizeof(gearman_st));
    gearman->options|= GEARMAN_ALLOCATED;
  }
  else
    memset(gearman, 0, sizeof(gearman_st));

  return gearman;
}

/* Clone a library instance structure. */
gearman_st *gearman_clone(gearman_st *gearman, gearman_st *from)
{
  gearman_con_st *con;

  gearman= gearman_create(gearman);
  if (gearman == NULL)
    return NULL;

  gearman->options|= (from->options & ~GEARMAN_ALLOCATED);

  for (con= from->con_list; con != NULL; con= con->next)
  {
    if (gearman_con_clone(gearman, NULL, con) == NULL)
    {
      gearman_free(gearman);
      return NULL;
    }
  }

  return gearman;
}

/* Free a library instance structure. */
void gearman_free(gearman_st *gearman)
{
  gearman_con_st *con;

  for (con= gearman->con_list; con != NULL; con= gearman->con_list)
    gearman_con_free(con);

  if (gearman->pfds != NULL)
    free(gearman->pfds);

  if (gearman->options & GEARMAN_ALLOCATED)
    free(gearman);
}

/* Return an error string for last library error encountered. */
char *gearman_error(gearman_st *gearman)
{
  return gearman->last_error;
}

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_errno(gearman_st *gearman)
{
  return gearman->last_errno;
}

/* Set options for a library instance structure. */
void gearman_set_options(gearman_st *gearman, gearman_options options,
                         uint32_t data)
{
  if (data)
    gearman->options |= options;
  else
    gearman->options &= ~options;
}

/* Wait for I/O on connections. */
gearman_return gearman_io_wait(gearman_st *gearman)
{
  gearman_con_st *con;
  struct pollfd *pfds;
  int x;
  int ret;

  if (gearman->pfds_size < gearman->con_count)
  {
    pfds= realloc(gearman->pfds, gearman->con_count * sizeof(struct pollfd));
    if (pfds == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_io_wait:realloc:%d", errno);
      gearman->last_errno= errno;
      return GEARMAN_ERRNO;
    }

    gearman->pfds= pfds;
    gearman->pfds_size= gearman->con_count;
  }
  else
    pfds= gearman->pfds;

  x= 0;
  for (con= gearman->con_list; con != NULL; con= con->next)
  {
#if 0
    if (con->events == 0)
      continue;
#endif

    pfds[x].fd= con->fd;
    pfds[x].events= con->events | POLLIN;
    pfds[x].revents= 0;
    x++;
  }

  ret= poll(pfds, x, -1);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_io_wait:poll:%d", errno);
    gearman->last_errno= errno;
    return GEARMAN_ERRNO;
  }

  x= 0;
  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->events == 0)
      continue;

    con->revents= pfds[x].revents;
    x++;
  }

  return GEARMAN_SUCCESS;
}

/* Get next connection that is ready for I/O. */
gearman_con_st *gearman_io_ready(gearman_st *gearman)
{
  if (gearman->con_ready == NULL)
    gearman->con_ready= gearman->con_list;
  else
    gearman->con_ready= gearman->con_ready->next;

  for (; gearman->con_ready != NULL;
       gearman->con_ready= gearman->con_ready->next)
  {
    if (gearman->con_ready->events == 0)
      continue;

    gearman->con_ready->events= 0;
    return gearman->con_ready;
  }

  return NULL;
}
