/* Gearman server and library
 * Copyright (C) 2011 Data Differential, http://datadifferential.com/
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <libtest/common.h>

#include "util/instance.h"
#include "util/operation.h"

using namespace gearman_util;

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libtest/server.h>
#include <libtest/wait.h>

#include <libgearman/gearman.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

class GetPid : public Instance::Finish
{
private:
  pid_t _pid;

public:
  GetPid() :
    _pid(-1)
  { }

  pid_t pid()
  {
    return _pid;
  }


  bool call(const bool success, const std::string &response)
  {
    _pid= -1;

    if (success and response.size())
    {
      _pid= atoi(response.c_str());
    }

    if (_pid < 1)
    {
      _pid= -1;
      return true;
    }

    return false;
  }
};

using namespace libtest;

class Gearmand : public Server
{
private:
public:
  Gearmand(const std::string& host_arg, in_port_t port_arg) :
    Server(host_arg, port_arg)
  { }

  pid_t get_pid()
  {
    GetPid *getpid;
    Instance instance(hostname(), port());
    instance.set_finish(getpid= new GetPid);

    instance.push(new Operation(test_literal_param("getpid\r\n"), true));

    instance.run();

    _pid= getpid->pid();

    return _pid;
  }

  bool ping()
  {
    int limit= 5; // sleep cycles
    while (--limit)
    {
      gearman_client_st *client= gearman_client_create(NULL);
      if (not client)
      {
        Error << "Could not allocate memory for gearman_client_create()";
        return false;
      }

      if (gearman_success(gearman_client_add_server(client, hostname().c_str(), port())))
      {
        gearman_return_t rc= gearman_client_echo(client, gearman_literal_param("This is my echo test"));

        if (gearman_success(rc))
        {
          gearman_client_free(client);
          return true;
        }

        if (rc == GEARMAN_COULD_NOT_CONNECT)
        {
          sleep();
          continue;
        }
      }

      gearman_client_free(client);
    }

    return false;;
  }

  const char *name()
  {
    return "gearmand";
  };

  const char *executable()
  {
    return "./gearmand/gearmand";
  }

  const char *pid_file_option()
  {
    return "--pid-file=";
  }

  const char *daemon_file_option()
  {
    return "--daemon";
  }

  const char *log_file_option()
  {
    return "--log_file=";
  }

  const char *port_option()
  {
    return "--port=";
  }

  bool is_libtool()
  {
    return true;
  }

  bool build(int argc, const char *argv[]);
};


#include <sstream>

bool Gearmand::build(int argc, const char *argv[])
{
  std::stringstream arg_buffer;

  if (getuid() == 0 or geteuid() == 0)
  {
    arg_buffer << " -u root ";
  }

  for (int x= 1 ; x < argc ; x++)
  {
    arg_buffer << " " << argv[x] << " ";
  }

  set_extra_args(arg_buffer.str());

  return true;
}

bool libtest::server_startup(server_startup_st& construct, in_port_t try_port, int argc, const char *argv[])
{
  Logn();

  // Look to see if we are being provided ports to use
  {
    char variable_buffer[1024];
    snprintf(variable_buffer, sizeof(variable_buffer), "LIBTEST_PORT_%lu", (unsigned long)construct.count());

    char *var;
    if ((var= getenv(variable_buffer)))
    {
      in_port_t tmp= in_port_t(atoi(var));

      if (tmp > 0)
        try_port= tmp;
    }
  }

  Gearmand *server= new Gearmand("localhost", try_port);
  assert(server);

  /*
    We will now cycle the server we have created.
  */
  if (not server->cycle())
  {
    Error << "Could not start up server " << *server;
    return false;
  }

  server->build(argc, argv);

  if (construct.is_debug())
  {
    Log << "Pausing for startup, hit return when ready.";
    std::string gdb_command= server->base_command();
    std::string options;
    Log << "run " << server->args(options);
    getchar();
  }
  else if (not server->start())
  {
    Error << "Failed to start " << *server;
    delete server;
    return false;
  }
  else
  {
    Log << "STARTING SERVER: " << server->running() << " pid:" << server->pid();
  }
  construct.push_server(server);

  if (default_port() == 0)
  {
    assert(server->has_port());
    set_default_port(server->port());
  }

  char port_str[NI_MAXSERV];
  snprintf(port_str, sizeof(port_str), "%u", int(server->port()));

  std::string server_config_string;
  server_config_string+= "--server=";
  server_config_string+= server->hostname();
  server_config_string+= ":";
  server_config_string+= port_str;
  server_config_string+= " ";

  construct.server_list+= server_config_string;

  Logn();

  srandom((unsigned int)time(NULL));

  return true;
}
