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
    //if (io_death == 0)
    if (0) /* For the moment assume IO death */
    {
      ssize_t read_length;
      char buffer[GEARMAN_MAX_BUFFER];

      //(void)gearman_do(ptr, "quit\r\n", 6, 1);

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
