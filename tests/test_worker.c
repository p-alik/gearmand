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

#include "test_worker.h"

pid_t test_worker_start(in_port_t port, const char *function_name,
                        gearman_worker_fn *function, const void *function_arg)
{
  pid_t worker_pid;
  gearman_worker_st worker;

  worker_pid= fork();
  assert(worker_pid != -1);

  if (worker_pid == 0)
  {
    assert(gearman_worker_create(&worker) != NULL);
    assert(gearman_worker_add_server(&worker, NULL, port) == GEARMAN_SUCCESS);
    assert(gearman_worker_add_function(&worker, function_name, 0, function,
           function_arg) == GEARMAN_SUCCESS);
    while (1)
    {
      int ret= gearman_worker_work(&worker);
      assert(ret == GEARMAN_SUCCESS);
    }

    /* TODO: unreachable - the only way out of the loop above is the assert 
     * gearman_worker_free(&worker);
     * exit(0);
     */
  }
  else
  {
    /* Wait for the server to start and bind the port. */
    sleep(1);
  }

  return worker_pid;
}

void test_worker_stop(pid_t worker_pid)
{
  assert(kill(worker_pid, SIGKILL) == 0);
  assert(waitpid(worker_pid, NULL, 0) == worker_pid);
}
