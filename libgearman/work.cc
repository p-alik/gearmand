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
#include <cstring>
#include <memory>

gearman_work_t gearman_work(gearman_job_priority_t priority)
{
  gearman_work_t local= { GEARMAN_WORK_KIND_FOREGROUND, priority, {0}, 0};

  return local;
}

gearman_work_t gearman_work_background(gearman_job_priority_t priority)
{
  gearman_work_t local= { GEARMAN_WORK_KIND_BACKGROUND, priority, {0}, 0};

  return local;
}

gearman_work_t gearman_work_epoch(time_t epoch)
{
  gearman_work_t local= { GEARMAN_WORK_KIND_BACKGROUND, GEARMAN_JOB_PRIORITY_NORMAL, { epoch }, 0};

  return local;
}

time_t gearman_workload_epoch(const gearman_work_t *self)
{
  if (not self)
    return 0;

  return self->options.epoch;
}

gearman_job_priority_t gearman_workload_priority(const gearman_work_t *self)
{
  if (not self)
    return GEARMAN_JOB_PRIORITY_NORMAL;

  return self->priority;
}

bool gearman_workload_background(const gearman_work_t *self)
{
  if (not self)
    return false;

  return (self->kind == GEARMAN_WORK_KIND_BACKGROUND or self->kind == GEARMAN_WORK_KIND_EPOCH);
}

void gearman_workload_set_context(gearman_work_t *self, void *context)
{
  if (not self)
    return;

  self->context= context;
}
