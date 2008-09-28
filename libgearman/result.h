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
  gearman_action action;
  gearman_allocated is_allocated;
  gearman_st *root;
  gearman_byte_array_st handle;
  gearman_byte_array_st value;
  uint32_t numerator;
  uint32_t denominator;
};

void gearman_result_free(gearman_result_st *result);
void gearman_result_reset(gearman_result_st *ptr);
gearman_result_st *gearman_result_create(gearman_st *ptr, 
                                         gearman_result_st *result);
char *gearman_result_value(gearman_result_st *ptr);
size_t gearman_result_length(gearman_result_st *ptr);

char *gearman_result_handle(gearman_result_st *ptr);
size_t gearman_result_handle_length(gearman_result_st *ptr);

gearman_return gearman_result_set_value(gearman_result_st *ptr, char *value,
                                        size_t length);
gearman_return gearman_result_set_handle(gearman_result_st *ptr, char *handle,
                                         size_t length);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_RESULT_H__ */
