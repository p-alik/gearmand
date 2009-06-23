/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server function declarations
 */

#ifndef __GEARMAN_SERVER_FUNCTION_H__
#define __GEARMAN_SERVER_FUNCTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_function Server Function Handling
 * @ingroup gearman_server
 * This is a low level interface for gearman server functions. This is used
 * internally by the server interface, so you probably want to look there first.
 * @{
 */

/**
 * Add a new function to a server instance.
 */
GEARMAN_API
gearman_server_function_st *
gearman_server_function_get(gearman_server_st *server,
                            const char *function_name,
                            size_t function_name_size);

/**
 * Initialize a server function structure.
 */
GEARMAN_API
gearman_server_function_st *
gearman_server_function_create(gearman_server_st *server,
                               gearman_server_function_st *function);

/**
 * Free a server function structure.
 */
GEARMAN_API
void gearman_server_function_free(gearman_server_function_st *function);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_FUNCTION_H__ */
