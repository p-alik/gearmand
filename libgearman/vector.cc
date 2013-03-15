/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand String
 *
 *  Copyright (C) 2011-2013 Data Differential, http://datadifferential.com/
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

#include "libgearman/assert.hpp"
#include "libgearman/is.hpp"
#include "libgearman/vector.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>

#include "util/memory.h"
using namespace org::tangent;

inline static bool _string_check(gearman_vector_st *string, const size_t need)
{
  assert_msg(string, "Programmer error, _string_check() was passed a null gearman_vector_st");
  if (string)
  {
    if (need and need > size_t(string->current_size - size_t(string->end - string->string)))
    {
      size_t current_offset= size_t(string->end - string->string);

      /* This is the block multiplier. To keep it larger and surive division errors we must round it up */
      size_t adjust= (need - size_t(string->current_size - size_t(string->end - string->string))) / GEARMAN_VECTOR_BLOCK_SIZE;
      adjust++;

      size_t new_size= sizeof(char) * size_t((adjust * GEARMAN_VECTOR_BLOCK_SIZE) + string->current_size);
      /* Test for overflow */
      if (new_size < need)
      {
        return false;
      }

      char* new_value= static_cast<char *>(realloc(string->string, new_size));
      if (new_value == NULL)
      {
        return false;
      }

      string->string= new_value;
      string->end= string->string + current_offset;

      string->current_size+= (GEARMAN_VECTOR_BLOCK_SIZE * adjust);
    }

    return true;
  }

  return false;
}

void gearman_vector_st::init()
{
  current_size= 0;
  end= string= NULL;
}

gearman_vector_st *gearman_string_create(gearman_vector_st *self, const char *str, size_t str_size)
{
  assert_msg(str, "Programmer error, gearman_string_clear() was passed a null string");
  if (str == NULL)
  {
    return NULL;
  }

  self= gearman_string_create(self, str_size);
  assert_vmsg(self, "Programmer error, gearman_string_create() returned a null gearman_vector_st() requesting a reserve of %u", uint32_t(str_size));
  if (self)
  {
    if ((gearman_string_append(self, str, str_size) == false))
    {
      assert_vmsg(self, "Programmer error, gearman_string_append() returned false while trying to append a string of %u length", uint32_t(str_size));
      gearman_string_free(self);
      return NULL;
    }
  }

  return self;
}

gearman_vector_st::gearman_vector_st(size_t initial_size) :
  end(NULL),
  string(NULL),
  current_size(0)
{
  if (initial_size)
  {
    _string_check(this, initial_size +1);
  }
}

gearman_vector_st *gearman_string_create(gearman_vector_st *self, size_t reserve_)
{
  /* Saving malloc calls :) */
  if (self == NULL)
  {
    self= new (std::nothrow) gearman_vector_st(reserve_);
    assert_vmsg(self, "Programmer error, new gearman_vector_st() failed reserve: %u", uint32_t(reserve_));

    if (self == NULL)
    {
      return NULL;
    }

    gearman_set_allocated(self, true);
  }
  else
  {
    self->clear();
    self->resize(reserve_);
  }
  gearman_set_initialized(self, true);

  assert_vmsg(reserve_ <= self->capacity(), "Programmer error, capacity: %u reserve: %u", uint32_t(self->capacity()), uint32_t(reserve_));
  if (reserve_ > self->capacity())
  {
    gearman_string_free(self);

    return NULL;
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
        if (gearman_string_append(clone, gearman_string_value(self), gearman_string_length(self)) == false)
        {
          gearman_string_free(clone);
          return NULL;
        }
      }
    }
  }

  return clone;
}

bool gearman_string_append_character(gearman_vector_st *string, char character)
{
  assert_msg(string, "Programmer error, gearman_string_append_character() was passed a null gearman_vector_st");
  if (_string_check(string, 1 +1) == false) // Null terminate
  {
    return false;
  }

  *string->end= character;
  string->end++;
  *string->end= 0;

  return true;
}

bool gearman_string_append(gearman_vector_st *string,
                           const char *value, size_t length)
{
  assert_msg(string, "Programmer error, gearman_string_append() was passed a null gearman_vector_st");
  if (_string_check(string, length +1) == false)
  {
    return false;
  }

  memcpy(string->end, value, length);
  string->end+= length;
  *string->end= 0; // Add a NULL

  return true;
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

void gearman_string_clear(gearman_vector_st *string)
{
  assert_msg(string, "Programmer error, gearman_string_clear() was passed a null gearman_vector_st");
  string->clear();
}

gearman_vector_st::~gearman_vector_st()
{
  if (string)
  {
    void* tmp_ptr= string;
    util::free__(tmp_ptr);
  }
}

void gearman_vector_st::resize(const size_t size_)
{
  if (size_ == 0)
  {
    void* tmp_ptr= string;
    util::free__(tmp_ptr);
    init();
  }
  else if (size_ > capacity())
  {
    reserve(size_);
  }
  else if (size_ < capacity())
  {
    size_t final_size= (size_ < size()) ?  size_ : size();
    char* new_value= static_cast<char *>(realloc(string, size_ +1));
    if (new_value == NULL)
    {
      return;
    }
    string= new_value;
    end= string +final_size;
    current_size= size_ +1;
    string[final_size]= 0;
  }
}

void gearman_string_free(gearman_vector_st *string)
{
  if (string)
  {
    if (gearman_is_allocated(string))
    {
      delete string;
      return;
    }

    assert(gearman_is_allocated(string) == false);
    string->resize(0);
    gearman_set_initialized(string, false);
  }
}

bool gearman_string_reserve(gearman_vector_st *string, size_t need_)
{
  if (need_)
  {
    return _string_check(string, need_ +1);
  }

  // Let _string_check handle the behavior of zero
  return _string_check(string, need_);
}

size_t gearman_vector_st::size() const
{
  assert(end >= string);
  return size_t(end -string);
}

void gearman_vector_st::reserve(size_t need_)
{
  if (need_)
  {
    _string_check(this, need_ +1);
  }

  // Let _string_check handle the behavior of zero
  _string_check(this, need_);
}

size_t gearman_string_length(const gearman_vector_st *self)
{
  if (self)
  {
    return self->size();
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
    self->init();
    return passable;
  }

  static gearman_string_t ret= {0, 0};
  return ret;
}
