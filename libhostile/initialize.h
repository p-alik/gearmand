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


#pragma once

#include <pthread.h>

#include <stdbool.h>

#include <libhostile/accept.h>
#include <libhostile/action.h>
#include <libhostile/getaddrinfo.h>
#include <libhostile/malloc.h>
#include <libhostile/realloc.h>
#include <libhostile/recv.h>
#include <libhostile/send.h>
#include <libhostile/setsockopt.h>
#include <libhostile/write.h>

#include <libhostile/hostile.h>

#ifdef __cplusplus
extern "C" {
#endif

union function_un {
  accept_fn *accept;
  malloc_fn *malloc;
  realloc_fn *realloc;
  send_fn *send;
  recv_fn *recv;
  getaddrinfo_fn *getaddrinfo;
  setsockopt_fn *setsockopt;
  write_fn *write;
  void *ptr;
};

struct function_st {
  const char *name;
  union function_un function;
  enum action_t action;
  int frequency;
};

void hostile_initialize(void);
struct function_st set_function(const char *name, const char *environ_name);
void set_action_frequency(enum action_t action, int frequency);

#ifdef __cplusplus
}
#endif
