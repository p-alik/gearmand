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

/* Protoypes (static) */
static gearman_return server_add(gearman_st *ptr, char *hostname, 
                                   unsigned int port);

static void host_reset(gearman_st *ptr, gearman_server_st *host, 
                       char *hostname, unsigned int port)
{
  memset(host,  0, sizeof(gearman_server_st));
  strncpy(host->hostname, hostname, GEARMAN_MAX_HOST_LENGTH - 1);
  host->root= ptr ? ptr : NULL;
  host->port= port;
  host->fd= -1;
  host->read_ptr= host->read_buffer;
  if (ptr)
    host->next_retry= ptr->retry_timeout;
  host->sockaddr_inited= GEARMAN_NOT_ALLOCATED;
}

gearman_return gearman_server_add_internal(gearman_server_st *ptr, int fd)
{
  memset(ptr,  0, sizeof(gearman_server_st));
  ptr->fd= fd;
  ptr->sockaddr_inited= GEARMAN_ALLOCATED;
  ptr->type= GEARMAN_SERVER_TYPE_INTERNAL;

  return GEARMAN_SUCCESS;
}

void server_list_free(gearman_st *ptr, gearman_server_st *servers)
{
  unsigned int x;

  if (servers == NULL)
    return;

  for (x= 0; x < ptr->number_of_hosts; x++)
    if (servers[x].address_info)
      freeaddrinfo(servers[x].address_info);

  free(servers);
}

/* FIX we are overwriting old hosts */
gearman_return gearman_server_push(gearman_st *ptr, gearman_st *list)
{
  unsigned int x;
  gearman_server_st *new_host_list;

  if (!list)
    return GEARMAN_SUCCESS;

  new_host_list= (gearman_server_st *)realloc(ptr->hosts, 
                                              sizeof(gearman_server_st) * (list->number_of_hosts));

  if (!new_host_list)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  ptr->hosts= new_host_list;
                                   
  for (x= 0; x < list->number_of_hosts; x++)
  {
    WATCHPOINT_ASSERT(list->hosts[x].hostname[0] != 0);
    host_reset(ptr, &ptr->hosts[ptr->number_of_hosts], list->hosts[x].hostname, 
               list->hosts[x].port);
    ptr->number_of_hosts++;
  }

  /* This assert will be wrong when we fix his function */
  WATCHPOINT_ASSERT(ptr->number_of_hosts == list->number_of_hosts);

  return GEARMAN_SUCCESS;
}

gearman_return gearman_server_add(gearman_st *ptr, 
                                      char *hostname, 
                                      unsigned int port)
{
  if (!port)
    port= GEARMAN_DEFAULT_PORT; 

  if (!hostname)
    hostname= "localhost"; 

  return server_add(ptr, hostname, port);
}

static gearman_return server_add(gearman_st *ptr, char *hostname, 
                                   unsigned int port)
{
  gearman_server_st *new_host_list;

  new_host_list= (gearman_server_st *)realloc(ptr->hosts, 
                                              sizeof(gearman_server_st) * (ptr->number_of_hosts+1));
  if (new_host_list == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  ptr->hosts= new_host_list;

  host_reset(ptr, &ptr->hosts[ptr->number_of_hosts], hostname, port);
  ptr->number_of_hosts++;

  return GEARMAN_SUCCESS;
}

bool gearman_server_buffered(gearman_server_st *ptr)
{
  return ptr->read_buffer_length ? true : false;
}

void gearman_server_free(gearman_server_st *ptr)
{
  if (ptr->fd != -1)
  {
    if (ptr->io_death == 0)
    {
      ssize_t read_length;
      char buffer[GEARMAN_MAX_BUFFER];

      /* read until socket is closed, or there is an error
       * closing the socket before all data is read
       * results in server throwing away all data which is
       * not read
       */
      while ((read_length=
	      gearman_io_read(ptr, buffer, GEARMAN_MAX_BUFFER)) > 0);
    }
    gearman_io_close(ptr);

    ptr->fd= -1;
    ptr->write_buffer_offset= 0;
    ptr->read_buffer_length= 0;
    ptr->read_ptr= ptr->read_buffer;
  }
}
