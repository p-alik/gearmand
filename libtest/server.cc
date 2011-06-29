/* Gearman server and library
 * Copyright (C) 2011 Data Differential, http://datadifferential.com/
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <assert.h>
#include <iostream>
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

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static bool wait_and_check_startup(const char *hostname, in_port_t port)
{
  gearman_client_st *client;

  client= gearman_client_create(NULL);
  if (not client)
  {
    return false;
  }

  gearman_return_t rc;
  if ((rc= gearman_client_add_server(client, hostname, port)) == GEARMAN_SUCCESS)
  {
    int counter= 5; // sleep cycles
    while (--counter)
    {
      rc= gearman_client_echo(client, gearman_literal_param("This is my echo test"));

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
    std::cerr << "wait_and_check_startup(" <<  gearman_client_error(client) << ")" << std::endl;
  }
  gearman_client_free(client);

  return rc == GEARMAN_SUCCESS;
}

#include <sstream>

pid_t test_gearmand_start(in_port_t port, int argc, const char *argv[])
{
  std::stringstream buffer;

  char file_buffer[1024];
  char log_buffer[1024];

  log_buffer[0]= 0;
  file_buffer[0]= 0;

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    buffer << "libtool --mode=execute gdb gearmand/gearmand";
  }
  else if (getenv("GEARMAN_VALGRIND"))
  {
    buffer << "libtool --mode=execute valgrind --log-file=tests/var/tmp/valgrind.out --leak-check=full  --show-reachable=yes ";
  }

  {
    snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/gearmand.pidXXXXXX");
    int fd;
    if ((fd= mkstemp(file_buffer)) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    close(fd);
  }

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    buffer << std::endl << "run --pid-file=" << file_buffer << " -vvvvvv --port=" << port;
  }
  else if (getenv("GEARMAN_LOG"))
  {
    snprintf(log_buffer, sizeof(log_buffer), "tests/var/log/gearmand.logXXXXXX");
    int log_fd;
    if ((log_fd= mkstemp(log_buffer)) == -1)
    {
      perror("mkstemp");
      return -1;
    }
    close(log_fd);
    buffer << "./gearmand/gearmand --pid-file=" << file_buffer << " --daemon --port=" << port << " -vvvvvv --log-file=" << log_buffer;
  }
  else
  {
    buffer << "./gearmand/gearmand --pid-file=" << file_buffer << " --daemon --port=" << port;
  }

  if (getuid() == 0 || geteuid() == 0)
  {
    buffer << " -u root ";
  }

  for (int x= 1 ; x < argc ; x++)
  {
    buffer << " " << argv[x] << " ";
  }

  if (getenv("GEARMAN_MANUAL_GDB"))
  {
    std::cerr << "Pausing for startup, hit return when ready." << std::endl;
    getchar();
  }
  else
  {
    setenv("GEARMAN_SERVER_STARTUP", buffer.str().c_str(), 1);
    int err= system(buffer.str().c_str());
    assert(err != -1);
  }

  libtest::Wait wait(file_buffer);
  
  if (not wait.successful())
  {
    return -1;
  }

  // Sleep to make sure the server is up and running (or we could poll....)
  pid_t gearmand_pid= -1;
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

    char fgets_buffer[1024];
    char *found= fgets(fgets_buffer, sizeof(fgets_buffer), file);
    fclose(file);
    if (not found)
    {
      return -1;
    }
    gearmand_pid= atoi(fgets_buffer);

    if (gearmand_pid > 0)
    {
      break;
    }
  }

  if (gearmand_pid == -1)
  {
    std::cerr << "Could not attach to gearman server, could server already be running on port(" << port << ")?" << std::endl;
    return -1;
  }

  if (not wait_and_check_startup(NULL, port))
  {
    test_gearmand_stop(gearmand_pid);
    std::cerr << "Failed wait_and_check_startup()" << std::endl;
    return -1;
  }

  return gearmand_pid;
}

void test_gearmand_stop(pid_t gearmand_pid)
{
  if ((kill(gearmand_pid, SIGTERM) == -1))
  {
    switch (errno)
    {
    case EPERM:
      perror(__func__);
      std::cerr << __func__ << " -> Does someone else have a gearmand server running locally?" << std::endl;
      return;
    case ESRCH:
      perror(__func__);
      std::cerr << "Process " << (int)gearmand_pid << " not found." << std::endl;
      return;
    default:
    case EINVAL:
      perror(__func__);
      return;
    }
  }

  int status= 0;
  pid_t pid= waitpid(gearmand_pid, &status, 0);
  (void)pid; // @todo update such that we look at the return value for waitpid()

  if (WCOREDUMP(status))
  {
    std::cerr << "A core dump was created from the server." << std::endl;
  }

  if (WIFEXITED(status))
    return;

  sleep(3);
}
