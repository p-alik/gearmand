/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Worker Declarations
 */

#ifndef __GEARMAN_SERVER_WORKER_H__
#define __GEARMAN_SERVER_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman_server_worker
 */
struct gearman_server_worker_st
{
  struct {
    bool allocated;
  } options;
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
 * @addtogroup gearman_server_worker Worker Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server workers. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

/**
 * Add a new worker to a server instance.
 */
GEARMAN_API
gearman_server_worker_st *
gearman_server_worker_add(gearman_server_con_st *con, const char *function_name,
                          size_t function_name_size, uint32_t timeout);

/**
 * Initialize a server worker structure.
 */
GEARMAN_API
gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *con,
                             gearman_server_function_st *function,
                             gearman_server_worker_st *worker);

/**
 * Free a server worker structure.
 */
GEARMAN_API
void gearman_server_worker_free(gearman_server_worker_st *worker);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_WORKER_H__ */
