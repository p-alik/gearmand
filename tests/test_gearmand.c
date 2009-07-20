/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test_gearmand.h"

#ifdef HAVE_LIBDRIZZLE
#include <libgearman/queue_libdrizzle.h>
#endif

#ifdef HAVE_LIBMEMCACHED
#include <libgearman/queue_libmemcached.h>
#endif

#ifdef HAVE_LIBSQLITE3
#include <libgearman/queue_libsqlite3.h>
#endif

pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          char *argv[], int argc)
{
  pid_t gearmand_pid;
  gearmand_st *gearmand;
  gearman_conf_st conf;

  assert((gearmand_pid= fork()) != -1);

  if (gearmand_pid == 0)
  {
    assert(gearman_conf_create(&conf) != NULL);
#ifdef HAVE_LIBDRIZZLE
    assert(gearman_queue_libdrizzle_conf(&conf) == GEARMAN_SUCCESS);
#endif
#ifdef HAVE_LIBMEMCACHED
    assert(gearman_queue_libmemcached_conf(&conf) == GEARMAN_SUCCESS);
#endif
#ifdef HAVE_LIBSQLITE3
    assert(gearman_queue_libsqlite3_conf(&conf) == GEARMAN_SUCCESS);
#endif

    assert(gearman_conf_parse_args(&conf, argc, argv) == GEARMAN_SUCCESS);

    gearmand= gearmand_create(NULL, port);
    assert(gearmand != NULL);

    if (queue_type != NULL)
    {
      assert(argc);
      assert(argv);
#ifdef HAVE_LIBDRIZZLE
      if (!strcmp(queue_type, "libdrizzle"))
        assert((gearmand_queue_libdrizzle_init(gearmand, &conf)) == GEARMAN_SUCCESS);
      else
#endif
#ifdef HAVE_LIBMEMCACHED
      if (!strcmp(queue_type, "libmemcached"))
        assert((gearmand_queue_libmemcached_init(gearmand, &conf)) == GEARMAN_SUCCESS);
      else
#endif
#ifdef HAVE_LIBSQLITE3
      if (!strcmp(queue_type, "libsqlite3"))
        assert((gearmand_queue_libsqlite3_init(gearmand, &conf)) == GEARMAN_SUCCESS);
      else
#endif
      {
        assert(1);
      }
    }

    assert(gearmand_run(gearmand) != GEARMAN_SUCCESS);

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
  assert(kill(gearmand_pid, SIGKILL) == 0);
  assert(waitpid(gearmand_pid, NULL, 0) == gearmand_pid);
}
