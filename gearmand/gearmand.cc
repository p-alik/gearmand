/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

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
#include <libgearman-server/plugins.h>
#include <libgearman-server/queue.h>

#define GEARMAND_LOG_REOPEN_TIME 60

#include "util/daemon.hpp"
#include "util/pidfile.hpp"

#include <boost/program_options.hpp>
#include <iostream>

using namespace datadifferential;

namespace error {

inline void perror(const char *message)
{
  char *errmsg_ptr;
  char errmsg[BUFSIZ];
  errmsg[0]= 0;

#ifdef STRERROR_R_CHAR_P
  errmsg_ptr= strerror_r(errno, errmsg, sizeof(errmsg));
#else
  strerror_r(errno, errmsg, sizeof(errmsg));
  errmsg_ptr= errmsg;
#endif
  std::cerr << "gearman: " << message << " (" << errmsg_ptr << ")" << std::endl;
}

inline void message(const char *arg)
{
  std::cerr << "gearmand: " << arg << std::endl;
}

inline void message(const char *arg, const char *arg2)
{
  std::cerr << "gearmand: " << arg << " : " << arg2 << std::endl;
}

inline void message(const std::string &arg, gearmand_error_t rc)
{
  std::cerr << "gearmand: " << arg << " : " << gearmand_strerror(rc) << std::endl;
}

} // namespace error

struct gearmand_log_syslog_st
{
public:

  gearmand_log_syslog_st()
  {
    openlog("gearmand", LOG_PID | LOG_NDELAY, LOG_USER);
  }

  ~gearmand_log_syslog_st()
  {
    closelog();
  }

private:
};

struct gearmand_log_info_st
{
  std::string filename;
  int fd;
  time_t reopen;

  gearmand_log_info_st(const std::string &filename_arg) :
    filename(filename_arg),
    fd(-1),
    reopen(0)
  {
  }

  int file() const
  {
    return fd;
  }

  ~gearmand_log_info_st()
  {
    if (fd != -1)
    {
      close(fd);
    }
  }
};

static bool _set_fdlimit(rlim_t fds);
static bool _switch_user(const char *user);

extern "C" {
static bool _set_signals(void);
}

static void _shutdown_handler(int signal_arg);
static void _log(const char *line, gearmand_verbose_t verbose, void *context);

static bool opt_syslog;

int main(int argc, char *argv[])
{
  int backlog;
  rlim_t fds= 0;
  uint32_t job_retries;
  uint32_t worker_wakeup;

  std::string host;
  std::string user;
  std::string log_file;
  std::string pid_file;
  std::string port;
  std::string protocol;
  std::string queue_type;
  std::string verbose_string;

  verbose_string.insert(verbose_string.begin(), size_t(GEARMAND_VERBOSE_NOTICE), 'v');

  uint32_t threads;
  bool opt_round_robin;
  bool opt_daemon;
  bool opt_check_args;

  boost::program_options::options_description general("General options");

  general.add_options()
  ("backlog,b", boost::program_options::value(&backlog)->default_value(32),
   "Number of backlog connections for listen.")

  ("check-args", boost::program_options::bool_switch(&opt_check_args)->default_value(false),
   "Check command line and configuration file argments and then exit.")

  ("daemon,d", boost::program_options::bool_switch(&opt_daemon)->default_value(false),
   "Daemon, detach and run in the background.")

  ("file-descriptors,f", boost::program_options::value(&fds),
   "Number of file descriptors to allow for the process (total connections will be slightly less). Default is max allowed for user.")

  ("help,h", "Print this help menu.")

  ("job-retries,j", boost::program_options::value(&job_retries)->default_value(0),
   "Number of attempts to run the job before the job server removes it. This is helpful to ensure a bad job does not crash all available workers. Default is no limit.")

  ("log-file,l", boost::program_options::value(&log_file),
   "Log file to write errors and information to. Turning this option on also forces the first verbose level to be enabled.")

  ("listen,L", boost::program_options::value(&host),
   "Address the server should listen on. Default is INADDR_ANY.")

  ("port,p", boost::program_options::value(&port)->default_value(GEARMAN_DEFAULT_TCP_PORT_STRING), 
   "Port the server should listen on.")

  ("pid-file,P", boost::program_options::value(&pid_file), 
   "File to write process ID out to.")

  ("protocol,r", boost::program_options::value(&protocol), 
   "Load protocol module.")

  ("round-robin,R", boost::program_options::bool_switch(&opt_round_robin)->default_value(false),
   "Assign work in round-robin order per worker connection. The default is to assign work in the order of functions added by the worker.")

  ("queue-type,q", boost::program_options::value(&queue_type),
   "Persistent queue type to use.")

  ("syslog", boost::program_options::bool_switch(&opt_syslog)->default_value(false),
   "Use syslog.")

  ("threads,t", boost::program_options::value(&threads)->default_value(4),
   "Number of I/O threads to use. Default=4.")

  ("user,u", boost::program_options::value(&user),
   "Switch to given user after startup.")

  ("verbose,v", boost::program_options::value(&verbose_string)->default_value(verbose_string),
   "Increase verbosity level by one.")

  ("version,V", "Display the version of gearmand and exit.")
  ("worker-wakeup,w", boost::program_options::value(&worker_wakeup)->default_value(0),
   "Number of workers to wakeup for each job received. The default is to wakeup all available workers.")
  ;

  boost::program_options::options_description all("Allowed options");
  all.add(general);

  gearmand::protocol::HTTP http;
  all.add(http.command_line_options());

  gearmand::plugins::initialize(all);

  boost::program_options::variables_map vm;
  try {
    store(parse_command_line(argc, argv, all), vm);
    notify(vm);
  }

  catch(std::exception &e)
  {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (opt_check_args)
  {
    return EXIT_SUCCESS;
  }

  if (vm.count("help"))
  {
    std::cout << all << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("version"))
  {
    std::cout << std::endl << "gearmand " << gearmand_version() << " - " <<  gearmand_bugreport() << std::endl;
    return EXIT_FAILURE;
  }

  if (fds > 0 && _set_fdlimit(fds))
  { 
    return EXIT_FAILURE;
  }

  if (not user.empty() and _switch_user(user.c_str()))
  {
    return EXIT_FAILURE;
  }

  if (opt_daemon)
  {
    util::daemonize(false, true);
  }

  if (_set_signals())
  {
    return EXIT_FAILURE;
  }

  gearmand_verbose_t verbose= GEARMAND_VERBOSE_ERROR;
  if (verbose_string.length() <= int(GEARMAND_VERBOSE_DEBUG))
  {
    verbose= static_cast<gearmand_verbose_t>(verbose_string.length());
  }
  else
  {
    verbose= GEARMAND_VERBOSE_DEBUG;
  }

  util::Pidfile _pid_file(pid_file);

  if (not _pid_file.create())
  {
    error::perror(_pid_file.error_message().c_str());
    return EXIT_FAILURE;
  }

  gearmand_log_syslog_st log_syslog;
  gearmand_log_info_st log_info(log_file);

  gearmand_st *_gearmand= gearmand_create(host.empty() ? NULL : host.c_str(),
                                          port.c_str(), threads, backlog,
                                          static_cast<uint8_t>(job_retries),
                                          static_cast<uint8_t>(worker_wakeup),
                                          _log, &log_info, verbose,
                                          opt_round_robin);
  if (_gearmand == NULL)
  {
    error::message("Could not create gearmand library instance.");
    return EXIT_FAILURE;
  }

  if (not queue_type.empty())
  {
    gearmand_error_t rc;
    if ((rc= gearmand::queue::initialize(_gearmand, queue_type.c_str())) != GEARMAN_SUCCESS)
    {
      error::message("Error while initializing the queue", protocol.c_str());
      gearmand_free(_gearmand);

      return EXIT_FAILURE;
    }
  }

  if (not protocol.compare("http"))
  {
    if (http.start(_gearmand) != GEARMAN_SUCCESS)
    {
      error::message("Error while enabling protocol module", protocol.c_str());
      gearmand_free(_gearmand);

      return EXIT_FAILURE;
    }
  }
  else if (not protocol.empty())
  {
    error::message("Unknown protocol module", protocol.c_str());
    gearmand_free(_gearmand);

    return EXIT_FAILURE;
  }

  if (opt_daemon)
  {
    bool close_io= int(verbose) == 0 or log_file.size();
    if (not util::daemon_is_ready(close_io))
    {
      return EXIT_FAILURE;
    }
  }

  gearmand_error_t ret= gearmand_run(_gearmand);

  gearmand_free(_gearmand);

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

  if (getuid() == 0 or geteuid() == 0)
  {
    struct passwd *pw= getpwnam(user);

    if (not pw)
    {
      error::message("Could not find user", user);
      return EXIT_FAILURE;
    }

    if (setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1)
    {
      error::message("Could not switch to user", user);
      return EXIT_FAILURE;
    }
  }
  else
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
  if (sigemptyset(&sa.sa_mask) == -1 or
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
  {
    gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL);
  }
  else
  {
    gearmand_wakeup(Gearmand(), GEARMAND_WAKEUP_SHUTDOWN);
  }
}

static void _log(const char *line, gearmand_verbose_t verbose, void *context)
{
  gearmand_log_info_st *log_info= static_cast<gearmand_log_info_st *>(context);
  int fd;

  if (log_info->filename.empty())
  {
    fd= 1;
  }
  else
  {
    time_t t= time(NULL);

    if (log_info->fd != -1 && log_info->reopen < t)
    {
      (void) close(log_info->fd);
      log_info->fd= -1;
    }

    if (log_info->fd == -1)
    {
      log_info->fd= open(log_info->filename.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
      if (log_info->fd == -1)
      {
	error::perror("Could not open log file for writing.");
        return;
      }

      log_info->reopen= t + GEARMAND_LOG_REOPEN_TIME;
    }

    fd= log_info->fd;
  }

  char buffer[GEARMAN_MAX_ERROR_SIZE];
  int buffer_length= snprintf(buffer, GEARMAN_MAX_ERROR_SIZE, "%7s %s\n", gearmand_verbose_name(verbose), line);
  if (opt_syslog)
  {
    syslog(int(verbose), "%.*s", buffer_length, buffer);
  }

  if (write(log_info->file(), buffer, strlen(buffer)) == -1)
  {
    error::perror("Could not write to log file.");
  }
}
