/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief HTTP Protocol Definitions
 */

#include "common.h"

#include <libgearman/protocol_http.h>

/**
 * @addtogroup gearman_protocol_http HTTP Protocol Functions
 * @ingroup gearman_protocol
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT 80

/*
 * Private declarations
 */

/* Protocol callback functions. */
static gearman_return_t _http_con_add(gearman_con_st *con);
static gearman_packet_st *_http_recv(gearman_con_st *con,
                                     gearman_packet_st *packet,
                                     gearman_return_t *ret_ptr, bool recv_data);
static gearman_return_t _http_send(gearman_con_st *con,
                                   gearman_packet_st *packet, bool flush);

/*
 * Public definitions
 */

modconf_return_t gearman_protocol_http_modconf(modconf_st *modconf)
{
  modconf_module_st *module;

  module= gmodconf_module_create(modconf, NULL, "http");
  if (module == NULL)
    return MODCONF_MEMORY_ALLOCATION_FAILURE;

  gmodconf_module_add_option(module, "port", 0, "PORT", "Port to listen on.");

  return gmodconf_return(modconf);
}

gearman_return_t gearmand_protocol_http_init(gearmand_st *gearmand,
                                             modconf_st *modconf)
{
  in_port_t port= GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT;
  modconf_module_st *module;
  const char *name;
  const char *value;

  GEARMAN_INFO(gearmand, "Initializing http module")

  /* Get module and parse the option values that were given. */
  module= gmodconf_module_find(modconf, "http");
  if (module == NULL)
  {
    GEARMAN_FATAL(gearmand,
                  "gearman_protocol_http_init:modconf_module_find:NULL")
    return GEARMAN_QUEUE_ERROR;
  }

  while (gmodconf_module_value(module, &name, &value))
  {
    if (!strcmp(name, "port"))
      port= atoi(value);
    else
    {
      gearmand_protocol_http_deinit(gearmand);
      GEARMAN_FATAL(gearmand, "gearman_protocol_http_init:Unknown argument: %s",
                    name)
      return GEARMAN_QUEUE_ERROR;
    }
  }

  return gearmand_port_add(gearmand, port, _http_con_add);
}

gearman_return_t gearmand_protocol_http_deinit(gearmand_st *gearmand __attribute__ ((unused)))
{
  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearman_return_t _http_con_add(gearman_con_st *con)
{
  gearman_con_set_recv_fn(con, _http_recv);
  gearman_con_set_send_fn(con, _http_send);

  return GEARMAN_SUCCESS;
}

static gearman_packet_st *_http_recv(gearman_con_st *con,
                                     gearman_packet_st *packet,
                                     gearman_return_t *ret_ptr, bool recv_data)
{
con->recv_fn= NULL;
  return gearman_con_recv(con, packet, ret_ptr, recv_data);
}

static gearman_return_t _http_send(gearman_con_st *con,
                                   gearman_packet_st *packet, bool flush)
{
con->send_fn= NULL;
  return gearman_con_send(con, packet, flush);
}
