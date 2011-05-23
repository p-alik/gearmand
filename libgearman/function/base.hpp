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

#include <cstring>
#include <memory>

struct _worker_function_st
{
  struct _options {
    bool packet_in_use:1;
    bool change:1;
    bool remove:1;

    _options() :
      packet_in_use(true),
      change(true),
      remove(false)
    { }

  } options;
  struct _worker_function_st *next;
  struct _worker_function_st *prev;
  char *function_name;
  size_t function_length;
  void *context;
  gearman_packet_st packet;

  _worker_function_st(void *context_arg) : 
    next(NULL),
    prev(NULL),
    function_name(NULL),
    function_length(0),
    context(context_arg)
  { }

  virtual bool has_callback() const= 0;

  virtual gearman_worker_error_t callback(gearman_job_st* job, void *context_arg)= 0;

  bool init(const char *name_arg, size_t size)
  {
    function_name= new (std::nothrow) char[size +1];
    if (not function_name)
    {
      return false;
    }

    memcpy(function_name, name_arg, size);
    function_length= size;
    function_name[size]= 0;

    return true;
  }

  const char *name() const
  {
    return function_name;
  }

  size_t length() const
  {
    return function_length;
  }

  virtual ~_worker_function_st()
  {
    if (options.packet_in_use)
      gearman_packet_free(&packet);

    delete [] function_name;
  }
};
