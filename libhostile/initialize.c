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

#include "config.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <libhostile/initialize.h>

static pthread_once_t start_key_once = PTHREAD_ONCE_INIT;

static void startup(void)
{
  time_t time_seed= time(NULL);

  fprintf(stderr, "--------------------------------------------------------\n\n");
  fprintf(stderr, "\t\tHostile Engaged\n\n");
  fprintf(stderr, "Seed used %lu\n", (unsigned long)time_seed);
  fprintf(stderr, "\n--------------------------------------------------------\n");
  srandom((unsigned int)time_seed);
}

struct function_st set_function(const char *name, const char *environ_name)
{
  struct function_st set;

  set.name= name;

  set.function.ptr= dlsym(RTLD_NEXT, set.name);

  char *error;
  if ((error= dlerror()) != NULL)
  {
    fprintf(stderr, "libhostile: %s(%s)", set.name, error);
    exit(1);
  }

  char *ptr;
  if ((ptr= getenv(environ_name)))
  {
    set.frequency= atoi(ptr);
  }
  else
  {
    set.frequency= 0;
  }

  if (set.frequency)
  {
    fprintf(stderr, "--------------------------------------------------------\n\n");
    fprintf(stderr, "\t\tHostile Engaged -> %s\n\n", set.name);
    fprintf(stderr, "Frequency used %d\n", set.frequency);
    fprintf(stderr, "\n--------------------------------------------------------\n");
  }

  return set;
}

void set_action_frequency(enum action_t action, int frequency)
{
  (void)frequency;
  switch (action)
  {
  case CLOSE_SOCKET_RECV:
    break;

  case CLOSE_SOCKET_SEND:
  default:
    break;
  }
}

void hostile_initialize(void)
{
  (void) pthread_once(&start_key_once, startup);
}
