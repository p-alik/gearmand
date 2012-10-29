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

#include "gear_config.h"
#include <libgearman/common.h>

#include "libgearman/assert.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

static void correct_from_errno(gearman_universal_st& universal)
{
  if (universal.error_code() == GEARMAN_ERRNO)
  {
    switch (universal.last_errno())
    {
    case EFAULT:
    case ENOMEM:
      universal.error_code(GEARMAN_MEMORY_ALLOCATION_FAILURE);
      break;

    case EINVAL:
      universal.error_code(GEARMAN_INVALID_ARGUMENT);
      break;

    case ECONNRESET:
    case EHOSTDOWN:
    case EPIPE:
      universal.error_code(GEARMAN_LOST_CONNECTION);
      break;

    case ECONNREFUSED:
    case ENETUNREACH:
    case ETIMEDOUT:
      universal.error_code(GEARMAN_COULD_NOT_CONNECT);
      break;

    default:
      break;
    }
  }
  else
  {
    universal.last_errno(0);
  }
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
gearman_return_t gearman_universal_set_error(gearman_universal_st& universal, 
                                             gearman_return_t rc,
                                             const char *function,
                                             const char *position,
                                             const char *format, ...)
{
  if (rc == GEARMAN_SUCCESS)
  {
    return GEARMAN_SUCCESS;
  }

  va_list args;

  universal._error.rc= rc;
  correct_from_errno(universal);

  char last_error[GEARMAN_MAX_ERROR_SIZE];
  va_start(args, format);
  int length= vsnprintf(last_error, GEARMAN_MAX_ERROR_SIZE, format, args);
  va_end(args);

  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal._error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
  }

  length= snprintf(universal._error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) %s -> %s", function, gearman_strerror(universal._error.rc), last_error, position);
  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal._error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal._error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return universal._error.rc;
}

gearman_return_t gearman_universal_set_gerror(gearman_universal_st& universal, 
                                              gearman_return_t rc,
                                              const char *func,
                                              const char *position)
{
  if (rc == GEARMAN_SUCCESS or rc == GEARMAN_IO_WAIT)
  {
    return rc;
  }

  universal._error.rc= rc;
  correct_from_errno(universal);

  int length= snprintf(universal._error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) -> %s", func, gearman_strerror(rc), position);
  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal._error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal._error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return rc;
}

gearman_return_t gearman_universal_set_perror(gearman_universal_st &universal,
                                              const char *function, const char *position, 
                                              const char *message)
{
  if (errno == 0)
  {
    return GEARMAN_SUCCESS;
  }

  switch (errno)
  {
  case ENOMEM:
    universal._error.rc= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    break;

  default:
    universal._error.rc= GEARMAN_ERRNO;
    break;
  }
  universal._error.last_errno= errno;

  correct_from_errno(universal);

  const char *errmsg_ptr;
  char errmsg[GEARMAN_MAX_ERROR_SIZE]; 
  errmsg[0]= 0; 

#ifdef STRERROR_R_CHAR_P
  errmsg_ptr= strerror_r(universal._error.last_errno, errmsg, sizeof(errmsg));
#else
  strerror_r(universal._error.last_errno, errmsg, sizeof(errmsg));
  errmsg_ptr= errmsg;
#endif

  int length;
  if (message)
  {
    length= snprintf(universal._error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) %s -> %s", function, errmsg_ptr, message, position);
  }
  else
  {
    length= snprintf(universal._error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) -> %s", function, errmsg_ptr, position);
  }

  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal._error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal._error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return universal._error.rc;
}
