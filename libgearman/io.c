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

typedef enum {
  MEM_READ,
  MEM_WRITE
} memc_read_or_write;

static ssize_t io_flush(gearman_server_st *ptr, gearman_return *error);

static gearman_return io_wait(gearman_server_st *ptr,
                                memc_read_or_write read_or_write)
{
  struct pollfd fds[1];
  short flags= 0;
  int error;

  if (read_or_write == MEM_WRITE) /* write */
    flags= POLLOUT |  POLLERR;
  else
    flags= POLLIN | POLLERR;

  memset(&fds, 0, sizeof(struct pollfd));
  fds[0].fd= ptr->fd;
  fds[0].events= flags;

  error= poll(fds, 1, ptr->root->poll_timeout);

  if (error == 1)
    return GEARMAN_SUCCESS;
  else if (error == 0)
  {
    return GEARMAN_TIMEOUT;
  }

  /* Imposssible for anything other then -1 */
  WATCHPOINT_ASSERT(error == -1);
  gearman_quit_server(ptr, 1);

  return GEARMAN_FAILURE;

}

ssize_t gearman_io_read(gearman_server_st *ptr,
                          char *buffer, size_t length)
{
  char *buffer_ptr;

  buffer_ptr= buffer;

  while (length)
  {
    uint8_t found_eof= 0;
    if (!ptr->read_buffer_length)
    {
      ssize_t data_read;

      while (1)
      {
        data_read= read(ptr->fd, 
                        ptr->read_buffer, 
                        GEARMAN_MAX_BUFFER);
        if (data_read > 0)
          break;
        else if (data_read == -1)
        {
          ptr->cached_errno= errno;
          switch (errno)
          {
          case EAGAIN:
            {
              gearman_return rc;

              rc= io_wait(ptr, MEM_READ);

              if (rc == GEARMAN_SUCCESS)
                continue;
            }
          /* fall trough */
          default:
            {
              if (ptr->type == GEARMAN_SERVER_TYPE_TCP)
                gearman_quit_server(ptr, 1);
              return -1;
            }
          }
        }
        else
        {
          found_eof= 1;
          break;
        }
      }

      ptr->read_data_length= data_read;
      ptr->read_buffer_length= data_read;
      ptr->read_ptr= ptr->read_buffer;
    }

    if (length > 1)
    {
      size_t difference;

      difference= (length > ptr->read_buffer_length) ? ptr->read_buffer_length : length;

      memcpy(buffer_ptr, ptr->read_ptr, difference);
      length -= difference;
      ptr->read_ptr+= difference;
      ptr->read_buffer_length-= difference;
      buffer_ptr+= difference;
    }
    else if (length == 1)
    {
      *buffer_ptr= *ptr->read_ptr;
      ptr->read_ptr++;
      ptr->read_buffer_length--;
      buffer_ptr++;
      break;
    }

    if (found_eof)
      break;
  }

  return (size_t)(buffer_ptr - buffer);
}

ssize_t gearman_io_write(gearman_server_st *ptr,
                           char *buffer, size_t length, char with_flush)
{
  size_t original_length;
  char* buffer_ptr;

  original_length= length;
  buffer_ptr= buffer;

  while (length)
  {
    char *write_ptr;
    size_t should_write;

    should_write= GEARMAN_MAX_BUFFER - ptr->write_buffer_offset;
    write_ptr= ptr->write_buffer + ptr->write_buffer_offset;

    should_write= (should_write < length) ? should_write : length;

    memcpy(write_ptr, buffer_ptr, should_write);
    ptr->write_buffer_offset+= should_write;
    buffer_ptr+= should_write;
    length-= should_write;

    if (ptr->write_buffer_offset == GEARMAN_MAX_BUFFER)
    {
      gearman_return rc;
      ssize_t sent_length;

      sent_length= io_flush(ptr, &rc);
      if (sent_length == -1)
        return -1;

      WATCHPOINT_ASSERT(sent_length == GEARMAN_MAX_BUFFER);
    }
  }

  if (with_flush)
  {
    gearman_return rc;
    if (io_flush(ptr, &rc) == -1)
      return -1;
  }

  return original_length;
}

gearman_return gearman_io_close(gearman_server_st *ptr)
{
  close(ptr->fd);

  return GEARMAN_SUCCESS;
}

static ssize_t io_flush(gearman_server_st *ptr,
                        gearman_return *error)
{
  size_t sent_length;
  size_t return_length;
  char *local_write_ptr= ptr->write_buffer;
  size_t write_length= ptr->write_buffer_offset;

  *error= GEARMAN_SUCCESS;

  if (ptr->write_buffer_offset == 0)
    return 0;

  /* Looking for memory overflows */
  if (write_length == GEARMAN_MAX_BUFFER)
    WATCHPOINT_ASSERT(ptr->write_buffer == local_write_ptr);
  WATCHPOINT_ASSERT((ptr->write_buffer + GEARMAN_MAX_BUFFER) >= (local_write_ptr + write_length));

  return_length= 0;
  while (write_length)
  {
    WATCHPOINT_ASSERT(write_length > 0);

    sent_length= 0;
    {
      WATCHPOINT_NUMBER(ptr->fd);
      if ((ssize_t)(sent_length= write(ptr->fd, local_write_ptr, 
                                       write_length)) == -1)
      {
        switch (errno)
        {
        case ENOBUFS:
          continue;
        case EAGAIN:
          {
            gearman_return rc;
            rc= io_wait(ptr, MEM_WRITE);

            if (rc == GEARMAN_SUCCESS)
              continue;

            gearman_quit_server(ptr, 1);
            return -1;
          }
        default:
          gearman_quit_server(ptr, 1);
          ptr->cached_errno= errno;
          *error= GEARMAN_ERRNO;
          return -1;
        }
      }
    }

    local_write_ptr+= sent_length;
    write_length-= sent_length;
    return_length+= sent_length;
  }

  WATCHPOINT_ASSERT(write_length == 0);
  WATCHPOINT_ASSERT(return_length == ptr->write_buffer_offset);
  ptr->write_buffer_offset= 0;

  return return_length;
}
