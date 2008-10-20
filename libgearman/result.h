/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GEARMAN_RESULT_H__
#define __GEARMAN_RESULT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
  gearman_result_st are used to internally represent the return values from
  gearman. We use a structure so that long term as identifiers are added
  to gearman we will be able to absorb new attributes without having 
  to addjust the entire API. 
*/ 

struct gearman_result_st {
  bool is_allocated;
  gearman_action action;
  gearman_byte_array_st *handle;
  gearman_byte_array_st *value;
  /* The following are just used for job status returns */
  bool is_known;
  bool is_running;
  long numerator;
  long denominator;
};

gearman_result_st *gearman_result_create(gearman_result_st *ptr);
gearman_return gearman_result_value_store(gearman_result_st *ptr, const uint8_t *value, ssize_t length);
gearman_return gearman_result_handle_store(gearman_result_st *ptr, const uint8_t *handle, ssize_t length);
void gearman_result_free(gearman_result_st *ptr);
uint8_t *gearman_result_value(gearman_result_st *result, ssize_t *length);
ssize_t gearman_result_value_length(gearman_result_st *ptr);
uint8_t *gearman_result_handle(gearman_result_st *result, ssize_t *length);
ssize_t gearman_result_handle_length(gearman_result_st *ptr);


#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_RESULT_H__ */
