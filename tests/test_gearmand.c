/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test_gearmand.h"

#ifdef HAVE_LIBDRIZZLE
#include <libgearman-server/queue_libdrizzle.h>
#endif

#ifdef HAVE_LIBMEMCACHED
#include <libgearman-server/queue_libmemcached.h>
#endif

#ifdef HAVE_LIBSQLITE3
#include <libgearman-server/queue_libsqlite3.h>
#endif

pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          char *argv[], int argc)
{
  pid_t gearmand_pid;
  gearmand_st *gearmand;
  gearman_conf_st conf;

  gearmand_pid= fork();
  assert(gearmand_pid != -1);

  if (gearmand_pid == 0)
  {
    gearman_conf_st *conf_ptr;
    gearman_return_t ret;

    conf_ptr= gearman_conf_create(&conf);
    assert(conf_ptr);
#ifdef HAVE_LIBDRIZZLE
    ret= gearman_server_queue_libdrizzle_conf(&conf);
    assert(ret == GEARMAN_SUCCESS);
#endif
#ifdef HAVE_LIBMEMCACHED
    ret= gearman_server_queue_libmemcached_conf(&conf);
    assert(ret == GEARMAN_SUCCESS);
#endif
#ifdef HAVE_LIBSQLITE3
    ret= gearman_server_queue_libsqlite3_conf(&conf);
    assert(ret == GEARMAN_SUCCESS);
#endif

    ret= gearman_conf_parse_args(&conf, argc, argv);
    assert(ret == GEARMAN_SUCCESS);

    gearmand= gearmand_create(NULL, port);
    assert(gearmand != NULL);

    if (queue_type != NULL)
    {
      assert(argc);
      assert(argv);
#ifdef HAVE_LIBDRIZZLE
      if (! strcmp(queue_type, "libdrizzle"))
      {
        ret= gearmand_queue_libdrizzle_init(gearmand, &conf);
        assert(ret == GEARMAN_SUCCESS);
      }
      else
#endif
#ifdef HAVE_LIBMEMCACHED
      if (! strcmp(queue_type, "libmemcached"))
      {
        ret= gearmand_queue_libmemcached_init(gearmand, &conf);
        assert(ret == GEARMAN_SUCCESS);
      }
      else
#endif
#ifdef HAVE_LIBSQLITE3
      if (! strcmp(queue_type, "libsqlite3"))
      {
        ret= gearmand_queue_libsqlite3_init(gearmand, &conf);
        assert(ret == GEARMAN_SUCCESS);
      }
      else
#endif
      {
        assert(1);
      }
    }

    ret= gearmand_run(gearmand);
    assert(ret != GEARMAN_SUCCESS);

    if (queue_type != NULL)
    {
#ifdef HAVE_LIBDRIZZLE
      if (!strcmp(queue_type, "libdrizzle"))
        gearmand_queue_libdrizzle_deinit(gearmand);
#endif
#ifdef HAVE_LIBMEMCACHED
      if (!strcmp(queue_type, "libmemcached"))
        gearmand_queue_libmemcached_deinit(gearmand);
#endif
#ifdef HAVE_LIBSQLITE3
      if (!strcmp(queue_type, "libmemcached"))
        gearmand_queue_libsqlite3_deinit(gearmand);
#endif
    }

    gearmand_free(gearmand);
    gearman_conf_free(&conf);
    exit(0);
  }
  else
  {
    /* Wait for the server to start and bind the port. */
    sleep(1);
  }

  return gearmand_pid;
}

void test_gearmand_stop(pid_t gearmand_pid)
{
  int ret;
  pid_t pid;

  ret= kill(gearmand_pid, SIGKILL);
  
  if (ret != 0)
  {
    if (ret == -1)
    {
      perror(strerror(errno));
    }

    assert(ret == 0); // If assert is enabled, we assert(), otherwise we return
    return;
  }

  pid= waitpid(gearmand_pid, NULL, 0);
  assert(pid == gearmand_pid);
}
