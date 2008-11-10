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

#ifndef __GEARMAN_CLIENT_H__
#define __GEARMAN_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a client structure. */
gearman_client_st *gearman_client_create(gearman_client_st *client);

/* Free a client structure. */
void gearman_client_free(gearman_client_st *client);

/* Return an error string for last library error encountered. */
char *gearman_client_error(gearman_client_st *client);

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_client_errno(gearman_client_st *client);

/* Set options for a library instance structure. */
void gearman_client_set_options(gearman_client_st *client,
                                gearman_options options,
                                uint32_t data);

/* Add a job server to a client. */
gearman_return gearman_client_server_add(gearman_client_st *client, char *host,
                                         in_port_t port);

/* Run a job. */
gearman_job_st *gearman_client_do(gearman_client_st *client,
                                  gearman_job_st *job, char *function_name, 
                                  uint8_t *workload, size_t workload_size,
                                  gearman_return *ret);

#if 0
char *gearman_client_do_background(gearman_client_st *client,
                                   const char *function_name,
                                   const uint8_t *workload, ssize_t workload_size,
                                   gearman_return *error);

/* Echo test */
gearman_return gearman_client_echo(gearman_client_st *client,
                                   const char *message, ssize_t message_length);

/* Status methods for Background Jobs */
gearman_return gearman_client_job_status(gearman_client_st *client,
                                         const char *job_id,
                                         bool *is_known,
                                         bool *is_running,
                                         long *numerator,
                                         long *denominator);
#endif

/* Data structures. */
struct gearman_client_st
{
  gearman_st gearman;
  gearman_client_options options;
  gearman_client_state state;
  gearman_job_st *job;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CLIENT_H__ */
