/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <libgearman/gearman.h>

#define GEARMAND_LOG_REOPEN_TIME 60

typedef struct
{
  char *file;
  int fd;
  time_t reopen;
} gearmand_log_info_st;

static gearmand_st *_gearmand;

static bool _set_fdlimit(rlim_t fds);
static bool _pid_write(const char *pid_file);
static void _pid_delete(const char *pid_file);
static bool _switch_user(const char *user);
static bool _set_signals(void);
static void _shutdown_handler(int signal);
static void _log(gearmand_st *gearmand __attribute__ ((unused)),
                 uint8_t verbose __attribute__ ((unused)), const char *line,
                 void *fn_arg);

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
  gearmand_log_info_st log_info;

  log_info.file= NULL;
  log_info.fd= -1;
  log_info.reopen= 0;

  while ((c = getopt(argc, argv, "b:df:hl:p:P:t:u:vV")) != EOF)
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

    case 'l':
      log_info.file= optarg;
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
      printf("\t-l <file>     - File to log errors and information to.\n"
             "\t                Turning this option on also makes forces the\n"
             "\t                first verbose level on.\n");
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

  if (log_info.file != NULL && verbose == 0)
    verbose++;

  if (_set_fdlimit(fds) || _switch_user(user) || _set_signals())
    return 1;

  if (pid_file != NULL && _pid_write(pid_file))
    return 1;

  _gearmand= gearmand_create(port);
  if (_gearmand == NULL)
  {
    fprintf(stderr, "gearmand: Could not create gearmand library instance\n");
    return 1;
  }

  gearmand_set_backlog(_gearmand, backlog);
  gearmand_set_threads(_gearmand, threads);
  gearmand_set_verbose(_gearmand, verbose);
  gearmand_set_log(_gearmand, _log, &log_info);

  ret= gearmand_run(_gearmand);

  gearmand_free(_gearmand);

  if (pid_file != NULL)
    _pid_delete(pid_file);

  if (log_info.fd != -1)
    (void) close(log_info.fd);

  return (ret == GEARMAN_SUCCESS || ret == GEARMAN_SHUTDOWN) ? 0 : 1;
}

static bool _set_fdlimit(rlim_t fds)
{
  struct rlimit rl;

  if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
  {
    fprintf(stderr, "gearmand: Could not get file descriptor limit:%d\n",
            errno);
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
        fprintf(stderr, "gearmand: Could not set file descriptor limit: %d\n",
                errno);
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
      fprintf(stderr, "gearmand: Failed to set limit for the number of file "
                      "descriptors (%d). Try running as root or giving a "
                      "smaller value to -f.\n",
              errno);
      return true;
    }
  }

  return false;
}

static bool _pid_write(const char *pid_file)
{
  FILE *f;

  f= fopen(pid_file, "w");
  if (f == NULL)
  {
    fprintf(stderr, "gearmand: Could not open pid file for writing: %s (%d)\n", pid_file,
            errno);
    return true;
  }

  fprintf(f, "%" PRId64 "\n", (int64_t)getpid());

  if (fclose(f) == -1)
  {
    fprintf(stderr, "gearmand: Could not close the pid file: %s (%d)\n", pid_file, errno);
    return true;
  }

  return false;
}

static void _pid_delete(const char *pid_file)
{
  if (unlink(pid_file) == -1)
  {
    fprintf(stderr, "gearmand: Could not remove the pid file: %s (%d)\n", pid_file,
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
              "gearmand: Must specify '-u root' if you want to run as root\n");
      return true;
    }

    pw= getpwnam(user);
    if (pw == NULL)
    {
      fprintf(stderr, "gearmand: Could not find user '%s'\n", user);
      return 1;
    }

    if (setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1)
    {
      fprintf(stderr, "gearmand: Could not switch to user '%s'\n", user);
      return 1;
    }
  }
  else if (user != NULL)
  {
    fprintf(stderr, "gearmand: Must be root to switch users\n");
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
    fprintf(stderr, "gearmand: Could not set SIGPIPE handler (%d)\n", errno);
    return true;
  }

  sa.sa_handler= _shutdown_handler;
  if (sigaction(SIGTERM, &sa, 0) == -1)
  {
    fprintf(stderr, "gearmand: Could not set SIGTERM handler (%d)\n", errno);
    return true;
  }

  if (sigaction(SIGINT, &sa, 0) == -1)
  {
    fprintf(stderr, "gearmand: Could not set SIGINT handler (%d)\n", errno);
    return true;
  }

  if (sigaction(SIGUSR1, &sa, 0) == -1)
  {
    fprintf(stderr, "gearmand: Could not set SIGUSR1 handler (%d)\n", errno);
    return true;
  }

  return false;
}

static void _shutdown_handler(int signal)
{
  if (signal == SIGUSR1)
    gearmand_wakeup(_gearmand, GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
  else
    gearmand_wakeup(_gearmand, GEARMAND_WAKEUP_SHUTDOWN);
}

static void _log(gearmand_st *gearmand __attribute__ ((unused)),
                 uint8_t verbose __attribute__ ((unused)), const char *line,
                 void *fn_arg)
{
  gearmand_log_info_st *log_info= (gearmand_log_info_st *)fn_arg;
  int fd;
  time_t t;
  char buffer[GEARMAN_MAX_ERROR_SIZE];

  if (log_info->file == NULL)
    fd= 1;
  else
  {
    t= time(NULL);

    if (log_info->fd != -1 && log_info->reopen < t)
    {
      (void) close(log_info->fd);
      log_info->fd= -1;
    }

    if (log_info->fd == -1)
    {
      log_info->fd= open(log_info->file, O_CREAT | O_WRONLY | O_APPEND, 0644);
      if (log_info->fd == -1)
      {
        fprintf(stderr, "gearmand: Could not open log file for writing (%d)\n",
                errno);
        return;
      }

      log_info->reopen= t + GEARMAND_LOG_REOPEN_TIME;
    }

    fd= log_info->fd;
  }

  snprintf(buffer, GEARMAN_MAX_ERROR_SIZE, "%s\n", line);
  if (write(fd, buffer, strlen(buffer)) == -1)
    fprintf(stderr, "gearmand: Could not write to log file: %d\n", errno);
}
