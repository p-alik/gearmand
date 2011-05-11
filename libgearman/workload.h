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

#pragma once

#include <time.h>
#include <libgearman/actions.h>
#include <libgearman/string.h>

struct gearman_argument_t {
  gearman_string_t name;
  gearman_string_t value;
};

struct gearman_workload_t {
  bool background;
  bool batch;
  gearman_job_priority_t priority;
  time_t epoch;
  void *context;
};

#ifdef __cplusplus
extern "C" {
#endif

#define  gearman_next(X) (X) ? (X)->next : NULL

GEARMAN_API
gearman_workload_t gearman_workload_make(void);

GEARMAN_API
gearman_argument_t gearman_argument_make(const char *value, size_t value_size);

GEARMAN_API
void gearman_workload_set_epoch(gearman_workload_t *, time_t);

GEARMAN_API
void gearman_workload_set_context(gearman_workload_t *, void *);

GEARMAN_API
void gearman_workload_set_priority(gearman_workload_t *, gearman_job_priority_t);

GEARMAN_API
void gearman_workload_set_background(gearman_workload_t *self, bool background);

GEARMAN_API
void gearman_workload_set_batch(gearman_workload_t *self, bool batch);

// Everything below here is private

GEARMAN_LOCAL
time_t gearman_workload_epoch(const gearman_workload_t *);

GEARMAN_LOCAL
gearman_job_priority_t gearman_workload_priority(const gearman_workload_t *);

GEARMAN_LOCAL
bool gearman_workload_background(const gearman_workload_t *);

#ifdef __cplusplus
}
#endif
