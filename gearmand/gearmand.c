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

#include <libgearman-server/gearmand.h>

#ifdef HAVE_LIBDRIZZLE
#include <libgearman-server/queue_libdrizzle.h>
#endif

#ifdef HAVE_LIBMEMCACHED
#include <libgearman-server/queue_libmemcached.h>
#endif

#ifdef HAVE_LIBSQLITE3
#include <libgearman-server/queue_libsqlite3.h>
#endif

#ifdef HAVE_LIBPQ
#include <libgearman-server/queue_libpq.h>
#endif

#ifdef HAVE_LIBTOKYOCABINET
#include <libgearman-server/queue_libtokyocabinet.h>
#endif

#include <libgearman-server/protocol_http.h>

#define GEARMAND_LOG_REOPEN_TIME 60
#define GEARMAND_LISTEN_BACKLOG 32

typedef struct
{
  const char *file;
  int fd;
  time_t reopen;
} gearmand_log_info_st;

static gearmand_st *_gearmand;

static bool _set_fdlimit(rlim_t fds);
static bool _pid_write(const char *pid_file);
static void _pid_delete(const char *pid_file);
static bool _switch_user(const char *user);
static bool _set_signals(void);
static void _shutdown_handler(int signal_arg);
static void _log(const char *line, gearman_verbose_t verbose, void *context);

int main(int argc, char *argv[])
{
  gearman_conf_st conf;
  gearman_conf_module_st module;
  const char *name;
  const char *value;
  int backlog= GEARMAND_LISTEN_BACKLOG;
  rlim_t fds= 0;
  uint8_t job_retries= 0;
  uint8_t worker_wakeup= 0;
  in_port_t port= 0;
  const char *host= NULL;
  const char *pid_file= NULL;
  const char *queue_type= NULL;
  uint32_t threads= 0;
  const char *user= NULL;
  uint8_t verbose= 0;
  gearman_return_t ret;
  gearmand_log_info_st log_info;
  bool close_stdio= false;
  int fd;
  bool round_robin= false;

  log_info.file= NULL;
  log_info.fd= -1;
  log_info.reopen= 0;

  if (gearman_conf_create(&conf) == NULL)
  {
    fprintf(stderr, "gearmand: gearman_conf_create: NULL\n");
    return 1;
  }

  if (gearman_conf_module_create(&conf, &module, NULL) == NULL)
  {
    fprintf(stderr, "gearmand: gearman_conf_module_create: NULL\n");
    return 1;
  }

  /* Add all main configuration options. */
#define MCO(__name, __short, __value, __help) \
  gearman_conf_module_add_option(&module, __name, __short, __value, __help);

  MCO("backlog", 'b', "BACKLOG", "Number of backlog connections for listen.")
  MCO("daemon", 'd', NULL, "Daemon, detach and run in the background.")
  MCO("file-descriptors", 'f', "FDS",
      "Number of file descriptors to allow for the process (total connections "
      "will be slightly less). Default is max allowed for user.")
  MCO("help", 'h', NULL, "Print this help menu.");
  MCO("job-retries", 'j', "RETRIES",
      "Number of attempts to run the job before the job server removes it. This"
      "is helpful to ensure a bad job does not crash all available workers. "
      "Default is no limit.")
  MCO("log-file", 'l', "FILE",
      "Log file to write errors and information to. Turning this option on "
      "also forces the first verbose level to be enabled.")
  MCO("listen", 'L', "ADDRESS",
      "Address the server should listen on. Default is INADDR_ANY.")
  MCO("port", 'p', "PORT", "Port the server should listen on.")
  MCO("pid-file", 'P', "FILE", "File to write process ID out to.")
  MCO("protocol", 'r', "PROTOCOL", "Load protocol module.")
  MCO("round-robin", 'R', NULL, "Assign work in round-robin order per worker"
      "connection. The default is to assign work in the order of functions "
      "added by the worker.")
  MCO("queue-type", 'q', "QUEUE", "Persistent queue type to use.")
  MCO("threads", 't', "THREADS", "Number of I/O threads to use. Default=0.")
  MCO("user", 'u', "USER", "Switch to given user after startup.")
  MCO("verbose", 'v', NULL, "Increase verbosity level by one.")
  MCO("version", 'V', NULL, "Display the version of gearmand and exit.")
  MCO("worker-wakeup", 'w', "WORKERS",
      "Number of workers to wakeup for each job received. The default is to "
      "wakeup all available workers.")

  /* Make sure none of the gearman_conf_module_add_option calls failed. */
  if (gearman_conf_return(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_conf_module_add_option: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }

  /* Add queue configuration options. */

#ifdef HAVE_LIBDRIZZLE
  if (gearman_server_queue_libdrizzle_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libdrizzle_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBMEMCACHED
  if (gearman_server_queue_libmemcached_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libmemcached_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }
#endif
#ifdef HAVE_LIBTOKYOCABINET
  if (gearman_server_queue_libtokyocabinet_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libtokyocabinet_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBSQLITE3
  if (gearman_server_queue_libsqlite3_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libsqlite3_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBPQ
  if (gearman_server_queue_libpq_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_queue_libpq_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }
#endif

  if (gearmand_protocol_http_conf(&conf) != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "gearmand: gearman_protocol_http_conf: %s\n",
            gearman_conf_error(&conf));
    return 1;
  }

  /* Let gearman conf parse the command line arguments. */
  if (gearman_conf_parse_args(&conf, argc, argv) != GEARMAN_SUCCESS)
  {
    printf("\n%s\n\n", gearman_conf_error(&conf));
    printf("gearmand %s - %s\n\n", gearman_version(), gearman_bugreport());
    printf("usage: %s [OPTIONS]\n", argv[0]);
    gearman_conf_usage(&conf);
    return 1;
  }

  /* Check for option values that were given. */
  while (gearman_conf_module_value(&module, &name, &value))
  {
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

      close_stdio= true;
    }
    else if (!strcmp(name, "file-descriptors"))
      fds= (rlim_t)atoi(value);
    else if (!strcmp(name, "help"))
    {
      printf("\ngearmand %s - %s\n\n", gearman_version(), gearman_bugreport());
      printf("usage: %s [OPTIONS]\n", argv[0]);
      gearman_conf_usage(&conf);
      return 1;
    }
    else if (!strcmp(name, "job-retries"))
      job_retries= (uint8_t)atoi(value);
    else if (!strcmp(name, "log-file"))
      log_info.file= value;
    else if (!strcmp(name, "listen"))
      host= value;
    else if (!strcmp(name, "port"))
      port= (in_port_t)atoi(value);
    else if (!strcmp(name, "pid-file"))
      pid_file= value;
    else if (!strcmp(name, "protocol"))
      continue;
    else if (!strcmp(name, "queue-type"))
      queue_type= value;
    else if (!strcmp(name, "threads"))
      threads= (uint32_t)atoi(value);
    else if (!strcmp(name, "user"))
      user= value;
    else if (!strcmp(name, "verbose"))
      verbose++;
    else if (!strcmp(name, "round-robin"))
      round_robin++;
    else if (!strcmp(name, "version"))
      printf("\ngearmand %s - %s\n", gearman_version(), gearman_bugreport());
    else if (!strcmp(name, "worker-wakeup"))
      worker_wakeup= (uint8_t)atoi(value);
    else
    {
      fprintf(stderr, "gearmand: Unknown option:%s\n", name);
      return 1;
    }
  }

  if (verbose == 0 && close_stdio)
  {
    /* If we can't remap stdio, it should not a fatal error. */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
      if (dup2(fd, STDIN_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      if (dup2(fd, STDOUT_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      if (dup2(fd, STDERR_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      close(fd);
    }
  }

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
  gearmand_set_job_retries(_gearmand, job_retries);
  gearmand_set_worker_wakeup(_gearmand, worker_wakeup);
  gearmand_set_log_fn(_gearmand, _log, &log_info, verbose);
  gearmand_set_round_robin(_gearmand, round_robin);

  if (queue_type != NULL)
  {
#ifdef HAVE_LIBDRIZZLE
    if (!strcmp(queue_type, "libdrizzle"))
    {
      ret= gearmand_queue_libdrizzle_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
#ifdef HAVE_LIBMEMCACHED
    if (!strcmp(queue_type, "libmemcached"))
    {
      ret= gearmand_queue_libmemcached_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
#ifdef HAVE_LIBSQLITE3
    if (!strcmp(queue_type, "libsqlite3"))
    {
      ret= gearmand_queue_libsqlite3_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
#ifdef HAVE_LIBPQ
    if (!strcmp(queue_type, "libpq"))
    {
      ret= gearmand_queue_libpq_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif
#ifdef HAVE_LIBTOKYOCABINET
    if (!strcmp(queue_type, "libtokyocabinet"))
    {
      ret= gearmand_queue_libtokyocabinet_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
#endif        
    {
      fprintf(stderr, "gearmand: Unknown queue module: %s\n", queue_type);
      return 1;
    }
  }

  while (gearman_conf_module_value(&module, &name, &value))
  {
    if (strcmp(name, "protocol"))
      continue;

    if (!strcmp(value, "http"))
    {
      ret= gearmand_protocol_http_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
    {
      fprintf(stderr, "gearmand: Unknown protocol module: %s\n", value);
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
#ifdef HAVE_LIBSQLITE3
    if (!strcmp(queue_type, "libsqlite3"))
      gearmand_queue_libsqlite3_deinit(_gearmand);
#endif
#ifdef HAVE_LIBPQ
    if (!strcmp(queue_type, "libpq"))
      gearmand_queue_libpq_deinit(_gearmand);
#endif
#ifdef HAVE_LIBTOKYOCABINET
    if (!strcmp(queue_type, "libtokyocabinet"))
      gearmand_queue_libtokyocabinet_deinit(_gearmand);
#endif
  }

  while (gearman_conf_module_value(&module, &name, &value))
  {
    if (strcmp(name, "protocol"))
      continue;

    if (!strcmp(value, "http"))
      gearmand_protocol_http_deinit(_gearmand);
  }

  gearmand_free(_gearmand);

  if (pid_file != NULL)
    _pid_delete(pid_file);

  if (log_info.fd != -1)
    (void) close(log_info.fd);

  gearman_conf_free(&conf);

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

static void _shutdown_handler(int signal_arg)
{
  if (signal_arg == SIGUSR1)
    gearmand_wakeup(_gearmand, GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
  else
    gearmand_wakeup(_gearmand, GEARMAND_WAKEUP_SHUTDOWN);
}

static void _log(const char *line, gearman_verbose_t verbose, void *context)
{
  gearmand_log_info_st *log_info= (gearmand_log_info_st *)context;
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

  snprintf(buffer, GEARMAN_MAX_ERROR_SIZE, "%5s %s\n",
           gearman_verbose_name(verbose), line);
  if (write(fd, buffer, strlen(buffer)) == -1)
    fprintf(stderr, "gearmand: Could not write to log file: %d\n", errno);
}
