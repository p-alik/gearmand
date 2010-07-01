/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include <sys/types.h>
#include <netdb.h>

pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          char *argv[], int argc);
void test_gearmand_stop(pid_t gearmand_pid);
