/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libgearman library
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

#pragma once

/**
  vectors are always under our control so we make some assumptions about them.

  1) is_initialized is always valid.
  2) A string once intialized will always be, until free where we
     unset this flag.
*/
struct gearman_vector_st {
  char *end;
  char *string;
  size_t current_size;
  struct Options {
    bool is_allocated;
    bool is_initialized;
  } options;

  void clear()
  {
    end= string;
  }
};


gearman_vector_st *gearman_string_create(gearman_vector_st *string,
                                         size_t initial_size);


gearman_vector_st *gearman_string_create(gearman_vector_st *self, const char *str, size_t initial_size);

#ifdef __cplusplus
extern "C" {
#endif



gearman_vector_st *gearman_string_clone(const gearman_vector_st *);


gearman_return_t gearman_string_check(gearman_vector_st *string, size_t need);

gearman_return_t gearman_string_reserve(gearman_vector_st *string, size_t need);

char *gearman_string_c_copy(gearman_vector_st *string);


gearman_return_t gearman_string_append_character(gearman_vector_st *string,
                                                 char character);

gearman_return_t gearman_string_append(gearman_vector_st *string,
                                       const char *value, size_t length);

void gearman_string_reset(gearman_vector_st *string);


void gearman_string_free(gearman_vector_st *string);


size_t gearman_string_length(const gearman_vector_st *self);


const char *gearman_string_value(const gearman_vector_st *self);


char *gearman_string_value_mutable(const gearman_vector_st *self);


gearman_string_t gearman_string(const gearman_vector_st *self);


gearman_string_t gearman_string_take_string(gearman_vector_st *self);

#ifdef __cplusplus
}
#endif
