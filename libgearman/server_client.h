/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server client declarations
 */

#ifndef __GEARMAN_SERVER_CLIENT_H__
#define __GEARMAN_SERVER_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_client Server Client Handling
 * This is a low level interface for gearman server clients. This is used
 * internally by the server interface, so you probably want to look there first.
 * @{
 */

/**
 * Add a new client to a server instance.
 */
gearman_server_client_st *
gearman_server_client_add(gearman_server_con_st *server_con);

/**
 * Initialize a server client structure.
 */
gearman_server_client_st *
gearman_server_client_create(gearman_server_con_st *server_con,
                             gearman_server_client_st *server_client);

/**
 * Free a server client structure.
 */
void gearman_server_client_free(gearman_server_client_st *server_client);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_CLIENT_H__ */
