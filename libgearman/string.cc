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


#include <libgearman/common.h>

#include <cstdlib>
#include <cstring>

#define GEARMAN_BLOCK_SIZE 1024*4

inline static gearman_return_t _string_check(gearman_string_st *string, size_t need)
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
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

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

static inline void _init_string(gearman_string_st *self)
{
  self->current_size= 0;
  self->end= self->string= NULL;
}

gearman_string_st *gearman_string_create(gearman_string_st *self, size_t initial_size)
{
  gearman_return_t rc;

  /* Saving malloc calls :) */
  if (self)
  {
    self->options.is_allocated= false;
  }
  else
  {
    self= static_cast<gearman_string_st *>(malloc(sizeof(gearman_string_st)));

    if (self == NULL)
    {
      return NULL;
    }

    self->options.is_allocated= true;
  }

  _init_string(self);

  rc=  _string_check(self, initial_size);
  if (rc != GEARMAN_SUCCESS)
  {
    free(self);

    return NULL;
  }

  return self;
}

gearman_return_t gearman_string_append_character(gearman_string_st *string,
                                                     char character)
{
  gearman_return_t rc;

  rc=  _string_check(string, 1);

  if (rc != GEARMAN_SUCCESS)
  {
    return rc;
  }

  *string->end= character;
  string->end++;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_string_append(gearman_string_st *string,
                                           const char *value, size_t length)
{
  gearman_return_t rc;

  rc= _string_check(string, length);

  if (rc != GEARMAN_SUCCESS)
  {
    return rc;
  }

  memcpy(string->end, value, length);
  string->end+= length;

  return GEARMAN_SUCCESS;
}

char *gearman_string_c_copy(gearman_string_st *string)
{
  char *c_ptr;

  if (gearman_string_length(string) == 0)
    return NULL;

  c_ptr= static_cast<char *>(malloc((gearman_string_length(string)+1) * sizeof(char)));

  if (c_ptr == NULL)
    return NULL;

  memcpy(c_ptr, gearman_string_value(string), gearman_string_length(string));
  c_ptr[gearman_string_length(string)]= 0;

  return c_ptr;
}

gearman_return_t gearman_string_reset(gearman_string_st *string)
{
  string->end= string->string;

  return GEARMAN_SUCCESS;
}

void gearman_string_free(gearman_string_st *ptr)
{
  if (ptr == NULL)
    return;

  if (ptr->string)
  {
    free(ptr->string);
  }

  if (ptr->options.is_allocated)
  {
    free(ptr);
  }
}

gearman_return_t gearman_string_check(gearman_string_st *string, size_t need)
{
  return _string_check(string, need);
}

size_t gearman_string_length(const gearman_string_st *self)
{
  return size_t(self->end - self->string);
}

size_t gearman_string_size(const gearman_string_st *self)
{
  return self->current_size;
}

const char *gearman_string_value(const gearman_string_st *self)
{
  return self->string;
}

char *gearman_string_value_mutable(const gearman_string_st *self)
{
  return self->string;
}

char *gearman_string_value_take(gearman_string_st *self)
{
  char *ptr= self->string;
  _init_string(self);
  return ptr;
}

void gearman_string_set_length(gearman_string_st *self, size_t length)
{
  self->end= self->string + length;
}
