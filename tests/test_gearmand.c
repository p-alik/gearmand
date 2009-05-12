/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test_gearmand.h"

pid_t test_gearmand_start(in_port_t port)
{
  pid_t gearmand_pid;
  gearmand_st *gearmand;

  assert((gearmand_pid= fork()) != -1);

  if (gearmand_pid == 0)
  {
    gearmand= gearmand_create(NULL, port);
    assert(gearmand != NULL);
    assert(gearmand_run(gearmand) != GEARMAN_SUCCESS);
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
