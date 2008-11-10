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

/*
  This closes all connections (forces flush of input as well).
  
  Maybe add a host specific, or key specific version? 
  
  The reason we send "quit" is that in case we have buffered IO, this 
  will force data to be completed.
*/

void gearman_quit_server(gearman_server_st *server, bool io_death)
{
  if (server->fd != -1)
  {
    /* For the moment assume IO death */
    io_death = true;
    if (io_death == false)
    {
      ssize_t read_length;
      uint8_t buffer[GEARMAN_MAX_BUFFER];

      /* (void)gearman_do(server, "quit\r\n", 6, 1); */

      /* read until socket is closed, or there is an error
       * closing the socket before all data is read
       * results in server throwing away all data which is
       * not read
       */
      while ((read_length=
	      gearman_io_read(server, buffer, sizeof(buffer)/sizeof(*buffer)))
	     > 0)
	{
	  continue;
	}
    }
    gearman_io_close(server);

    server->fd= -1;
    server->write_buffer_offset= 0;
    server->read_buffer_length= 0;
    server->read_ptr= server->read_buffer;
    gearman_server_response_reset(server);
  }
}

void gearman_quit(gearman_server_list_st *list)
{
  uint32_t x;

  if (list->hosts == NULL || 
      list->number_of_hosts == 0)
    return;

  if (list->hosts && list->number_of_hosts)
  {
    for (x= 0; x < list->number_of_hosts; x++)
      gearman_quit_server(&list->hosts[x], false);
  }
}
