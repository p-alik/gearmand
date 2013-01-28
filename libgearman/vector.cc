/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand String
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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
#include <cstring>

#include "util/memory.h"
using namespace org::tangent;

#define GEARMAN_BLOCK_SIZE 1024*4

inline static gearman_return_t _string_check(gearman_vector_st *string, const size_t need)
{
  if (string)
  {
    if (need && need > size_t(string->current_size - size_t(string->end - string->string)))
    {
      size_t current_offset= size_t(string->end - string->string);
      char *new_value;
      size_t adjust;
      size_t new_size;

      /* This is the block multiplier. To keep it larger and surive division errors we must round it up */
      adjust= (need - size_t(string->current_size - size_t(string->end - string->string))) / GEARMAN_BLOCK_SIZE;
      adjust++;

      new_size= sizeof(char) * size_t((adjust * GEARMAN_BLOCK_SIZE) + string->current_size);
      /* Test for overflow */
      if (new_size < need)
      {
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }

      new_value= static_cast<char *>(realloc(string->string, new_size));
      if (new_value == NULL)
      {
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }

      string->string= new_value;
      string->end= string->string + current_offset;

      string->current_size+= (GEARMAN_BLOCK_SIZE * adjust);
    }

    return GEARMAN_SUCCESS;
  }

  return GEARMAN_INVALID_ARGUMENT;
}

static inline void _init_string(gearman_vector_st *self)
{
  self->current_size= 0;
  self->end= self->string= NULL;
}

gearman_vector_st *gearman_string_create(gearman_vector_st *self, const char *str, size_t initial_size)
{
  if (str == NULL)
  {
    return NULL;
  }

  self= gearman_string_create(self, initial_size);
  if (self)
  {
    if (gearman_failed(gearman_string_append(self, str, initial_size)))
    {
      gearman_string_free(self);
      return NULL;
    }
  }

   return self;
}

gearman_vector_st *gearman_string_create(gearman_vector_st *self, size_t initial_size)
{
  /* Saving malloc calls :) */
  if (self)
  {
    gearman_set_allocated(self, false);
  }
  else
  {
    self= static_cast<gearman_vector_st *>(malloc(sizeof(gearman_vector_st)));

    if (self == NULL)
    {
      return NULL;
    }

    gearman_set_allocated(self, true);
  }
  gearman_set_initialized(self, true);

  _init_string(self);

  if (gearman_failed(_string_check(self, initial_size)))
  {
    if (gearman_is_allocated(self))
    {
      void* tmp_ptr= self;
      util::free__(tmp_ptr);
    }
    else
    {
      gearman_set_initialized(self, false);
    }

    return NULL;
  }

  if (initial_size)
  {
    self->string[0]= 0;
  }

  return self;
}

gearman_vector_st *gearman_string_clone(const gearman_vector_st *self)
{
  gearman_vector_st *clone= NULL;
  if (self)
  {
    clone= gearman_string_create(NULL, gearman_string_length(self));

    if (clone)
    {
      if (gearman_string_length(self))
      {
        if (gearman_failed(gearman_string_append(clone, gearman_string_value(self), gearman_string_length(self))))
        {
          gearman_string_free(clone);
          return NULL;
        }
      }
    }
  }

  return clone;
}

gearman_return_t gearman_string_append_character(gearman_vector_st *string, char character)
{
  gearman_return_t rc;


  if (gearman_failed(rc= _string_check(string, 1 +1))) // Null terminate
  {
    return rc;
  }

  *string->end= character;
  string->end++;
  *string->end= 0;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_string_append(gearman_vector_st *string,
                                       const char *value, size_t length)
{
  gearman_return_t rc;

  if (gearman_failed(rc= _string_check(string, length +1)))
  {
    return rc;
  }

  memcpy(string->end, value, length);
  string->end+= length;
  *string->end= 0; // Add a NULL

  return GEARMAN_SUCCESS;
}

char *gearman_string_c_copy(gearman_vector_st *string)
{
  char *c_ptr= NULL;

  if (gearman_string_length(string) == 0)
  {
    c_ptr= static_cast<char *>(malloc((gearman_string_length(string) +1) * sizeof(char)));
    if (c_ptr)
    {
      memcpy(c_ptr, gearman_string_value(string), gearman_string_length(string));
      c_ptr[gearman_string_length(string)]= 0;
    }
  }

  return c_ptr;
}

void gearman_string_reset(gearman_vector_st *string)
{
  assert(string);
  string->clear();
}

void gearman_string_free(gearman_vector_st *ptr)
{
  if (ptr)
  {
    if (ptr->string)
    {
      void* tmp_ptr= ptr->string;
      util::free__(tmp_ptr);
    }

    if (gearman_is_allocated(ptr))
    {
      void* tmp_ptr= ptr;
      util::free__(tmp_ptr);
      return;
    }

    assert(gearman_is_allocated(ptr) == false);
    gearman_set_initialized(ptr, false);
    ptr->string= NULL;
    ptr->end= NULL;
    ptr->current_size= 0;
  }
}

gearman_return_t gearman_string_check(gearman_vector_st *string, size_t need)
{
  return _string_check(string, need);
}

gearman_return_t gearman_string_reserve(gearman_vector_st *string, size_t need)
{
  return _string_check(string, need);
}

size_t gearman_string_length(const gearman_vector_st *self)
{
  if (self)
  {
    return size_t(self->end - self->string);
  }

  return 0;
}

const char *gearman_string_value(const gearman_vector_st *self)
{
  if (self)
  {
    return self->string;
  }

  return NULL;
}

gearman_string_t gearman_string(const gearman_vector_st *self)
{
  assert(self);
  gearman_string_t passable= { gearman_string_value(self), gearman_string_length(self) };
  return passable;
}

gearman_string_t gearman_string_take_string(gearman_vector_st *self)
{
  assert(self);
  if (gearman_string_length(self))
  {
    gearman_string_t passable= gearman_string(self);
    _init_string(self);
    return passable;
  }

  static gearman_string_t ret= {0, 0};
  return ret;
}
