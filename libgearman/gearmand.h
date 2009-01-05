/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Basic Server declarations
 */

#ifndef __GEARMAND_H__
#define __GEARMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand Default Embedded Server
 * This is a server implementation using the gearman_server interface.
 * @{
 */

/**
 * Create a server instance.
 * @param port Port for the server to listen on.
 * @return Pointer to an allocated gearmand structure.
 */
gearmand_st *gearmand_create(in_port_t port);

/**
 * Free resources used by a server instace.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 */
void gearmand_free(gearmand_st *gearmand);

/**
 * Set socket backlog for listening connection.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 * @param backlog Number of backlog connections to set during listen.
 */
void gearmand_set_backlog(gearmand_st *gearmand, int backlog);

/**
 * Set verbosity level for server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 * @param verbose Verbosity level.
 */
void gearmand_set_verbose(gearmand_st *gearmand, uint8_t verbose);

/**
 * Return an error string for the last error encountered.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 * @return Pointer to static buffer in library that holds an error string.
 */
const char *gearmand_error(gearmand_st *gearmand);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 * @return An errno value as defined in your system errno.h file.
 */
int gearmand_errno(gearmand_st *gearmand);

/**
 * Run the server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 * @return Standard gearman return value.
 */
gearman_return_t gearmand_run(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_H__ */
