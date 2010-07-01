/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection Declarations
 */

#ifndef __GEARMAN_SERVER_CON_H__
#define __GEARMAN_SERVER_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_con Connection Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server connections. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

struct gearman_server_con_st
{
  gearman_connection_st con; /* This must be the first struct member. */
  bool is_sleeping;
  bool is_exceptions;
  bool is_dead;
  bool is_noop_sent;
  gearman_return_t ret;
  bool io_list;
  bool proc_list;
  bool proc_removed;
  uint32_t io_packet_count;
  uint32_t proc_packet_count;
  uint32_t worker_count;
  uint32_t client_count;
  gearman_server_thread_st *thread;
  gearman_server_con_st *next;
  gearman_server_con_st *prev;
  gearman_server_packet_st *packet;
  gearman_server_packet_st *io_packet_list;
  gearman_server_packet_st *io_packet_end;
  gearman_server_packet_st *proc_packet_list;
  gearman_server_packet_st *proc_packet_end;
  gearman_server_con_st *io_next;
  gearman_server_con_st *io_prev;
  gearman_server_con_st *proc_next;
  gearman_server_con_st *proc_prev;
  gearman_server_worker_st *worker_list;
  gearman_server_client_st *client_list;
  const char *host;
  const char *port;
  char id[GEARMAN_SERVER_CON_ID_SIZE];
};

/**
 * Add a connection to a server thread. This goes into a list of connections
 * that is used later with server_thread_run, no socket I/O happens here.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param fd File descriptor for a newly accepted connection.
 * @param data Application data pointer.
 * @return Gearman server connection pointer.
 */
GEARMAN_API
gearman_server_con_st *gearman_server_con_add(gearman_server_thread_st *thread,
                                              int fd, void *data);

/**
 * Initialize a server connection structure.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_con_create(gearman_server_thread_st *thread);

/**
 * Free a server connection structure.
 */
GEARMAN_API
void gearman_server_con_free(gearman_server_con_st *con);

/**
 * Get gearman connection pointer the server connection uses.
 */
GEARMAN_API
gearman_connection_st *gearman_server_con_con(gearman_server_con_st *con);

/**
 * Get application data pointer.
 */
GEARMAN_API
const void *gearman_server_con_data(const gearman_server_con_st *con);

/**
 * Set application data pointer.
 */
GEARMAN_API
void gearman_server_con_set_data(gearman_server_con_st *con, void *data);

/**
 * Get client host.
 */
GEARMAN_API
const char *gearman_server_con_host(gearman_server_con_st *con);

/**
 * Set client host.
 */
GEARMAN_API
void gearman_server_con_set_host(gearman_server_con_st *con, const char *host);

/**
 * Get client port.
 */
GEARMAN_API
const char *gearman_server_con_port(gearman_server_con_st *con);

/**
 * Set client port.
 */
GEARMAN_API
void gearman_server_con_set_port(gearman_server_con_st *con, const char *port);

/**
 * Get client id.
 */
GEARMAN_API
const char *gearman_server_con_id(gearman_server_con_st *con);

/**
 * Set client id.
 */
GEARMAN_API
void gearman_server_con_set_id(gearman_server_con_st *con, char *id,
                               size_t size);

/**
 * Free server worker struction with name for a server connection.
 */
GEARMAN_API
void gearman_server_con_free_worker(gearman_server_con_st *con,
                                    char *function_name,
                                    size_t function_name_size);

/**
 * Free all server worker structures for a server connection.
 */
GEARMAN_API
void gearman_server_con_free_workers(gearman_server_con_st *con);

/**
 * Add connection to the io thread list.
 */
GEARMAN_API
void gearman_server_con_io_add(gearman_server_con_st *con);

/**
 * Remove connection from the io thread list.
 */
GEARMAN_API
void gearman_server_con_io_remove(gearman_server_con_st *con);

/**
 * Get next connection from the io thread list.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_con_io_next(gearman_server_thread_st *thread);

/**
 * Add connection to the proc thread list.
 */
GEARMAN_API
void gearman_server_con_proc_add(gearman_server_con_st *con);

/**
 * Remove connection from the proc thread list.
 */
GEARMAN_API
void gearman_server_con_proc_remove(gearman_server_con_st *con);

/**
 * Get next connection from the proc thread list.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_con_proc_next(gearman_server_thread_st *thread);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_CON_H__ */
