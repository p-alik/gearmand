/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
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

#ifndef __GEARMAN_TYPES_H__
#define __GEARMAN_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gearman_st gearman_st;
typedef struct gearman_con_st gearman_con_st;
typedef struct gearman_packet_st gearman_packet_st;
typedef struct gearman_command_info_st gearman_command_info_st;
typedef struct gearman_task_st gearman_task_st;
typedef struct gearman_client_st gearman_client_st;
typedef struct gearman_job_st gearman_job_st;
typedef struct gearman_worker_st gearman_worker_st;

typedef gearman_return (gearman_workload_fn)(gearman_task_st *task);
typedef gearman_return (gearman_created_fn)(gearman_task_st *task);
typedef gearman_return (gearman_data_fn)(gearman_task_st *task);
typedef gearman_return (gearman_status_fn)(gearman_task_st *task);
typedef gearman_return (gearman_complete_fn)(gearman_task_st *task);
typedef gearman_return (gearman_fail_fn)(gearman_task_st *task);

typedef void* (gearman_worker_fn)(gearman_job_st *job, void *fn_arg,
                                  const void *workload, size_t workload_size,
                                  size_t *result_size, gearman_return *ret_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_TYPES_H__ */
