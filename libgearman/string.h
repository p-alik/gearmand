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
  Strings are always under our control so we make some assumptions
  about them.

  1) is_initialized is always valid.
  2) A string once intialized will always be, until free where we
     unset this flag.
*/

struct gearman_string_st {
  char *end;
  char *string;
  size_t current_size;
  struct {
    bool is_allocated:1;
  } options;
};

struct gearman_string_t {
  size_t size;
  const char *c_str;
};

#ifdef __cplusplus
extern "C" {
#endif

GEARMAN_LOCAL
gearman_string_st *gearman_string_create(gearman_string_st *string,
                                         size_t initial_size);
GEARMAN_LOCAL
gearman_return_t gearman_string_check(gearman_string_st *string, size_t need);

GEARMAN_LOCAL
char *gearman_string_c_copy(gearman_string_st *string);

GEARMAN_LOCAL
gearman_return_t gearman_string_append_character(gearman_string_st *string,
                                                     char character);
GEARMAN_LOCAL
gearman_return_t gearman_string_append(gearman_string_st *string,
                                           const char *value, size_t length);
GEARMAN_LOCAL
gearman_return_t gearman_string_reset(gearman_string_st *string);

GEARMAN_LOCAL
void gearman_string_free(gearman_string_st *string);

GEARMAN_LOCAL
size_t gearman_string_length(const gearman_string_st *self);

GEARMAN_LOCAL
size_t gearman_string_size(const gearman_string_st *self);

GEARMAN_LOCAL
const char *gearman_string_value(const gearman_string_st *self);

GEARMAN_LOCAL
char *gearman_string_value_mutable(const gearman_string_st *self);

GEARMAN_LOCAL
char *gearman_string_value_take(gearman_string_st *self);

GEARMAN_LOCAL
void gearman_string_set_length(gearman_string_st *self, size_t length);

#ifdef __cplusplus
}
#endif

#ifdef BUILDING_LIBMEMCACHED

#ifdef __cplusplus
#define gearman_string_with_size(X) (X), (static_cast<size_t>((sizeof(X) - 1)))
#define gearman_string_make(X) (static_cast<size_t>((sizeof(X) - 1))), (X)
#else
#define gearman_string_with_size(X) (X), ((size_t)((sizeof(X) - 1)))
#define gearman_string_make(X) (((size_t)((sizeof(X) - 1))), (X)
#endif

#define gearman_string_make_from_cstr(X) (X), ((X) ? strlen(X) : 0)

#endif
