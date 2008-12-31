/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Basic Server definitions
 */

#include "common.h"

/* All thread-safe libevent functions are not in libevent 1.3x, and this is the
   common package version. Make this work for these earlier versions. */
#ifndef HAVE_EVENT_BASE_NEW
#define event_base_new event_init
#define event_base_get_method(__base) event_get_method()
#endif

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_private Private Basic Server Functions
 * @ingroup gearmand
 * @{
 */

#ifdef HAVE_EVENT_H
static gearman_return_t _listen_init(gearmand_st *gearmand);

static void _listen_accept(int fd, short events __attribute__ ((unused)),
                           void *arg);

static gearman_return_t _con_watch(gearman_con_st *con, short events,
                                   void *arg __attribute__ ((unused)));

static void _con_ready(int fd __attribute__ ((unused)), short events,
                       void *arg);

static void _event_del_all(gearmand_st *gearmand);
#endif

/** @} */

/*
 * Public definitions
 */

gearmand_st *gearmand_create(in_port_t port)
{
#ifdef HAVE_EVENT_H
  gearmand_st *gearmand;

  gearmand= malloc(sizeof(gearmand_st));
  if (gearmand == NULL)
    return NULL;

  memset(gearmand, 0, sizeof(gearmand_st));
  
  gearmand->listen_fd= -1;
  gearmand->port= port;

  if (gearman_server_create(&(gearmand->server)) == NULL)
  {
    free(gearmand);
    return NULL;
  }

  gearman_server_set_event_watch(&(gearmand->server), _con_watch, NULL);

  gearmand->base= event_base_new();
  if (gearmand->base == NULL)
  {
    gearmand_free(gearmand);
    return NULL;
  }

  return gearmand;
#else
  (void) port;
  (void) backlog;

  if (verbose > 0)
    printf("Library not built with libevent support!\n");

  return NULL;
#endif
}

void gearmand_free(gearmand_st *gearmand)
{
#ifdef HAVE_EVENT_H
  if (gearmand->base != NULL)
    event_base_free(gearmand->base);

  gearman_server_free(&(gearmand->server));
  free(gearmand);
#else
  (void) gearmand;
#endif
}
 
void gearmand_set_backlog(gearmand_st *gearmand, int backlog)
{
  gearmand->backlog= backlog;
}
 
void gearmand_set_verbose(gearmand_st *gearmand, uint8_t verbose)
{
  gearmand->verbose= verbose;
}

const char *gearmand_error(gearmand_st *gearmand)
{
  return gearman_server_error(&(gearmand->server));
}

int gearmand_errno(gearmand_st *gearmand)
{
  return gearman_server_errno(&(gearmand->server));
}

gearman_return_t gearmand_run(gearmand_st *gearmand)
{
#ifdef HAVE_EVENT_H
  gearmand_con_st *dcon;

  if (gearmand->verbose > 0)
    printf("Method for libevent: %s\n", event_base_get_method(gearmand->base));

  gearmand->ret= _listen_init(gearmand);
  if (gearmand->ret != GEARMAN_SUCCESS)
    return gearmand->ret;

  if (event_base_loop(gearmand->base, 0) == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "gearmand_run",
                      "event_base_loop:-1")
    gearmand->ret= GEARMAN_EVENT;
  }

  close(gearmand->listen_fd);
  gearmand->listen_fd= -1;

  for (dcon= gearmand->dcon_list; dcon != NULL; dcon= gearmand->dcon_list)
  {
    gearmand->dcon_list= dcon->next;
    gearman_server_con_free(&(dcon->server_con));
    free(dcon);
  }

  return gearmand->ret;
#else
  (void) gearmand;

  GEARMAN_ERROR_SET(gearmand->server.gearman, "gearmand_run",
                    "Library not built with libevent support!")
  return GEARMAN_EVENT;
#endif
}

/*
 * Private definitions
 */

#ifdef HAVE_EVENT_H
static gearman_return_t _listen_init(gearmand_st *gearmand)
{
  struct sockaddr_in sa;
  int opt= 1;

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "signal:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port= htons(gearmand->port);
  sa.sin_addr.s_addr = INADDR_ANY;

  gearmand->listen_fd= socket(sa.sin_family, SOCK_STREAM, 0);
  if (gearmand->listen_fd == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "socket:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  if (setsockopt(gearmand->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "setsockopt:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  if (bind(gearmand->listen_fd, (struct sockaddr *)(&sa), sizeof(sa)) == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "bind:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  if (listen(gearmand->listen_fd, gearmand->backlog) == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "listen:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  event_set(&(gearmand->listen_event), gearmand->listen_fd,
            EV_READ | EV_PERSIST, _listen_accept, gearmand);
  event_base_set(gearmand->base, &(gearmand->listen_event));

  if (event_add(&(gearmand->listen_event), NULL) == -1)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_init", "event_add:-1")
    return GEARMAN_EVENT;
  }

  if (gearmand->verbose > 0)
    printf("Listening on port %u\n", gearmand->port);

  return GEARMAN_SUCCESS;
}

static void _listen_accept(int fd, short events __attribute__ ((unused)),
                           void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  gearmand_con_st *dcon;
  socklen_t sa_len;

  dcon= malloc(sizeof(gearmand_con_st));
  if (dcon == NULL)
  {
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_accept", "malloc")
    gearmand->ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    _event_del_all(gearmand);
    return;
  }

  memset(dcon, 0, sizeof(gearmand_con_st));

  sa_len= sizeof(dcon->sa);
  dcon->fd= accept(fd, (struct sockaddr *)(&(dcon->sa)), &sa_len);
  if (dcon->fd == -1)
  {
    free(dcon);
    GEARMAN_ERROR_SET(gearmand->server.gearman, "_listen_accept", "accept:%d",
                      errno)
    gearmand->ret= GEARMAN_ERRNO;;
    _event_del_all(gearmand);
    return;
  }

  if (gearmand->verbose > 0)
  {
    printf("%15s:%5u Connected (%u current, %u total)\n",
           inet_ntoa(dcon->sa.sin_addr), ntohs(dcon->sa.sin_port),
           gearmand->dcon_count + 1, gearmand->dcon_total + 1);
  }

  dcon->gearmand= gearmand;

  if (gearman_server_add_con(&(gearmand->server), &(dcon->server_con), dcon->fd,
                             dcon) == NULL)
  {
    close(dcon->fd);
    free(dcon);
    gearmand->ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    _event_del_all(gearmand);
    return;
  }

  GEARMAN_LIST_ADD(gearmand->dcon, dcon,)
  gearmand->dcon_total++;
}

static gearman_return_t _con_watch(gearman_con_st *con, short events,
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

  event_set(&(dcon->event), dcon->fd, set_events, _con_ready, dcon);
  event_base_set(dcon->gearmand->base, &(dcon->event));

  if (event_add(&(dcon->event), NULL) == -1)
  {
    GEARMAN_ERROR_SET(dcon->gearmand->server.gearman, "_con_watch",
                      "event_add:-1")
    return GEARMAN_EVENT;
  }

  if (dcon->gearmand->verbose > 1)
  {
    printf("%15s:%5u Watching %8s%8s\n", inet_ntoa(dcon->sa.sin_addr),
           ntohs(dcon->sa.sin_port), events & POLLIN ? "POLLIN" : "",
           events & POLLOUT ? "POLLOUT" : "");
  }

  return GEARMAN_SUCCESS;
}

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
    printf("%15s:%5u Ready    %8s%8s\n", inet_ntoa(dcon->sa.sin_addr),
           ntohs(dcon->sa.sin_port), revents & POLLIN ? "POLLIN" : "",
           revents & POLLOUT ? "POLLOUT" : "");
  }

  while (1)
  {
    server_con= gearman_server_run(&(gearmand->server), &(gearmand->ret));
    if (gearmand->ret == GEARMAN_SUCCESS || gearmand->ret == GEARMAN_IO_WAIT)
      return;

    if (server_con == NULL)
    {
      _event_del_all(gearmand);
      return;
    }

    if (gearmand->ret != GEARMAN_EOF)
    {
      _event_del_all(gearmand);
      return;
    }

    dcon= (gearmand_con_st *)gearman_server_con_data(server_con);

    if (gearmand->verbose > 0)
    {
      printf("%15s:%5u Disconnected\n", inet_ntoa(dcon->sa.sin_addr),
             ntohs(dcon->sa.sin_port));
    }

    assert(event_del(&(dcon->event)) == 0);
    gearman_server_con_free(&(dcon->server_con));

    GEARMAN_LIST_DEL(gearmand->dcon, dcon,)

    free(dcon);
  }
}

static void _event_del_all(gearmand_st *gearmand)
{
  gearmand_con_st *dcon;

  assert(event_del(&(gearmand->listen_event)) == 0);

  for (dcon= gearmand->dcon_list; dcon != NULL; dcon= dcon->next)
    assert(event_del(&(dcon->event)) == 0);
}
#endif
