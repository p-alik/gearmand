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

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_private Private Gearman Daemon Functions
 * @ingroup gearmand
 * @{
 */

static void _log(gearman_server_st *server __attribute__ ((unused)),
                 gearman_verbose_t verbose, const char *line, void *arg);

static gearman_return_t _listen_init(gearmand_st *gearmand);
static void _listen_close(gearmand_st *gearmand);
static gearman_return_t _listen_watch(gearmand_st *gearmand);
static void _listen_clear(gearmand_st *gearmand);
static void _listen_event(int fd, short events __attribute__ ((unused)),
                          void *arg);

static gearman_return_t _wakeup_init(gearmand_st *gearmand);
static void _wakeup_close(gearmand_st *gearmand);
static gearman_return_t _wakeup_watch(gearmand_st *gearmand);
static void _wakeup_clear(gearmand_st *gearmand);
static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg);

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

  gearmand= malloc(sizeof(gearmand_st));
  if (gearmand == NULL)
    return NULL;

  if (gearman_server_create(&(gearmand->server)) == NULL)
  {
    free(gearmand);
    return NULL;
  }

  gearmand->options= 0;
  gearmand->verbose= 0;
  gearmand->ret= 0;

  if (port == 0)
    gearmand->port= GEARMAN_DEFAULT_TCP_PORT;
  else
    gearmand->port= port;

  gearmand->backlog= GEARMAN_DEFAULT_BACKLOG;
  gearmand->threads= 0;
  gearmand->listen_count= 0;
  gearmand->thread_count= 0;
  gearmand->free_dcon_count= 0;
  gearmand->max_thread_free_dcon_count= 0;
  gearmand->wakeup_fd[0]= -1;
  gearmand->wakeup_fd[1]= -1;
  gearmand->host= host;
  gearmand->log_fn= NULL;
  gearmand->log_fn_arg= NULL;
  gearmand->base= NULL;
  gearmand->addrinfo= NULL;
  gearmand->addrinfo_next= NULL;
  gearmand->listen_fd= NULL;
  gearmand->listen_event= NULL;
  gearmand->thread_list= NULL;
  gearmand->thread_add_next= NULL;
  gearmand->free_dcon_list= NULL;

  return gearmand;
}

void gearmand_free(gearmand_st *gearmand)
{
  gearmand_con_st *dcon;

  _close_events(gearmand);

  if (gearmand->threads > 0)
    GEARMAN_INFO(gearmand, "Shutting down all threads")

  while (gearmand->thread_list != NULL)
    gearmand_thread_free(gearmand->thread_list);

  while (gearmand->free_dcon_list != NULL)
  {
    dcon= gearmand->free_dcon_list;
    gearmand->free_dcon_list= dcon->next;
    free(dcon);
  }

  if (gearmand->addrinfo != NULL)
    freeaddrinfo(gearmand->addrinfo);

  if (gearmand->base != NULL)
    event_base_free(gearmand->base);

  gearman_server_free(&(gearmand->server));

  if (gearmand->listen_fd != NULL)
    free(gearmand->listen_fd);

  if (gearmand->listen_event != NULL)
    free(gearmand->listen_event);

  GEARMAN_INFO(gearmand, "Shutdown complete")

  free(gearmand);
}

void gearmand_set_backlog(gearmand_st *gearmand, int backlog)
{
  gearmand->backlog= backlog;
}

void gearmand_set_threads(gearmand_st *gearmand, uint32_t threads)
{
  gearmand->threads= threads;
}

void gearmand_set_log(gearmand_st *gearmand, gearmand_log_fn log_fn,
                      void *log_fn_arg, gearman_verbose_t verbose)
{
  gearman_server_set_log(&(gearmand->server), _log, gearmand, verbose);
  gearmand->log_fn= log_fn;
  gearmand->log_fn_arg= log_fn_arg;
  gearmand->verbose= verbose;
}

gearman_return_t gearmand_run(gearmand_st *gearmand)
{
  uint32_t x;

  /* Initialize server components. */
  if (gearmand->base == NULL)
  {
    GEARMAN_INFO(gearmand, "Starting up")

    if (gearmand->threads > 0)
    {
#ifndef HAVE_EVENT_BASE_NEW
      GEARMAN_FATAL(gearmand, "Multi-threaded gearmand requires libevent 1.4 "                                "or later, libevent 1.3 does not provided a "
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

    GEARMAN_DEBUG(gearmand, "Initializing libevent for main thread")

    gearmand->base= event_base_new();
    if (gearmand->base == NULL)
    {
      GEARMAN_FATAL(gearmand, "gearmand_run:event_base_new:NULL")
      return GEARMAN_EVENT;
    }

    GEARMAN_DEBUG(gearmand, "Method for libevent: %s",
                  event_base_get_method(gearmand->base));

    gearmand->ret= _listen_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

    gearmand->ret= _wakeup_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

    GEARMAN_DEBUG(gearmand, "Creating %u threads", gearmand->threads)

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

  GEARMAN_INFO(gearmand, "Entering main event loop")

  if (event_base_loop(gearmand->base, 0) == -1)
  {
    GEARMAN_FATAL(gearmand, "gearmand_run:event_base_loop:-1")
    return GEARMAN_EVENT;
  }

  GEARMAN_INFO(gearmand, "Exited main event loop")

  return gearmand->ret;
}

void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup)
{
  uint8_t buffer= wakeup;

  /* If this fails, there is not much we can really do. This should never fail
     though if the main gearmand thread is still active. */
  if (write(gearmand->wakeup_fd[1], &buffer, 1) != 1)
    GEARMAN_ERROR(gearmand, "gearmand_wakeup:write:%d", errno)
}

/*
 * Private definitions
 */

static void _log(gearman_server_st *server __attribute__ ((unused)),
                 gearman_verbose_t verbose, const char *line, void *fn_arg)
{
  gearmand_st *gearmand= (gearmand_st *)fn_arg;
  (*gearmand->log_fn)(gearmand, verbose, line, gearmand->log_fn_arg);
}

static gearman_return_t _listen_init(gearmand_st *gearmand)
{
  struct addrinfo ai;
  int ret;
  int opt;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  int fd;
  int *fd_list;
  uint32_t x;

  snprintf(port, NI_MAXSERV, "%u", gearmand->port);

  memset(&ai, 0, sizeof(struct addrinfo));
  ai.ai_flags  = AI_PASSIVE;
  ai.ai_family = AF_UNSPEC;
  ai.ai_socktype = SOCK_STREAM;
  ai.ai_protocol= IPPROTO_TCP;

  ret= getaddrinfo(gearmand->host, port, &ai, &(gearmand->addrinfo));
  if (ret != 0)
  {
    GEARMAN_FATAL(gearmand, "_listen_init:getaddrinfo:%s", gai_strerror(ret))
    return GEARMAN_ERRNO;
  }

  for (gearmand->addrinfo_next= gearmand->addrinfo;
       gearmand->addrinfo_next != NULL;
       gearmand->addrinfo_next= gearmand->addrinfo_next->ai_next)
  {
    ret= getnameinfo(gearmand->addrinfo_next->ai_addr,
                     gearmand->addrinfo_next->ai_addrlen, host, NI_MAXHOST,
                     port, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0)
    {
      GEARMAN_ERROR(gearmand, "_listen_init:getnameinfo:%s", gai_strerror(ret))
      strcpy(host, "-");
      strcpy(port, "-");
    }

    GEARMAN_DEBUG(gearmand, "Trying to listen on %s:%s", host, port)

    /* Calls to socket() can fail for some getaddrinfo results, try another. */
    fd= socket(gearmand->addrinfo_next->ai_family,
               gearmand->addrinfo_next->ai_socktype,
               gearmand->addrinfo_next->ai_protocol);
    if (fd == -1)
    {
      GEARMAN_ERROR(gearmand, "Failed to listen on %s:%s", host, port)
      continue;
    }

    opt= 1;
    ret= setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
      close(fd);
      GEARMAN_FATAL(gearmand, "_listen_init:setsockopt:%d", errno)
      return GEARMAN_ERRNO;
    }

    ret= bind(fd, gearmand->addrinfo_next->ai_addr,
              gearmand->addrinfo_next->ai_addrlen);
    if (ret == -1)
    {
      close(fd);
      if (errno == EADDRINUSE)
      {
        if (gearmand->listen_fd == NULL)
          GEARMAN_ERROR(gearmand, "Address already in use %s:%s", host, port)

        continue;
      }

      GEARMAN_FATAL(gearmand, "_listen_init:bind:%d", errno)
      return GEARMAN_ERRNO;
    }

    if (listen(fd, gearmand->backlog) == -1)
    {
      close(fd);
      GEARMAN_FATAL(gearmand, "_listen_init:listen:%d", errno)
      return GEARMAN_ERRNO;
    }

    fd_list= realloc(gearmand->listen_fd,
                     sizeof(int) * (gearmand->listen_count + 1));
    if (fd_list == NULL)
    {
      close(fd);
      GEARMAN_FATAL(gearmand, "_listen_init:realloc:%d", errno)
      return GEARMAN_ERRNO;
    }

    x= gearmand->listen_count;
    gearmand->listen_fd= fd_list;
    gearmand->listen_fd[x]= fd;
    gearmand->listen_count++;

    GEARMAN_INFO(gearmand, "Listening on %s:%s (%d)", host, port, fd)
  }

  /* Report last socket() error if we couldn't find an address to bind. */
  if (gearmand->listen_fd == NULL)
  {
    GEARMAN_FATAL(gearmand,
                  "_listen_init:Could not bind/listen to any addresses")
    return GEARMAN_ERRNO;
  }

  gearmand->listen_event= malloc(sizeof(struct event) * gearmand->listen_count);
  if (gearmand->listen_event == NULL)
  {
    GEARMAN_FATAL(gearmand, "_listen_init:malloc:%d", errno)
    return GEARMAN_ERRNO;
  }

  for (x= 0; x < gearmand->listen_count; x++)
  {
    event_set(&(gearmand->listen_event[x]), gearmand->listen_fd[x],
              EV_READ | EV_PERSIST, _listen_event, gearmand);
    event_base_set(gearmand->base, &(gearmand->listen_event[x]));
  }

  return GEARMAN_SUCCESS;
}

static void _listen_close(gearmand_st *gearmand)
{
  uint32_t x;

  _listen_clear(gearmand);

  for (x= 0; x < gearmand->listen_count; x++)
  {
    if (gearmand->listen_fd[x] >= 0)
    {
      GEARMAN_INFO(gearmand, "Closing listening socket (%d)",
                   gearmand->listen_fd[x])
      close(gearmand->listen_fd[x]);
      gearmand->listen_fd[x]= -1;
    }
  }
}

static gearman_return_t _listen_watch(gearmand_st *gearmand)
{
  uint32_t x;

  if (gearmand->options & GEARMAND_LISTEN_EVENT)
    return GEARMAN_SUCCESS;

  for (x= 0; x < gearmand->listen_count; x++)
  {
    GEARMAN_INFO(gearmand, "Adding event for listening socket (%d)",
                 gearmand->listen_fd[x])

    if (event_add(&(gearmand->listen_event[x]), NULL) == -1)
    {
      GEARMAN_FATAL(gearmand, "_listen_watch:event_add:-1")
      return GEARMAN_EVENT;
    }
  }

  gearmand->options|= GEARMAND_LISTEN_EVENT;
  return GEARMAN_SUCCESS;
}

static void _listen_clear(gearmand_st *gearmand)
{
  uint32_t x;

  if (!(gearmand->options & GEARMAND_LISTEN_EVENT))
    return;

  for (x= 0; x < gearmand->listen_count; x++)
  {
    GEARMAN_INFO(gearmand, "Clearing event for listening socket (%d)",
                 gearmand->listen_fd[x])
    assert(event_del(&(gearmand->listen_event[x])) == 0);
  }

  gearmand->options&= ~GEARMAND_LISTEN_EVENT;
}

static void _listen_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  struct sockaddr sa;
  socklen_t sa_len;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  int ret;

  sa_len= sizeof(sa);
  fd= accept(fd, &sa, &sa_len);
  if (fd == -1)
  {
    if (errno == EINTR)
      return;
    else if (errno == EMFILE)
    {
      GEARMAN_ERROR(gearmand, "_listen_event:accept:too many open files")
      return;
    }

    _clear_events(gearmand);
    GEARMAN_FATAL(gearmand, "_listen_event:accept:%d", errno)
    gearmand->ret= GEARMAN_ERRNO;
    return;
  }

  /* Since this is numeric, it should never fail. Even if it did we don't want
     to really error from it. */
  ret= getnameinfo(&sa, sa_len, host, NI_MAXHOST, port, NI_MAXSERV,
                   NI_NUMERICHOST | NI_NUMERICSERV);
  if (ret != 0)
  {
    GEARMAN_ERROR(gearmand, "_listen_event:getnameinfo:%s", gai_strerror(ret))
    strcpy(host, "-");
    strcpy(port, "-");
  }

  GEARMAN_INFO(gearmand, "Accepted connection from %s:%s", host, port)

  gearmand->ret= gearmand_con_create(gearmand, fd, host, port);
  if (gearmand->ret != GEARMAN_SUCCESS)
    _clear_events(gearmand);
}

static gearman_return_t _wakeup_init(gearmand_st *gearmand)
{
  int ret;

  GEARMAN_INFO(gearmand, "Creating wakeup pipe")

  ret= pipe(gearmand->wakeup_fd);
  if (ret == -1)
  {
    GEARMAN_FATAL(gearmand, "_wakeup_init:pipe:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_GETFL, 0);
  if (ret == -1)
  {
    GEARMAN_FATAL(gearmand, "_wakeup_init:fcntl:F_GETFL:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    GEARMAN_FATAL(gearmand, "_wakeup_init:fcntl:F_SETFL:%d", errno)
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
    GEARMAN_INFO(gearmand, "Closing wakeup pipe")
    close(gearmand->wakeup_fd[0]);
    gearmand->wakeup_fd[0]= -1;
    close(gearmand->wakeup_fd[1]);
    gearmand->wakeup_fd[1]= -1;
  }
}

static gearman_return_t _wakeup_watch(gearmand_st *gearmand)
{
  if (gearmand->options & GEARMAND_WAKEUP_EVENT)
    return GEARMAN_SUCCESS;

  GEARMAN_INFO(gearmand, "Adding event for wakeup pipe")

  if (event_add(&(gearmand->wakeup_event), NULL) == -1)
  {
    GEARMAN_FATAL(gearmand, "_wakeup_watch:event_add:-1")
    return GEARMAN_EVENT;
  }

  gearmand->options|= GEARMAND_WAKEUP_EVENT;
  return GEARMAN_SUCCESS;
}

static void _wakeup_clear(gearmand_st *gearmand)
{
  if (gearmand->options & GEARMAND_WAKEUP_EVENT)
  {
    GEARMAN_INFO(gearmand, "Clearing event for wakeup pipe")
    assert(event_del(&(gearmand->wakeup_event)) == 0);
    gearmand->options&= ~GEARMAND_WAKEUP_EVENT;
  }
}

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  uint8_t buffer[GEARMAN_PIPE_BUFFER_SIZE];
  ssize_t ret;
  ssize_t x;
  gearmand_thread_st *thread;

  while (1)
  {
    ret= read(fd, buffer, GEARMAN_PIPE_BUFFER_SIZE);
    if (ret == 0)
    {
      _clear_events(gearmand);
      GEARMAN_FATAL(gearmand, "_wakeup_event:read:EOF")
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
      GEARMAN_FATAL(gearmand, "_wakeup_event:read:%d", errno)
      gearmand->ret= GEARMAN_ERRNO;
      return;
    }

    for (x= 0; x < ret; x++)
    {
      switch ((gearmand_wakeup_t)buffer[x])
      {
      case GEARMAND_WAKEUP_PAUSE:
        GEARMAN_INFO(gearmand, "Received PAUSE wakeup event")
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_PAUSE;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
        GEARMAN_INFO(gearmand, "Received SHUTDOWN_GRACEFUL wakeup event")
        _listen_close(gearmand);

        for (thread= gearmand->thread_list; thread != NULL;
             thread= thread->next)
        {
          gearmand_thread_wakeup(thread, GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
        }

        gearmand->ret= GEARMAN_SHUTDOWN_GRACEFUL;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        GEARMAN_INFO(gearmand, "Received SHUTDOWN wakeup event")
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_SHUTDOWN;
        break;

      case GEARMAND_WAKEUP_CON:
      case GEARMAND_WAKEUP_RUN:
      default:
        GEARMAN_FATAL(gearmand, "Received unknown wakeup event (%u)",
                      buffer[x])
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
