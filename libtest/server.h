/* Gearman server and library
 * Copyright (C) 2011 Data Differential, http://datadifferential.com/
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include "libtest/visibility.h"

#include <sys/types.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBTEST_API
pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          int argc, const char *argv[]);

LIBTEST_API
void test_gearmand_stop(pid_t gearmand_pid);

#ifdef __cplusplus
}
#endif
