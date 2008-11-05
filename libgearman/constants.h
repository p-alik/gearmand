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

#ifndef __GEARMAN_CONSTANTS_H__
#define __GEARMAN_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines. */
#define GEARMAN_DEFAULT_TCP_HOST "127.0.0.1"
#define GEARMAN_DEFAULT_TCP_PORT 7003
#define GEARMAN_DEFAULT_SOCKET_TIMEOUT 10
#define GEARMAN_DEFAULT_SOCKET_SEND_SIZE 32768
#define GEARMAN_DEFAULT_SOCKET_RECV_SIZE 32768
#define GEARMAN_PACKET_BUFFER_SIZE 64
#define GEARMAN_READ_BUFFER_SIZE 8192
#define GEARMAN_ERROR_SIZE 1024
#define GEARMAN_MAX_COMMAND_ARGS 8
#if 0
#define GEARMAN_WHEEL_SIZE 1024
#define GEARMAN_STRIDE 4
#define GEARMAN_DEFAULT_TIMEOUT INT32_MAX
#define LIBGEARMAN_H_VERSION_STRING "0.19"
#endif

/* Return codes. */
typedef enum
{
  GEARMAN_SUCCESS,
  GEARMAN_IO_WAIT,
  GEARMAN_ERRNO,
  GEARMAN_TOO_MANY_ARGS,
  GEARMAN_INVALID_MAGIC,
  GEARMAN_INVALID_COMMAND,
  GEARMAN_INVALID_SIZE,
  GEARMAN_INVALID_ARGS,
  GEARMAN_INVALID_PACKET,
  GEARMAN_GETADDRINFO,
  GEARMAN_NO_SERVERS,
  GEARMAN_EOF,
  GEARMAN_MEMORY_ALLOCATION_FAILURE,
  GEARMAN_MAX_RETURN /* Always add new error code before */
#if 0
  GEARMAN_NOT_FOUND,
  /* Failure types */
  GEARMAN_FAILURE,
  GEARMAN_HOST_LOOKUP_FAILURE,
  GEARMAN_CONNECTION_FAILURE,
  GEARMAN_CONNECTION_BIND_FAILURE,
  GEARMAN_WRITE_FAILURE,
  GEARMAN_READ_FAILURE,
  GEARMAN_UNKNOWN_READ_FAILURE,
  GEARMAN_PROTOCOL_ERROR,
  GEARMAN_CLIENT_ERROR,
  GEARMAN_SERVER_ERROR,
  GEARMAN_CONNECTION_SOCKET_CREATE_FAILURE,
  GEARMAN_PARTIAL_READ,
  GEARMAN_SOME_ERRORS,
  GEARMAN_NO_SERVERS,
  GEARMAN_NOT_SUPPORTED,
  GEARMAN_NO_KEY_PROVIDED,
  GEARMAN_FETCH_NOTFINISHED,
  GEARMAN_TIMEOUT,
  GEARMAN_BAD_KEY_PROVIDED,
  /*  End of Failure */
  GEARMAN_BUFFERED,
  GEARMAN_STILL_RUNNING,
#endif
} gearman_return;

/* Options for gearman_st. */
typedef enum
{
  GEARMAN_ALLOCATED=    (1 << 0),
  GEARMAN_NON_BLOCKING= (1 << 1)
} gearman_options;

/* Options for gearman_client_st. */
typedef enum
{
  GEARMAN_CLIENT_ALLOCATED= (1 << 0)
} gearman_client_options;

/* States for gearman_client_st. */
typedef enum
{
  GEARMAN_CLIENT_STATE_INIT,
  GEARMAN_CLIENT_STATE_SUBMIT_JOB,
  GEARMAN_CLIENT_STATE_JOB_CREATED,
  GEARMAN_CLIENT_STATE_RESULT
} gearman_client_state;

/* Options for gearman_con_st. */
typedef enum
{
  GEARMAN_CON_ALLOCATED= (1 << 0)
} gearman_con_options;

/* States for gearman_con_st. */
typedef enum
{
  GEARMAN_CON_STATE_ADDRINFO,
  GEARMAN_CON_STATE_CONNECT,
  GEARMAN_CON_STATE_CONNECTING,
  GEARMAN_CON_STATE_IDLE,
  GEARMAN_CON_STATE_SEND,
  GEARMAN_CON_STATE_RECV
} gearman_con_state;

/* Options for gearman_job_st. */
typedef enum
{
  GEARMAN_JOB_ALLOCATED= (1 << 0)
} gearman_job_options;

/* Options for gearman_packet_st. */
typedef enum
{
  GEARMAN_PACKET_ALLOCATED= (1 << 0),
  GEARMAN_PACKET_PACKED=    (1 << 1)
} gearman_packet_options;

/* Options for gearman_worker_st. */
typedef enum
{
  GEARMAN_WORKER_ALLOCATED=     (1 << 0),
  GEARMAN_WORKER_PACKET_IN_USE= (1 << 1)
} gearman_worker_options;

/* States for gearman_worker_st. */
typedef enum
{
  GEARMAN_WORKER_STATE_INIT,
  GEARMAN_WORKER_STATE_GRAB_JOB,
  GEARMAN_WORKER_STATE_GRAB_JOB_SEND,
  GEARMAN_WORKER_STATE_GRAB_JOB_RECV,
  GEARMAN_WORKER_STATE_GRAB_JOB_NEXT,
  GEARMAN_WORKER_STATE_PRE_SLEEP
} gearman_worker_state;

/* Magic types. */
typedef enum
{
  GEARMAN_MAGIC_NONE,
  GEARMAN_MAGIC_REQUEST,
  GEARMAN_MAGIC_RESPONSE
} gearman_magic;

/* Command types. */
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
  GEARMAN_COMMAND_SUBMIT_JOB_BJ,
  GEARMAN_COMMAND_ERROR,
  GEARMAN_COMMAND_STATUS_RES,
  GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
  GEARMAN_COMMAND_SET_CLIENT_ID,
  GEARMAN_COMMAND_CAN_DO_TIMEOUT,
  GEARMAN_COMMAND_ALL_YOURS,
  GEARMAN_COMMAND_SUBMIT_JOB_SCHED,
  GEARMAN_COMMAND_SUBMIT_JOB_EPOCH,
  GEARMAN_COMMAND_MAX /* Always add new commands before this. */
} gearman_command;

#if 0
typedef enum {
  GEARMAN_BEHAVIOR_JOB_BACKGROUND= 1
} gearman_job_behavior;

typedef enum {
  GEARMAN_SERVER_TYPE_TCP= 0, /* Lease as default for initial state */
  GEARMAN_SERVER_TYPE_INTERNAL
} gearman_server_type;

typedef enum {
  GEARMAN_CONNECTION_STATE_LISTENING= 0,
  GEARMAN_CONNECTION_STATE_READ,
  GEARMAN_CONNECTION_STATE_WRITE,
  GEARMAN_CONNECTION_STATE_CLOSE
} gearman_connection_state;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONSTANTS_H__ */
