/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman daemon definitions
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

static gearman_return_t _listen_init(gearmand_st *gearmand);

static void _listen_event(int fd, short events __attribute__ ((unused)),
                          void *arg);

static gearman_return_t _wakeup_init(gearmand_st *gearmand);

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg);

static gearman_return_t _watch_events(gearmand_st *gearmand);

static void _clear_events(gearmand_st *gearmand);

static void _close_events(gearmand_st *gearmand);

/** @} */

/*
 * Public definitions
 */

gearmand_st *gearmand_create(in_port_t port)
{
  gearmand_st *gearmand;

  gearmand= malloc(sizeof(gearmand_st));
  if (gearmand == NULL)
    return NULL;

  memset(gearmand, 0, sizeof(gearmand_st));

  gearmand->listen_fd= -1;
  gearmand->wakeup_fd[0]= -1;
  gearmand->wakeup_fd[1]= -1;
  gearmand->backlog= GEARMAN_DEFAULT_BACKLOG;
  if (port == 0)
    gearmand->port= GEARMAN_DEFAULT_TCP_PORT;
  else
    gearmand->port= port;

  if (gearman_server_create(&(gearmand->server)) == NULL)
  {
    free(gearmand);
    return NULL;
  }

  gearman_server_set_event_watch(&(gearmand->server), gearmand_con_watch, NULL);

  return gearmand;
}

void gearmand_free(gearmand_st *gearmand)
{
  while (gearmand->thread_list != NULL)
    gearmand_thread_free(gearmand->thread_list);

  _close_events(gearmand);

  if (gearmand->addrinfo != NULL)
    freeaddrinfo(gearmand->addrinfo);

  if (gearmand->base != NULL)
    event_base_free(gearmand->base);

  gearman_server_free(&(gearmand->server));
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

void gearmand_set_verbose(gearmand_st *gearmand, uint8_t verbose)
{
  gearmand->verbose= verbose;
}

const char *gearmand_error(gearmand_st *gearmand)
{
  return (const char *)(gearmand->last_error);
}

int gearmand_errno(gearmand_st *gearmand)
{
  return gearmand->last_errno;
}

gearman_return_t gearmand_run(gearmand_st *gearmand)
{
  uint32_t x;

  /* Initialize server components. */
  if (gearmand->base == NULL)
  {
    gearmand->base= event_base_new();
    if (gearmand->base == NULL)
    {
      GEARMAN_ERROR_SET(gearmand, "gearmand_run", "event_base_new:NULL")
      return GEARMAN_EVENT;
    }

    if (gearmand->verbose > 0)
    {
      printf("Method for libevent: %s\n",
             event_base_get_method(gearmand->base));
    }

    gearmand->ret= _listen_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

    gearmand->ret= _wakeup_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
      return gearmand->ret;

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
  }

  gearmand->ret= _watch_events(gearmand);
  if (gearmand->ret != GEARMAN_SUCCESS)
    return gearmand->ret;

  /* Main server loop. */
  if (event_base_loop(gearmand->base, 0) == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "gearmand_run", "event_base_loop:-1")
    return GEARMAN_EVENT;
  }

  return gearmand->ret;
}

void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup)
{
  uint8_t buffer= wakeup;

  /* If this fails, there is not much we can really do. This should never fail
     though if the main gearmand thread is still active. */
  (void) write(gearmand->wakeup_fd[1], &buffer, 1);
}

/*
 * Private definitions
 */

static gearman_return_t _listen_init(gearmand_st *gearmand)
{
  char port_str[NI_MAXSERV];
  struct addrinfo ai;
  int ret;
  int opt;

  snprintf(port_str, NI_MAXSERV, "%u", gearmand->port);

  memset(&ai, 0, sizeof(struct addrinfo));
  ai.ai_flags  = AI_PASSIVE;
  ai.ai_family = AF_UNSPEC;
  ai.ai_socktype = SOCK_STREAM;
  ai.ai_protocol= IPPROTO_TCP;

  ret= getaddrinfo(NULL, port_str, &ai, &(gearmand->addrinfo));
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_listen_init", "getaddrinfo:%s",
                       gai_strerror(ret))
    return GEARMAN_ERRNO;
  }

  for (gearmand->addrinfo_next= gearmand->addrinfo;
       gearmand->addrinfo_next != NULL;
       gearmand->addrinfo_next= gearmand->addrinfo_next->ai_next)
  {
    /* Calls to socket() can fail for some getaddrinfo results, try another. */
    gearmand->listen_fd= socket(gearmand->addrinfo_next->ai_family,
                                gearmand->addrinfo_next->ai_socktype,
                                gearmand->addrinfo_next->ai_protocol);
    if (gearmand->listen_fd == -1)
      continue;

    opt= 1;
    ret= setsockopt(gearmand->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                    sizeof(opt));
    if (ret == -1)
    {
      GEARMAND_ERROR_SET(gearmand, "_listen_init", "setsockopt:%d", errno)
      return GEARMAN_ERRNO;
    }

    ret= bind(gearmand->listen_fd, gearmand->addrinfo_next->ai_addr,
              gearmand->addrinfo_next->ai_addrlen);
    if (ret == -1)
    {
      GEARMAND_ERROR_SET(gearmand, "_listen_init", "bind:%d", errno)
      return GEARMAN_ERRNO;
    }

    if (listen(gearmand->listen_fd, gearmand->backlog) == -1)
    {
      GEARMAND_ERROR_SET(gearmand, "_listen_init", "listen:%d", errno)
      return GEARMAN_ERRNO;
    }

    break;
  }

  /* Report last socket() error if we couldn't find an address to bind. */
  if (gearmand->listen_fd == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_listen_init", "socket:%d", errno)
    return GEARMAN_ERRNO;
  }

  event_set(&(gearmand->listen_event), gearmand->listen_fd,
            EV_READ | EV_PERSIST, _listen_event, gearmand);
  event_base_set(gearmand->base, &(gearmand->listen_event));

  if (gearmand->verbose > 0)
    printf("Listening on port %u\n", gearmand->port);

  return GEARMAN_SUCCESS;
}

static void _listen_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  struct sockaddr sa;
  socklen_t sa_len;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];

  sa_len= sizeof(sa);
  fd= accept(fd, &sa, &sa_len);
  if (fd == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_listen_event", "accept:%d", errno)
    gearmand->ret= GEARMAN_ERRNO;
    _close_events(gearmand);
    return;
  }

  /* Since this is numeric, it should never fail. Even if it did we don't want
     to really error from it. */
  (void) getnameinfo(&sa, sa_len, host, NI_MAXHOST, port, NI_MAXSERV,
                     NI_NUMERICHOST | NI_NUMERICSERV);

  gearmand->ret= gearmand_con_create(gearmand, fd, host, port);
  if (gearmand->ret != GEARMAN_SUCCESS)
    _close_events(gearmand);
}

static gearman_return_t _wakeup_init(gearmand_st *gearmand)
{
  int ret;

  ret= pipe(gearmand->wakeup_fd);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_wakeup_init", "pipe:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_GETFL, 0);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_wakeup_init", "fcntl:F_GETFL:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(gearmand->wakeup_fd[0], F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_wakeup_init", "fcntl:F_SETFL:%d", errno)
    return GEARMAN_ERRNO;
  }

  event_set(&(gearmand->wakeup_event), gearmand->wakeup_fd[0],
            EV_READ | EV_PERSIST, _wakeup_event, gearmand);
  event_base_set(gearmand->base, &(gearmand->wakeup_event));

  if (gearmand->verbose > 0)
    printf("Wakeup pipe created\n");

  return GEARMAN_SUCCESS;
}

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  uint8_t buffer[GEARMAN_PIPE_BUFFER_SIZE];
  ssize_t ret;
  ssize_t x;

  while (1)
  {
    ret= read(fd, buffer, GEARMAN_PIPE_BUFFER_SIZE);
    if (ret == 0)
    {
      GEARMAND_ERROR_SET(gearmand, "_wakeup_event", "read:EOF");
      gearmand->ret= GEARMAN_PIPE_EOF;
      _close_events(gearmand);
      return;
    }
    else if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      if (errno == EAGAIN)
        break;

      GEARMAND_ERROR_SET(gearmand, "_wakeup_event", "read:%d", errno);
      gearmand->ret= GEARMAN_ERRNO;
      _close_events(gearmand);
      return;
    }

    for (x= 0; x < ret; x++)
    {
      switch ((gearmand_wakeup_t)buffer[x])
      {
      case GEARMAND_WAKEUP_PAUSE:
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_PAUSE;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_UNKNOWN_STATE;
        break;

      default:
        GEARMAND_ERROR_SET(gearmand, "_wakeup_event", "unknown state");
        gearmand->ret= GEARMAN_UNKNOWN_STATE;
        _close_events(gearmand);
        break;
      }
    }
  }
}

static gearman_return_t _watch_events(gearmand_st *gearmand)
{
  if (event_add(&(gearmand->listen_event), NULL) == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_watch_events", "event_add:-1")
    return GEARMAN_EVENT;
  }

  gearmand->options|= GEARMAND_LISTEN_EVENT;

  if (event_add(&(gearmand->wakeup_event), NULL) == -1)
  {
    GEARMAND_ERROR_SET(gearmand, "_watch_events", "event_add:-1")
    return GEARMAN_EVENT;
  }

  gearmand->options|= GEARMAND_WAKEUP_EVENT;

  return GEARMAN_SUCCESS;
}

static void _clear_events(gearmand_st *gearmand)
{
  if (gearmand->options & GEARMAND_LISTEN_EVENT)
  {
    assert(event_del(&(gearmand->listen_event)) == 0);
    gearmand->options&= ~GEARMAND_LISTEN_EVENT;
  }

  if (gearmand->options & GEARMAND_WAKEUP_EVENT)
  {
    assert(event_del(&(gearmand->wakeup_event)) == 0);
    gearmand->options&= ~GEARMAND_WAKEUP_EVENT;
  }
}

static void _close_events(gearmand_st *gearmand)
{
  _clear_events(gearmand);

  if (gearmand->listen_fd >= 0)
  {
    close(gearmand->listen_fd);
    gearmand->listen_fd= -1;
  }

  if (gearmand->wakeup_fd[0] >= 0)
  {
    close(gearmand->wakeup_fd[0]);
    gearmand->wakeup_fd[0]= -1;
    close(gearmand->wakeup_fd[1]);
    gearmand->wakeup_fd[1]= -1;
  }
}
