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
 * @addtogroup gearmand Basic Server
 * This is a basic server implementation using the gearman_server interface.
 * @{
 */

/**
 * Initialize a server instance.
 * @param port Port for the server to listen on.
 * @param backlog Number of backlog connections to set during listen.
 * @return Pointer to an allocated gearmand structure.
 */
gearmand_st *gearmand_init(in_port_t port, int backlog);

/**
 * Free resources used by a server instace.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_init.
 */
void gearmand_destroy(gearmand_st *gearmand);

/**
 * Run the server instance.
 * @param server Server instance structure previously initialized with
 *        gearmand_init.
 */
void gearmand_run(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_H__ */
