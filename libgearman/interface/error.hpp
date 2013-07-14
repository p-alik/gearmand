/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2013 Data Differential, http://datadifferential.com/
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

#include <cstdio>

struct error_st {
  error_st():
    _rc(GEARMAN_SUCCESS),
    _last_errno(0)
  {
    _last_error[0]= 0;
  }

  error_st(gearman_return_t rc_):
    _rc(rc_),
    _last_errno(0)
  {
    _last_error[0]= 0;
  }

  void clear()
  {
    _rc= GEARMAN_UNKNOWN_STATE;
    _last_error[0]= 0;
    _last_errno= 0;
  }

  void system_error(const int errno_)
  {
    _last_errno= errno_;
  }

  int system_error() const
  {
    return _last_errno;
  }

  gearman_return_t error_code(const gearman_return_t rc_)
  {
    return _rc= rc_;
  }

  gearman_return_t error_code() const
  {
    return _rc;
  }

  const char* error() const
  {
    if (_last_error[0] == 0)
    {
      return NULL;
    }

    return _last_error;
  }

  const char* error(const char *format__, ...) // __printflike(1, 2);
  {
    if (format__)
    {
      va_list args;

      va_start(args, format__);
      vsnprintf(_last_error, GEARMAN_MAX_ERROR_SIZE, format__, args);
      va_end(args);

      _last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;

      return _last_error;
    }
    _last_error[0]= 0;

    return NULL;
  }

private:
  gearman_return_t _rc;
  int _last_errno;
  char _last_error[GEARMAN_MAX_ERROR_SIZE];

};
