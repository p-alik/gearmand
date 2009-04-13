/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Connection Definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_con_private Private Gearmand Connection Functions
 * @ingroup gearmand_con
 * @{
 */

static void _con_ready(int fd __attribute__ ((unused)), short events,
                       void *arg);

#if 0
static void _event_del_all(gearmand_st *gearmand);
#endif

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearmand_con_create(gearmand_st *gearmand, int fd,
                                     const char *host, const char *port)
{
  gearmand_con_st *dcon;

  if (gearmand->free_dcon_count > 0)
  {
    dcon= gearmand->free_dcon_list;
    GEARMAN_LIST_DEL(gearmand->free_dcon, dcon,)
  }
  else
  {
    dcon= malloc(sizeof(gearmand_con_st));
    if (dcon == NULL)
    {
      GEARMAND_ERROR_SET(gearmand, "gearmand_con_create", "malloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }
  }

  memset(dcon, 0, sizeof(gearmand_con_st));
  dcon->gearmand= gearmand;
  dcon->fd= fd;
  strcpy(dcon->host, host);
  strcpy(dcon->port, port);

  if (gearman_server_add_con(&(gearmand->server), &(dcon->server_con), dcon->fd,
                             dcon) == NULL)
  {
    close(dcon->fd);
    free(dcon);
    /*_event_del_all(gearmand);*/
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearman_server_con_set_addr(&(dcon->server_con), host);

  if (gearmand->verbose > 0)
  {
    printf("%15s:%5s Connected (%u current, %u total)\n", dcon->host,
           dcon->port, gearmand->dcon_count + 1, gearmand->dcon_total + 1);
  }

  GEARMAN_LIST_ADD(gearmand->dcon, dcon,)
  gearmand->dcon_total++;

  return GEARMAN_SUCCESS;
}

void gearmand_con_free(gearmand_con_st *dcon)
{
  if (dcon->gearmand->verbose > 0)
  {
    if (dcon->gearmand->ret != GEARMAN_LOST_CONNECTION)
    {
      printf("%15s:%5s %s\n", dcon->host, dcon->port,
             gearman_server_error(&(dcon->gearmand->server)));
    }

    printf("%15s:%5s Disconnected\n", dcon->host, dcon->port);
  }

  gearman_server_con_free(&(dcon->server_con));
  assert(event_del(&(dcon->event)) == 0);

  /* This gets around a libevent bug when both POLLIN and POLLOUT are set. */
  event_set(&(dcon->event), dcon->fd, EV_READ, _con_ready, dcon);
  event_base_set(dcon->gearmand->base, &(dcon->event));
  event_add(&(dcon->event), NULL);
  assert(event_del(&(dcon->event)) == 0);

  GEARMAN_LIST_DEL(dcon->gearmand->dcon, dcon,)
  close(dcon->fd);
  if (dcon->gearmand->free_dcon_count < GEARMAN_MAX_FREE_SERVER_CON)
    GEARMAN_LIST_ADD(dcon->gearmand->free_dcon, dcon,)
  else
    free(dcon);
}

gearman_return_t gearmand_con_watch(gearman_con_st *con, short events,
                                    void *arg __attribute__ ((unused)))
{
  gearmand_con_st *dcon;
  short set_events= 0;

  dcon= (gearmand_con_st *)gearman_con_data(con);
  dcon->con= con;

  if (events & POLLIN)
    set_events|= EV_READ;
  if (events & POLLOUT)
    set_events|= EV_WRITE;

  if (dcon->last_events != set_events)
  {
    if (dcon->last_events != 0)
      assert(event_del(&(dcon->event)) == 0);
    event_set(&(dcon->event), dcon->fd, set_events | EV_PERSIST, _con_ready,
              dcon);
    event_base_set(dcon->gearmand->base, &(dcon->event));

    if (event_add(&(dcon->event), NULL) == -1)
    {
      GEARMAND_ERROR_SET(dcon->gearmand, "_con_watch", "event_add:-1")
      return GEARMAN_EVENT;
    }

    dcon->last_events= set_events;
  }

  if (dcon->gearmand->verbose > 1)
  {
    printf("%15s:%5s Watching %8s%8s\n", dcon->host, dcon->port,
           events & POLLIN ? "POLLIN" : "", events & POLLOUT ? "POLLOUT" : "");
  }

  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static void _con_ready(int fd __attribute__ ((unused)), short events,
                       void *arg)
{
  gearmand_st *gearmand;
  gearmand_con_st *dcon= (gearmand_con_st *)arg;
  gearman_server_con_st *server_con;
  short revents= 0;

  gearmand= dcon->gearmand;

  if (events & EV_READ)
    revents|= POLLIN;
  if (events & EV_WRITE)
    revents|= POLLOUT;

  gearman_con_set_revents(dcon->con, revents);

  if (gearmand->verbose > 1)
  {
    printf("%15s:%5s Ready    %8s%8s\n", dcon->host, dcon->port,
           revents & POLLIN ? "POLLIN" : "",
           revents & POLLOUT ? "POLLOUT" : "");
  }

  while (1)
  {
    server_con= gearman_server_run(&(gearmand->server), &(gearmand->ret));
    if (gearmand->ret == GEARMAN_SUCCESS || gearmand->ret == GEARMAN_IO_WAIT)
      return;

#if 0
    if (gearmand->ret == GEARMAN_SHUTDOWN_GRACEFUL)
    {
      _event_del_listen(gearmand);
      return;
    }

    if (server_con == NULL)
    {
      _event_del_all(gearmand);
      return;
    }
#endif

    dcon= (gearmand_con_st *)gearman_server_con_data(server_con);
    gearmand_con_free(dcon);
  }
}

#if 0
static void _event_del_all(gearmand_st *gearmand)
{
  gearmand_con_st *dcon;

  for (dcon= gearmand->dcon_list; dcon != NULL; dcon= dcon->next)
  {
    if (dcon->last_events != 0)
      assert(event_del(&(dcon->event)) == 0);

    /* This gets around a libevent bug when both POLLIN and POLLOUT are set. */
    event_set(&(dcon->event), dcon->fd, EV_READ, _con_ready, dcon);
    event_base_set(dcon->gearmand->base, &(dcon->event));
    event_add(&(dcon->event), NULL);
    assert(event_del(&(dcon->event)) == 0);
  }
}
#endif
