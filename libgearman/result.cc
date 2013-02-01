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

#include "gear_config.h"
#include <libgearman/common.h>

#include "libgearman/assert.hpp"

#include <cstdlib>
#include <limits>
#include <memory>

#include <libgearman/result.hpp>

gearman_result_st::gearman_result_st() :
  _is_null(true),
  type(GEARMAN_RESULT_BOOLEAN)
{
  memset(&value.string, 0, sizeof(gearman_vector_st));
  value.boolean= false;
}

gearman_result_st::gearman_result_st(size_t initial_size) :
  _is_null(true),
  type(GEARMAN_RESULT_BINARY)
{
  gearman_vector_st *allocated_str;
  int limit= 2;
  while (--limit)
  {
    if ((allocated_str= gearman_string_create(&value.string, initial_size)))
    {
      assert_msg(allocated_str == &value.string, "Programmer error, gearman_string_create() is not returning a correct value");
      return;
    }

    assert_vmsg(allocated_str == NULL, "gearman_string_create() failed to allocation an intial string with %lu", (unsigned long)(initial_size));

    // if we fail to allocate on the initial size, try to fail to "something"
    initial_size= 0;
  }

  // We should never reach this point
  assert_msg(allocated_str, "We should never exit with no allocation, most likely something in memory allocation is broken");
}

bool gearman_result_is_null(const gearman_result_st *self)
{
  return self->is_null();
}

gearman_result_st::~gearman_result_st()
{
  if (type == GEARMAN_RESULT_BINARY)
  {
    assert_msg(gearman_is_initialized(&value.string), "Somehow we have a GEARMAN_RESULT_BINARY, but no valid string");
    gearman_string_free(&value.string);
  }
}

int64_t gearman_result_integer(const gearman_result_st *self)
{
  if (self)
  {
    switch (self->type)
    {
    case GEARMAN_RESULT_BINARY:
      return atoll(gearman_string_value(&self->value.string));

    case GEARMAN_RESULT_BOOLEAN:
      return self->value.boolean;

    case GEARMAN_RESULT_INTEGER:
      return self->value.integer;
    }
  }

  return false;
}

bool gearman_result_boolean(const gearman_result_st *self)
{
  if (self)
  {
    switch (self->type)
    {
    case GEARMAN_RESULT_BINARY:
      return gearman_string_length(&self->value.string);

    case GEARMAN_RESULT_BOOLEAN:
      return self->value.boolean;

    case GEARMAN_RESULT_INTEGER:
      return self->value.integer ? true : false;
    }
  }

  return false;
}

size_t gearman_result_size(const gearman_result_st *self)
{
  if (self and self->type == GEARMAN_RESULT_BINARY)
  {
    return gearman_string_length(&self->value.string);
  }

  return 0;
}

const char *gearman_result_value(const gearman_result_st *self)
{
  if (self and self->type == GEARMAN_RESULT_BINARY)
  {
    gearman_string_t ret= gearman_string(&self->value.string);
    return gearman_c_str(ret);
  }

  return NULL;
}

gearman_string_t gearman_result_string(const gearman_result_st *self)
{
  if (not self or self->type != GEARMAN_RESULT_BINARY)
  {
    gearman_string_t ret= {0, 0};
    return ret;
  }

  return gearman_string(&self->value.string);
}

gearman_string_t gearman_result_take_string(gearman_result_st *self)
{
  assert(self); // Programming error
  if (self->type == GEARMAN_RESULT_BINARY and gearman_result_size(self))
  {
    gearman_string_t ret_string= gearman_string_take_string(&self->value.string);
    self->type= GEARMAN_RESULT_BOOLEAN; // Set to default type
    self->_is_null= false;

    return ret_string;
  }

  static gearman_string_t ret= {0, 0};
  return ret;
}

gearman_return_t gearman_result_store_string(gearman_result_st *self, gearman_string_t arg)
{
  return gearman_result_store_value(self, gearman_string_param(arg));
}

gearman_return_t gearman_result_store_value(gearman_result_st *self, const void *value, size_t size)
{
  if (self == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (self->type == GEARMAN_RESULT_BINARY)
  {
    gearman_string_reset(&self->value.string);
  }
  else
  {
    if (gearman_string_create(&self->value.string, size) == NULL)
    {
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }
    self->type= GEARMAN_RESULT_BINARY;
  }
  self->_is_null= false;

  return gearman_string_append(&self->value.string, static_cast<const char *>(value), size);
}

void gearman_result_store_integer(gearman_result_st *self, int64_t value)
{
  if (self)
  {
    if (self->type == GEARMAN_RESULT_BINARY)
    {
      gearman_string_free(&self->value.string);
    }

    self->type= GEARMAN_RESULT_INTEGER;
    self->value.integer= value;
    self->_is_null= false;
  }
}
