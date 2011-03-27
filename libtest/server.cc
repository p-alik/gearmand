/* Gearman server and library
 * Copyright (C) 2011 Data Differential, http://datadifferential.com/
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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libtest/server.h>
#include <libtest/wait.h>

#include <libgearman/gearman.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

static bool wait_and_check_startup(const char *hostname, in_port_t port)
{
  gearman_client_st *client;

  client= gearman_client_create(NULL);
  if (! client)
  {
    return false;
  }

  gearman_return_t rc;

  if ((rc= gearman_client_add_server(client, hostname, port)) == GEARMAN_SUCCESS)
  {
    const char *value= "This is my echo test";
    size_t value_length= strlen(value);

    int counter= 5; // sleep cycles
    while (--counter)
    {
      rc= gearman_client_echo(client, (uint8_t *)value, value_length);

      if (rc == GEARMAN_SUCCESS)
        break;

      if (rc == GEARMAN_ERRNO && gearman_client_errno(client) == ECONNREFUSED)
      {
        sleep(1);
        continue;
      }
    }
  }
  
  if (rc != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "wait_and_check_startup(%s)", gearman_client_error(client));
  }
  gearman_client_free(client);

  return rc == GEARMAN_SUCCESS;
}


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
    snprintf(buffer_ptr, sizeof(buffer), "libtool --mode=execute valgrind --log-file=tests/var/tmp/valgrind.out --leak-check=full  --show-reachable=yes ");
    buffer_ptr+= strlen(buffer_ptr);
  }

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/gearmand.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    snprintf(buffer_ptr, sizeof(buffer), "\nrun --pid-file=%s -vvvvvv --port=%u", file_buffer, (uint32_t)port);
    buffer_ptr+= strlen(buffer_ptr);
  }
  else if (getenv("GEARMAN_LOG"))
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/gearmand.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    snprintf(log_buffer, sizeof(log_buffer), "tests/var/log/gearmand.logXXXXXX");
    if (mkstemp(log_buffer) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    snprintf(buffer_ptr, sizeof(buffer), "./gearmand/gearmand --pid-file=%s --daemon --port=%u -vvvvvv --log-file=%s", file_buffer, (uint32_t)port, log_buffer);
    buffer_ptr+= strlen(buffer_ptr);
  }
  else
  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/gearmand.pidXXXXXX");
    if (mkstemp(file_buffer) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    snprintf(buffer_ptr, sizeof(buffer), "./gearmand/gearmand --pid-file=%s --daemon --port=%u", file_buffer, (uint32_t)port);
    buffer_ptr+= strlen(buffer_ptr);
  }

  if (queue_type)
  {
    snprintf(buffer_ptr, sizeof(buffer), " --queue-type=%s ", queue_type);
    buffer_ptr+= strlen(buffer_ptr);
  }

  if (getuid() == 0 || geteuid() == 0)
  {
    snprintf(buffer_ptr, sizeof(buffer), " -u root ");
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

  libtest::Wait wait(file_buffer);
  
  if (not wait.successful())
    return -1;

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

    char *found= fgets(buffer, sizeof(buffer), file);
    if (!found)
    {
      return -1;
    }
    gearmand_pid= atoi(buffer);
    fclose(file);
  }

  if (gearmand_pid == -1)
  {
    fprintf(stderr, "Could not attach to gearman server, could server already be running on port %u\n", (uint32_t)port);
    return -1;
  }

  if (! wait_and_check_startup(NULL, port))
  {
    test_gearmand_stop(gearmand_pid);
    fprintf(stderr, "Failed wait_and_check_startup()\n");
    return -1;
  }

  return gearmand_pid;
}

void test_gearmand_stop(pid_t gearmand_pid)
{
  pid_t pid;
  
  if ((kill(gearmand_pid, SIGTERM) == -1))
  {
    switch (errno)
    {
    case EPERM:
      perror(__func__);
      fprintf(stderr, "%s -> Does someone else have a gearmand server running locally?\n", __func__);
      return;
    case ESRCH:
      perror(__func__);
      fprintf(stderr, "Process %d not found.\n", (int)gearmand_pid);
      return;
    default:
    case EINVAL:
      perror(__func__);
      return;
    }
  }

  int status= 0;
  pid= waitpid(gearmand_pid, &status, 0);

  if (WCOREDUMP(status))
  {
    fprintf(stderr, "A core dump was created from the server\n");
  }

  if (WIFEXITED(status))
    return;

  sleep(3);
}
