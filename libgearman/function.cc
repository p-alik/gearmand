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
  bool background;
  gearman_job_priority_t priority;
  time_t epoch;
  struct gearman_client_st *client;
  size_t size;
  char *name;
};

#pragma GCC diagnostic ignored "-Wold-style-cast"

gearman_function_t *gearman_function_create(struct gearman_client_st *client, const char *name, size_t size)
{
  errno= 0;
  if (not client or not name or not size)
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
  function->background= false;
  function->priority= GEARMAN_JOB_PRIORITY_NORMAL;
  function->epoch= 0;
  function->client= client;
  function->size= size;

  return function;
}

void gearman_function_free(gearman_function_t *function)
{
  free(function);
}

void gearman_function_set_priority(gearman_function_t *self, const gearman_job_priority_t priority)
{
  if (not self)
    return;

  self->priority= priority;
}

void gearman_function_set_epoch(gearman_function_t *self, time_t epoch)
{
  if (! self)
    return;

  self->epoch= epoch;
}

void gearman_function_set_background(gearman_function_t *self, time_t background)
{
  if (! self)
    return;

  self->background= background;
}

const char *gearman_function_name(gearman_function_t *self)
{
  if (not self)
    return 0;

  return self->name;
}

size_t gearman_function_size(gearman_function_t *self)
{
  if (not self)
    return 0;

  return self->size;
}


time_t gearman_function_epoch(gearman_function_t *self)
{
  if (not self)
    return 0;

  return self->epoch;
}

gearman_job_priority_t gearman_function_priority(gearman_function_t *self)
{
  if (not self)
    return GEARMAN_JOB_PRIORITY_NORMAL;

  return self->priority;
}

bool gearman_function_background(gearman_function_t *self)
{
  if (not self)
    return false;

  return self->background;
}
