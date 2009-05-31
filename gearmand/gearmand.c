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

#include <libmodconf/modconf.h>

#include <libgearman/gearman.h>

#ifdef HAVE_LIBDRIZZLE
#include <libgearman/queue_libdrizzle.h>
#endif

#ifdef HAVE_LIBMEMCACHED
#include <libgearman/queue_libmemcached.h>
#endif

#define GEARMAND_LOG_REOPEN_TIME 60
#define GEARMAND_LISTEN_BACKLOG 32

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
                 gearman_verbose_t verbose __attribute__ ((unused)),
                 const char *line, void *fn_arg);

int main(int argc, char *argv[])
{
  int backlog= GEARMAND_LISTEN_BACKLOG;
  rlim_t fds= 0;
  in_port_t port= 0;
  char *host= NULL;
  char *pid_file= NULL;
  char *queue_type= NULL;
  uint32_t threads= 0;
  char *user= NULL;
  uint8_t verbose= 0;
  gearman_return_t ret;
  gearmand_log_info_st log_info;
  modconf_st modconf;
  modconf_module_st module;

  log_info.file= NULL;
  log_info.fd= -1;
  log_info.reopen= 0;

  if (modconf_create(&modconf) == NULL)
  {
    fprintf(stderr, "gearmand: modconf_create: NULL\n");
    return 1;
  }

  if (modconf_module_create(&modconf, &module, NULL) == NULL)
  {
    fprintf(stderr, "gearmand: modconf_module_create: NULL\n");
    return 1;
  }

  modconf_module_add_option(&module, "backlog", 'b', "BACKLOG",
                            "Number of backlog connections for listen.");
  modconf_module_add_option(&module, "daemon", 'd', NULL,
                            "Daemon, detach and run in the background.");
  modconf_module_add_option(&module, "file-descriptors", 'f', "FDS",
                            "Number of file descriptors to allow for the "
                            "process (total connections will be slightly "
                            "less). Default is max allowed for user.");
  modconf_module_add_option(&module, "help", 'h', NULL,
                            "Print this help menu.");
  modconf_module_add_option(&module, "log-file", 'l', "FILE",
                            "Log file to write errors and information to. "
                            "Turning this option on also forces the first "
                            "verbose level to be enabled.");
  modconf_module_add_option(&module, "listen", 'L', "ADDRESS",
                            "Address the server should listen on. Default is "
                            "INADDR_ANY.");
  modconf_module_add_option(&module, "port", 'p', "PORT",
                            "Port the server should listen on.");
  modconf_module_add_option(&module, "pid-file", 'P', "FILE",
                            "File to write process ID out to.");
  modconf_module_add_option(&module, "queue-type", 'q', "QUEUE",
                            "Persistent queue type to use.");
  modconf_module_add_option(&module, "threads", 't', "THREADS",
                            "Number of I/O threads to use. Default=0.");
  modconf_module_add_option(&module, "user", 'u', "USER",
                            "Switch to given user after startup.");
  modconf_module_add_option(&module, "verbose", 'v', NULL,
                            "Increase verbosity level by one.");
  modconf_module_add_option(&module, "version", 'V', NULL,
                            "Display the version of gearmand and exit.");

  if (modconf_return(&modconf) != MODCONF_SUCCESS)
  {
    fprintf(stderr, "gearmand: modconf_module_add_option: %s\n",
            modconf_error(&modconf));
    return 1;
  }

#ifdef HAVE_LIBDRIZZLE
  if (gearman_queue_libdrizzle_modconf(&modconf) != MODCONF_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libdrizzle_modconf: %s\n",
            modconf_error(&modconf));
    return 1;
  }
#endif
#ifdef HAVE_LIBMEMCACHED
  if (gearman_queue_libmemcached_modconf(&modconf) != MODCONF_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libmemcached_modconf: %s\n",
            modconf_error(&modconf));
    return 1;
  }
#endif

  if (modconf_parse_args(&modconf, argc, argv) != MODCONF_SUCCESS)
  {
    printf("\ngearmand %s - %s\n\n", gearman_version(), gearman_bugreport());
    printf("usage: %s [OPTIONS]\n", argv[0]);
    modconf_usage(&modconf);
    return 1;
  }

  //while(modconf_module_value(&module, &name, &value))
  {
char *name="help";
char *value="1";
    if (!strcmp(name, "backlog"))
      backlog= atoi(value);
    else if (!strcmp(name, "daemon"))
    {
      switch (fork())
      {
      case -1:
        fprintf(stderr, "gearmand: fork:%d\n", errno);
        return 1;

      case 0:
        break;

      default:
        return 0;
      }

      if (setsid() == -1)
      {
        fprintf(stderr, "gearmand: setsid:%d\n", errno);
        return 1;
      }
    }
    else if (!strcmp(name, "file-descriptors"))
      fds= atoi(optarg);
    else if (!strcmp(name, "help"))
    {
      printf("\ngearmand %s - %s\n\n", gearman_version(), gearman_bugreport());
      printf("usage: %s [OPTIONS]\n", argv[0]);
      modconf_usage(&modconf);
      return 1;
    }
    else if (!strcmp(name, "log-file"))
      log_info.file= optarg;
    else if (!strcmp(name, "listen"))
      host= optarg;
    else if (!strcmp(name, "port"))
      port= (in_port_t)atoi(optarg);
    else if (!strcmp(name, "pid-file"))
      pid_file= optarg;
    else if (!strcmp(name, "queue-type"))
      queue_type= optarg;
    else if (!strcmp(name, "threads"))
      threads= atoi(optarg);
    else if (!strcmp(name, "user"))
      user= optarg;
    else if (!strcmp(name, "verbose"))
      verbose++;
    else if (!strcmp(name, "version"))
      printf("\ngearmand %s - %s\n", gearman_version(), gearman_bugreport());
  }

  if (log_info.file != NULL && verbose == 0)
    verbose++;

  if ((fds > 0 && _set_fdlimit(fds)) || _switch_user(user) || _set_signals())
    return 1;

  if (pid_file != NULL && _pid_write(pid_file))
    return 1;

  _gearmand= gearmand_create(host, port);
  if (_gearmand == NULL)
  {
    fprintf(stderr, "gearmand: Could not create gearmand library instance\n");
    return 1;
  }

  gearmand_set_backlog(_gearmand, backlog);
  gearmand_set_threads(_gearmand, threads);
  gearmand_set_log(_gearmand, _log, &log_info, verbose);

  if (queue_type != NULL)
  {
#ifdef HAVE_LIBDRIZZLE
    if (!strcmp(queue_type, "libdrizzle"))
    {
      ret= gearmand_queue_libdrizzle_init(_gearmand);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
#ifdef HAVE_LIBMEMCACHED
    if (!strcmp(queue_type, "libmemcached"))
    {
      ret= gearmand_queue_libmemcached_init(_gearmand);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
    {
      fprintf(stderr, "gearmand: Unknown queue type: %s\n", queue_type);
      return 1;
    }
  }

  ret= gearmand_run(_gearmand);

  if (queue_type != NULL)
  {
#ifdef HAVE_LIBDRIZZLE
    if (!strcmp(queue_type, "libdrizzle"))
      gearmand_queue_libdrizzle_deinit(_gearmand);
#endif
#ifdef HAVE_LIBMEMCACHED
    if (!strcmp(queue_type, "libmemcached"))
      gearmand_queue_libmemcached_deinit(_gearmand);
#endif
  }

  gearmand_free(_gearmand);

  if (pid_file != NULL)
    _pid_delete(pid_file);

  if (log_info.fd != -1)
    (void) close(log_info.fd);

  modconf_free(&modconf);

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

  return false;
}

static bool _pid_write(const char *pid_file)
{
  FILE *f;

  f= fopen(pid_file, "w");
  if (f == NULL)
  {
    fprintf(stderr, "gearmand: Could not open pid file for writing: %s (%d)\n",
            pid_file, errno);
    return true;
  }

  fprintf(f, "%" PRId64 "\n", (int64_t)getpid());

  if (fclose(f) == -1)
  {
    fprintf(stderr, "gearmand: Could not close the pid file: %s (%d)\n",
            pid_file, errno);
    return true;
  }

  return false;
}

static void _pid_delete(const char *pid_file)
{
  if (unlink(pid_file) == -1)
  {
    fprintf(stderr, "gearmand: Could not remove the pid file: %s (%d)\n",
            pid_file, errno);
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
                 gearman_verbose_t verbose __attribute__ ((unused)),
                 const char *line, void *fn_arg)
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
