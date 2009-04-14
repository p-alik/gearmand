/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static gearmand_st *_gearmand;

static bool _set_fdlimit(rlim_t fds, uint8_t verbose);
static bool _pid_write(const char *pid_file);
static void _pid_delete(const char *pid_file);
static bool _switch_user(const char *user);
static bool _set_signals(void);
static void _term_handler(int signal __attribute__ ((unused)));

int main(int argc, char *argv[])
{
  int c;
  int backlog= 32;
  rlim_t fds= 0;
  in_port_t port= 0;
  char *pid_file= NULL;
  uint32_t threads= 0;
  char *user= NULL;
  uint8_t verbose= 0;
  gearman_return_t ret;

  while ((c = getopt(argc, argv, "b:df:hp:P:t:u:vV")) != EOF)
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

    case 'f':
      fds= atoi(optarg);
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

    case 'u':
      user= optarg;
      break;

    case 'v':
      verbose++;
      break;

    case 'V':
      printf("\ngearmand %s - %s\n", gearman_version(), gearman_bugreport());
      return 1;

    case 'h':
    default:
      printf("\ngearmand %s - %s\n\n", gearman_version(), gearman_bugreport());
      printf("usage: %s [options]\n\n", argv[0]);
      printf("\t-b <backlog>  - Number of backlog connections for listen.\n"
             "\t                Default is 32.\n");
      printf("\t-d            - Detach and run in the background (daemon).\n");
      printf("\t-f <fds>      - Number of file descriptors to allow for the\n"
             "\t                process (total connections will be slightly\n"
             "\t                less). Default is max allowed.\n");
      printf("\t-h            - Print this help menu.\n");
      printf("\t-p <port>     - Port the server should listen on. Default\n"
             "\t                is %u.\n", GEARMAN_DEFAULT_TCP_PORT);
      printf("\t-P <pid_file> - File to write process ID out to.\n");
      printf("\t-t <threads>  - Number of I/O threads to use. Default is 0.\n");
      printf("\t-u <user>     - Switch to given user after startup.\n");
      printf("\t-v            - Increase verbosity level by one.\n");
      printf("\t-V            - Display the version of gearmand and exit.\n");
      return 1;
    }
  }

  if (_set_fdlimit(fds, verbose) || _switch_user(user) || _set_signals())
    return 1;

  if (pid_file != NULL && _pid_write(pid_file))
    return 1;

  _gearmand= gearmand_create(port);
  if (_gearmand == NULL)
  {
    fprintf(stderr, "gearmand_create:NULL\n");
    return 1;
  }

  gearmand_set_backlog(_gearmand, backlog);
  gearmand_set_threads(_gearmand, threads);
  gearmand_set_verbose(_gearmand, verbose);

  ret= gearmand_run(_gearmand);
  if (ret != GEARMAN_SUCCESS && ret != GEARMAN_SHUTDOWN &&
      ret != GEARMAN_SHUTDOWN_GRACEFUL)
  {
    fprintf(stderr, "gearmand_run:%s\n", gearmand_error(_gearmand));
  }

  gearmand_free(_gearmand);

  if (pid_file != NULL)
    _pid_delete(pid_file);

  return ret == GEARMAN_SUCCESS ? 0 : 1;
}

static bool _set_fdlimit(rlim_t fds, uint8_t verbose)
{
  struct rlimit rl;

  if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
  {
    fprintf(stderr, "getrlimit(RLIMIT_NOFILE):%d\n", errno);
    return true;
  }

  if (fds == 0)
  {
    fds= rl.rlim_max;
    rl.rlim_cur= RLIM_INFINITY;
    rl.rlim_max= RLIM_INFINITY;

    if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
      rl.rlim_cur= fds;
      rl.rlim_max= fds;

      if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
      {
        fprintf(stderr, "setrlimit(RLIMIT_NOFILE):%d\n", errno);
        return true;
      }
    }
  }
  else
  {
    rl.rlim_cur= fds;
    if (rl.rlim_max < rl.rlim_cur)
      rl.rlim_max= rl.rlim_cur;

    if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
      fprintf(stderr, "Failed to set limit for the number of file descriptors "
              "(%d). Try running as root or giving a smaller value to -f.\n",
              errno);
      return true;
    }
  }

  if (verbose > 0)
    printf("Max file descriptors set to %" PRIu64 "\n", (uint64_t) rl.rlim_cur);

  return false;
}

static bool _pid_write(const char *pid_file)
{
  FILE *f;

  f= fopen(pid_file, "w");
  if (f == NULL)
  {
    fprintf(stderr, "Could not open pid file for writing: %s (%d)\n", pid_file,
            errno);
    return true;
  }

  fprintf(f, "%" PRId64 "\n", (int64_t)getpid());

  if (fclose(f) == -1)
  {
    fprintf(stderr, "Could not close the pid file: %s (%d)\n", pid_file, errno);
    return true;
  }

  return false;
}

static void _pid_delete(const char *pid_file)
{
  if (unlink(pid_file) == -1)
  {
    fprintf(stderr, "Could not remove the pid file: %s (%d)\n", pid_file,
            errno);
  }
}

static bool _switch_user(const char *user)
{
  struct passwd *pw;

  if (getuid() == 0 || geteuid() == 0)
  {
    if (user == NULL || user[0] == 0)
    {
      fprintf(stderr,
              "Must specify '-u root' if you really want to run as root\n");
      return true;
    }

    pw= getpwnam(user);
    if (pw == NULL)
    {
      fprintf(stderr, "Could not find user '%s' to switch to\n", user);
      return 1;
    }

    if (setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1)
    {
      fprintf(stderr, "Could not switch to user '%s'\n", user);
      return 1;
    }
  }
  else if (user != NULL)
  {
    fprintf(stderr, "Must be root to switch users\n");
    return true;
  }

  return false;
}

static bool _set_signals(void)
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(struct sigaction));

  sa.sa_handler= SIG_IGN;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(SIGPIPE, &sa, 0) == -1)
  {
    fprintf(stderr, "Could not set SIGPIPE handler (%d)\n", errno);
    return true;
  }

  sa.sa_handler= _term_handler;
  if (sigaction(SIGTERM, &sa, 0) == -1)
  {
    fprintf(stderr, "Could not set SIGTERM handler (%d)\n", errno);
    return true;
  }

  if (sigaction(SIGINT, &sa, 0) == -1)
  {
    fprintf(stderr, "Could not set SIGINT handler (%d)\n", errno);
    return true;
  }

  return false;
}

static void _term_handler(int signal __attribute__ ((unused)))
{
  gearmand_wakeup(_gearmand, GEARMAND_WAKEUP_SHUTDOWN);
}
