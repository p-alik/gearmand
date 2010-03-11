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

#ifndef __GEARMAN_SERVER_CONSTANTS_H__
#define __GEARMAN_SERVER_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_constants Constants
 * @ingroup gearman_server
 * @{
 */

/* Defines. */
#define GEARMAN_DEFAULT_BACKLOG 64
#define GEARMAN_DEFAULT_MAX_QUEUE_SIZE 0
#define GEARMAN_SERVER_CON_ID_SIZE 128
#define GEARMAN_JOB_HASH_SIZE 383
#define GEARMAN_MAX_FREE_SERVER_CON 1000
#define GEARMAN_MAX_FREE_SERVER_PACKET 2000
#define GEARMAN_MAX_FREE_SERVER_JOB 1000
#define GEARMAN_MAX_FREE_SERVER_CLIENT 1000
#define GEARMAN_MAX_FREE_SERVER_WORKER 1000
#define GEARMAN_TEXT_RESPONSE_SIZE 8192
#define GEARMAN_PIPE_BUFFER_SIZE 256
#define GEARMAN_CONF_MAX_OPTION_SHORT 128
#define GEARMAN_CONF_DISPLAY_WIDTH 80

/** @} */

/**
 * @ingroup gearman_server
 * Options for gearman_server_st.
 */
typedef enum
{
  GEARMAN_SERVER_ALLOCATED=    (1 << 0),
  GEARMAN_SERVER_PROC_THREAD=  (1 << 1),
  GEARMAN_SERVER_QUEUE_REPLAY= (1 << 2),
  GEARMAN_SERVER_RR_ORDER=     (1 << 3)
} gearman_server_options_t;

/**
 * @ingroup gearman_server_con
 * Options for gearman_server_con_st.
 */
typedef enum
{
  GEARMAN_SERVER_CON_SLEEPING=   (1 << 0),
  GEARMAN_SERVER_CON_EXCEPTIONS= (1 << 1),
  GEARMAN_SERVER_CON_DEAD=       (1 << 2),
  GEARMAN_SERVER_CON_NOOP_SENT=  (1 << 3)
} gearman_server_con_options_t;

/**
 * @ingroup gearman_server_job
 * Options for gearman_server_job_st.
 */
typedef enum
{
  GEARMAN_SERVER_JOB_ALLOCATED= (1 << 0),
  GEARMAN_SERVER_JOB_QUEUED=    (1 << 1),
  GEARMAN_SERVER_JOB_IGNORE=    (1 << 2)
} gearman_server_job_options_t;

/**
 * @ingroup gearmand
 * Options for gearmand_st.
 */
typedef enum
{
  GEARMAND_LISTEN_EVENT= (1 << 0),
  GEARMAND_WAKEUP_EVENT= (1 << 1)
} gearmand_options_t;

/**
 * @ingroup gearmand
 * Wakeup events for gearmand_st.
 */
typedef enum
{
  GEARMAND_WAKEUP_PAUSE=             (1 << 0),
  GEARMAND_WAKEUP_SHUTDOWN=          (1 << 1),
  GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL= (1 << 2),
  GEARMAND_WAKEUP_CON=               (1 << 3),
  GEARMAND_WAKEUP_RUN=               (1 << 4)
} gearmand_wakeup_t;

/**
 * @ingroup gearmand_thread
 * Options for gearmand_thread_st.
 */
typedef enum
{
  GEARMAND_THREAD_WAKEUP_EVENT= (1 << 0),
  GEARMAND_THREAD_LOCK=         (1 << 1)
} gearmand_thread_options_t;

/**
 * @ingroup gearman_conf_module
 * Options for gearman_conf_module_st.
 */
typedef enum
{
  GEARMAN_CONF_MODULE_ALLOCATED= (1 << 0)
} gearman_conf_module_options_t;


/**
 * @addtogroup gearman_server_types Types
 * @ingroup gearman_server
 * @{
 */

/* Types. */
typedef struct gearman_server_st gearman_server_st;
typedef struct gearman_server_thread_st gearman_server_thread_st;
typedef struct gearman_server_con_st gearman_server_con_st;
typedef struct gearman_server_packet_st gearman_server_packet_st;
typedef struct gearman_server_function_st gearman_server_function_st;
typedef struct gearman_server_client_st gearman_server_client_st;
typedef struct gearman_server_worker_st gearman_server_worker_st;
typedef struct gearman_server_job_st gearman_server_job_st;
typedef struct gearmand_st gearmand_st;
typedef struct gearmand_port_st gearmand_port_st;
typedef struct gearmand_con_st gearmand_con_st;
typedef struct gearmand_thread_st gearmand_thread_st;
typedef struct gearman_conf_st gearman_conf_st;
typedef struct gearman_conf_option_st gearman_conf_option_st;
typedef struct gearman_conf_module_st gearman_conf_module_st;

/* Function types. */
typedef void (gearman_server_thread_run_fn)(gearman_server_thread_st *thread,
                                            void *context);

typedef gearman_return_t (gearman_queue_add_fn)(gearman_server_st *server,
                                                void *context,
                                                const void *unique,
                                                size_t unique_size,
                                                const void *function_name,
                                                size_t function_name_size,
                                                const void *data,
                                                size_t data_size,
                                               gearman_job_priority_t priority);
typedef gearman_return_t (gearman_queue_flush_fn)(gearman_server_st *server,
                                                  void *context);
typedef gearman_return_t (gearman_queue_done_fn)(gearman_server_st *server,
                                                 void *context,
                                                 const void *unique,
                                                 size_t unique_size,
                                                 const void *function_name,
                                                 size_t function_name_size);
typedef gearman_return_t (gearman_queue_replay_fn)(gearman_server_st *server,
                                                   void *context,
                                                   gearman_queue_add_fn *add_fn,
                                                   void *add_context);

typedef gearman_return_t (gearman_connection_add_fn)(gearman_connection_st *con);

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

#endif /* __GEARMAN_SERVER_CONSTANTS_H__ */
