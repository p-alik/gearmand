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

#ifndef __GEARMAN_JOB_H__
#define __GEARMAN_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a connection structure. */
gearman_job_st *gearman_job_create(gearman_st *gearman, gearman_job_st *job);

/* Free a connection structure. */
void gearman_job_free(gearman_job_st *job);

/* Send result for a job. */
gearman_return gearman_job_send_result(gearman_job_st *job, uint8_t *result,
                                       size_t result_size);

/* Get job attributes. */
char *gearman_job_uuid(gearman_job_st *job);
char *gearman_job_handle(gearman_job_st *job);
char *gearman_job_function(gearman_job_st *job);
uint8_t *gearman_job_workload(gearman_job_st *job);
size_t gearman_job_workload_size(gearman_job_st *job);
uint8_t *gearman_job_result(gearman_job_st *job);
size_t gearman_job_result_size(gearman_job_st *job);

/* Data structures. */
struct gearman_job_st
{
  gearman_st *gearman;
  gearman_job_st *next;
  gearman_job_st *prev;
  gearman_job_options options;
  gearman_con_st *con;
  char uuid[37];
  uint32_t numerator;
  uint32_t denominator;
  gearman_packet_st packet;
  gearman_packet_st result;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_JOB_H__ */
