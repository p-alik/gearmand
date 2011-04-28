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
#include <libgearman/unique.h>

struct gearman_workload_t {
  bool background;
  gearman_job_priority_t priority;
  time_t epoch;
  const char *c_str;
  size_t size;
  void *context;
  gearman_unique_t unique;
  gearman_task_st *task;
};

struct gearman_batch_t {
  gearman_function_st *function;
  gearman_workload_t workload;
};

#ifdef __cplusplus
extern "C" {
#endif

GEARMAN_API
gearman_workload_t gearman_workload_make(const char *arg, size_t arg_size);

GEARMAN_API
gearman_workload_t gearman_workload_make_unique(const char *arg, size_t arg_size, const char *unique, size_t unique_size);

GEARMAN_API
size_t gearman_workload_size(gearman_workload_t *self);

GEARMAN_API
void gearman_workload_set_epoch(gearman_workload_t *, time_t);

GEARMAN_LOCAL
time_t gearman_workload_epoch(const gearman_workload_t *);

GEARMAN_API
void gearman_workload_set_context(gearman_workload_t *, void *);

GEARMAN_LOCAL
void *gearman_workload_context(const gearman_workload_t *);

GEARMAN_LOCAL
gearman_job_priority_t gearman_workload_priority(const gearman_workload_t *);

GEARMAN_API
void gearman_workload_set_priority(gearman_workload_t *, gearman_job_priority_t);

GEARMAN_API
void gearman_workload_set_background(gearman_workload_t *self, bool background);

GEARMAN_LOCAL
bool gearman_workload_background(const gearman_workload_t *);

GEARMAN_API
gearman_task_st *gearman_workload_task(const gearman_workload_t *);

GEARMAN_LOCAL
void gearman_workload_set_task(gearman_workload_t *, gearman_task_st *);

GEARMAN_LOCAL
const gearman_unique_t *gearman_workload_unique(const gearman_workload_t *self);

GEARMAN_API
bool gearman_workload_compare(const gearman_workload_t *self, const char*, size_t);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define gearman_literal_param(X) (X), static_cast<size_t>(sizeof(X) - 1)
#define gearman_workload_array_size(X) size_t(sizeof(X)/sizeof(gearman_workload_t))
#define gearman_workload_batch_array_size(X) size_t(sizeof(X)/sizeof(gearman_batch_t))
#else
#define gearman_literal_param(X) (X), (size_t)(sizeof(X) - 1)
#define gearman_workload_array_size(X) (size_t)(sizeof(X)/sizeof(gearman_workload_t))
#define gearman_workload_batch_array_size(X) (size_t)(sizeof(X)/sizeof(gearman_batch_t))
#endif

#define gearman_string_param(X) (X) ? (X) : NULL, (X) ? strlen(X) : 0
#define gearman_param(X) (X) ? (X)->c_str : NULL, (X) ? (X)->size : 0
#define gearman_workload_context(X) (X)->context
