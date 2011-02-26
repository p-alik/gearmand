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

#include <libgearman-server/io.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gearmand_io_st
{
  struct {
    bool ready;
    bool packet_in_use;
    bool external_fd;
    bool ignore_lost_connection;
    bool close_after_flush;
  } options;
  enum {
    GEARMAN_CON_UNIVERSAL_INVALID,
    GEARMAN_CON_UNIVERSAL_CONNECTED
  } _state;
  enum {
    GEARMAN_CON_SEND_STATE_NONE,
    GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH,
    GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH,
    GEARMAN_CON_SEND_UNIVERSAL_FLUSH,
    GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA
  } send_state;
  enum {
    GEARMAN_CON_RECV_UNIVERSAL_NONE,
    GEARMAN_CON_RECV_UNIVERSAL_READ,
    GEARMAN_CON_RECV_STATE_READ_DATA
  } recv_state;
  short events;
  short revents;
  int fd;
  uint32_t created_id;
  uint32_t created_id_next;
  size_t send_buffer_size;
  size_t send_data_size;
  size_t send_data_offset;
  size_t recv_buffer_size;
  size_t recv_data_size;
  size_t recv_data_offset;
  gearmand_connection_list_st *universal;
  gearmand_io_st *next;
  gearmand_io_st *prev;
  gearmand_con_st *context;
  char *send_buffer_ptr;
  gearmand_packet_st *recv_packet;
  char *recv_buffer_ptr;
  gearmand_packet_st packet;
  gearman_server_con_st *root;
  char send_buffer[GEARMAN_SEND_BUFFER_SIZE];
  char recv_buffer[GEARMAN_RECV_BUFFER_SIZE];
};

/**
 * @addtogroup gearman_server_con Connection Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server connections. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

/*
  Free list for these are stored in gearman_server_thread_st[], otherwise they are owned by gearmand_con_st[]
  */
struct gearman_server_con_st
{
  gearmand_io_st con;
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
  const char *_host; // client host
  const char *_port; // client port
  char id[GEARMAN_SERVER_CON_ID_SIZE];
  struct {
    void *context;
    gearmand_connection_protocol_context_free_fn *context_free_fn;
    gearmand_packet_pack_fn *packet_pack_fn;
    gearmand_packet_unpack_fn *packet_unpack_fn;
  } protocol;
};

/**
 * Add a connection to a server thread. This goes into a list of connections
 * that is used later with server_thread_run, no socket I/O happens here.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_init.
 * @param fd File descriptor for a newly accepted connection.
 * @param data Application data pointer.
 * @return Gearman server connection pointer.
 */
GEARMAN_API
gearman_server_con_st *gearman_server_con_add(gearman_server_thread_st *thread, gearmand_con_st *dcon,
                                              gearman_return_t *ret);

/**
 * Free a server connection structure.
 */
GEARMAN_API
void gearman_server_con_free(gearman_server_con_st *con);

/**
 * Get gearman connection pointer the server connection uses.
 */
GEARMAN_API
gearmand_io_st *gearman_server_con_con(gearman_server_con_st *con);

/**
 * Get application data pointer.
 */
GEARMAN_API
gearmand_con_st *gearman_server_con_data(gearman_server_con_st *con);

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

/**
 * Set protocol context pointer.
 */
GEARMAN_INTERNAL_API
void gearmand_connection_set_protocol(gearman_server_con_st *connection, 
                                      void *context,
                                      gearmand_connection_protocol_context_free_fn *free_fn,
                                      gearmand_packet_pack_fn *pack,
                                      gearmand_packet_unpack_fn *unpack);

GEARMAN_INTERNAL_API
void *gearmand_connection_protocol_context(const gearman_server_con_st *connection);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_CON_H__ */
