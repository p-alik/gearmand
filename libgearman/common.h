/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief System include files
 */

#ifndef __GEARMAN_COMMON_H__
#define __GEARMAN_COMMON_H__

#include "config.h"

#include "gearman.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_EVENT_H
#include <event.h>
#endif

/**
 * @ingroup gearmand
 */
struct gearmand
{
  in_port_t port;
  int backlog;
  uint8_t verbose;
  gearman_server_st server;
  int listen_fd;
  gearman_return_t ret;
#ifdef HAVE_EVENT_H
  struct event_base *base;
  struct event listen_event;
#endif
};

/**
 * @ingroup gearmand
 */
struct gearmand_con
{
  int fd;
  struct sockaddr_in sa;
  gearmand_st *gearmand;
  gearman_server_con_st server_con;
#ifdef HAVE_EVENT_H
  struct event event;
#endif
  gearman_con_st *con;
};

#endif /* __GEARMAN_COMMON_H__ */
