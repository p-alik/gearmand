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
  This function is used to modify the behabior of running client.

  We quit all connections so we can reset the sockets.
*/

static void set_behavior_flag(gearman_st *ptr, gearman_flags temp_flag,
                              uint64_t data)
{
  if (data)
    ptr->flags|= temp_flag;
  else
    ptr->flags&= ~temp_flag;
}

gearman_return gearman_behavior_set(gearman_st *ptr, gearman_behavior flag,
                                    uint64_t data)
{
  switch (flag)
  {
  case GEARMAN_BEHAVIOR_SUPPORT_CAS:
    set_behavior_flag(ptr, GEAR_SUPPORT_CAS, data);
    break;
  case GEARMAN_BEHAVIOR_NO_BLOCK:
    set_behavior_flag(ptr, GEAR_NO_BLOCK, data);
    gearman_quit(ptr);
  case GEARMAN_BEHAVIOR_BUFFER_REQUESTS:
    set_behavior_flag(ptr, GEAR_BUFFER_REQUESTS, data);
    gearman_quit(ptr);
    break;
  case GEARMAN_BEHAVIOR_TCP_NODELAY:
    set_behavior_flag(ptr, GEAR_TCP_NODELAY, data);
    gearman_quit(ptr);
    break;
  case GEARMAN_BEHAVIOR_DISTRIBUTION:
    ptr->distribution= (gearman_server_distribution)(data);
    break;
  case GEARMAN_BEHAVIOR_HASH:
    ptr->hash= (gearman_hash)(data);
    break;
  case GEARMAN_BEHAVIOR_CACHE_LOOKUPS:
    set_behavior_flag(ptr, GEAR_USE_CACHE_LOOKUPS, data);
    gearman_quit(ptr);
    break;
  case GEARMAN_BEHAVIOR_VERIFY_KEY:
    set_behavior_flag(ptr, GEAR_VERIFY_KEY, data);
    break;
  case GEARMAN_BEHAVIOR_KETAMA:
    set_behavior_flag(ptr, GEAR_USE_KETAMA, data);
    break;
  case GEARMAN_BEHAVIOR_SORT_HOSTS:
    set_behavior_flag(ptr, GEAR_USE_SORT_HOSTS, data);
    break;
  case GEARMAN_BEHAVIOR_POLL_TIMEOUT:
    ptr->poll_timeout= (int32_t)data;
    break;
  case GEARMAN_BEHAVIOR_CONNECT_TIMEOUT:
    ptr->connect_timeout= (int32_t)data;
    break;
  case GEARMAN_BEHAVIOR_RETRY_TIMEOUT:
    ptr->retry_timeout= (int32_t)data;
    break;
  case GEARMAN_BEHAVIOR_SOCKET_SEND_SIZE:
    ptr->send_size= (int32_t)data;
    gearman_quit(ptr);
    break;
  case GEARMAN_BEHAVIOR_SOCKET_RECV_SIZE:
    ptr->recv_size= (int32_t)data;
    gearman_quit(ptr);
    break;
  case GEARMAN_BEHAVIOR_USER_DATA:
    return GEARMAN_FAILURE;
  }

  return GEARMAN_SUCCESS;
}

uint64_t gearman_behavior_get(gearman_st *ptr, gearman_behavior flag)
{
  gearman_flags temp_flag= 0;

  switch (flag)
  {
  case GEARMAN_BEHAVIOR_SUPPORT_CAS:
    temp_flag= GEAR_SUPPORT_CAS;
    break;
  case GEARMAN_BEHAVIOR_CACHE_LOOKUPS:
    temp_flag= GEAR_USE_CACHE_LOOKUPS;
    break;
  case GEARMAN_BEHAVIOR_NO_BLOCK:
    temp_flag= GEAR_NO_BLOCK;
    break;
  case GEARMAN_BEHAVIOR_BUFFER_REQUESTS:
    temp_flag= GEAR_BUFFER_REQUESTS;
    break;
  case GEARMAN_BEHAVIOR_TCP_NODELAY:
    temp_flag= GEAR_TCP_NODELAY;
    break;
  case GEARMAN_BEHAVIOR_VERIFY_KEY:
    temp_flag= GEAR_VERIFY_KEY;
    break;
  case GEARMAN_BEHAVIOR_DISTRIBUTION:
    return ptr->distribution;
  case GEARMAN_BEHAVIOR_HASH:
    return ptr->hash;
  case GEARMAN_BEHAVIOR_KETAMA:
    temp_flag= GEAR_USE_KETAMA;
    break;
  case GEARMAN_BEHAVIOR_SORT_HOSTS:
    temp_flag= GEAR_USE_SORT_HOSTS;
    break;
  case GEARMAN_BEHAVIOR_POLL_TIMEOUT:
    {
      return (uint64_t)ptr->poll_timeout;
    }
  case GEARMAN_BEHAVIOR_CONNECT_TIMEOUT:
    {
      return (uint64_t)ptr->connect_timeout;
    }
  case GEARMAN_BEHAVIOR_RETRY_TIMEOUT:
    {
      return (uint64_t)ptr->retry_timeout;
    }
  case GEARMAN_BEHAVIOR_SOCKET_SEND_SIZE:
    {
      int sock_size;
      socklen_t sock_length= sizeof(int);

      /* REFACTOR */
      /* We just try the first host, and if it is down we return zero */
      if ((gearman_connect(&ptr->hosts[0])) != GEARMAN_SUCCESS)
        return 0;

      if (getsockopt(ptr->hosts[0].fd, SOL_SOCKET, 
                     SO_SNDBUF, &sock_size, &sock_length))
        return 0; /* Zero means error */

      return sock_size;
    }
  case GEARMAN_BEHAVIOR_SOCKET_RECV_SIZE:
    {
      int sock_size;
      socklen_t sock_length= sizeof(int);

      /* REFACTOR */
      /* We just try the first host, and if it is down we return zero */
      if ((gearman_connect(&ptr->hosts[0])) != GEARMAN_SUCCESS)
        return 0;

      if (getsockopt(ptr->hosts[0].fd, SOL_SOCKET, 
                     SO_RCVBUF, &sock_size, &sock_length))
        return 0; /* Zero means error */

      return sock_size;
    }
  case GEARMAN_BEHAVIOR_USER_DATA:
    return GEARMAN_FAILURE;
  }

  WATCHPOINT_ASSERT(temp_flag); /* Programming mistake if it gets this far */
  if (ptr->flags & temp_flag)
    return 1;
  else
    return 0;

  return GEARMAN_SUCCESS;
}
