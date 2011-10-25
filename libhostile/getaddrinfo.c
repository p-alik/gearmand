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
  Random getaddrinfo failing library for testing getaddrinfo() failures.
  LD_PRELOAD="/usr/lib/libdl.so ./util/libhostile_getaddrinfo.so" ./binary
*/

#include <dlfcn.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <libhostile/initialize.h>

__thread bool is_in_getaddrinfo= 0;

static int not_until= 500;

static struct function_st __function;

static pthread_once_t function_lookup_once = PTHREAD_ONCE_INIT;
static void set_local(void)
{
  __function= set_function("getaddrinfo", "HOSTILE_GETADDRINFO");
}

bool is_getaddrinfo(void)
{
  return is_in_getaddrinfo;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res)
{

  hostile_initialize();

  (void) pthread_once(&function_lookup_once, set_local);

  if (__function.frequency)
  {
    if (--not_until < 0 && random() % __function.frequency)
    {
#if 0
      perror("HOSTILE CLOSE() of socket during getaddrinfo()");
#endif
      return EAI_FAIL;
    }
  }

  is_in_getaddrinfo= true;
  int ret= __function.function.getaddrinfo(node, service, hints, res);
  is_in_getaddrinfo= false;

  return ret;
}


