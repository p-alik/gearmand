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

#ifndef __GEARMAN_WORKER_H__
#define __GEARMAN_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a worker structure. */
gearman_worker_st *gearman_worker_create(gearman_worker_st *worker);

/* Clone a worker structure using 'from' as the source. */
gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker,
                                        gearman_worker_st *from);

/* Free a worker structure. */
void gearman_worker_free(gearman_worker_st *worker);

/* Return an error string for last library error encountered. */
char *gearman_worker_error(gearman_worker_st *worker);

/* Reset state for a worker structure. */
void gearman_worker_reset(gearman_worker_st *worker);

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_worker_errno(gearman_worker_st *worker);

/* Set options for a library instance structure. */
void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_options options,
                                uint32_t data);

/* Add a job server to a worker. */
gearman_return gearman_worker_server_add(gearman_worker_st *worker, char *host,
                                         in_port_t port);

/* Register function with job servers. */
gearman_return gearman_worker_register_function(gearman_worker_st *worker,
                                                const char *function_name,
                                             gearman_worker_function *function);

/* Get a job from one of the job servers. */
gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return *ret);

/* Go into a loop and answer a single job. */
gearman_return gearman_worker_work(gearman_worker_st *worker);

/* Data structures. */
struct gearman_worker_st
{
  gearman_st gearman;
  gearman_worker_options options;
  gearman_worker_state state;
  gearman_packet_st packet;
  gearman_packet_st grab_job;
  gearman_packet_st pre_sleep;
  gearman_con_st *con;
  gearman_job_st *job;
  char *function_name;
  gearman_worker_function *function;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_WORKER_H__ */
