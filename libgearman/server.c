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

static void host_reset(gearman_server_list_st *list,
                       gearman_server_st *host, 
                       const char *hostname,
                       uint16_t port)
{
  memset(host,  0, sizeof(gearman_server_st));
  strncpy(host->hostname, hostname, GEARMAN_MAX_HOST_LENGTH - 1);
  host->root= list ? list : NULL;
  host->port= port;
  host->fd= -1;
  host->read_ptr= host->read_buffer;
  host->sockaddr_inited= false;
}

static gearman_return server_add(gearman_server_list_st *list, 
                                 const char *hostname,
                                 uint16_t port)
{
  gearman_server_st *new_host_list;

  new_host_list= (gearman_server_st *)realloc(list->hosts, 
                                              sizeof(gearman_server_st) * (list->number_of_hosts+1));
  if (new_host_list == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  list->hosts= new_host_list;

  host_reset(list, &list->hosts[list->number_of_hosts], hostname, port);
  list->number_of_hosts++;

  return GEARMAN_SUCCESS;
}

gearman_return gearman_server_add(gearman_server_list_st *list,
                                  const char *hostname,
                                  uint16_t port)
{
  if (!port)
    port= GEARMAN_DEFAULT_PORT; 

  if (!hostname)
    hostname= "localhost"; 

  return server_add(list, hostname, port);
}

gearman_return gearman_server_copy(gearman_server_list_st *destination, 
                                   gearman_server_list_st *source)
{
  uint32_t x;

  for (x= 0; x < source->number_of_hosts; x++)
  {
    gearman_return rc;
    rc= gearman_server_add(destination,
                           source->hosts[x].hostname,
                           source->hosts[x].port);

    assert(rc == GEARMAN_SUCCESS);
  }
  return GEARMAN_SUCCESS;
}

static void gearman_server_free(gearman_server_st *host)
{
  if (host->fd != -1)
  {
    if (host->io_death == 0)
    {
      ssize_t read_length;
      uint8_t buffer[GEARMAN_MAX_BUFFER];

      /* read until socket is closed, or there is an error
       * closing the socket before all data is read
       * results in server throwing away all data which is
       * not read
       */
      while ((read_length=
              gearman_io_read(host, buffer, GEARMAN_MAX_BUFFER)) > 0);
    }
    gearman_io_close(host);

    host->fd= -1;
    host->write_buffer_offset= 0;
    host->read_buffer_length= 0;
    host->read_ptr= host->read_buffer;
  }
}

void gearman_server_list_free(gearman_server_list_st *list)
{
  uint32_t x;

  if (list->number_of_hosts == 0)
    return;

  for (x= 0; x < list->number_of_hosts; x++)
    gearman_server_free(&list->hosts[x]);

  free(list->hosts);
}
