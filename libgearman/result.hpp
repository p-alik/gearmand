
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

#include "libgearman/assert.hpp"
#include <cstring>

struct gearman_result_st
{
  bool _is_null;
  enum gearman_result_t type;

  union {
    bool boolean;
    int64_t integer;
    gearman_vector_st string;
  } value;

  gearman_result_st();

  explicit gearman_result_st(size_t initial_size);

  bool is_null() const
  {
    return _is_null;
  }

  void reserve(size_t reserve_size)
  {
    if (type == GEARMAN_RESULT_BINARY)
    {
      gearman_string_reserve(&value.string, reserve_size);
    }
    else
    {
      type= GEARMAN_RESULT_BINARY;
      gearman_string_create(&value.string, reserve_size);
      _is_null= true;
    }
  }

  void clear()
  {
    switch (type)
    {
    case GEARMAN_RESULT_BINARY:
      value.string.clear();
      break;

    case GEARMAN_RESULT_INTEGER:
      value.integer= 0;
      break;

    case GEARMAN_RESULT_BOOLEAN:
      value.boolean= false;
      break;
    }

    _is_null= true;
  }

  gearman_vector_st *string()
  {
    if (type == GEARMAN_RESULT_BINARY)
    {
      return &value.string;
    }

    return NULL;
  }

  int64_t integer()
  {
    if (type == GEARMAN_RESULT_INTEGER)
    {
      return value.integer;
    }

    return 0;
  }

  ~gearman_result_st();

private:
  gearman_result_st( const gearman_result_st& );
  const gearman_result_st& operator=( const gearman_result_st& );
};
