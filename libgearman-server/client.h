/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Client Declarations
 */

#ifndef __GEARMAN_SERVER_CLIENT_H__
#define __GEARMAN_SERVER_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman_server_client
 */
struct gearman_server_client_st
{
  struct {
    bool allocated;
  } options;
  gearman_server_con_st *con;
  gearman_server_client_st *con_next;
  gearman_server_client_st *con_prev;
  gearman_server_job_st *job;
  gearman_server_client_st *job_next;
  gearman_server_client_st *job_prev;
};

/**
 * @addtogroup gearman_server_client Client Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server clients. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

/**
 * Add a new client to a server instance.
 */
GEARMAN_API
gearman_server_client_st *
gearman_server_client_add(gearman_server_con_st *con);

/**
 * Initialize a server client structure.
 */
GEARMAN_API
gearman_server_client_st *
gearman_server_client_create(gearman_server_con_st *con,
                             gearman_server_client_st *client);

/**
 * Free a server client structure.
 */
GEARMAN_API
void gearman_server_client_free(gearman_server_client_st *client);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_CLIENT_H__ */
