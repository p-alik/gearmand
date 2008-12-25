/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server Connection declarations
 */

#ifndef __GEARMAN_SERVER_CON_H__
#define __GEARMAN_SERVER_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_con Server Connection Handling
 * This is a low level interface for gearman server connections. This is used
 * internally by the servert interface, so you probably want to there first.
 * @{
 */

/**
 * Initialize a server connection structure.
 */
gearman_server_con_st *gearman_server_con_create(gearman_server_st *server,
                                             gearman_server_con_st *server_con);

/**
 * Free a server_connection structure.
 */
gearman_return_t gearman_server_con_free(gearman_server_con_st *server_con);

/**
 * Get application data pointer.
 */
void *gearman_server_con_data(gearman_server_con_st *server_con);

/**
 * Set application data pointer.
 */
void gearman_server_con_set_data(gearman_server_con_st *server_con, void *data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_CON_H__ */
