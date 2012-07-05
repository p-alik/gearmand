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

#include <config.h>
#include <libgearman-server/common.h>

#include <cerrno>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <set>
#include <string>

#include <libgearman-server/gearmand.h>

#include <libgearman-server/struct/port.h>
#include <libgearman-server/plugins.h>
#include <libgearman-server/timer.h>

using namespace gearmand;

/*
 * Private declarations
 */

/**
 * @addtogroup gearmand_private Private Gearman Daemon Functions
 * @ingroup gearmand
 * @{
 */

static gearmand_error_t _listen_init(gearmand_st *gearmand);
static void _listen_close(gearmand_st *gearmand);
static gearmand_error_t _listen_watch(gearmand_st *gearmand);
static void _listen_clear(gearmand_st *gearmand);
static void _listen_event(int fd, short events, void *arg);

static gearmand_error_t _wakeup_init(gearmand_st *gearmand);
static void _wakeup_close(gearmand_st *gearmand);
static gearmand_error_t _wakeup_watch(gearmand_st *gearmand);
static void _wakeup_clear(gearmand_st *gearmand);
static void _wakeup_event(int fd, short events, void *arg);

static gearmand_error_t _watch_events(gearmand_st *gearmand);
static void _clear_events(gearmand_st *gearmand);
static void _close_events(gearmand_st *gearmand);

static bool gearman_server_create(gearman_server_st *server,
                                  uint8_t job_retries,
                                  uint8_t worker_wakeup,
                                  bool round_robin);
static void gearmand_set_log_fn(gearmand_st *gearmand, gearmand_log_fn *function,
                                void *context, const gearmand_verbose_t verbose);


static void gearman_server_free(gearman_server_st *server)
{
  /* All threads should be cleaned up before calling this. */
  assert(server->thread_list == NULL);

  for (uint32_t key= 0; key < GEARMAND_JOB_HASH_SIZE; key++)
  {
    while (server->job_hash[key] != NULL)
    {
      gearman_server_job_free(server->job_hash[key]);
    }
  }

  while (server->function_list != NULL)
  {
    gearman_server_function_free(server, server->function_list);
  }

  while (server->free_packet_list != NULL)
  {
    gearman_server_packet_st *packet= server->free_packet_list;
    server->free_packet_list= packet->next;
    delete packet;
  }

  while (server->free_job_list != NULL)
  {
    gearman_server_job_st* job= server->free_job_list;
    server->free_job_list= job->next;
    delete job;
  }

  while (server->free_client_list != NULL)
  {
    gearman_server_client_st* client= server->free_client_list;
    server->free_client_list= client->con_next;
    delete client;
  }

  while (server->free_worker_list != NULL)
  {
    gearman_server_worker_st* worker= server->free_worker_list;
    server->free_worker_list= worker->con_next;
    delete worker;
  }
}

/** @} */

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/*
 * Public definitions
 */

static gearmand_st *_global_gearmand= NULL;

gearmand_st *Gearmand(void)
{
  if (!_global_gearmand)
  {
    gearmand_error("Gearmand() was called before it was allocated");
    assert(! "Gearmand() was called before it was allocated");
  }
  assert(_global_gearmand);
  return _global_gearmand;
}

gearmand_st *gearmand_create(const char *host_arg,
                             uint32_t threads_arg,
                             int backlog_arg,
                             uint8_t job_retries,
                             uint8_t worker_wakeup,
                             gearmand_log_fn *log_function, void *log_context, const gearmand_verbose_t verbose_arg,
                             bool round_robin)
{
  gearmand_st *gearmand;

  assert(_global_gearmand == NULL);
  if (_global_gearmand)
  {
    gearmand_error("You have called gearmand_create() twice within your application.");
    _exit(EXIT_FAILURE);
  }

  gearmand= new (std::nothrow) gearmand_st;
  if (gearmand == NULL)
  {
    gearmand_merror("new", gearmand_st, 0);
    return NULL;
  }

  if (gearman_server_create(&(gearmand->server), job_retries, worker_wakeup, round_robin) == false)
  {
    delete gearmand;
    return NULL;
  }

  gearmand->is_listen_event= false;
  gearmand->is_wakeup_event= false;
  gearmand->verbose= verbose_arg;
  gearmand->timeout= -1;
  gearmand->ret= GEARMAN_SUCCESS;
  gearmand->backlog= backlog_arg;
  gearmand->threads= threads_arg;
  gearmand->port_count= 0;
  gearmand->thread_count= 0;
  gearmand->free_dcon_count= 0;
  gearmand->max_thread_free_dcon_count= 0;
  gearmand->wakeup_fd[0]= -1;
  gearmand->wakeup_fd[1]= -1;
  gearmand->host= host_arg;
  gearmand->log_fn= NULL;
  gearmand->log_context= NULL;
  gearmand->base= NULL;
  gearmand->port_list= NULL;
  gearmand->thread_list= NULL;
  gearmand->thread_add_next= NULL;
  gearmand->free_dcon_list= NULL;

  _global_gearmand= gearmand;

  gearmand_set_log_fn(gearmand, log_function, log_context, verbose_arg);

  return gearmand;
}

void gearmand_free(gearmand_st *gearmand)
{
  _close_events(gearmand);

  if (gearmand->threads > 0)
  {
    gearmand_debug("Shutting down all threads");
  }

  while (gearmand->thread_list != NULL)
  {
    gearmand_thread_free(gearmand->thread_list);
  }

  while (gearmand->free_dcon_list != NULL)
  {
    gearmand_con_st *dcon;

    dcon= gearmand->free_dcon_list;
    gearmand->free_dcon_list= dcon->next;
    free(dcon);
  }

  if (gearmand->base != NULL)
  {
    event_base_free(gearmand->base);
  }

  gearman_server_free(&(gearmand->server));

  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    if (gearmand->port_list[x].listen_fd != NULL)
    {
      free(gearmand->port_list[x].listen_fd);
    }

    if (gearmand->port_list[x].listen_event != NULL)
    {
      free(gearmand->port_list[x].listen_event);
    }
  }

  if (gearmand->port_list != NULL)
  {
    free(gearmand->port_list);
  }

  gearmand_info("Shutdown complete");

  delete gearmand;
}

static void gearmand_set_log_fn(gearmand_st *gearmand, gearmand_log_fn *function,
                                void *context, const gearmand_verbose_t verbose)
{
  gearmand->log_fn= function;
  gearmand->log_context= context;
  gearmand->verbose= verbose;
}

gearmand_error_t gearmand_port_add(gearmand_st *gearmand, const char *port,
                                   gearmand_connection_add_fn *function)
{
  gearmand_port_st *port_list;

  port_list= (gearmand_port_st *)realloc(gearmand->port_list,
                                         sizeof(gearmand_port_st) * (gearmand->port_count + 1));
  if (port_list == NULL)
  {
    gearmand_perror("realloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  strncpy(port_list[gearmand->port_count].port, port, NI_MAXSERV);
  port_list[gearmand->port_count].listen_count= 0;
  port_list[gearmand->port_count].add_fn= function;
  port_list[gearmand->port_count].listen_fd= NULL;
  port_list[gearmand->port_count].listen_event= NULL;

  gearmand->port_list= port_list;
  gearmand->port_count++;

  return GEARMAN_SUCCESS;
}

gearman_server_st *gearmand_server(gearmand_st *gearmand)
{
  return &gearmand->server;
}

gearmand_error_t gearmand_run(gearmand_st *gearmand)
{
  libgearman::server::Epoch epoch;

  /* Initialize server components. */
  if (gearmand->base == NULL)
  {
    gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Starting up, verbose set to %s", 
                      gearmand_verbose_name(gearmand->verbose));

    if (gearmand->threads > 0)
    {
      /* Set the number of free connection structures each thread should keep
         around before the main thread is forced to take them. We compute this
         here so we don't need to on every new connection. */
      gearmand->max_thread_free_dcon_count= ((GEARMAN_MAX_FREE_SERVER_CON /
                                              gearmand->threads) / 2);
    }

    gearmand->base= static_cast<struct event_base *>(event_base_new());
    if (gearmand->base == NULL)
    {

      gearmand_fatal("event_base_new(NULL)");
      return GEARMAN_EVENT;
    }

    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Method for libevent: %s", event_base_get_method(gearmand->base));

    gearmand->ret= _listen_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
    {
      return gearmand->ret;
    }

    gearmand->ret= _wakeup_init(gearmand);
    if (gearmand->ret != GEARMAN_SUCCESS)
    {
      return gearmand->ret;
    }

    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Creating %u threads", gearmand->threads);

    /* If we have 0 threads we still need to create a fake one for context. */
    uint32_t x= 0;
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
    {
      return gearmand->ret;
    }
  }

  gearmand->ret= _watch_events(gearmand);
  if (gearmand->ret != GEARMAN_SUCCESS)
  {
    return gearmand->ret;
  }

  gearmand_debug("Entering main event loop");

  if (event_base_loop(gearmand->base, 0) == -1)
  {
    gearmand_fatal("event_base_loop(-1)");
    return GEARMAN_EVENT;
  }

  gearmand_debug("Exited main event loop");

  return gearmand->ret;
}

void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup)
{
  uint8_t buffer= wakeup;

  /* If this fails, there is not much we can really do. This should never fail
     though if the main gearmand thread is still active. */
  ssize_t written;
  if ((written= write(gearmand->wakeup_fd[1], &buffer, 1)) != 1)
  {
    if (written < 0)
    {
      gearmand_perror(gearmand_strwakeup(wakeup));
    }
    else
    {
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, 
                         "gearmand_wakeup() incorrectly wrote %lu bytes of data.", (unsigned long)written);
    }
  }
}


/*
 * Private definitions
 */

static const uint32_t bind_timeout= 6; // Number is not special, but look at INFO messages if you decide to change it.

typedef std::pair<std::string, std::string> host_port_t;

static gearmand_error_t _listen_init(gearmand_st *gearmand)
{
  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    struct linger ling= {0, 0};
    struct addrinfo hints;
    struct addrinfo *addrinfo;

    gearmand_port_st *port= &gearmand->port_list[x];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags= AI_PASSIVE;
    hints.ai_socktype= SOCK_STREAM;

    {
      int ret= getaddrinfo(gearmand->host, port->port, &hints, &addrinfo);
      if (ret != 0)
      {
        char buffer[1024];

        int length= snprintf(buffer, sizeof(buffer), "%s:%s", gearmand->host ? gearmand->host : "<any>", port->port);
        if (length <= 0 or size_t(length) >= sizeof(buffer))
        {
          buffer[0]= 0;
        }
        gearmand_gai_error(buffer, ret);
        return GEARMAN_ERRNO;
      }
    }

    std::set<host_port_t> unique_hosts;
    for (struct addrinfo *addrinfo_next= addrinfo; addrinfo_next != NULL;
         addrinfo_next= addrinfo_next->ai_next)
    {
      int fd;
      char host[NI_MAXHOST];

      {
        int ret= getnameinfo(addrinfo_next->ai_addr, addrinfo_next->ai_addrlen, host,
                             NI_MAXHOST, port->port, NI_MAXSERV,
                             NI_NUMERICHOST | NI_NUMERICSERV);
        if (ret != 0)
        {
          gearmand_gai_error("getaddrinfo", ret);
          strncpy(host, "-", sizeof(host));
          strncpy(port->port, "-", sizeof(port->port));
        }
      }

      std::string host_string(host);
      std::string port_string(port->port);
      host_port_t check= std::make_pair(host_string, port_string);
      if (unique_hosts.find(check) != unique_hosts.end())
      {
        gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Already listening on %s:%s", host, port->port);
        continue;
      }
      unique_hosts.insert(check);

      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Trying to listen on %s:%s", host, port->port);

      /* Call to socket() can fail for some getaddrinfo results, try another. */
      fd= socket(addrinfo_next->ai_family, addrinfo_next->ai_socktype,
                 addrinfo_next->ai_protocol);
      if (fd == -1)
      {
        gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "Failed to listen on %s:%s", host, port->port);
        continue;
      }

      int flags= 1;
#ifdef IPV6_V6ONLY
      if (addrinfo_next->ai_family == AF_INET6)
      {
        flags= 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &flags, sizeof(flags)) == -1)
        {
          gearmand_perror("setsockopt(IPV6_V6ONLY)");
          return GEARMAN_ERRNO;
        }
      }
#endif

      {
        int ret= fcntl(fd, F_SETFD, FD_CLOEXEC);
        if (ret != 0 || !(fcntl(fd, F_GETFD, 0) & FD_CLOEXEC))
        {
          gearmand_perror("fcntl(FD_CLOEXEC)");
          return GEARMAN_ERRNO;
        }
      }

      {
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) == -1)
        {
          gearmand_perror("setsockopt(SO_REUSEADDR)");
          return GEARMAN_ERRNO;
        }
      }

      {
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags)) == -1)
        {
          gearmand_perror("setsockopt(SO_KEEPALIVE)");
          return GEARMAN_ERRNO;
        }
      }

      {
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) == -1)
        {
          gearmand_perror("setsockopt(SO_LINGER)");
          return GEARMAN_ERRNO;
        }
      }

      {
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags)) == -1)
        {
          gearmand_perror("setsockopt(TCP_NODELAY)");
          return GEARMAN_ERRNO;
        }
      }

      /*
        @note logic for this pulled from Drizzle.

        Sometimes the port is not released fast enough when stopping and
        restarting the server. This happens quite often with the test suite
        on busy Linux systems. Retry to bind the address at these intervals:
        Sleep intervals: 1, 2, 4,  6,  9, 13, 17, 22, ...
        Retry at second: 1, 3, 7, 13, 22, 35, 52, 74, ...
        Limit the sequence by drizzled_bind_timeout.
      */
      uint32_t waited;
      uint32_t this_wait;
      uint32_t retry;
      int ret= -1;
      for (waited= 0, retry= 1; ; retry++, waited+= this_wait)
      {
        if (((ret= bind(fd, addrinfo_next->ai_addr, addrinfo_next->ai_addrlen)) == 0) or
            (errno != EADDRINUSE) || (waited >= bind_timeout))
        {
          break;
        }

        // We are in single user threads, so strerror() is fine.
        gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Retrying bind(%s) on %s:%s %u >= %u", 
                           strerror(errno), host, port->port,
                           waited, bind_timeout);
        this_wait= retry * retry / 3 + 1;
        sleep(this_wait);
      }

      if (ret < 0)
      {
        gearmand_perror("bind");

        gearmand_sockfd_close(fd);

        if (errno == EADDRINUSE)
        {
          if (port->listen_fd == NULL)
          {
            gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "Address already in use %s:%s", host, port->port);
          }

          continue;
        }

        gearmand_perror("bind");
        return GEARMAN_ERRNO;
      }

      if (listen(fd, gearmand->backlog) == -1)
      {
        gearmand_perror("listen");

        gearmand_sockfd_close(fd);

        return GEARMAN_ERRNO;
      }

      // Scoping note for eventual transformation
      {
        int *fd_list;

        fd_list= (int *)realloc(port->listen_fd, sizeof(int) * (port->listen_count + 1));
        if (fd_list == NULL)
        {
          gearmand_perror("realloc");

          gearmand_sockfd_close(fd);

          return GEARMAN_ERRNO;
        }

        port->listen_fd= fd_list;
      }

      port->listen_fd[port->listen_count]= fd;
      port->listen_count++;

      gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Listening on %s:%s (%d)", host, port->port, fd);
    }

    freeaddrinfo(addrinfo);

    /* Report last socket() error if we couldn't find an address to bind. */
    if (port->listen_fd == NULL)
    {
      gearmand_fatal("Could not bind/listen to any addresses");
      return GEARMAN_ERRNO;
    }

    port->listen_event= (struct event *)malloc(sizeof(struct event) * port->listen_count);
    if (port->listen_event == NULL)
    {
      return gearmand_merror("malloc", struct event, port->listen_count);
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
        gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Closing listening socket (%d)", gearmand->port_list[x].listen_fd[y]);
        gearmand_sockfd_close(gearmand->port_list[x].listen_fd[y]);
        gearmand->port_list[x].listen_fd[y]= -1;
      }
    }
  }
}

static gearmand_error_t _listen_watch(gearmand_st *gearmand)
{
  if (gearmand->is_listen_event)
  {
    return GEARMAN_SUCCESS;
  }

  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    for (uint32_t y= 0; y < gearmand->port_list[x].listen_count; y++)
    {
      gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Adding event for listening socket (%d)",
                        gearmand->port_list[x].listen_fd[y]);

      if (event_add(&(gearmand->port_list[x].listen_event[y]), NULL) < 0)
      {
        gearmand_perror("event_add");
        return GEARMAN_EVENT;
      }
    }
  }

  gearmand->is_listen_event= true;
  return GEARMAN_SUCCESS;
}

static void _listen_clear(gearmand_st *gearmand)
{
  if (gearmand->is_listen_event == false)
  {
    return;
  }

  for (uint32_t x= 0; x < gearmand->port_count; x++)
  {
    for (uint32_t y= 0; y < gearmand->port_list[x].listen_count; y++)
    {
      gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, 
                        "Clearing event for listening socket (%d)",
                        gearmand->port_list[x].listen_fd[y]);

      if (event_del(&(gearmand->port_list[x].listen_event[y])) < 0)
      {
        gearmand_perror("We tried to event_del() an event which no longer existed");
        assert(! "We tried to event_del() an event which no longer existed");
      }
    }
  }

  gearmand->is_listen_event= false;
}

static void _listen_event(int fd, short events __attribute__ ((unused)), void *arg)
{
  gearmand_port_st *port= (gearmand_port_st *)arg;
  struct sockaddr sa;

  socklen_t sa_len= sizeof(sa);
  fd= accept(fd, &sa, &sa_len);
  if (fd == -1)
  {
    if (errno == EINTR)
    {
      return;
    }
    else if (errno == ECONNABORTED)
    {
      gearmand_perror("accept");
      return;
    }
    else if (errno == EMFILE)
    {
      gearmand_perror("accept");
      return;
    }

    _clear_events(Gearmand());
    gearmand_perror("accept");
    Gearmand()->ret= GEARMAN_ERRNO;
    return;
  }

  /* 
    Since this is numeric, it should never fail. Even if it did we don't want to really error from it.
  */
  char host[NI_MAXHOST];
  char port_str[NI_MAXSERV];
  int error= getnameinfo(&sa, sa_len, host, NI_MAXHOST, port_str, NI_MAXSERV,
                         NI_NUMERICHOST | NI_NUMERICSERV);
  if (error != 0)
  {
    gearmand_gai_error("getnameinfo", error);
    strncpy(host, "-", sizeof(host));
    strncpy(port_str, "-", sizeof(port_str));
  }

  gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Accepted connection from %s:%s", host, port_str);

  gearmand_error_t ret;
  ret= gearmand_con_create(Gearmand(), fd, host, port_str, port->add_fn);
  if (ret == GEARMAN_MEMORY_ALLOCATION_FAILURE)
  {
    gearmand_sockfd_close(fd);
    return;
  }
  else if (ret != GEARMAN_SUCCESS)
  {
    Gearmand()->ret= ret;
    _clear_events(Gearmand());
  }
}

static gearmand_error_t _wakeup_init(gearmand_st *gearmand)
{
  gearmand_debug("Creating wakeup pipe");

  if (pipe(gearmand->wakeup_fd) < 0)
  {
    gearmand_perror("pipe");
    return GEARMAN_ERRNO;
  }

  int returned_flags;
  if ((returned_flags= fcntl(gearmand->wakeup_fd[0], F_GETFL, 0)) < 0)
  {
    gearmand_perror("fcntl:F_GETFL");
    return GEARMAN_ERRNO;
  }

  if (fcntl(gearmand->wakeup_fd[0], F_SETFL, returned_flags | O_NONBLOCK) < 0)
  {
    gearmand_perror("F_SETFL");
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
    gearmand_debug("Closing wakeup pipe");
    gearmand_pipe_close(gearmand->wakeup_fd[0]);
    gearmand->wakeup_fd[0]= -1;
    gearmand_pipe_close(gearmand->wakeup_fd[1]);
    gearmand->wakeup_fd[1]= -1;
  }
}

static gearmand_error_t _wakeup_watch(gearmand_st *gearmand)
{
  if (gearmand->is_wakeup_event)
  {
    return GEARMAN_SUCCESS;
  }

  gearmand_debug("Adding event for wakeup pipe");

  if (event_add(&(gearmand->wakeup_event), NULL) < 0)
  {
    gearmand_perror("event_add");
    return GEARMAN_EVENT;
  }

  gearmand->is_wakeup_event= true;
  return GEARMAN_SUCCESS;
}

static void _wakeup_clear(gearmand_st *gearmand)
{
  if (gearmand->is_wakeup_event)
  {
    gearmand_debug("Clearing event for wakeup pipe");
    if (event_del(&(gearmand->wakeup_event)) < 0)
    {
      gearmand_perror("We tried to event_del() an event which no longer existed");
      assert(! "We tried to event_del() an event which no longer existed");
    }
    gearmand->is_wakeup_event= false;
  }
}

static void _wakeup_event(int fd, short events __attribute__ ((unused)),
                          void *arg)
{
  gearmand_st *gearmand= (gearmand_st *)arg;
  uint8_t buffer[GEARMAN_PIPE_BUFFER_SIZE];
  gearmand_thread_st *thread;

  while (1)
  {
    ssize_t ret;
    ret= read(fd, buffer, GEARMAN_PIPE_BUFFER_SIZE);
    if (ret == 0)
    {
      _clear_events(gearmand);
      gearmand_fatal("read(EOF)");
      gearmand->ret= GEARMAN_PIPE_EOF;
      return;
    }
    else if (ret == -1)
    {
      if (errno == EINTR)
      {
        continue;
      }

      if (errno == EAGAIN)
      {
        break;
      }

      _clear_events(gearmand);
      gearmand_perror("_wakeup_event:read");
      gearmand->ret= GEARMAN_ERRNO;
      return;
    }

    for (ssize_t x= 0; x < ret; x++)
    {
      switch ((gearmand_wakeup_t)buffer[x])
      {
      case GEARMAND_WAKEUP_PAUSE:
        gearmand_debug("Received PAUSE wakeup event");
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_PAUSE;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
        gearmand_debug("Received SHUTDOWN_GRACEFUL wakeup event");
        _listen_close(gearmand);

        for (thread= gearmand->thread_list; thread != NULL;
             thread= thread->next)
        {
          gearmand_thread_wakeup(thread, GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
        }

        gearmand->ret= GEARMAN_SHUTDOWN_GRACEFUL;
        break;

      case GEARMAND_WAKEUP_SHUTDOWN:
        gearmand_debug("Received SHUTDOWN wakeup event");
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_SHUTDOWN;
        break;

      case GEARMAND_WAKEUP_CON:
      case GEARMAND_WAKEUP_RUN:
        gearmand_log_fatal(GEARMAN_DEFAULT_LOG_PARAM, "Received unknown wakeup event (%u)", buffer[x]);
        _clear_events(gearmand);
        gearmand->ret= GEARMAN_UNKNOWN_STATE;
        break;
      }
    }
  }
}

static gearmand_error_t _watch_events(gearmand_st *gearmand)
{
  gearmand_error_t ret;

  ret= _listen_watch(gearmand);
  if (ret != GEARMAN_SUCCESS)
  {
    return ret;
  }

  ret= _wakeup_watch(gearmand);
  if (ret != GEARMAN_SUCCESS)
  {
    return ret;
  }

  return GEARMAN_SUCCESS;
}

static void _clear_events(gearmand_st *gearmand)
{
  _listen_clear(gearmand);
  _wakeup_clear(gearmand);

  /*
    If we are not threaded, tell the fake thread to shutdown now to clear
    connections. Otherwise we will never exit the libevent loop.
  */
  if (gearmand->threads == 0 && gearmand->thread_list != NULL)
  {
    gearmand_thread_wakeup(gearmand->thread_list, GEARMAND_WAKEUP_SHUTDOWN);
  }
}

static void _close_events(gearmand_st *gearmand)
{
  _listen_close(gearmand);
  _wakeup_close(gearmand);
}

/** @} */

/*
 * Public Definitions
 */

const char *gearmand_version(void)
{
    return PACKAGE_VERSION;
}

const char *gearmand_bugreport(void)
{
    return PACKAGE_BUGREPORT;
}

const char *gearmand_verbose_name(gearmand_verbose_t verbose)
{
  switch (verbose)
  {
  case GEARMAND_VERBOSE_FATAL:
    return "FATAL";

  case GEARMAND_VERBOSE_ALERT:
    return "ALERT";

  case GEARMAND_VERBOSE_CRITICAL:
    return "CRITICAL";

  case GEARMAND_VERBOSE_ERROR:
    return "ERROR";

  case GEARMAND_VERBOSE_WARN:
    return "WARNING";

  case GEARMAND_VERBOSE_NOTICE:
    return "NOTICE";

  case GEARMAND_VERBOSE_INFO:
    return "INFO";

  case GEARMAND_VERBOSE_DEBUG:
    return "DEBUG";

  default:
    break;
  }

  return "UNKNOWN";
}

bool gearmand_verbose_check(const char *name, gearmand_verbose_t& level)
{
  bool success= true;
  if (strcmp("FATAL", name) == 0)
  {
    level= GEARMAND_VERBOSE_FATAL;
  }
  else if (strcmp("ALERT", name) == 0)
  {
    level= GEARMAND_VERBOSE_ALERT;
  }
  else if (strcmp("CRITICAL", name) == 0)
  {
    level= GEARMAND_VERBOSE_CRITICAL;
  }
  else if (strcmp("ERROR", name) == 0)
  {
    level= GEARMAND_VERBOSE_ERROR;
  }
  else if (strcmp("WARNING", name) == 0)
  {
    level= GEARMAND_VERBOSE_WARN;
  }
  else if (strcmp("NOTICE", name) == 0)
  {
    level= GEARMAND_VERBOSE_NOTICE;
  }
  else if (strcmp("INFO", name) == 0)
  {
    level= GEARMAND_VERBOSE_INFO;
  }
  else if (strcmp("DEBUG", name) == 0)
  {
    level= GEARMAND_VERBOSE_DEBUG;
  }
  else
  {
    success= false;
  }

  return success;
}

static bool gearman_server_create(gearman_server_st *server, 
                                  uint8_t job_retries_arg,
                                  uint8_t worker_wakeup_arg,
                                  bool round_robin_arg)
{
  struct utsname un;
  assert(server);

  server->state.queue_startup= false;
  server->flags.round_robin= round_robin_arg;
  server->flags.threaded= false;
  server->shutdown= false;
  server->shutdown_graceful= false;
  server->proc_wakeup= false;
  server->proc_shutdown= false;
  server->job_retries= job_retries_arg;
  server->worker_wakeup= worker_wakeup_arg;
  server->thread_count= 0;
  server->free_packet_count= 0;
  server->function_count= 0;
  server->job_count= 0;
  server->unique_count= 0;
  server->free_job_count= 0;
  server->free_client_count= 0;
  server->free_worker_count= 0;
  server->thread_list= NULL;
  server->free_packet_list= NULL;
  server->function_list= NULL;
  server->free_job_list= NULL;
  server->free_client_list= NULL;
  server->free_worker_list= NULL;

  server->queue_version= QUEUE_VERSION_FUNCTION;

  memset(server->queue.raw, 0, sizeof(server->queue));

  memset(server->job_hash, 0,
         sizeof(gearman_server_job_st *) * GEARMAND_JOB_HASH_SIZE);
  memset(server->unique_hash, 0,
         sizeof(gearman_server_job_st *) * GEARMAND_JOB_HASH_SIZE);

  if (uname(&un) == -1)
  {
    gearman_server_free(server);
    return false;
  }

  int checked_length= snprintf(server->job_handle_prefix, GEARMAND_JOB_HANDLE_SIZE, "H:%s", un.nodename);
  if (checked_length >= GEARMAND_JOB_HANDLE_SIZE or checked_length <= 0)
  {
    gearman_server_free(server);
    return false;
  }

  server->job_handle_count= 1;

  return true;
}
