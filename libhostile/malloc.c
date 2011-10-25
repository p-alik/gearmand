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

/*
  Random malloc failing library for testing malloc failures.
  LD_PRELOAD="/usr/lib/libdl.so ./util/libhostile_malloc.so" ./binary
*/

#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <libhostile/initialize.h>

static int not_until= 500;

static struct function_st __function;

static pthread_once_t function_lookup_once = PTHREAD_ONCE_INIT;
static void set_malloc(void)
{
  __function= set_function("malloc", "HOSTILE_MALLOC");
}

void *malloc(size_t size)
{
  hostile_initialize();
  (void) pthread_once(&function_lookup_once, set_malloc);

  if (__function.frequency)
  {
    if (--not_until < 0 && random() % __function.frequency)
    {
      fprintf(stderr, "Mid=evil on malloc()\n");
      errno= ENOMEM;
      return NULL;
    }
  }

  return __function.function.malloc(size);

}
