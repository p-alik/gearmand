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

/**
 * @file
 * @brief Struct definitions
 */

#ifndef __GEARMAN_STRUCTS_H__
#define __GEARMAN_STRUCTS_H__

#ifdef __cplusplus
extern "C" {
#endif
 
/**
 * @ingroup gearman
 */
struct gearman_st
{
  gearman_options options;
  gearman_con_st *con_list;
  uint32_t con_count;
  gearman_job_st *job_list;
  uint32_t job_count;
  gearman_task_st *task_list;
  uint32_t task_count;
  gearman_packet_st *packet_list;
  uint32_t packet_count;
  struct pollfd *pfds;
  uint32_t pfds_size;
  gearman_con_st *con_ready;
  uint32_t sending;
  int last_errno;
  char last_error[GEARMAN_ERROR_SIZE];
};

/**
 * @ingroup gearman_packet
 */
struct gearman_packet_st
{
  gearman_st *gearman;
  gearman_packet_st *next;
  gearman_packet_st *prev;
  gearman_packet_options options;
  gearman_magic magic;
  gearman_command command;
  uint8_t argc;
  uint8_t *arg[GEARMAN_MAX_COMMAND_ARGS];
  size_t arg_size[GEARMAN_MAX_COMMAND_ARGS];
  uint8_t *args;
  uint8_t args_buffer[GEARMAN_ARGS_BUFFER_SIZE];
  size_t args_size;
  const void *data;
  size_t data_size;
};

/**
 * @ingroup gearman_packet
 */
struct gearman_command_info_st
{
  const char *name;
  const uint8_t argc;
  const bool data;
};

/**
 * @ingroup gearman_con
 */
struct gearman_con_st
{
  gearman_st *gearman;
  gearman_con_st *next;
  gearman_con_st *prev;
  gearman_con_state state;
  gearman_con_options options;
  void *data;
  char host[NI_MAXHOST];
  in_port_t port;
  struct addrinfo *addrinfo;
  struct addrinfo *addrinfo_next;
  int fd;
  short events;
  short revents;
  uint32_t created_id;
  uint32_t created_id_next;
  gearman_packet_st packet;

  gearman_con_send_state send_state;
  uint8_t send_buffer[GEARMAN_SEND_BUFFER_SIZE];
  uint8_t *send_buffer_ptr;
  size_t send_buffer_size;
  size_t send_data_size;
  size_t send_data_offset;

  gearman_con_recv_state recv_state;
  gearman_packet_st *recv_packet;
  uint8_t recv_buffer[GEARMAN_RECV_BUFFER_SIZE];
  uint8_t *recv_buffer_ptr;
  size_t recv_buffer_size;
  size_t recv_data_size;
  size_t recv_data_offset;
};

/**
 * @ingroup gearman_task
 */
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

/**
 * @ingroup gearman_job
 */
struct gearman_job_st
{
  gearman_st *gearman;
  gearman_job_st *next;
  gearman_job_st *prev;
  gearman_job_options options;
  gearman_con_st *con;
  gearman_packet_st assigned;
  gearman_packet_st work;
};

/**
 * @ingroup gearman_client
 */
struct gearman_client_st
{
  gearman_st *gearman;
  gearman_st gearman_static;
  gearman_client_state state;
  gearman_client_options options;
  uint32_t new;
  uint32_t running;
  gearman_con_st *con;
  gearman_task_st *task;
  gearman_task_st do_task;
  void *do_data;
  size_t do_data_size;
  size_t do_data_offset;
  bool do_fail;
};

/**
 * @ingroup gearman_worker
 */
struct gearman_worker_st
{
  gearman_st *gearman;
  gearman_st gearman_static;
  gearman_worker_options options;
  gearman_worker_state state;
  gearman_packet_st packet;
  gearman_packet_st grab_job;
  gearman_packet_st pre_sleep;
  gearman_con_st *con;
  gearman_job_st *job;
  char *function_name;
  gearman_worker_fn *worker_fn;
  const void *fn_arg;
};

/**
 * @ingroup gearman_worker
 */
struct gearman_worker_list_st
{
  const char *function_name;
  gearman_worker_fn *worker_fn;
  const void *fn_arg;
};

#endif /* __GEARMAN_STRUCTS_H__ */
