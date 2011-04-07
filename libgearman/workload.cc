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

gearman_workload_t gearman_workload_make(const char *arg, size_t arg_size)
{
  gearman_workload_t local= { false, GEARMAN_JOB_PRIORITY_NORMAL, 0, arg, arg_size, 0};

  return local;
}

size_t gearman_workload_size(gearman_workload_t *self)
{
  if (! self)
    return 0;

  return self->size;
}


time_t gearman_workload_epoch(const gearman_workload_t *self)
{
  if (not self)
    return 0;

  return self->epoch;
}

void gearman_workload_set_epoch(gearman_workload_t *self, time_t epoch)
{
  if (! self)
    return;

  self->epoch= epoch;
}

gearman_job_priority_t gearman_workload_priority(const gearman_workload_t *self)
{
  if (not self)
    return GEARMAN_JOB_PRIORITY_NORMAL;

  return self->priority;
}

bool gearman_workload_background(const gearman_workload_t *self)
{
  if (not self)
    return false;

  return self->background;
}

void gearman_workload_set_priority(gearman_workload_t *self, const gearman_job_priority_t priority)
{
  if (not self)
    return;

  self->priority= priority;
}

void gearman_workload_set_background(gearman_workload_t *self, bool background)
{
  if (! self)
    return;

  self->background= background;
}

void gearman_workload_set_context(gearman_workload_t *self, void *context)
{
  if (not self)
    return;

  self->context= context;
}

#if 0
void *gearman_workload_context(const gearman_workload_t *self)
{
  if (not self)
    return NULL;

  return self->context;
}
#endif
