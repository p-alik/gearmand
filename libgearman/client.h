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

/* Clone a client structure using 'from' as the source. */
gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        gearman_client_st *from);

/* Free a client structure. */
void gearman_client_free(gearman_client_st *client);

/* Reset state for a client structure. */
void gearman_client_reset(gearman_client_st *client);

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

/* Run a task, returns result. */
uint8_t *gearman_client_do(gearman_client_st *client,
                           const char *function_name,
                           const uint8_t *workload,
                           size_t workload_size,
                           size_t *result_size,
                           gearman_return *ret_ptr);

/* Run a high priority task, returns result. */
uint8_t *gearman_client_do_high(gearman_client_st *client,
                                const char *function_name,
                                const uint8_t *workload,
                                size_t workload_size,
                                size_t *result_size,
                                gearman_return *ret_ptr);

/* Run a task in the background, returns job handle. */
char *gearman_client_do_bg(gearman_client_st *client,
                           const char *function_name,
                           const uint8_t *workload,
                           size_t workload_size,
                           gearman_return *ret_ptr);

/* Get the status for a backgound task. */
gearman_return gearman_client_task_status(gearman_client_st *client,
                                          const char *job_handle,
                                          bool *is_known,
                                          bool *is_running,
                                          long *numerator,
                                          long *denominator);

#if 0
/* Run multiple tasks in parallel. */
gearman_return gearman_client_run_tasks(gearman_client_st *client,
                                        gearman_task_st *task_list,
                                        size_t task_list_size);
#endif

/* Send a message to all servers and see if they return it. */
gearman_return gearman_client_echo(gearman_client_st *client,
                                   const uint8_t *message,
                                   size_t message_size);

/* Data structures. */
struct gearman_client_st
{
  gearman_st gearman;
  gearman_client_state state;
  gearman_client_options options;
  gearman_task_st task;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CLIENT_H__ */
