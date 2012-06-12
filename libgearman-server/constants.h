/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Defines, typedefs, and enums
 */

#pragma once

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <libgearman-1.0/limits.h>
#include <libgearman-1.0/priority.h>
#include <libgearman-server/verbose.h>

/**
 * @addtogroup gearman_server_constants Constants
 * @ingroup gearman_server
 * @{
 */

/* Defines. */
#define GEARMAN_ARGS_BUFFER_SIZE 128
#define GEARMAN_CONF_DISPLAY_WIDTH 80
#define GEARMAN_CONF_MAX_OPTION_SHORT 128
#define GEARMAN_DEFAULT_BACKLOG 64
#define GEARMAN_DEFAULT_MAX_QUEUE_SIZE 0
#define GEARMAN_DEFAULT_SOCKET_RECV_SIZE 32768
#define GEARMAN_DEFAULT_SOCKET_SEND_SIZE 32768
#define GEARMAN_DEFAULT_SOCKET_TIMEOUT 10
#define GEARMAND_JOB_HANDLE_SIZE 64
#define GEARMAND_JOB_HASH_SIZE 383
#define GEARMAN_MAX_COMMAND_ARGS 8
#define GEARMAN_MAX_FREE_SERVER_CLIENT 1000
#define GEARMAN_MAX_FREE_SERVER_CON 1000
#define GEARMAN_MAX_FREE_SERVER_JOB 1000
#define GEARMAN_MAX_FREE_SERVER_PACKET 2000
#define GEARMAN_MAX_FREE_SERVER_WORKER 1000
#define GEARMAN_OPTION_SIZE 64
#define GEARMAN_PACKET_HEADER_SIZE 12
#define GEARMAN_PIPE_BUFFER_SIZE 256
#define GEARMAN_RECV_BUFFER_SIZE 8192
#define GEARMAN_SEND_BUFFER_SIZE 8192
#define GEARMAN_SERVER_CON_ID_SIZE 128
#define GEARMAN_TEXT_RESPONSE_SIZE 8192

/** @} */

/** @} */

typedef enum
{
  GEARMAND_CON_READY,
  GEARMAND_CON_PACKET_IN_USE,
  GEARMAND_CON_EXTERNAL_FD,
  GEARMAND_CON_CLOSE_AFTER_FLUSH,
  GEARMAND_CON_MAX
} gearmand_connection_options_t;


#ifdef __cplusplus

struct gearman_server_thread_st;
struct gearman_server_st;
struct gearman_server_con_st;
struct gearmand_io_st;

#else /* Types. */

typedef struct gearman_server_client_st gearman_server_client_st;
typedef struct gearman_server_con_st gearman_server_con_st;
typedef struct gearman_server_function_st gearman_server_function_st;
typedef struct gearman_server_job_st gearman_server_job_st;
typedef struct gearman_server_packet_st gearman_server_packet_st;
typedef struct gearman_server_st gearman_server_st;
typedef struct gearman_server_thread_st gearman_server_thread_st;
typedef struct gearman_server_worker_st gearman_server_worker_st;
typedef struct gearmand_con_st gearmand_con_st;
typedef struct gearmand_io_st gearmand_io_st;
typedef struct gearmand_port_st gearmand_port_st;
typedef struct gearmand_st gearmand_st;
typedef struct gearmand_thread_st gearmand_thread_st;

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Function types. */
typedef void (gearman_server_thread_run_fn)(gearman_server_thread_st *thread,
                                            void *context);

typedef gearmand_error_t (gearman_queue_add_fn)(gearman_server_st *server,
                                                void *context,
                                                const char *unique,
                                                size_t unique_size,
                                                const char *function_name,
                                                size_t function_name_size,
                                                const void *data,
                                                size_t data_size,
                                                gearman_job_priority_t priority,
                                                int64_t when);

typedef gearmand_error_t (gearman_queue_flush_fn)(gearman_server_st *server,
                                                 void *context);

typedef gearmand_error_t (gearman_queue_done_fn)(gearman_server_st *server,
                                                 void *context,
                                                 const char *unique,
                                                 size_t unique_size,
                                                 const char *function_name,
                                                 size_t function_name_size);
typedef gearmand_error_t (gearman_queue_replay_fn)(gearman_server_st *server,
                                                   void *context,
                                                   gearman_queue_add_fn *add_fn,
                                                   void *add_context);

typedef gearmand_error_t (gearmand_connection_add_fn)(gearman_server_con_st *con);

typedef void (gearman_log_server_fn)(const char *line, gearmand_verbose_t verbose,
                                     struct gearmand_thread_st *context);

typedef gearmand_error_t (gearmand_event_watch_fn)(gearmand_io_st *con,
                                                   short events, void *context);

typedef struct gearmand_packet_st gearmand_packet_st;

typedef void (gearmand_connection_protocol_context_free_fn)(gearman_server_con_st *con,
                                                            void *context);

typedef void (gearmand_log_fn)(const char *line, gearmand_verbose_t verbose,
                              void *context);


/** @} */

/**
 * @addtogroup gearman_server_protocol Protocol Plugins
 * @ingroup gearman_server
 */

/**
 * @addtogroup gearman_server_queue Queue Plugins
 * @ingroup gearman_server
 */

#ifdef __cplusplus
}
#endif
