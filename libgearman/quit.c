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

void gearman_quit_server(gearman_server_st *ptr, uint8_t io_death)
{
  if (ptr->fd != -1)
  {
    /* For the moment assume IO death */
    io_death = 1;
    if (io_death == 0)
    {
      ssize_t read_length;
      char buffer[GEARMAN_MAX_BUFFER];

      /* (void)gearman_do(ptr, "quit\r\n", 6, 1); */

      /* read until socket is closed, or there is an error
       * closing the socket before all data is read
       * results in server throwing away all data which is
       * not read
       */
      while ((read_length=
	      gearman_io_read(ptr, buffer, sizeof(buffer)/sizeof(*buffer)))
	     > 0)
	{
	  ;
	}
    }
    gearman_io_close(ptr);

    ptr->fd= -1;
    ptr->write_buffer_offset= 0;
    ptr->read_buffer_length= 0;
    ptr->read_ptr= ptr->read_buffer;
    gearman_server_response_reset(ptr);
  }
}

void gearman_quit(gearman_st *ptr)
{
  unsigned int x;

  if (ptr->hosts == NULL || 
      ptr->number_of_hosts == 0)
    return;

  if (ptr->hosts && ptr->number_of_hosts)
  {
    for (x= 0; x < ptr->number_of_hosts; x++)
      gearman_quit_server(&ptr->hosts[x], 0);
  }
}
