/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libhostile
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

/*
  Random recv failing library for testing recv() failures.
  LD_PRELOAD="/usr/lib/libdl.so ./util/libhostile_recv.so" ./binary
*/

#include <dlfcn.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>

#include <libhostile/initialize.h>


static int not_until= 500;

static struct function_st __function;

static pthread_once_t function_lookup_once = PTHREAD_ONCE_INIT;
static void set_local(void)
{
  __function= set_function("recv", "HOSTILE_RECV");
}

void set_recv_close(bool arg, int frequency, int not_until_arg)
{
  if (arg)
  {
    __function.frequency= frequency;
    not_until= not_until_arg;
  }
  else
  {
    __function.frequency= 0;
    not_until= 0;
  }
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{

  hostile_initialize();

  (void) pthread_once(&function_lookup_once, set_local);

  if (is_getaddrinfo() == false && __function.frequency)
  {
    if (--not_until < 0 && random() % __function.frequency)
    {
      shutdown(sockfd, SHUT_RDWR);
      close(sockfd);
      errno= 0;
#if 0
      perror("HOSTILE CLOSE() of socket during recv()");
#endif
      return 0;
    }
  }

  return __function.function.recv(sockfd, buf, len, flags);
}

