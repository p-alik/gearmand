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

#ifdef HAVE_LIBTOKYOCABINET
#include <libgearman-server/queue_libtokyocabinet.h>
#endif

pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          char *argv[], int argc)
{
  pid_t gearmand_pid;

  char file_buffer[1024];
  char buffer[8196];
  char *buffer_ptr= buffer;

  if (getenv("GEARMAN_VALGRIND"))
  {
    snprintf(buffer_ptr, sizeof(buffer), "libtool --mode=execute valgrind --log-file=tests/valgrind.out --leak-check=full  --show-reachable=yes ");
    buffer_ptr+= strlen(buffer_ptr);
  }

  snprintf(file_buffer, sizeof(file_buffer), "tests/gearman.pidXXXXXX");
  mkstemp(file_buffer);
  snprintf(buffer_ptr, sizeof(buffer), "./gearmand/gearmand --pid-file=%s --daemon --port=%u", file_buffer, port);
  buffer_ptr+= strlen(buffer_ptr);

  if (queue_type)
  {
    snprintf(buffer_ptr, sizeof(buffer), " --queue-type=%s ", queue_type);
    buffer_ptr+= strlen(buffer_ptr);
  }

  for (int x= 1 ; x < argc ; x++)
  {
    snprintf(buffer_ptr, sizeof(buffer), " %s ", argv[x]);
  }

  fprintf(stderr, "%s\n", buffer);
  system(buffer);

  FILE *file;
  file= fopen(file_buffer, "r");
  assert(file);
  fgets(buffer, 8196, file);
  gearmand_pid= atoi(buffer);
  assert(gearmand_pid);
  fclose(file);

  sleep(3);

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

    return;
  }

  pid= waitpid(gearmand_pid, NULL, 0);

  sleep(3);
}
