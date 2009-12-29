/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Struct Definitions
 */

#ifndef __GEARMAN_SERVER_STRUCTS_H__
#define __GEARMAN_SERVER_STRUCTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman_server
 */
struct gearman_server_st
{
  gearman_server_options_t options;
  bool shutdown;
  bool shutdown_graceful;
  bool proc_wakeup;
  bool proc_shutdown;
  uint8_t job_retries;
  uint8_t worker_wakeup;
  uint32_t job_handle_count;
  uint32_t thread_count;
  uint32_t function_count;
  uint32_t job_count;
  uint32_t unique_count;
  uint32_t free_packet_count;
  uint32_t free_job_count;
  uint32_t free_client_count;
  uint32_t free_worker_count;
  gearman_universal_st *gearman;
  gearman_server_thread_st *thread_list;
  gearman_server_function_st *function_list;
  gearman_server_packet_st *free_packet_list;
  gearman_server_job_st *free_job_list;
  gearman_server_client_st *free_client_list;
  gearman_server_worker_st *free_worker_list;
  gearman_log_fn *log_fn;
  const void *log_context;
  const void *queue_context;
  gearman_queue_add_fn *queue_add_fn;
  gearman_queue_flush_fn *queue_flush_fn;
  gearman_queue_done_fn *queue_done_fn;
  gearman_queue_replay_fn *queue_replay_fn;
  gearman_universal_st gearman_universal_static;
  pthread_mutex_t proc_lock;
  pthread_cond_t proc_cond;
  pthread_t proc_id;
  char job_handle_prefix[GEARMAN_JOB_HANDLE_SIZE];
  gearman_server_job_st *job_hash[GEARMAN_JOB_HASH_SIZE];
  gearman_server_job_st *unique_hash[GEARMAN_JOB_HASH_SIZE];
};

/**
 * @ingroup gearman_server_thread
 */
struct gearman_server_thread_st
{
  gearman_server_thread_options_t options;
  uint32_t con_count;
  uint32_t io_count;
  uint32_t proc_count;
  uint32_t free_con_count;
  uint32_t free_packet_count;
  gearman_universal_st *gearman;
  gearman_server_st *server;
  gearman_server_thread_st *next;
  gearman_server_thread_st *prev;
  gearman_log_fn *log_fn;
  const void *log_context;
  gearman_server_thread_run_fn *run_fn;
  void *run_fn_arg;
  gearman_server_con_st *con_list;
  gearman_server_con_st *io_list;
  gearman_server_con_st *proc_list;
  gearman_server_con_st *free_con_list;
  gearman_server_packet_st *free_packet_list;
  gearman_universal_st gearman_universal_static;
  pthread_mutex_t lock;
};

/**
 * @ingroup gearman_server_con
 */
struct gearman_server_con_st
{
  gearman_connection_st con; /* This must be the first struct member. */
  gearman_server_con_options_t options;
  gearman_return_t ret;
  bool io_list;
  bool proc_list;
  bool proc_removed;
  uint32_t io_packet_count;
  uint32_t proc_packet_count;
  uint32_t worker_count;
  uint32_t client_count;
  gearman_server_thread_st *thread;
  gearman_server_con_st *next;
  gearman_server_con_st *prev;
  gearman_server_packet_st *packet;
  gearman_server_packet_st *io_packet_list;
  gearman_server_packet_st *io_packet_end;
  gearman_server_packet_st *proc_packet_list;
  gearman_server_packet_st *proc_packet_end;
  gearman_server_con_st *io_next;
  gearman_server_con_st *io_prev;
  gearman_server_con_st *proc_next;
  gearman_server_con_st *proc_prev;
  gearman_server_worker_st *worker_list;
  gearman_server_client_st *client_list;
  const char *host;
  const char *port;
  char id[GEARMAN_SERVER_CON_ID_SIZE];
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
  gearman_server_function_options_t options;
  uint32_t worker_count;
  uint32_t job_count;
  uint32_t job_total;
  uint32_t job_running;
  uint32_t max_queue_size;
  size_t function_name_size;
  gearman_server_st *server;
  gearman_server_function_st *next;
  gearman_server_function_st *prev;
  char *function_name;
  gearman_server_worker_st *worker_list;
  gearman_server_job_st *job_list[GEARMAN_JOB_PRIORITY_MAX];
  gearman_server_job_st *job_end[GEARMAN_JOB_PRIORITY_MAX];
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
  uint32_t job_count;
  uint32_t timeout;
  gearman_server_con_st *con;
  gearman_server_worker_st *con_next;
  gearman_server_worker_st *con_prev;
  gearman_server_function_st *function;
  gearman_server_worker_st *function_next;
  gearman_server_worker_st *function_prev;
  gearman_server_job_st *job_list;
};

/**
 * @ingroup gearman_server_job
 */
struct gearman_server_job_st
{
  uint8_t retries;
  gearman_server_job_options_t options;
  gearman_job_priority_t priority;
  uint32_t job_handle_key;
  uint32_t unique_key;
  uint32_t client_count;
  uint32_t numerator;
  uint32_t denominator;
  size_t data_size;
  gearman_server_st *server;
  gearman_server_job_st *next;
  gearman_server_job_st *prev;
  gearman_server_job_st *unique_next;
  gearman_server_job_st *unique_prev;
  gearman_server_job_st *worker_next;
  gearman_server_job_st *worker_prev;
  gearman_server_function_st *function;
  gearman_server_job_st *function_next;
  const void *data;
  gearman_server_client_st *client_list;
  gearman_server_worker_st *worker;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  char unique[GEARMAN_UNIQUE_SIZE];
};

/**
 * @ingroup gearmand
 */
struct gearmand_st
{
  gearmand_options_t options;
  gearman_verbose_t verbose;
  gearman_return_t ret;
  int backlog;
  uint32_t port_count;
  uint32_t threads;
  uint32_t thread_count;
  uint32_t free_dcon_count;
  uint32_t max_thread_free_dcon_count;
  int wakeup_fd[2];
  const char *host;
  gearman_log_fn *log_fn;
  const void *log_context;
  struct event_base *base;
  gearmand_port_st *port_list;
  gearmand_thread_st *thread_list;
  gearmand_thread_st *thread_add_next;
  gearmand_con_st *free_dcon_list;
  gearman_server_st server;
  struct event wakeup_event;
};

/**
 * @ingroup gearmand
 */
struct gearmand_port_st
{
  in_port_t port;
  uint32_t listen_count;
  gearmand_st *gearmand;
  gearman_connection_add_fn *add_fn;
  int *listen_fd;
  struct event *listen_event;
};

/**
 * @ingroup gearmand_thread
 */
struct gearmand_thread_st
{
  gearmand_thread_options_t options;
  uint32_t count;
  uint32_t dcon_count;
  uint32_t dcon_add_count;
  uint32_t free_dcon_count;
  int wakeup_fd[2];
  gearmand_thread_st *next;
  gearmand_thread_st *prev;
  gearmand_st *gearmand;
  struct event_base *base;
  gearmand_con_st *dcon_list;
  gearmand_con_st *dcon_add_list;
  gearmand_con_st *free_dcon_list;
  gearman_server_thread_st server_thread;
  struct event wakeup_event;
  pthread_t id;
  pthread_mutex_t lock;
};

/**
 * @ingroup gearmand_con
 */
struct gearmand_con_st
{
  short last_events;
  int fd;
  gearmand_thread_st *thread;
  gearmand_con_st *next;
  gearmand_con_st *prev;
  gearman_server_con_st *server_con;
  gearman_connection_st *con;
  gearman_connection_add_fn *add_fn;
  struct event event;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
};

/**
 * @ingroup gearman_conf
 */
struct gearman_conf_st
{
  gearman_conf_options_t options;
  gearman_return_t last_return;
  int last_errno;
  size_t module_count;
  size_t option_count;
  size_t short_count;
  gearman_conf_module_st **module_list;
  gearman_conf_option_st *option_list;
  struct option *option_getopt;
  char option_short[GEARMAN_CONF_MAX_OPTION_SHORT];
  char last_error[GEARMAN_MAX_ERROR_SIZE];
};

/**
 * @ingroup gearman_conf
 */
struct gearman_conf_option_st
{
  size_t value_count;
  gearman_conf_module_st *module;
  const char *name;
  const char *value_name;
  const char *help;
  char **value_list;
};

/**
 * @ingroup gearman_conf_module
 */
struct gearman_conf_module_st
{
  gearman_conf_module_options_t options;
  size_t current_option;
  size_t current_value;
  gearman_conf_st *conf;
  const char *name;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_STRUCTS_H__ */
