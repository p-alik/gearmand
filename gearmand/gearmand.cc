/* Gearman server and library
 *
 * Copyright (C) 2011 Data Differential LLC
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>
#include <configmake.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
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
#include <libgearman-server/queue.hpp>

#define GEARMAND_LOG_REOPEN_TIME 60

#include "util/daemon.hpp"
#include "util/pidfile.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>

#include "gearmand/error.hpp"
#include "gearmand/log.hpp"

using namespace datadifferential;
using namespace gearmand;

static bool _set_fdlimit(rlim_t fds);
static bool _switch_user(const char *user);

extern "C" {
static bool _set_signals(void);
}

static void _log(const char *line, gearmand_verbose_t verbose, void *context);

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
  std::string protocol;
  std::string queue_type;
  std::string verbose_string= "ERROR";
  std::string config_file;

  uint32_t threads;
  bool opt_round_robin;
  bool opt_daemon;
  bool opt_check_args;
  bool opt_syslog;

  boost::program_options::options_description general("General options");

  general.add_options()
  ("backlog,b", boost::program_options::value(&backlog)->default_value(32),
   "Number of backlog connections for listen.")

  ("daemon,d", boost::program_options::bool_switch(&opt_daemon)->default_value(false),
   "Daemon, detach and run in the background.")

  ("file-descriptors,f", boost::program_options::value(&fds),
   "Number of file descriptors to allow for the process (total connections will be slightly less). Default is max allowed for user.")

  ("help,h", "Print this help menu.")

  ("job-retries,j", boost::program_options::value(&job_retries)->default_value(0),
   "Number of attempts to run the job before the job server removes it. This is helpful to ensure a bad job does not crash all available workers. Default is no limit.")

  ("log-file,l", boost::program_options::value(&log_file)->default_value(LOCALSTATEDIR"/log/gearmand.log"),
   "Log file to write errors and information to. If the log-file paramater is specified as 'stderr', then output will go to stderr. If 'none', then no logfile will be generated.")

  ("listen,L", boost::program_options::value(&host),
   "Address the server should listen on. Default is INADDR_ANY.")

  ("pid-file,P", boost::program_options::value(&pid_file)->default_value(GEARMAND_PID),
   "File to write process ID out to.")

  ("protocol,r", boost::program_options::value(&protocol),
   "Load protocol module.")

  ("round-robin,R", boost::program_options::bool_switch(&opt_round_robin)->default_value(false),
   "Assign work in round-robin order per worker connection. The default is to assign work in the order of functions added by the worker.")

  ("queue-type,q", boost::program_options::value(&queue_type)->default_value("builtin"),
   "Persistent queue type to use.")

  ("config-file", boost::program_options::value(&config_file)->default_value(GEARMAND_CONFIG),
   "Can be specified with '@name', too")

  ("syslog", boost::program_options::bool_switch(&opt_syslog)->default_value(false),
   "Use syslog.")

  ("threads,t", boost::program_options::value(&threads)->default_value(4),
   "Number of I/O threads to use. Default=4.")

  ("user,u", boost::program_options::value(&user),
   "Switch to given user after startup.")

  ("verbose", boost::program_options::value(&verbose_string)->default_value(verbose_string),
   "Set verbose level (FATAL, ALERT, CRITICAL, ERROR, WARNING, NOTICE, INFO, DEBUG).")

  ("version,V", "Display the version of gearmand and exit.")
  ("worker-wakeup,w", boost::program_options::value(&worker_wakeup)->default_value(0),
   "Number of workers to wakeup for each job received. The default is to wakeup all available workers.")
  ;

  boost::program_options::options_description all("Allowed options");
  all.add(general);

  gearmand::protocol::HTTP http;
  all.add(http.command_line_options());

  gearmand::protocol::Gear gear;
  all.add(gear.command_line_options());

  gearmand::plugins::initialize(all);

  boost::program_options::positional_options_description positional;
  positional.add("provided", -1);

  // Now insert all options that we want to make visible to the user
  boost::program_options::options_description visible("Allowed options");
  visible.add(all);

  boost::program_options::options_description hidden("Hidden options");
  hidden.add_options()
  ("check-args", boost::program_options::bool_switch(&opt_check_args)->default_value(false),
   "Check command line and configuration file argments and then exit.");
  all.add(hidden);

  boost::program_options::variables_map vm;
  try {
    // Disable allow_guessing
    int style= boost::program_options::command_line_style::default_style ^ boost::program_options::command_line_style::allow_guessing;
    boost::program_options::parsed_options parsed= boost::program_options::command_line_parser(argc, argv)
      .options(all)
      .positional(positional)
      .style(style)
      .run();
    store(parsed, vm);
    notify(vm);

    if (config_file.empty() == false)
    {
      // Load the file and tokenize it
      std::ifstream ifs(config_file.c_str());
      if (ifs)
      {
        // Read the whole file into a string
        std::stringstream ss;
        ss << ifs.rdbuf();
        // Split the file content
        boost::char_separator<char> sep(" \n\r");
        std::string sstr= ss.str();
        boost::tokenizer<boost::char_separator<char> > tok(sstr, sep);
        std::vector<std::string> args;
        std::copy(tok.begin(), tok.end(), back_inserter(args));

        for (std::vector<std::string>::iterator iter= args.begin();
             iter != args.end();
             ++iter)
        {
          std::cerr << *iter << std::endl;
        }

        // Parse the file and store the options
        store(boost::program_options::command_line_parser(args).options(visible).run(), vm);
      }
      else if (config_file.compare(GEARMAND_CONFIG))
      {
        error::message("Could not open configuration file.");
        return EXIT_FAILURE;
      }
    }

    notify(vm);
  }
  catch(boost::program_options::validation_error &e)
  {
    error::message(e.what());
    return EXIT_FAILURE;
  }
  catch(std::exception &e)
  {
    if (e.what() and strncmp("-v", e.what(), 2) == 0)
    {
      error::message("Option -v has been deprecated, please use --verbose");
    }
    else
    {
      error::message(e.what());
    }

    return EXIT_FAILURE;
  }

  gearmand_verbose_t verbose= GEARMAND_VERBOSE_ERROR;
  if (gearmand_verbose_check(verbose_string.c_str(), verbose) == false)
  {
    error::message("Invalid value for --verbose supplied");
    return EXIT_FAILURE;
  }

  if (opt_check_args)
  {
    return EXIT_SUCCESS;
  }

  if (vm.count("help"))
  {
    std::cout << visible << std::endl;
    return EXIT_SUCCESS;
  }

  if (vm.count("version"))
  {
    std::cout << std::endl << "gearmand " << gearmand_version() << " - " <<  gearmand_bugreport() << std::endl;
    return EXIT_SUCCESS;
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

  util::Pidfile _pid_file(pid_file);

  if (_pid_file.create() == false and pid_file.compare(GEARMAND_PID))
  {
    error::perror(_pid_file.error_message().c_str());
    return EXIT_FAILURE;
  }

  gearmand::gearmand_log_info_st log_info(log_file, opt_syslog);

  if (log_info.initialized() == false)
  {
    return EXIT_FAILURE;
  }

  gearmand_st *_gearmand= gearmand_create(host.empty() ? NULL : host.c_str(),
                                          threads, backlog,
                                          static_cast<uint8_t>(job_retries),
                                          static_cast<uint8_t>(worker_wakeup),
                                          _log, &log_info, verbose,
                                          opt_round_robin);
  if (_gearmand == NULL)
  {
    error::message("Could not create gearmand library instance.");
    return EXIT_FAILURE;
  }

  if (queue_type.empty() == false)
  {
    gearmand_error_t rc;
    if ((rc= gearmand::queue::initialize(_gearmand, queue_type.c_str())) != GEARMAN_SUCCESS)
    {
      error::message("Error while initializing the queue", queue_type.c_str());
      gearmand_free(_gearmand);

      return EXIT_FAILURE;
    }
  }

  if (gear.start(_gearmand) != GEARMAN_SUCCESS)
  {
    error::message("Error while enabling Gear protocol module");
    gearmand_free(_gearmand);

    return EXIT_FAILURE;
  }

  if (protocol.compare("http") == 0)
  {
    if (http.start(_gearmand) != GEARMAN_SUCCESS)
    {
      error::message("Error while enabling protocol module", protocol.c_str());
      gearmand_free(_gearmand);

      return EXIT_FAILURE;
    }
  }
  else if (protocol.empty() == false)
  {
    error::message("Unknown protocol module", protocol.c_str());
    gearmand_free(_gearmand);

    return EXIT_FAILURE;
  }

  if (opt_daemon)
  {
    if (util::daemon_is_ready(true) == false)
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
  {
    rl.rlim_max= rl.rlim_cur;
  }

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

static void _reset_log_handler(int signal_arg)
{
  gearmand_log_info_st *log_info= static_cast<gearmand_log_info_st *>(Gearmand()->log_context);

  log_info->reset();
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

  sa.sa_handler= _reset_log_handler;
  if (sigaction(SIGHUP, &sa, 0) == -1)
  {
    error::perror("Could not set SIGHUP handler.");
    return true;
  }

  return false;
}
}

static void _log(const char *mesg, gearmand_verbose_t verbose, void *context)
{
  gearmand_log_info_st *log_info= static_cast<gearmand_log_info_st *>(context);

  log_info->write(verbose, mesg);
}
