/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
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

enum gearman_task_state_t {
  GEARMAN_TASK_STATE_NEW,
  GEARMAN_TASK_STATE_SUBMIT,
  GEARMAN_TASK_STATE_WORKLOAD,
  GEARMAN_TASK_STATE_WORK,
  GEARMAN_TASK_STATE_CREATED,
  GEARMAN_TASK_STATE_DATA,
  GEARMAN_TASK_STATE_WARNING,
  GEARMAN_TASK_STATE_STATUS,
  GEARMAN_TASK_STATE_COMPLETE,
  GEARMAN_TASK_STATE_EXCEPTION,
  GEARMAN_TASK_STATE_FAIL,
  GEARMAN_TASK_STATE_FINISHED
};

enum gearman_task_kind_t {
  GEARMAN_TASK_KIND_ADD_TASK,
  GEARMAN_TASK_KIND_EXECUTE,
  GEARMAN_TASK_KIND_DO
};

struct gearman_task_st
{
  struct {
    bool allocated;
    bool send_in_use;
    bool is_known;
    bool is_running;
    bool was_reduced;
    bool is_paused;
    bool is_initialized;
  } options;
  enum gearman_task_kind_t type;
  enum gearman_task_state_t state;
  uint32_t magic_;
  uint32_t created_id;
  uint32_t numerator;
  uint32_t denominator;
  uint32_t client_count;
  gearman_client_st *client;
  gearman_task_st *next;
  gearman_task_st *prev;
  void *context;
  gearman_connection_st *con;
  gearman_packet_st *recv;
  gearman_packet_st send;
  struct gearman_actions_t func;
  gearman_return_t result_rc;
  struct gearman_result_st *result_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  size_t unique_length;
  char unique[GEARMAN_MAX_UNIQUE_SIZE];
};
