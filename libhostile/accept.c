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
  Random accept failing library for testing accept failures.
  LD_PRELOAD="/usr/lib/libdl.so ./util/libhostile_accept.so" ./binary
*/

//#define _GNU_SOURCE
#include <dlfcn.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include <libhostile/initialize.h>


static int not_until= 500;

static struct function_st __function;

static pthread_once_t function_lookup_once = PTHREAD_ONCE_INIT;
static void set_local(void)
{
  __function= set_function("accept", "HOSTILE_SEND");
}

void set_accept_close(bool arg, int frequency, int not_until_arg)
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

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{

  hostile_initialize();

  (void) pthread_once(&function_lookup_once, set_local);

  if (is_getaddrinfo() == false && __function.frequency)
  {
    if (--not_until < 0 && random() % __function.frequency)
    {
      if (random() % 1)
      {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        errno= ECONNABORTED;
        return -1;
      }
      else
      {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        errno= EMFILE;
        return -1;
      }
    }
  }

  return __function.function.accept(sockfd, addr, addrlen);
}
