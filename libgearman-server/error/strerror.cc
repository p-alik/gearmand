/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2010-2012 Data Differential, http://datadifferential.com/
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
 * @brief gearman_strerror()
 */

#include "gear_config.h"
#include <libgearman-server/error/strerror.h>

const char *gearmand_strerror(gearmand_error_t rc)
{
  switch (rc)
  {
  case GEARMAN_SUCCESS:
    return "SUCCESS";
  case GEARMAN_IO_WAIT:
    return "IO_WAIT";
  case GEARMAN_SHUTDOWN:
    return "SHUTDOWN";
  case GEARMAN_SHUTDOWN_GRACEFUL:
    return "SHUTDOWN_GRACEFUL";
  case GEARMAN_ERRNO:
    return "ERRNO";
  case GEARMAN_EVENT:
    return "EVENT";
  case GEARMAN_TOO_MANY_ARGS:
    return "TOO_MANY_ARGS";
  case GEARMAN_NO_ACTIVE_FDS:
    return "NO_ACTIVE_FDS";
  case GEARMAN_INVALID_MAGIC:
    return "INVALID_MAGIC";
  case GEARMAN_INVALID_COMMAND:
    return "INVALID_COMMAND";
  case GEARMAN_INVALID_PACKET:
    return "INVALID_PACKET";
  case GEARMAN_UNEXPECTED_PACKET:
    return "UNEXPECTED_PACKET";
  case GEARMAN_GETADDRINFO:
    return "GETADDRINFO";
  case GEARMAN_NO_SERVERS:
    return "NO_SERVERS";
  case GEARMAN_LOST_CONNECTION:
    return "LOST_CONNECTION";
  case GEARMAN_MEMORY_ALLOCATION_FAILURE:
    return "MEMORY_ALLOCATION_FAILURE";
  case GEARMAN_JOB_EXISTS:
    return "JOB_EXISTS";
  case GEARMAN_JOB_QUEUE_FULL:
    return "JOB_JOB_QUEUE_FULL";
  case GEARMAN_SERVER_ERROR:
    return "SERVER_ERROR";
  case GEARMAN_WORK_ERROR:
    return "WORK_ERROR";
  case GEARMAN_WORK_DATA:
    return "WORK_DATA";
  case GEARMAN_WORK_WARNING:
    return "WORK_WARNING";
  case GEARMAN_WORK_STATUS:
    return "WORK_STATUS";
  case GEARMAN_WORK_EXCEPTION:
    return "WORK_EXCEPTION";
  case GEARMAN_WORK_FAIL:
    return "WORK_FAIL";
  case GEARMAN_NOT_CONNECTED:
    return "NOT_CONNECTED";
  case GEARMAN_COULD_NOT_CONNECT:
    return "COULD_NOT_CONNECT";
  case GEARMAN_SEND_IN_PROGRESS:
    return "SEND_IN_PROGRESS";
  case GEARMAN_RECV_IN_PROGRESS:
    return "RECV_IN_PROGRESS";
  case GEARMAN_NOT_FLUSHING:
    return "NOT_FLUSHING";
  case GEARMAN_DATA_TOO_LARGE:
    return "DATA_TOO_LARGE";
  case GEARMAN_INVALID_FUNCTION_NAME:
    return "INVALID_FUNCTION_NAME";
  case GEARMAN_INVALID_WORKER_FUNCTION:
    return "INVALID_WORKER_FUNCTION";
  case GEARMAN_NO_REGISTERED_FUNCTION:
    return "NO_REGISTERED_FUNCTION";
  case GEARMAN_NO_REGISTERED_FUNCTIONS:
    return "NO_REGISTERED_FUNCTIONS";
  case GEARMAN_NO_JOBS:
    return "NO_JOBS";
  case GEARMAN_ECHO_DATA_CORRUPTION:
    return "ECHO_DATA_CORRUPTION";
  case GEARMAN_NEED_WORKLOAD_FN:
    return "NEED_WORKLOAD_FN";
  case GEARMAN_PAUSE:
    return "PAUSE";
  case GEARMAN_UNKNOWN_STATE:
    return "UNKNOWN_STATE";
  case GEARMAN_PTHREAD:
    return "PTHREAD";
  case GEARMAN_PIPE_EOF:
    return "PIPE_EOF";
  case GEARMAN_QUEUE_ERROR:
    return "QUEUE_ERROR";
  case GEARMAN_FLUSH_DATA:
    return "FLUSH_DATA";
  case GEARMAN_SEND_BUFFER_TOO_SMALL:
    return "SEND_BUFFER_TOO_SMALL";
  case GEARMAN_IGNORE_PACKET:
    return "IGNORE_PACKET";
  case GEARMAN_UNKNOWN_OPTION:
    return "UNKNOWN_OPTION";
  case GEARMAN_TIMEOUT:
    return "TIMEOUT";
  case GEARMAN_ARGUMENT_TOO_LARGE:
    return "The argument was too large for Gearman to handle.";
  case GEARMAN_INVALID_ARGUMENT:
    return "An invalid argument was passed to a function.";
  case GEARMAN_MAX_RETURN:
  default:
    return "Gibberish returned!";
  }
}
