/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Definitions
 */

#include "common.h"
#include "gearmand.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_private Private Gearman Daemon Functions
 * @ingroup gearmand
 * @{
 */

static void _log(const char *line, gearman_verbose_t verbose, void *context);

static gearman_return_t _listen_init(gearmand_st *gearmand);
static void _listen_close(gearmand_st *gearmand);
static gearman_return_t _listen_watch(gearmand_st *gearmand);
static void _listen_clear(gearmand_st *gearmand);
static void _listen_event(int fd, short events, void *arg);

static gearman_return_t _wakeup_init(gearmand_st *gearmand);
static void _wakeup_close(gearmand_st *gearmand);
static gearman_return_t _wakeup_watch(gearmand_st *gearmand);
static void _wakeup_clear(gearmand_st *gearmand);
static void _wakeup_event(int fd, short events, void *arg);

static gearman_return_t _watch_events(gearmand_st *gearmand);
static void _clear_events(gearmand_st *gearmand);
static void _close_events(gearmand_st *gearmand);

/** @} */

/*
 * Public definitions
 */

gearmand_st *gearmand_create(const char *host, in_port_t port)
{
  gearmand_st *gearmand;

  gearmand= (gearmand_st *)malloc(sizeof(gearmand_st));
  if (gearmand == NULL)
    return NULL;

  if (gearman_server_create(&(gearmand->server)) == NULL)
  {
    free(gearmand);
    return NULL;
  }

  gearmand->is_listen_event= false;
  gearmand->is_wakeup_event= false;
  gearmand->verbose= 0;
  gearmand->ret= 0;
  gearmand->backlog= GEARMAN_DEFAULT_BACKLOG;
  gearmand->threads= 0;
  gearmand->port_count= 0;
  gearmand->thread_count= 0;
  gearmand->free_dcon_count= 0;
  gearmand->max_thread_free_dcon_count= 0;
  gearmand->wakeup_fd[0]= -1;
  gearmand->wakeup_fd[1]= -1;
  gearmand->host= host;
  gearmand->log_fn= NULL;
  gearmand->log_context= NULL;
  gearmand->base= NULL;
  gearmand->port_list= NULL;
  gearmand->thread_list= NULL;
  gearmand->thread_add_next= NULL;
  gearmand->free_dcon_list= NULL;

  if (port == 0)
    port= GEARMAN_DEFAULT_TCP_PORT;

  if (gearmand_port_add(gearmand, port, NULL) != GEARMAN_SUCCESS)
  {
    gearmand_free(gearmand);
    return NULL;
  }

  return gearmand;
}

void gearmand_free(gearmand_st *gearmand)
{
  gearmand_con_st *dcon;
  uint32_t x;

  _close_events(gearmand);

  if (gearmand->threads > 0)
    gearmand_log_info(gearmand, "Shutting down all threads");

  while (gearmand->thread_list != NULL)
    gearmand_thread_free(gearmand->thread_list);

  while (gearmand->free_dcon_list != NULL)
  {
    dcon= gearmand->free_dcon_list;
    gearmand->free_dcon_list= dcon->next;
    free(dcon);
  }

  if (gearmand->base != NULL)
    event_base_free(gearmand->base);

  gearman_server_free(&(gearmand->server));

  for (x= 0; x < gearmand->port_count; x++)
  {
    if (gearmand->port_list[x].listen_fd != NULL)
      free(gearmand->port_list[x].listen_fd);

    if (gearmand->port_list[x].listen_event != NULL)
      free(gearmand->port_list[x].listen_event);
  }

  if (gearmand->port_list != NULL)
    free(gearmand->port_list);

  gearmand_log_info(gearmand, "Shutdown complete");

  free(gearmand);
}

void gearmand_set_backlog(gearmand_st *gearmand, int backlog)
{
  gearmand->backlog= backlog;
}

void gearmand_set_job_retries(gearmand_st *gearmand, uint8_t job_retries)
{
  gearman_server_set_job_retries(&(gearmand->server), job_retries);
}

void gearmand_set_worker_wakeup(gearmand_st *gearmand, uint8_t worker_wakeup)
{
  gearman_server_set_worker_wakeup(&(gearmand->server), worker_wakeup);
}

void gearmand_set_threads(gearmand_st *gearmand, uint32_t threads)
{
  gearmand->threads= threads;
}

void gearmand_set_log_fn(gearmand_st *gearmand, gearman_log_fn *function,
                         void *context, gearman_verbose_t verbose)
{
  gearman_server_set_log_fn(&(gearmand->server), _log, gearmand, verbose);
  gearmand->log_fn= function;
  gearmand->log_context= context;
  gearmand->verbose= verbose;
}

gearman_return_t gearmand_port_add(gearmand_st *gearmand, in_port_t port,
                                   gearman_connection_add_fn *function)
{
  gearmand_port_st *port_list;

  port_list= (gearmand_port_st *)realloc(gearmand->port_list,
                                         sizeof(gearmand_port_st) * (gearmand->port_count + 1));
  if (port_list == NULL)
  {
    gearmand_log_fatal(gearmand, "gearmand_port_add:realloc:NULL");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  port_list[gearmand->port_count].port= port;
  port_list[gearmand->port_count].listen_count= 0;
  port_list[gearmand->port_count].gearmand= gearmand;
  port_list[gearmand->port_count].add_fn= function;
  port_list[gearmand->port_count].listen_fd= NULL;
  port_list[gearmand->port_count].listen_event= NULL;

  gearmand->port_list= port_list;
  gearmand->port_count++;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_run(gearmand_st *gearmand)
{
  uint32_t x;

  /* Initialize server components. */
  if (gearmand->base == NULL)
  {
    gearmand_log_info(gearmand, "Starting up");

    if (gearmand->threads > 0)
    {
#ifndef HAVE_EVENT_BASE_NEW
      gearmand_log_fatal(gearmand, "Multi-threaded gearmand requires libevent 1.4 or later, libevent 1.3 does not provided a "
                         "thread-safe interface.");
      return GEARMAN_EVENT;
#else
      /* Set the number of free connection structures each thread should keep
         around before the main thread is forced to take them. We compute this
         here so we don't need to on every new connection. */
      gearmand->max_thread_free_dcon_count= ((GEARMAN_MAX_FREE_SERVER_CON /
                                              gearmand->threads) / 2);
#endif
    }

    gearmand_log_debug(gearmand, "Initializing libevent for main thread");

    gearmand->base= event_base_new();
    if (gearmand->base == NULL)
    {
      gearmand_log_fatal(gearmand, "gearmand_run:event_base_new:NULL");
      return GEARMAN_EVENT;
    }

    gearmand_log_debug(gearmand, "Method for libevent: %s",
                       event_base_get_method(gearmand->base));

    gearmand->ret= _listen_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

    gearmand->ret= _wakeup_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

    gearmand_log_debug(gearmand, "Creating %u threads", gearmand->threads);

    /* If we have 0 threads we still need to create a fake one for context. */
    x= 0;
    do
    {
      gearmand->ret= gearmand_thread_create(gearmand);
      if (gearmand->ret != GEARMAN_SUCCESS)
        return gearmand->ret;
      x++;
    }
    while (x < gearmand->threads);

    gearmand->ret= gearman_server_queue_replay(&(gearmand->server));
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;
  }

  gearmand->ret= _watch_events(gearmand);
  if (gearmand->ret != GEARMAN_SUCCESS)
    return gearmand->ret;

  gearmand_log_info(gearmand, "Entering main event loop");

  if (event_base_loop(gearmand->base, 0) == -1)
  {
    gearmand_log_fatal(gearmand, "gearmand_run:event_base_loop:-1");
    return GEARMAN_EVENT;
  }

  gearmand_log_info(gearmand, "Exited main event loop");

  return gearmand->ret;
}

void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup)
{
  uint8_t buffer= wakeup;

  /* If this fails, there is not much we can really do. This should never fail
     though if the main gearmand thread is still active. */
  if (write(gearmand->wakeup_fd[1], &buffer, 1) != 1)
    gearmand_log_error(gearmand, "gearmand_wakeup:write:%d", errno);
}

void gearmand_set_round_robin(gearmand_st *gearmand, bool round_robin)
{
  gearmand->server.flags.round_robin= round_robin;
}


/*
 * Private definitions
 */

static void _log(const char *line, gearman_verbose_t verbose, void *context)
{
  gearmand_st *gearmand= (gearmand_st *)context;
  (*gearmand->log_fn)(line, verbose, (void *)gearmand->log_context);
}

static gearman_return_t _listen_init(gearmand_st *gearmand)
{
  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    int ret;
    struct gearmand_port_st *port;
    char port_str[NI_MAXSERV];
    struct addrinfo ai;
    struct addrinfo *addrinfo;

    port= &gearmand->port_list[x];

    snprintf(port_str, NI_MAXSERV, "%u", (uint32_t)(port->port));

    memset(&ai, 0, sizeof(struct addrinfo));
    ai.ai_flags  = AI_PASSIVE;
    ai.ai_family = AF_UNSPEC;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol= IPPROTO_TCP;

    ret= getaddrinfo(gearmand->host, port_str, &ai, &addrinfo);
    if (ret != 0)
    {
      gearmand_log_fatal(gearmand, "_listen_init:getaddrinfo:%s", gai_strerror(ret));
      return GEARMAN_ERRNO;
    }

    for (struct addrinfo *addrinfo_next= addrinfo; addrinfo_next != NULL;
         addrinfo_next= addrinfo_next->ai_next)
    {
      int opt;
      int fd;
      char host[NI_MAXHOST];

      ret= getnameinfo(addrinfo_next->ai_addr, addrinfo_next->ai_addrlen, host,
                       NI_MAXHOST, port_str, NI_MAXSERV,
                       NI_NUMERICHOST | NI_NUMERICSERV);
      if (ret != 0)
      {
        gearmand_log_error(gearmand, "_listen_init:getnameinfo:%s", gai_strerror(ret));
        strcpy(host, "-");
        strcpy(port_str, "-");
      }

      gearmand_log_debug(gearmand, "Trying to listen on %s:%s", host, port_str);

      /* Call to socket() can fail for some getaddrinfo results, try another. */
      fd= socket(addrinfo_next->ai_family, addrinfo_next->ai_socktype,
                 addrinfo_next->ai_protocol);
      if (fd == -1)
      {
        gearmand_log_error(gearmand, "Failed to listen on %s:%s", host, port_str);
        continue;
      }

      opt= 1;
      ret= setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      if (ret == -1)
      {
        close(fd);
        gearmand_log_fatal(gearmand, "_listen_init:setsockopt:%d", errno);
        return GEARMAN_ERRNO;
      }

      ret= bind(fd, addrinfo_next->ai_addr, addrinfo_next->ai_addrlen);
      if (ret == -1)
      {
        close(fd);
        if (errno == EADDRINUSE)
        {
          if (port->listen_fd == NULL)
          {
            gearmand_log_error(gearmand, "Address already in use %s:%s", host, port_str);
          }

          continue;
        }

        gearmand_log_fatal(gearmand, "_listen_init:bind:%d", errno);
        return GEARMAN_ERRNO;
      }

      if (listen(fd, gearmand->backlog) == -1)
      {
        close(fd);
        gearmand_log_fatal(gearmand, "_listen_init:listen:%d", errno);
        return GEARMAN_ERRNO;
      }

      // Scoping note for eventual transformation
      {
        int *fd_list;

        fd_list= (int *)realloc(port->listen_fd, sizeof(int) * (port->listen_count + 1));
        if (fd_list == NULL)
        {
          close(fd);
          gearmand_log_fatal(gearmand, "_listen_init:realloc:%d", errno);
            return GEARMAN_ERRNO;
        }

        port->listen_fd= fd_list;
      }

      port->listen_fd[port->listen_count]= fd;
      port->listen_count++;

      gearmand_log_info(gearmand, "Listening on %s:%s (%d)", host, port_str, fd);
    }

    freeaddrinfo(addrinfo);

    /* Report last socket() error if we couldn't find an address to bind. */
    if (port->listen_fd == NULL)
    {
      gearmand_log_fatal(gearmand, "_listen_init:Could not bind/listen to any addresses");
      return GEARMAN_ERRNO;
    }

    port->listen_event= (struct event *)malloc(sizeof(struct event) * port->listen_count);
    if (port->listen_event == NULL)
    {
      gearmand_log_fatal(gearmand, "_listen_init:malloc:%d", errno);
      return GEARMAN_ERRNO;
    }

    for (uint32_t y= 0; y < port->listen_count; y++)
    {
      event_set(&(port->listen_event[y]), port->listen_fd[y],
                EV_READ | EV_PERSIST, _listen_event, port);
      event_base_set(gearmand->base, &(port->listen_event[y]));
    }
  }

  return GEARMAN_SUCCESS;
}

static void _listen_close(gearmand_st *gearmand)
{
  _listen_clear(gearmand);

  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    for (uint32_t y= 0; y < gearmand->port_list[x].listen_count; y++)
    {
      if (gearmand->port_list[x].listen_fd[y] >= 0)
      {
        gearmand_log_info(gearmand, "Closing listening socket (%d)", gearmand->port_list[x].listen_fd[y]);
        close(gearmand->port_list[x].listen_fd[y]);
        gearmand->port_list[x].listen_fd[y]= -1;
      }
    }
  }
}

static gearman_return_t _listen_watch(gearmand_st *gearmand)
{
  if (gearmand->is_listen_event)
    return GEARMAN_SUCCESS;

  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    for (uint32_t y= 0; y < gearmand->port_list[x].listen_count; y++)
    {
      gearmand_log_info(gearmand, "Adding event for listening socket (%d)",
                        gearmand->port_list[x].listen_fd[y]);

        if (event_add(&(gearmand->port_list[x].listen_event[y]), NULL) == -1)
      {
        gearmand_log_fatal(gearmand, "_listen_watch:event_add:-1");
        return GEARMAN_EVENT;
      }
    }
  }

  gearmand->is_listen_event= true;
  return GEARMAN_SUCCESS;
}

static void _listen_clear(gearmand_st *gearmand)
{
  if (! (gearmand->is_listen_event))
    return;

  int del_ret= 0;
  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    for (uint32_t y= 0; y < gearmand->port_list[x].listen_count; y++)
    {
      gearmand_log_info(gearmand, "Clearing event for listening socket (%d)",
                        gearmand->port_list[x].listen_fd[y]);
        del_ret= event_del(&(gearmand->port_list[x].listen_event[y]));
      assert(del_ret == 0);
    }
  }

  gearmand->is_listen_event= false;
}

static void _listen_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_port_st *port= (gearmand_port_st *)arg;
  struct sockaddr sa;
  socklen_t sa_len;
  char host[NI_MAXHOST];
  char port_str[NI_MAXSERV];
  int ret;

  sa_len= sizeof(sa);
  fd= accept(fd, &sa, &sa_len);
  if (fd == -1)
  {
    if (errno == EINTR)
      return;
    else if (errno == EMFILE)
    {
      gearmand_log_error(port->gearmand, "_listen_event:accept:too many open files");
      return;
    }

    _clear_events(port->gearmand);
    gearmand_log_fatal(port->gearmand, "_listen_event:accept:%d", errno);
    port->gearmand->ret= GEARMAN_ERRNO;
    return;
  }

  /* Since this is numeric, it should never fail. Even if it did we don't want
     to really error from it. */
  ret= getnameinfo(&sa, sa_len, host, NI_MAXHOST, port_str, NI_MAXSERV,
                   NI_NUMERICHOST | NI_NUMERICSERV);
  if (ret != 0)
  {
    gearmand_log_error(port->gearmand, "_listen_event:getnameinfo:%s", gai_strerror(ret));
    strcpy(host, "-");
    strcpy(port_str, "-");
  }

  gearmand_log_info(port->gearmand, "Accepted connection from %s:%s", host, port_str);

  port->gearmand->ret= gearmand_con_create(port->gearmand, fd, host, port_str,
                                           port->add_fn);
  if (port->gearmand->ret != GEARMAN_SUCCESS)
    _clear_events(port->gearmand);
}

static gearman_return_t _wakeup_init(gearmand_st *gearmand)
{
  int ret;

  gearmand_log_info(gearmand, "Creating wakeup pipe");

  ret= pipe(gearmand->wakeup_fd);
  if (ret == -1)
  {
    gearmand_log_fatal(gearmand, "_wakeup_init:pipe:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_GETFL, 0);
  if (ret == -1)
  {
    gearmand_log_fatal(gearmand, "_wakeup_init:fcntl:F_GETFL:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    gearmand_log_fatal(gearmand, "_wakeup_init:fcntl:F_SETFL:%d", errno);
    return GEARMAN_ERRNO;
  }

  event_set(&(gearmand->wakeup_event), gearmand->wakeup_fd[0],
            EV_READ | EV_PERSIST, _wakeup_event, gearmand);
  event_base_set(gearmand->base, &(gearmand->wakeup_event));

  return GEARMAN_SUCCESS;
}

static void _wakeup_close(gearmand_st *gearmand)
{
  _wakeup_clear(gearmand);

  if (gearmand->wakeup_fd[0] >= 0)
  {
    gearmand_log_info(gearmand, "Closing wakeup pipe");
    close(gearmand->wakeup_fd[0]);
    gearmand->wakeup_fd[0]= -1;
    close(gearmand->wakeup_fd[1]);
    gearmand->wakeup_fd[1]= -1;
  }
}

static gearman_return_t _wakeup_watch(gearmand_st *gearmand)
{
  if (gearmand->is_wakeup_event)
    return GEARMAN_SUCCESS;

  gearmand_log_info(gearmand, "Adding event for wakeup pipe");

  if (event_add(&(gearmand->wakeup_event), NULL) == -1)
  {
    gearmand_log_fatal(gearmand, "_wakeup_watch:event_add:-1");
    return GEARMAN_EVENT;
  }

  gearmand->is_wakeup_event= true;
  return GEARMAN_SUCCESS;
}

static void _wakeup_clear(gearmand_st *gearmand)
{
  if (gearmand->is_wakeup_event)
  {
    gearmand_log_info(gearmand, "Clearing event for wakeup pipe");
    int del_ret= event_del(&(gearmand->wakeup_event));
    assert(del_ret == 0);
    gearmand->is_wakeup_event= false;
  }
}

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  uint8_t buffer[GEARMAN_PIPE_BUFFER_SIZE];
  ssize_t ret;
  gearmand_thread_st *thread;

  while (1)
  {
    ret= read(fd, buffer, GEARMAN_PIPE_BUFFER_SIZE);
    if (ret == 0)
    {
      _clear_events(gearmand);
      gearmand_log_fatal(gearmand, "_wakeup_event:read:EOF");
      gearmand->ret= GEARMAN_PIPE_EOF;
      return;
    }
    else if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      if (errno == EAGAIN)
        break;

      _clear_events(gearmand);
      gearmand_log_fatal(gearmand, "_wakeup_event:read:%d", errno);
      gearmand->ret= GEARMAN_ERRNO;
      return;
    }

    for (ssize_t x= 0; x < ret; x++)
    {
      switch ((gearmand_wakeup_t)buffer[x])
      {
      case GEARMAND_WAKEUP_PAUSE:
        gearmand_log_info(gearmand, "Received PAUSE wakeup event");
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_PAUSE;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
        gearmand_log_info(gearmand, "Received SHUTDOWN_GRACEFUL wakeup event");
        _listen_close(gearmand);

        for (thread= gearmand->thread_list; thread != NULL;
             thread= thread->next)
        {
          gearmand_thread_wakeup(thread, GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
        }

        gearmand->ret= GEARMAN_SHUTDOWN_GRACEFUL;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        gearmand_log_info(gearmand, "Received SHUTDOWN wakeup event");
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_SHUTDOWN;
        break;

      case GEARMAND_WAKEUP_CON:
      case GEARMAND_WAKEUP_RUN:
      default:
        gearmand_log_fatal(gearmand, "Received unknown wakeup event (%u)", buffer[x]);
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_UNKNOWN_STATE;
        break;
      }
    }
  }
}

static gearman_return_t _watch_events(gearmand_st *gearmand)
{
  gearman_return_t ret;

  ret= _listen_watch(gearmand);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  ret= _wakeup_watch(gearmand);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  return GEARMAN_SUCCESS;
}

static void _clear_events(gearmand_st *gearmand)
{
  _listen_clear(gearmand);
  _wakeup_clear(gearmand);

  /* If we are not threaded, tell the fake thread to shutdown now to clear
     connections. Otherwise we will never exit the libevent loop. */
  if (gearmand->threads == 0 && gearmand->thread_list != NULL)
    gearmand_thread_wakeup(gearmand->thread_list, GEARMAND_WAKEUP_SHUTDOWN);
}

static void _close_events(gearmand_st *gearmand)
{
  _listen_close(gearmand);
  _wakeup_close(gearmand);
}
