/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief Defines, typedefs, and enums
 */

#ifndef __GEARMAN_CONSTANTS_H__
#define __GEARMAN_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_constants Gearman Constants
 * @{
 */

/* Defines. */
#define GEARMAN_DEFAULT_TCP_HOST "127.0.0.1"
#define GEARMAN_DEFAULT_TCP_PORT 7003
#define GEARMAN_DEFAULT_SOCKET_TIMEOUT 10
#define GEARMAN_DEFAULT_SOCKET_SEND_SIZE 32768
#define GEARMAN_DEFAULT_SOCKET_RECV_SIZE 32768

#define GEARMAN_ERROR_SIZE 1024
#define GEARMAN_PACKET_HEADER_SIZE 12
#define GEARMAN_JOB_HANDLE_SIZE 64
#define GEARMAN_MAX_COMMAND_ARGS 8
#define GEARMAN_ARGS_BUFFER_SIZE 128
#define GEARMAN_SEND_BUFFER_SIZE 8192
#define GEARMAN_RECV_BUFFER_SIZE 8192

/**
 * Macro to set error string.
 */
#define GEARMAN_ERROR_SET(__gearman, ...) { \
  snprintf((__gearman)->last_error, GEARMAN_ERROR_SIZE, __VA_ARGS__); }

/* Types. */
typedef struct gearman_st gearman_st;
typedef struct gearman_con_st gearman_con_st;
typedef struct gearman_packet_st gearman_packet_st;
typedef struct gearman_command_info_st gearman_command_info_st;
typedef struct gearman_task_st gearman_task_st;
typedef struct gearman_client_st gearman_client_st;
typedef struct gearman_job_st gearman_job_st;
typedef struct gearman_worker_st gearman_worker_st;
typedef struct gearman_worker_function_st gearman_worker_function_st;

/**
 * Return codes.
 */
typedef enum
{
  GEARMAN_SUCCESS,
  GEARMAN_IO_WAIT,
  GEARMAN_ERRNO,
  GEARMAN_TOO_MANY_ARGS,
  GEARMAN_INVALID_MAGIC,
  GEARMAN_INVALID_COMMAND,
  GEARMAN_INVALID_PACKET,
  GEARMAN_GETADDRINFO,
  GEARMAN_NO_SERVERS,
  GEARMAN_EOF,
  GEARMAN_MEMORY_ALLOCATION_FAILURE,
  GEARMAN_JOB_EXISTS,
  GEARMAN_WORK_ERROR,
  GEARMAN_WORK_DATA,
  GEARMAN_WORK_STATUS,
  GEARMAN_WORK_FAIL,
  GEARMAN_NOT_CONNECTED,
  GEARMAN_COULD_NOT_CONNECT,
  GEARMAN_SEND_IN_PROGRESS,
  GEARMAN_RECV_IN_PROGRESS,
  GEARMAN_NOT_FLUSHING,
  GEARMAN_DATA_TOO_LARGE,
  GEARMAN_INVALID_FUNCTION_NAME,
  GEARMAN_MAX_RETURN /* Always add new error code before */
} gearman_return_t;

/* Function types. */
typedef gearman_return_t (gearman_workload_fn)(gearman_task_st *task);
typedef gearman_return_t (gearman_created_fn)(gearman_task_st *task);
typedef gearman_return_t (gearman_data_fn)(gearman_task_st *task);
typedef gearman_return_t (gearman_status_fn)(gearman_task_st *task);
typedef gearman_return_t (gearman_complete_fn)(gearman_task_st *task);
typedef gearman_return_t (gearman_fail_fn)(gearman_task_st *task);

typedef void* (gearman_worker_fn)(gearman_job_st *job, void *fn_arg,
                                  size_t *result_size,
                                  gearman_return_t *ret_ptr);

/** @} */

/**
 * @ingroup gearman
 * Options for gearman_st.
 */
typedef enum
{
  GEARMAN_ALLOCATED=    (1 << 0),
  GEARMAN_NON_BLOCKING= (1 << 1)
} gearman_options_t;

/**
 * @ingroup gearman_con
 * Options for gearman_con_st.
 */
typedef enum
{
  GEARMAN_CON_ALLOCATED= (1 << 0)
} gearman_con_options_t;

/**
 * @ingroup gearman_con
 * States for gearman_con_st.
 */
typedef enum
{
  GEARMAN_CON_STATE_ADDRINFO,
  GEARMAN_CON_STATE_CONNECT,
  GEARMAN_CON_STATE_CONNECTING,
  GEARMAN_CON_STATE_CONNECTED
} gearman_con_state_t;

/**
 * @ingroup gearman_con
 * Send states for gearman_con_st.
 */
typedef enum
{
  GEARMAN_CON_SEND_STATE_NONE,
  GEARMAN_CON_SEND_STATE_PRE_FLUSH,
  GEARMAN_CON_SEND_STATE_FORCE_FLUSH,
  GEARMAN_CON_SEND_STATE_FLUSH,
  GEARMAN_CON_SEND_STATE_FLUSH_DATA
} gearman_con_send_state_t;

/**
 * @ingroup gearman_con
 * Recv states for gearman_con_st.
 */
typedef enum
{
  GEARMAN_CON_RECV_STATE_NONE,
  GEARMAN_CON_RECV_STATE_READ,
  GEARMAN_CON_RECV_STATE_READ_DATA
} gearman_con_recv_state_t;

/**
 * @ingroup gearman_packet
 * Options for gearman_packet_st.
 */
typedef enum
{
  GEARMAN_PACKET_ALLOCATED= (1 << 0),
  GEARMAN_PACKET_COMPLETE=  (1 << 1)
} gearman_packet_options_t;

/**
 * @ingroup gearman_packet
 * Magic types.
 */
typedef enum
{
  GEARMAN_MAGIC_NONE,
  GEARMAN_MAGIC_REQUEST,
  GEARMAN_MAGIC_RESPONSE
} gearman_magic_t;

/**
 * @ingroup gearman_packet
 * Command types.
 */
typedef enum
{
  GEARMAN_COMMAND_NONE,
  GEARMAN_COMMAND_CAN_DO,
  GEARMAN_COMMAND_CANT_DO,
  GEARMAN_COMMAND_RESET_ABILITIES,
  GEARMAN_COMMAND_PRE_SLEEP,
  GEARMAN_COMMAND_UNUSED,
  GEARMAN_COMMAND_NOOP,
  GEARMAN_COMMAND_SUBMIT_JOB,
  GEARMAN_COMMAND_JOB_CREATED,
  GEARMAN_COMMAND_GRAB_JOB,
  GEARMAN_COMMAND_NO_JOB,
  GEARMAN_COMMAND_JOB_ASSIGN,
  GEARMAN_COMMAND_WORK_STATUS,
  GEARMAN_COMMAND_WORK_COMPLETE,
  GEARMAN_COMMAND_WORK_FAIL,
  GEARMAN_COMMAND_GET_STATUS,
  GEARMAN_COMMAND_ECHO_REQ,
  GEARMAN_COMMAND_ECHO_RES,
  GEARMAN_COMMAND_SUBMIT_JOB_BG,
  GEARMAN_COMMAND_ERROR,
  GEARMAN_COMMAND_STATUS_RES,
  GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
  GEARMAN_COMMAND_SET_CLIENT_ID,
  GEARMAN_COMMAND_CAN_DO_TIMEOUT,
  GEARMAN_COMMAND_ALL_YOURS,
  GEARMAN_COMMAND_SUBMIT_JOB_SCHED,
  GEARMAN_COMMAND_SUBMIT_JOB_EPOCH,
  GEARMAN_COMMAND_WORK_DATA,
  GEARMAN_COMMAND_MAX /* Always add new commands before this. */
} gearman_command_t;

/**
 * @ingroup gearman_task
 * Options for gearman_task_st.
 */
typedef enum
{
  GEARMAN_TASK_ALLOCATED= (1 << 0)
} gearman_task_options_t;

/**
 * @ingroup gearman_task
 * States for gearman_task_st.
 */
typedef enum
{
  GEARMAN_TASK_STATE_NEW,
  GEARMAN_TASK_STATE_SUBMIT,
  GEARMAN_TASK_STATE_WORKLOAD,
  GEARMAN_TASK_STATE_WORK,
  GEARMAN_TASK_STATE_CREATED,
  GEARMAN_TASK_STATE_DATA,
  GEARMAN_TASK_STATE_STATUS,
  GEARMAN_TASK_STATE_COMPLETE,
  GEARMAN_TASK_STATE_FAIL,
  GEARMAN_TASK_STATE_FINISHED
} gearman_task_state_t;

/**
 * @ingroup gearman_job
 * Options for gearman_job_st.
 */
typedef enum
{
  GEARMAN_JOB_ALLOCATED=   (1 << 0),
  GEARMAN_JOB_WORK_IN_USE= (1 << 1)
} gearman_job_options_t;

/**
 * @ingroup gearman_client
 * Options for gearman_client_st.
 */
typedef enum
{
  GEARMAN_CLIENT_ALLOCATED=      (1 << 0),
  GEARMAN_CLIENT_GEARMAN_STATIC= (1 << 1),
  GEARMAN_CLIENT_TASK_IN_USE=    (1 << 2)
} gearman_client_options_t;

/**
 * @ingroup gearman_client
 * States for gearman_client_st.
 */
typedef enum
{
  GEARMAN_CLIENT_STATE_IDLE,
  GEARMAN_CLIENT_STATE_NEW,
  GEARMAN_CLIENT_STATE_SUBMIT,
  GEARMAN_CLIENT_STATE_PACKET
} gearman_client_state_t;

/**
 * @ingroup gearman_worker
 * Options for gearman_worker_st.
 */
typedef enum
{
  GEARMAN_WORKER_ALLOCATED=      (1 << 0),
  GEARMAN_WORKER_GEARMAN_STATIC= (1 << 1),
  GEARMAN_WORKER_PACKET_IN_USE=  (1 << 2)
} gearman_worker_options_t;

/**
 * @ingroup gearman_worker
 * States for gearman_worker_st.
 */
typedef enum
{
  GEARMAN_WORKER_STATE_INIT,
  GEARMAN_WORKER_STATE_GRAB_JOB,
  GEARMAN_WORKER_STATE_GRAB_JOB_SEND,
  GEARMAN_WORKER_STATE_GRAB_JOB_RECV,
  GEARMAN_WORKER_STATE_GRAB_JOB_NEXT,
  GEARMAN_WORKER_STATE_PRE_SLEEP
} gearman_worker_state_t;

/**
 * @ingroup gearman_worker
 * Work states for gearman_worker_st.
 */
typedef enum
{
  GEARMAN_WORKER_WORK_STATE_GRAB_JOB,
  GEARMAN_WORKER_WORK_STATE_FUNCTION,
  GEARMAN_WORKER_WORK_STATE_COMPLETE,
  GEARMAN_WORKER_WORK_STATE_FAIL
} gearman_worker_work_state_t;

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONSTANTS_H__ */
