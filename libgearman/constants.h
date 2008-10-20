/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
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

/* Public defines */
#define GEARMAN_DEFAULT_PORT 7003
#define GEARMAN_MAX_BUFFER 8192
#define GEARMAN_MAX_HOST_LENGTH 64
#define GEARMAN_WHEEL_SIZE 1024
#define GEARMAN_STRIDE 4
#define GEARMAN_DEFAULT_TIMEOUT INT32_MAX
#define LIBGEARMAN_H_VERSION_STRING "0.19"

#ifdef GEARMAN_INTERNAL 

/* Get a consistent bool type */
#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
  typedef enum {false = 0, true = 1} bool;
#endif

#endif /* GEARMAN_INTERNAL */

typedef enum {
  GEARMAN_SUCCESS,
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
  GEARMAN_MEMORY_ALLOCATION_FAILURE,
  GEARMAN_PARTIAL_READ,
  GEARMAN_SOME_ERRORS,
  GEARMAN_NO_SERVERS,
  GEARMAN_ERRNO,
  GEARMAN_NOT_SUPPORTED,
  GEARMAN_NO_KEY_PROVIDED,
  GEARMAN_FETCH_NOTFINISHED,
  GEARMAN_TIMEOUT,
  GEARMAN_BAD_KEY_PROVIDED,
  /*  End of Failure */
  GEARMAN_BUFFERED,
  GEARMAN_STILL_RUNNING,
  GEARMAN_MAXIMUM_RETURN /* Always add new error code before */
} gearman_return;

typedef enum {
  GEARMAN_DISTRIBUTION_MODULA,
  GEARMAN_DISTRIBUTION_CONSISTENT
} gearman_server_distribution;

typedef enum {
  GEARMAN_BEHAVIOR_NO_BLOCK,
  GEARMAN_BEHAVIOR_TCP_NODELAY,
  GEARMAN_BEHAVIOR_HASH,
  GEARMAN_BEHAVIOR_KETAMA,
  GEARMAN_BEHAVIOR_SOCKET_SEND_SIZE,
  GEARMAN_BEHAVIOR_SOCKET_RECV_SIZE,
  GEARMAN_BEHAVIOR_CACHE_LOOKUPS,
  GEARMAN_BEHAVIOR_SUPPORT_CAS,
  GEARMAN_BEHAVIOR_POLL_TIMEOUT,
  GEARMAN_BEHAVIOR_DISTRIBUTION,
  GEARMAN_BEHAVIOR_BUFFER_REQUESTS,
  GEARMAN_BEHAVIOR_USER_DATA,
  GEARMAN_BEHAVIOR_SORT_HOSTS,
  GEARMAN_BEHAVIOR_VERIFY_KEY,
  GEARMAN_BEHAVIOR_CONNECT_TIMEOUT,
  GEARMAN_BEHAVIOR_RETRY_TIMEOUT
} gearman_behavior;

typedef enum {
  GEARMAN_BEHAVIOR_JOB_BACKGROUND= 1
} gearman_job_behavior;

typedef enum {
  GEARMAN_CALLBACK_USER_DATA,
  GEARMAN_CALLBACK_CLEANUP_FUNCTION,
  GEARMAN_CALLBACK_CLONE_FUNCTION,
  GEARMAN_CALLBACK_MALLOC_FUNCTION,
  GEARMAN_CALLBACK_REALLOC_FUNCTION,
  GEARMAN_CALLBACK_FREE_FUNCTION,
  GEARMAN_CALLBACK_GET_FAILURE,
  GEARMAN_CALLBACK_DELETE_TRIGGER
} gearman_callback;

typedef enum {
  GEARMAN_HASH_DEFAULT= 0,
  GEARMAN_HASH_MD5,
  GEARMAN_HASH_CRC,
  GEARMAN_HASH_FNV1_64,
  GEARMAN_HASH_FNV1A_64,
  GEARMAN_HASH_FNV1_32,
  GEARMAN_HASH_FNV1A_32,
  GEARMAN_HASH_KETAMA,
  GEARMAN_HASH_HSIEH,
  GEARMAN_HASH_MURMUR
} gearman_hash;

typedef enum {
  GEARMAN_NOT_ALLOCATED,
  GEARMAN_ALLOCATED,
  GEARMAN_USED
} gearman_allocated;

typedef enum {
  GEARMAN_SERVER_TYPE_TCP= 0, /* Lease as default for initial state */
  GEARMAN_SERVER_TYPE_INTERNAL
} gearman_server_type;

typedef enum {
  GEARMAN_CONNECTION_STATE_LISTENING= 0, /* Leave as default for initial state */
  GEARMAN_CONNECTION_STATE_READ,
  GEARMAN_CONNECTION_STATE_WRITE,
  GEARMAN_CONNECTION_STATE_CLOSE
} gearman_connection_state;

typedef enum {
  GEARMAN_CAN_DO= 1,           /* W->J: FUNC*/
  GEARMAN_CAN_DO_TIMEOUT= 23,  /* W->J: FUNC[0]TIMEOUT*/
  GEARMAN_CANT_DO= 2,          /* W->J: FUNC*/
  GEARMAN_RESET_ABILITIES= 3,  /* W->J: --*/
  GEARMAN_SET_CLIENT_ID= 22,   /* W->J: [RANDOM_STRING_NO_WHITESPACE]*/
  GEARMAN_PRE_SLEEP= 4,        /* W->J: --*/

  GEARMAN_NOOP= 6,             /* J->W: --*/
  GEARMAN_SUBMIT_JOB= 7,       /* C->J: FUNC[0]UNIQ[0]ARGS*/
  GEARMAN_SUBMIT_JOB_HIGH= 21, /* C->J: FUNC[0]UNIQ[0]ARGS */
  GEARMAN_SUBMIT_JOB_BJ= 18,   /* C->J: FUNC[0]UNIQ[0]ARGS */
  /* 
    Suggested... hate Alan, he suggested CRON (he apologizes) 

    minute        0-59
    hour          0-23
    day of month  1-31
    month         1-12 (or names, see below)
    day of week   0-7 (0 or 7 is Sun, or use names)

  */
  GEARMAN_SUBMIT_JOB_SCHEDULUED= 24,   /* C->J: FUNC[0]UNIQ[0]ARGS[0]MIN[0]HOUR[0]DAY_OF_MONTH[0]MONTH[0]DAY_OF_WEEK */
  GEARMAN_SUBMIT_JOB_FUTURE= 25,   /* C->J: FUNC[0]UNIQ[0]ARGS[0]EPOC */

  GEARMAN_JOB_CREATED= 8,      /* J->C: HANDLE */
  GEARMAN_GRAB_JOB= 9,         /* W->J: -- */
  GEARMAN_NO_JOB= 10,          /* J->W: --*/
  GEARMAN_JOB_ASSIGN= 11,       /* J->W: HANDLE[0]FUNC[0]ARG */

  GEARMAN_WORK_STATUS= 12,     /* W->J/C: HANDLE[0]NUMERATOR[0]DENOMINATOR*/
  GEARMAN_WORK_COMPLETE= 13,   /* W->J/C: HANDLE[0]RES*/
  GEARMAN_WORK_FAIL= 14,       /* W->J/C: HANDLE*/

  GEARMAN_GET_STATUS= 15,      /* C->J: HANDLE*/
  GEARMAN_STATUS_RES= 20,      /* C->J: HANDLE[0]KNOWN[0]RUNNING[0]NUM[0]DENOM*/

  GEARMAN_ECHO_REQ= 16,        /* ?->J: TEXT*/
  GEARMAN_ECHO_RES= 17,        /* J->?: TEXT*/

  GEARMAN_ERROR= 19           /* J->?: ERRCODE[0]ERR_TEXT*/
} gearman_action;


#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONSTANTS_H__ */
