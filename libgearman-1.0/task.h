/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
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

/**
 * @file
 * @brief Task Declarations
 */

#pragma once

#include <libgearman-1.0/actions.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_task Task Declarations
 * @ingroup gearman_client
 *
 * The task functions are used to manage tasks being run by clients. They are
 * most commonly used with the client interface.
 *
 * @{
 */

/**
 * @ingroup gearman_task
 */

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
  } options;
  enum gearman_task_kind_t type;
  enum gearman_task_state_t state;
  uint32_t created_id;
  uint32_t numerator;
  uint32_t denominator;
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
};

/**
 * Free a task structure.
 *
 * @param[in] task Structure previously initialized with one of the
 *  gearman_client_add_task() functions.
 */
GEARMAN_API
void gearman_task_free(gearman_task_st *task);


/**
 * Get context for a task.
 */
GEARMAN_API
void *gearman_task_context(const gearman_task_st *task);

/**
 * Set context for a task.
 */
GEARMAN_API
void gearman_task_set_context(gearman_task_st *task, void *context);

/**
 * Get function name associated with a task.
 */
GEARMAN_API
const char *gearman_task_function_name(const gearman_task_st *task);

/**
 * Get unique identifier for a task.
 */
GEARMAN_API
const char *gearman_task_unique(const gearman_task_st *task);

/**
 * Get job handle for a task.
 */
GEARMAN_API
const char *gearman_task_job_handle(const gearman_task_st *task);

/**
 * Get status on whether a task is known or not.
 */
GEARMAN_API
bool gearman_task_is_known(const gearman_task_st *task);

/**
 * Get status on whether a task is running or not.
 */
GEARMAN_API
bool gearman_task_is_running(const gearman_task_st *task);

/**
 * Get the numerator of percentage complete for a task.
 */
GEARMAN_API
uint32_t gearman_task_numerator(const gearman_task_st *task);

/**
 * Get the denominator of percentage complete for a task.
 */
GEARMAN_API
uint32_t gearman_task_denominator(const gearman_task_st *task);

/**
 * Give allocated memory to task. After this, the library will be responsible
 * for freeing the workload memory when the task is destroyed.
 */
GEARMAN_API
void gearman_task_give_workload(gearman_task_st *task, const void *workload,
                                size_t workload_size);

/**
 * Send packet workload for a task.
 */
GEARMAN_API
size_t gearman_task_send_workload(gearman_task_st *task, const void *workload,
                                  size_t workload_size,
                                  gearman_return_t *ret_ptr);

/**
 * Get result data being returned for a task.
 */
GEARMAN_API
const void *gearman_task_data(const gearman_task_st *task);

/**
 * Get result data size being returned for a task.
 */
GEARMAN_API
size_t gearman_task_data_size(const gearman_task_st *task);

/**
 * Take allocated result data from task. After this, the caller is responsible
 * for free()ing the memory.
 */
GEARMAN_API
void *gearman_task_take_data(gearman_task_st *task, size_t *data_size);

/**
 * Read result data into a buffer for a task.
 */
GEARMAN_API
size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                              size_t data_size, gearman_return_t *ret_ptr);

GEARMAN_API
const char *gearman_task_error(const gearman_task_st *task);

GEARMAN_API
gearman_result_st *gearman_task_result(gearman_task_st *task);

GEARMAN_API
gearman_return_t gearman_task_return(const gearman_task_st *task);

/** @} */

#ifdef __cplusplus
}
#endif
