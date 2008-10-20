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

#ifndef __GEARMAN_JOB_H__
#define __GEARMAN_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_job_st {
  gearman_action action;
  gearman_allocated is_allocated;
  gearman_st *root;
  gearman_byte_array_st function;
  gearman_byte_array_st unique;
  gearman_byte_array_st value;
  gearman_byte_array_st handle;
  uint32_t flags;
  uint32_t cursor;
};

/* Result Struct */
void gearman_job_free(gearman_job_st *job);
void gearman_job_reset(gearman_job_st *ptr);

gearman_job_st *gearman_job_create(gearman_st *ptr, gearman_job_st *job);

gearman_return gearman_job_submit(gearman_job_st *ptr);
gearman_return gearman_job_result(gearman_job_st *ptr, 
                                  gearman_result_st *result);

gearman_return gearman_job_set_function(gearman_job_st *ptr, char *function);
gearman_return gearman_job_set_value(gearman_job_st *ptr, char *value,
                                     size_t length);

uint64_t gearman_job_get_behavior(gearman_st *ptr, gearman_job_behavior flag);
gearman_return gearman_job_set_behavior(gearman_job_st *ptr, 
                                        gearman_job_behavior flag, 
                                        uint64_t data);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_JOB_H__ */
