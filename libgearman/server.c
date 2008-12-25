/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_server_private Private Server Functions
 * @ingroup gearman_server
 * @{
 */

/**
 * Allocate a server structure.
 */
static gearman_server_st *_server_allocate(gearman_server_st *server);

/** @} */

/*
 * Public definitions
 */

gearman_server_st *gearman_server_create(gearman_server_st *server)
{
  server= _server_allocate(server);
  if (server == NULL)
    return NULL;

  server->gearman= gearman_create(&(server->gearman_static));
  if (server->gearman == NULL)
  {
    gearman_server_free(server);
    return NULL;
  }

  gearman_set_options(server->gearman, GEARMAN_NON_BLOCKING, 1);

  return server;
}

gearman_server_st *gearman_server_clone(gearman_server_st *server,
                                        gearman_server_st *from)
{
  if (from == NULL)
    return NULL;

  server= _server_allocate(server);
  if (server == NULL)
    return NULL;

  server->options|= (from->options & ~GEARMAN_SERVER_ALLOCATED);

  server->gearman= gearman_clone(&(server->gearman_static), from->gearman);
  if (server->gearman == NULL)
  {
    gearman_server_free(server);
    return NULL;
  }

  return server;
}

void gearman_server_free(gearman_server_st *server)
{
  gearman_free(server->gearman);

  if (server->options & GEARMAN_SERVER_ALLOCATED)
    free(server);
}

const char *gearman_server_error(gearman_server_st *server)
{
  return gearman_error(server->gearman);
}

int gearman_server_errno(gearman_server_st *server)
{
  return gearman_errno(server->gearman);
}

void gearman_server_set_options(gearman_server_st *server,
                                gearman_server_options_t options,
                                uint32_t data)
{
  if (data)
    server->options |= options;
  else
    server->options &= ~options;
}

void gearman_server_set_event_cb(gearman_server_st *server,
                                 gearman_event_watch_fn *event_watch,
                                 gearman_event_close_fn *event_close,
                                 void *arg)
{
  gearman_set_event_cb(server->gearman, event_watch, event_close, arg);
}

gearman_server_con_st *gearman_server_add_con(gearman_server_st *server,
                                              gearman_server_con_st *server_con,
                                              int fd)
{
  gearman_return_t ret;

  server_con= gearman_server_con_create(server, server_con);
  if (server_con == NULL)
    return NULL;

  if (gearman_con_set_fd(&(server_con->con), fd) != GEARMAN_SUCCESS)
  {
    gearman_server_con_free(server_con);
    return NULL;
  }

  ret= gearman_con_set_events(&(server_con->con), POLLIN);
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_server_con_free(server_con);
    return NULL;
  }

  return server_con;
}

gearman_return_t gearman_server_run(gearman_server_st *server)
{
  gearman_con_st *con;
  gearman_server_con_st *server_con;

  while ((con= gearman_con_ready(server->gearman)) != NULL)
  {
    /* Inherited classes anyone? Some people would call this a hack, I call it
       clean (avoids extra ptrs). Brian, I'll give you your C99 0-byte arrays
       at the ends of structs for this. :) */
    server_con= (gearman_server_con_st *)con;

    switch (server_con->state)
    {
    case GEARMAN_SERVER_CON_STATE_READ:
      break;

    case GEARMAN_SERVER_CON_STATE_WRITE:
      break;
    }
  }

  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearman_server_st *_server_allocate(gearman_server_st *server)
{
  if (server == NULL)
  {
    server= malloc(sizeof(gearman_server_st));
    if (server == NULL)
      return NULL;

    memset(server, 0, sizeof(gearman_server_st));
    server->options|= GEARMAN_SERVER_ALLOCATED;
  }
  else
    memset(server, 0, sizeof(gearman_server_st));

  return server;
}
