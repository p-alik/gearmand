/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include <stdint.h>

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

#include "util/error.h"
#include "util/pidfile.h"

using namespace gearman_util;

struct gearmand_log_info_st
{
  const char *file;
  int fd;
  time_t reopen;

  gearmand_log_info_st() :
    file(NULL),
    fd(-1),
    reopen(0)
  {
  }
};

static bool _set_fdlimit(rlim_t fds);
static bool _switch_user(const char *user);
extern "C" {
static bool _set_signals(void);
}
static void _shutdown_handler(int signal_arg);
static void _log(const char *line, gearman_verbose_t verbose, void *context);

static gearman_return_t queue_init(gearmand_st *_gearmand, gearman_conf_st &conf, const char *queue_type);
static void queue_deinit(gearmand_st *_gearmand, const char *queue_type);
static int queue_configure(gearman_conf_st &conf);

int main(int argc, char *argv[])
{
  int backlog= GEARMAND_LISTEN_BACKLOG;
  rlim_t fds= 0;
  uint8_t job_retries= 0;
  uint8_t worker_wakeup= 0;
  char port[NI_MAXSERV];
  const char *host= NULL;
  const char *pid_file= "";
  const char *queue_type= NULL;
  uint32_t threads= 4;
  const char *user= NULL;
  uint8_t verbose_count= 0;
  gearmand_log_info_st log_info;
  bool close_stdio= false;
  bool round_robin= false;

  port[0]= 0;

  gearman_conf_st conf;
  if (gearman_conf_create(&conf) == NULL)
  {
    error::message("gearman_conf_create: NULL");
    return 1;
  }

  gearman_conf_module_st module;
  if (gearman_conf_module_create(&conf, &module, NULL) == NULL)
  {
    error::message("gearman_conf_module_create: NULL");
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
  MCO("threads", 't', "THREADS", "Number of I/O threads to use. Default=4.")
  MCO("user", 'u', "USER", "Switch to given user after startup.")
  MCO("verbose", 'v', NULL, "Increase verbosity level by one.")
  MCO("version", 'V', NULL, "Display the version of gearmand and exit.")
  MCO("worker-wakeup", 'w', "WORKERS",
      "Number of workers to wakeup for each job received. The default is to "
      "wakeup all available workers.")

  /* Make sure none of the gearman_conf_module_add_option calls failed. */
  if (gearman_conf_return(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_conf_module_add_option", gearman_conf_error(&conf));
    return 1;
  }

  /* Add queue configuration options. */
  if (queue_configure(conf))
    return 1;


  if (gearmand_protocol_http_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_protocol_http_conf", gearman_conf_error(&conf));
    return 1;
  }

  /* Let gearman conf parse the command line arguments. */
  if (gearman_conf_parse_args(&conf, argc, argv) != GEARMAN_SUCCESS)
  {
    printf("\n%s\n\n", gearman_conf_error(&conf));
    printf("gearmand %s - %s\n\n", gearmand_version(), gearmand_bugreport());
    printf("usage: %s [OPTIONS]\n", argv[0]);
    gearman_conf_usage(&conf);
    return 1;
  }

  /* Check for option values that were given. */
  const char *name;
  const char *value;
  while (gearman_conf_module_value(&module, &name, &value))
  {
    if (not strcmp(name, "backlog"))
    {
      backlog= atoi(value);
    }
    else if (not strcmp(name, "daemon"))
    {
      switch (fork())
      {
      case -1:
	error::perror("fork");
        return 1;

      case 0:
        break;

      default:
        return 0;
      }

      if (setsid() == -1)
      {
	error::perror("setsid");
        return 1;
      }

      close_stdio= true;
    }
    else if (not strcmp(name, "file-descriptors"))
    {
      fds= static_cast<rlim_t>(atoi(value));
    }
    else if (not strcmp(name, "help"))
    {
      printf("\ngearmand %s - %s\n\n", gearmand_version(), gearmand_bugreport());
      printf("usage: %s [OPTIONS]\n", argv[0]);
      gearman_conf_usage(&conf);
      return 1;
    }
    else if (not strcmp(name, "job-retries"))
    {
      job_retries= static_cast<uint8_t>(atoi(value));
    }
    else if (not strcmp(name, "log-file"))
    {
      log_info.file= value;
    }
    else if (not strcmp(name, "listen"))
    {
      host= value;
    }
    else if (not strcmp(name, "port"))
    {
      strncpy(port, value, NI_MAXSERV);
    }
    else if (not strcmp(name, "pid-file"))
    {
      pid_file= value;
    }
    else if (not strcmp(name, "protocol"))
    {
      continue;
    }
    else if (not strcmp(name, "queue-type"))
    {
      queue_type= value;
    }
    else if (not strcmp(name, "threads"))
    {
      threads= static_cast<uint32_t>(atoi(value));
    }
    else if (not strcmp(name, "user"))
    {
      user= value;
    }
    else if (not strcmp(name, "verbose"))
    {
      verbose_count++;
    }
    else if (not strcmp(name, "round-robin"))
    {
      round_robin= true;
    }
    else if (not strcmp(name, "version"))
    {
      printf("\ngearmand %s - %s\n", gearmand_version(), gearmand_bugreport());
    }
    else if (not strcmp(name, "worker-wakeup"))
    {
      worker_wakeup= static_cast<uint8_t>(atoi(value));
    }
    else
    {
      error::message("Unknown option", name);
      return 1;
    }
  }

  if (verbose_count == 0 && close_stdio)
  {
    int fd;

    /* If we can't remap stdio, it should not a fatal error. */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
      if (dup2(fd, STDIN_FILENO) == -1)
      {
	error::perror("dup2");
        return 1;
      }

      if (dup2(fd, STDOUT_FILENO) == -1)
      {
	error::perror("dup2");
        return 1;
      }

      if (dup2(fd, STDERR_FILENO) == -1)
      {
	error::perror("dup2");
        return 1;
      }

      close(fd);
    }
  }

  if ((fds > 0 && _set_fdlimit(fds)) || _switch_user(user) || _set_signals())
    return 1;

  gearman_verbose_t verbose= verbose_count > static_cast<int>(GEARMAN_VERBOSE_CRAZY) ? GEARMAN_VERBOSE_CRAZY : static_cast<gearman_verbose_t>(verbose_count);

  Pidfile _pid_file(pid_file);

  gearmand_st *_gearmand;
  _gearmand= gearmand_create(host, port, threads, backlog, job_retries, worker_wakeup,
                             _log, &log_info, verbose,
                             round_robin);
  if (_gearmand == NULL)
  {
    error::message("Could not create gearmand library instance.");
    return 1;
  }

  if (queue_type)
  {
    gearman_return_t rc;
    if ((rc= queue_init(_gearmand, conf, queue_type)) != GEARMAN_SUCCESS)
    {
      std::string error_message;
      error_message+= "Failed to initialize queue ";
      error_message+= queue_type;
      error::message(error_message, rc);
      return 1;
    }
  }

  while (gearman_conf_module_value(&module, &name, &value))
  {
    if (strcmp(name, "protocol"))
      continue;

    if (not strcmp(value, "http"))
    {
      gearman_return_t ret;
      ret= gearmand_protocol_http_init(_gearmand, &conf);
      if (ret != GEARMAN_SUCCESS)
        return 1;
    }
    else
    {
      error::message("Unknown protocol module", value);
      return 1;
    }
  }

  gearman_return_t ret;
  ret= gearmand_run(_gearmand);

  if (queue_type)
  {
    queue_deinit(_gearmand, queue_type);
  }

  while (gearman_conf_module_value(&module, &name, &value))
  {
    if (strcmp(name, "protocol"))
      continue;

    if (not strcmp(value, "http"))
      gearmand_protocol_http_deinit(_gearmand);
  }

  gearmand_free(_gearmand);

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
    error::perror("Could not get file descriptor limit");
    return true;
  }

  rl.rlim_cur= fds;
  if (rl.rlim_max < rl.rlim_cur)
    rl.rlim_max= rl.rlim_cur;

  if (setrlimit(RLIMIT_NOFILE, &rl) == -1)
  {
    error::perror("Failed to set limit for the number of file "
		  "descriptors.  Try running as root or giving a "
		  "smaller value to -f.");
    return true;
  }

  return false;
}

static bool _switch_user(const char *user)
{
  struct passwd *pw;

  if (getuid() == 0 || geteuid() == 0)
  {
    if (user == NULL || user[0] == 0)
    {
      error::message("Must specify '-u root' if you want to run as root");
      return true;
    }

    pw= getpwnam(user);
    if (pw == NULL)
    {
      error::message("Could not find user", user);
      return 1;
    }

    if (setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1)
    {
      error::message("Could not switch to user", user);
      return 1;
    }
  }
  else if (user != NULL)
  {
    error::message("Must be root to switch users.");
    return true;
  }

  return false;
}

extern "C" {
static bool _set_signals(void)
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(struct sigaction));

  sa.sa_handler= SIG_IGN;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(SIGPIPE, &sa, 0) == -1)
  {
    error::perror("Could not set SIGPIPE handler.");
    return true;
  }

  sa.sa_handler= _shutdown_handler;
  if (sigaction(SIGTERM, &sa, 0) == -1)
  {
    error::perror("Could not set SIGTERM handler.");
    return true;
  }

  if (sigaction(SIGINT, &sa, 0) == -1)
  {
    error::perror("Could not set SIGINT handler.");
    return true;
  }

  if (sigaction(SIGUSR1, &sa, 0) == -1)
  {
    error::perror("Could not set SIGUSR1 handler.");
    return true;
  }

  return false;
}
}

static void _shutdown_handler(int signal_arg)
{
  if (signal_arg == SIGUSR1)
    gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
  else
    gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN);
}

static void _log(const char *line, gearman_verbose_t verbose, void *context)
{
  gearmand_log_info_st *log_info= static_cast<gearmand_log_info_st *>(context);
  int fd;
  time_t t;
  char buffer[GEARMAN_MAX_ERROR_SIZE];

  if (log_info->file == NULL)
  {
    fd= 1;
  }
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
	error::perror("Could not open log file for writing.");
        return;
      }

      log_info->reopen= t + GEARMAND_LOG_REOPEN_TIME;
    }

    fd= log_info->fd;
  }

  snprintf(buffer, GEARMAN_MAX_ERROR_SIZE, "%5s %s\n",
           gearmand_verbose_name(verbose), line);
  if (write(fd, buffer, strlen(buffer)) == -1)
  {
    error::perror("Could not write to log file.");
  }
}

static gearman_return_t queue_init(gearmand_st *_gearmand, gearman_conf_st &conf, const char *queue_type)
{
#ifdef HAVE_LIBDRIZZLE
  if (not strcmp(queue_type, "libdrizzle"))
  {
    return gearmand_queue_libdrizzle_init(_gearmand, &conf);
  }
  else
#endif
#ifdef HAVE_LIBMEMCACHED
  if (not strcmp(queue_type, "libmemcached"))
  {
    return gearmand_queue_libmemcached_init(_gearmand, &conf);
  }
  else
#endif
#ifdef HAVE_LIBSQLITE3
  if (not strcmp(queue_type, "libsqlite3"))
  {
    return gearmand_queue_libsqlite3_init(_gearmand, &conf);
  }
  else
#endif
#ifdef HAVE_LIBPQ
  if (not strcmp(queue_type, "libpq"))
  {
    return gearmand_queue_libpq_init(_gearmand, &conf);
  }
  else
#endif
#ifdef HAVE_LIBTOKYOCABINET
  if (not strcmp(queue_type, "libtokyocabinet"))
  {
    return gearmand_queue_libtokyocabinet_init(_gearmand, &conf);
  }
#endif        

  error::message("Unknown queue module", queue_type);

  return GEARMAN_UNKNOWN_OPTION;
}


static void queue_deinit(gearmand_st *_gearmand, const char *queue_type)
{
#ifdef HAVE_LIBDRIZZLE
  if (not strcmp(queue_type, "libdrizzle"))
    gearmand_queue_libdrizzle_deinit(_gearmand);
#endif
#ifdef HAVE_LIBMEMCACHED
  if (not strcmp(queue_type, "libmemcached"))
    gearmand_queue_libmemcached_deinit(_gearmand);
#endif
#ifdef HAVE_LIBSQLITE3
  if (not strcmp(queue_type, "libsqlite3"))
    gearmand_queue_libsqlite3_deinit(_gearmand);
#endif
#ifdef HAVE_LIBPQ
  if (not strcmp(queue_type, "libpq"))
    gearmand_queue_libpq_deinit(_gearmand);
#endif
#ifdef HAVE_LIBTOKYOCABINET
  if (not strcmp(queue_type, "libtokyocabinet"))
    gearmand_queue_libtokyocabinet_deinit(_gearmand);
#endif
}

static int queue_configure(gearman_conf_st &conf)
{
#ifdef HAVE_LIBDRIZZLE
  if (gearman_server_queue_libdrizzle_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_queue_libdrizzle_conf", gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBMEMCACHED
  if (gearman_server_queue_libmemcached_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_queue_libmemcached_conf", gearman_conf_error(&conf));
    return 1;
  }
#endif
#ifdef HAVE_LIBTOKYOCABINET
  if (gearman_server_queue_libtokyocabinet_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_queue_libtokyocabinet_conf", gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBSQLITE3
  if (gearman_server_queue_libsqlite3_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_queue_libsqlite3_conf", gearman_conf_error(&conf));
    return 1;
  }
#endif

#ifdef HAVE_LIBPQ
  if (gearman_server_queue_libpq_conf(&conf) != GEARMAN_SUCCESS)
  {
    error::message("gearman_queue_libpq_conf", gearman_conf_error(&conf));
    return 1;
  }
#endif

  return 0;
}
