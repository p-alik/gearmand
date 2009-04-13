/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static void _pid_write(const char *pid_file);
static void _pid_delete(const char *pid_file);

int main(int argc, char *argv[])
{
  int c;
  in_port_t port= 0;
  int backlog= 32;
  uint32_t threads= 0;
  uint8_t verbose= 0;
  char *pid_file= NULL;
  gearmand_st *gearmand;
  gearman_return_t ret;

  while ((c = getopt(argc, argv, "b:dhp:P:t:vV")) != EOF)
  {
    switch(c)
    {
    case 'b':
      backlog= atoi(optarg);
      break;

    case 'd':
      switch (fork())
      {
      case -1:
        fprintf(stderr, "fork:%d\n", errno);
        return 1;

      case 0:
        break;

      default:
        return 0;
      }

      if (setsid() == -1)
      {
        fprintf(stderr, "setsid:%d\n", errno);
        return 1;
      }

      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 'P':
      pid_file= optarg;
      break;

    case 't':
      threads= atoi(optarg);
      break;

    case 'v':
      verbose++;
      break;

    case 'V':
      printf("\ngearmand %s - %s\n", gearman_version(), gearman_bugreport());
      return 1;

    case 'h':
    default:
      printf("\ngearmand %s - %s\n", gearman_version(), gearman_bugreport());
      printf("usage: %s [options]\n", argv[0]);
      printf("\t-b <backlog>  - number of backlog connections for listen\n");
      printf("\t-d            - detach and run in the background\n");
      printf("\t-h            - print this help menu\n");
      printf("\t-p <port>     - port for server to listen on\n");
      printf("\t-P <pid_file> - file to write pid out to\n");
      printf("\t-t <threads>  - number of I/O threads to use\n");
      printf("\t-v            - increase verbosity level by one\n");
      printf("\t-V            - display the version of gearmand and exit\n");
      return 1;
    }
  }

  gearmand= gearmand_create(port);
  if (gearmand == NULL)
  {
    fprintf(stderr, "gearmand_create:NULL\n");
    return 1;
  }

  gearmand_set_backlog(gearmand, backlog);
  gearmand_set_threads(gearmand, threads);
  gearmand_set_verbose(gearmand, verbose);

  if (pid_file != NULL)
    _pid_write(pid_file);

  ret= gearmand_run(gearmand);
  if (ret != GEARMAN_SHUTDOWN && ret != GEARMAN_SUCCESS)
    fprintf(stderr, "gearmand_run:%s\n", gearmand_error(gearmand));

  gearmand_free(gearmand);

  if (pid_file != NULL)
    _pid_delete(pid_file);

  return ret == GEARMAN_SUCCESS ? 0 : 1;
}

static void _pid_write(const char *pid_file)
{
  FILE *f;

  f= fopen(pid_file, "w");
  if (f == NULL)
  {
    fprintf(stderr, "Could not open pid file for writing: %s (%d)\n", pid_file,
            errno);
    return;
  }

  fprintf(f, "%" PRId64 "\n", (int64_t)getpid());

  if (fclose(f) == -1)
  {
    fprintf(stderr, "Could not close the pid file: %s (%d)\n", pid_file, errno);
    return;
  }
}

static void _pid_delete(const char *pid_file)
{
  (void) unlink(pid_file);
}
