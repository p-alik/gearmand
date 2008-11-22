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

#ifndef __GEARMAN_TASK_H__
#define __GEARMAN_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a task structure. */
gearman_task_st *gearman_task_create(gearman_st *gearman,
                                     gearman_task_st *task);

/* Free a task structure. */
void gearman_task_free(gearman_task_st *task);

/* Get task attributes. */
void *gearman_task_fn_arg(gearman_task_st *task);
char *gearman_task_function(gearman_task_st *task);
char *gearman_task_uuid(gearman_task_st *task);
char *gearman_task_job_handle(gearman_task_st *task);
bool gearman_task_is_known(gearman_task_st *task);
bool gearman_task_is_running(gearman_task_st *task);
uint32_t gearman_task_numerator(gearman_task_st *task);
uint32_t gearman_task_denominator(gearman_task_st *task);
size_t gearman_task_data_size(gearman_task_st *task);
size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                              size_t data_size, gearman_return *ret_ptr);

/* Data structures. */
struct gearman_task_st
{
  gearman_st *gearman;
  gearman_task_st *next;
  gearman_task_st *prev;
  gearman_task_options options;
  gearman_task_state state;
  const void *fn_arg;
  gearman_con_st *con;
  uint32_t created_id;
  gearman_packet_st send;
  gearman_packet_st *recv;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_TASK_H__ */
