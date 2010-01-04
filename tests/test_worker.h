/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include <sys/types.h>
#include <libgearman/gearman.h>

pid_t test_worker_start(in_port_t port, const char *function_name,
                        gearman_worker_fn *function, void *function_arg);
void test_worker_stop(pid_t gearmand_pid);
