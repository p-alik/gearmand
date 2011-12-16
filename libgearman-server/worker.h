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

#pragma once

#include <libgearman-server/struct/worker.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup gearman_server_worker Worker Declarations @ingroup
 * gearman_server
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
 * Free a server worker structure.
 */
GEARMAN_API
void gearman_server_worker_free(gearman_server_worker_st *worker);

/** @} */

#ifdef __cplusplus
}
#endif
