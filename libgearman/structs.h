/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
  gearman_options_t options;
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
  uint32_t sending;
  int last_errno;
  char last_error[GEARMAN_MAX_ERROR_SIZE];
  gearman_event_watch_fn *event_watch;
  void *event_watch_arg;
  gearman_memory_alloc_fn *memory_alloc;
  gearman_memory_free_fn *memory_free;
  void *memory_arg;
};

/**
 * @ingroup gearman_packet
 */
struct gearman_packet_st
{
  gearman_st *gearman;
  gearman_packet_st *next;
  gearman_packet_st *prev;
  gearman_packet_options_t options;
  gearman_magic_t magic;
  gearman_command_t command;
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
  gearman_con_state_t state;
  gearman_con_options_t options;
  void *data;
  char host[NI_MAXHOST];
  in_port_t port;
  struct addrinfo *addrinfo;
  struct addrinfo *addrinfo_next;
  int fd;
  short events;
  short revents;
  short last_revents;
  uint32_t created_id;
  uint32_t created_id_next;
  gearman_packet_st packet;

  gearman_con_send_state_t send_state;
  uint8_t send_buffer[GEARMAN_SEND_BUFFER_SIZE];
  uint8_t *send_buffer_ptr;
  size_t send_buffer_size;
  size_t send_data_size;
  size_t send_data_offset;

  gearman_con_recv_state_t recv_state;
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
  gearman_task_options_t options;
  gearman_task_state_t state;
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
  gearman_job_options_t options;
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
  gearman_client_state_t state;
  gearman_client_options_t options;
  uint32_t new;
  uint32_t running;
  gearman_con_st *con;
  gearman_task_st *task;
  gearman_task_st do_task;
  void *do_data;
  size_t do_data_size;
  bool do_fail;
};

/**
 * @ingroup gearman_worker
 */
struct gearman_worker_st
{
  gearman_st *gearman;
  gearman_st gearman_static;
  gearman_worker_options_t options;
  gearman_worker_state_t state;
  gearman_packet_st packet;
  gearman_packet_st grab_job;
  gearman_packet_st pre_sleep;
  gearman_con_st *con;
  gearman_job_st *job;
  gearman_worker_function_st *function_list;
  uint32_t function_count;
  gearman_worker_work_state_t work_state;
  gearman_job_st work_job;
  gearman_worker_function_st *work_function;
  uint8_t *work_result;
  size_t work_result_size;
};

/**
 * @ingroup gearman_worker
 */
struct gearman_worker_function_st
{
  char *function_name;
  gearman_worker_fn *worker_fn;
  const void *fn_arg;
};

/**
 * @ingroup gearman_server
 */
struct gearman_server_st
{
  gearman_st *gearman;
  gearman_st gearman_static;
  gearman_server_options_t options;
  char job_handle_prefix[GEARMAN_JOB_HANDLE_SIZE];
  uint32_t job_handle_count;
  gearman_server_con_st *con_list;
  uint32_t con_count;
  gearman_server_con_st *active_list;
  uint32_t active_count;
  gearman_server_function_st *function_list;
  uint32_t function_count;
  gearman_server_job_st *job_hash[GEARMAN_JOB_HASH_SIZE];
  uint32_t job_count;
  gearman_server_job_st *unique_hash[GEARMAN_JOB_HASH_SIZE];
  uint32_t unique_count;
  gearman_server_con_st *free_con_list;
  uint32_t free_con_count;
  gearman_server_packet_st *free_packet_list;
  uint32_t free_packet_count;
  gearman_server_job_st *free_job_list;
  uint32_t free_job_count;
  gearman_server_client_st *free_client_list;
  uint32_t free_client_count;
  gearman_server_worker_st *free_worker_list;
  uint32_t free_worker_count;
};

/**
 * @ingroup gearman_server_con
 */
struct gearman_server_con_st
{
  gearman_con_st con; /* This must be the first struct member. */
  gearman_server_st *server;
  gearman_server_con_st *next;
  gearman_server_con_st *prev;
  gearman_server_con_options_t options;
  gearman_server_packet_st *packet_list;
  gearman_server_packet_st *packet_end;
  uint32_t packet_count;
  gearman_server_con_st *active_next;
  gearman_server_con_st *active_prev;
  gearman_server_worker_st *worker_list;
  uint32_t worker_count;
  gearman_server_client_st *client_list;
  uint32_t client_count;
  uint8_t *id;
  size_t id_size;
};

/**
 * @ingroup gearman_server_con
 */
struct gearman_server_packet_st
{
  gearman_packet_st packet;
  gearman_server_packet_st *next;
};

/**
 * @ingroup gearman_server_function
 */
struct gearman_server_function_st
{
  gearman_server_st *server;
  gearman_server_function_st *next;
  gearman_server_function_st *prev;
  gearman_server_function_options_t options;
  char *function_name;
  size_t function_name_size;
  gearman_server_worker_st *worker_list;
  uint32_t worker_count;
  gearman_server_job_st *job_list;
  gearman_server_job_st *job_high_end;
  gearman_server_job_st *job_end;
  uint32_t job_count;
};

/**
 * @ingroup gearman_server_client
 */
struct gearman_server_client_st
{
  gearman_server_client_options_t options;
  gearman_server_con_st *con;
  gearman_server_client_st *con_next;
  gearman_server_client_st *con_prev;
  gearman_server_job_st *job;
  gearman_server_client_st *job_next;
  gearman_server_client_st *job_prev;
};

/**
 * @ingroup gearman_server_worker
 */
struct gearman_server_worker_st
{
  gearman_server_worker_options_t options;
  gearman_server_con_st *con;
  gearman_server_worker_st *con_next;
  gearman_server_worker_st *con_prev;
  gearman_server_function_st *function;
  gearman_server_worker_st *function_next;
  gearman_server_worker_st *function_prev;
  uint32_t timeout;
  gearman_server_job_st *job;
};

/**
 * @ingroup gearman_server_job
 */
struct gearman_server_job_st
{
  gearman_server_st *server;
  gearman_server_job_st *next;
  gearman_server_job_st *prev;
  gearman_server_job_st *unique_next;
  gearman_server_job_st *unique_prev;
  gearman_server_function_st *function;
  gearman_server_job_st *function_next;
  gearman_server_job_options_t options;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint32_t job_handle_key;
  char unique[GEARMAN_UNIQUE_SIZE];
  uint32_t unique_key;
  const void *data;
  size_t data_size;
  gearman_server_client_st *client_list;
  uint32_t client_count;
  gearman_server_worker_st *worker;
  uint32_t numerator;
  uint32_t denominator;
};

#endif /* __GEARMAN_STRUCTS_H__ */
