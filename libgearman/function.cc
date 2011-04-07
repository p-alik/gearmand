/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libgearman/common.h>

struct gearman_function_t {
  size_t size;
  char *name;
};

#pragma GCC diagnostic ignored "-Wold-style-cast"

gearman_function_t *gearman_function_create(const char *name, size_t size)
{
  errno= 0;
  if (not name or not size)
  {
    errno= EINVAL;
    return NULL;
  }

  gearman_function_t *function= (struct gearman_function_t *)malloc(sizeof(struct gearman_function_t) +size +1);;

  if (not function)
  {
    return NULL;
  }

  function->name= ((char *)function) + sizeof(struct gearman_function_t);
  memcpy(function->name, name, size);
  function->name[size]= 0;
  function->size= size;

  return function;
}

void gearman_function_free(gearman_function_t *function)
{
  free(function);
}

const char *gearman_function_name(const gearman_function_t *self)
{
  if (not self)
    return 0;

  return self->name;
}

size_t gearman_function_size(const gearman_function_t *self)
{
  if (not self)
    return 0;

  return self->size;
}
