/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server declarations
 */

#ifndef __GEARMAN_SERVER_H__
#define __GEARMAN_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server Server Interface
 * This is the interface gearman servers should use.
 *
 * @ref main_page_server "See Main Page for full details."
 * @{
 */

/**
 * Initialize a server structure. This cannot fail if the caller supplies a
 * server structure.
 * @param server Caller allocated server structure, or NULL to allocate one.
 * @return Pointer to an allocated server structure if server parameter was
 *         NULL, or the server parameter pointer if it was not NULL.
 */
gearman_server_st *gearman_server_create(gearman_server_st *server);

/**
 * Clone a server structure.
 * @param server Caller allocated server structure, or NULL to allocate one.
 * @param from Server structure to use as a source to clone from.
 * @return Pointer to an allocated server structure if server parameter was
 *         NULL, or the server parameter pointer if it was not NULL.
 */
gearman_server_st *gearman_server_clone(gearman_server_st *server,
                                        gearman_server_st *from);

/**
 * Free resources used by a server structure.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 */
void gearman_server_free(gearman_server_st *server);

/**
 * Return an error string for the last error encountered.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @return Pointer to static buffer in library that holds an error string.
 */
const char *gearman_server_error(gearman_server_st *server);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @return An errno value as defined in your system errno.h file.
 */
int gearman_server_errno(gearman_server_st *server);

/**
 * Set options for a server structure.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @param options Available options for gearman_server structs.
 * @param data For options that require parameters, the value of that parameter.
 *        For all other option flags, this should be 0 to clear the option or 1
 *        to set.
 */
void gearman_server_set_options(gearman_server_st *server,
                                gearman_server_options_t options,
                                uint32_t data);

/**
 * Set custom I/O event watch callback.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @param event_watch Function to be called when events need to be watched.
 */
void gearman_server_set_event_watch(gearman_server_st *server,
                                    gearman_event_watch_fn *event_watch,
                                    void *event_watch_arg);

/**
 * Add a job server to a server. This goes into a list of servers than can be
 * used to run tasks. No socket I/O happens here, it is just added to a list.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @param fd File descriptor for a newly accepted connection.
 * @param data Application data pointer.
 * @return Gearman connection pointer.
 */
gearman_server_con_st *gearman_server_add_con(gearman_server_st *server,
                                              gearman_server_con_st *server_con,
                                              int fd, void *data);

/**
 * Add connection to the active server list.
 */
void gearman_server_active_list_add(gearman_server_con_st *server_con);

/**
 * Remove connection from the active server list.
 */
void gearman_server_active_list_remove(gearman_server_con_st *server_con);

/**
 * Get next connection from the active server list.
 */
gearman_server_con_st *gearman_server_active_list_next(gearman_server_st *server);

/**
 * Process server connections.
 * @param server Server structure previously initialized with
 *        gearman_server_create or gearman_server_clone.
 * @return Standard gearman return value.
 */
gearman_server_con_st *gearman_server_run(gearman_server_st *server,
                                          gearman_return_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_H__ */
