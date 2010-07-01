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


pid_t test_gearmand_start(in_port_t port, const char *queue_type,
                          char *argv[], int argc)
{
  pid_t gearmand_pid= -1;

  char file_buffer[1024];
  char log_buffer[1024];
  char buffer[8196];
  char *buffer_ptr= buffer;

  log_buffer[0]= 0;
  file_buffer[0]= 0;

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    snprintf(buffer_ptr, sizeof(buffer), "libtool --mode=execute gdb gearmand/gearmand");
    buffer_ptr+= strlen(buffer_ptr);
  }
  else if (getenv("GEARMAN_VALGRIND"))
  {
    snprintf(buffer_ptr, sizeof(buffer), "libtool --mode=execute valgrind --log-file=tests/valgrind.out --leak-check=full  --show-reachable=yes ");
    buffer_ptr+= strlen(buffer_ptr);
  }

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/gearman.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      abort();
    }
    snprintf(buffer_ptr, sizeof(buffer), "\nrun --pid-file=%s -vvvvvv --port=%u", file_buffer, port);
    buffer_ptr+= strlen(buffer_ptr);
  }
  else if (getenv("GEARMAN_LOG"))
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/gearman.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      abort();
    }
    snprintf(log_buffer, sizeof(log_buffer), "tests/gearmand.logXXXXXX");
    if (mkstemp(log_buffer) == -1)
    {
      perror("mkstemp");
      abort();
    }
    snprintf(buffer_ptr, sizeof(buffer), "./gearmand/gearmand --pid-file=%s --daemon --port=%u -vvvvvv --log-file=%s", file_buffer, port, log_buffer);
    buffer_ptr+= strlen(buffer_ptr);
  }
  else
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/gearman.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      abort();
    }
    snprintf(buffer_ptr, sizeof(buffer), "./gearmand/gearmand --pid-file=%s --daemon --port=%u", file_buffer, port);
    buffer_ptr+= strlen(buffer_ptr);
  }

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
  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    fprintf(stderr, "Pausing for startup, hit return when ready.\n");
    getchar();
  }
  else
  {
    int err= system(buffer);
    assert(err != -1);
  }

  // Sleep to make sure the server is up and running (or we could poll....)
  uint32_t counter= 3;
  while (--counter)
  {
    sleep(1);

    FILE *file;
    file= fopen(file_buffer, "r");
    if (file == NULL)
    {
      continue;
    }
    char *found= fgets(buffer, 8196, file);
    if (!found)
    {
      abort();
    }
    gearmand_pid= atoi(buffer);
    fclose(file);
  }

  if (gearmand_pid == -1)
  {
    fprintf(stderr, "Could not attach to gearman server, could server already be running on port %u\n", port);
    abort();
  }
  unlink(file_buffer);


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
