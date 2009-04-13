/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Connection Declarations
 */

#ifndef __GEARMAND_CON_H__
#define __GEARMAND_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand_con Gearmand Connections
 * Connection handling for gearmand.
 * @{
 */

/**
 * Create a new gearmand connection.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param fd File descriptor of new connection.
 * @param host Host of peer connection.
 * @param port Port of peer connection.
 * @return Pointer to an allocated gearmand structure.
 */
gearman_return_t gearmand_con_create(gearmand_st *gearmand, int fd,
                                     const char *host, const char *port);

/**
 * Free resources used by a connection.
 * @param dcon Connection previously initialized with gearmand_con_create.
 */
void gearmand_con_free(gearmand_con_st *dcon);

/**
 * Callback function used for setting events in libevent.
 */
gearman_return_t gearmand_con_watch(gearman_con_st *con, short events,
                                    void *arg __attribute__ ((unused)));

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_CON_H__ */
