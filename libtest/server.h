/*
 * Copyright (C) 2011 Data Differential, http://datadifferential.com/
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <cassert>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace libtest {

struct Server {
private:
  std::string _pid_file;
  std::string _log_file;
  std::string _base_command; // executable command which include libtool, valgrind, gdb, etc
  std::string _running; // Current string being used for system()

protected:
  pid_t _pid;
  in_port_t _port;
  std::string _hostname;
  std::string _extra_args;

public:
  Server(const std::string& hostname, in_port_t port_arg);

  Server(const std::string& socket_file);

  virtual ~Server();

  virtual const char *name()= 0;
  virtual const char *executable()= 0;
  virtual const char *port_option()= 0;
  virtual const char *pid_file_option()= 0;
  virtual const char *daemon_file_option()= 0;
  virtual const char *log_file_option()= 0;
  virtual bool is_libtool()= 0;

  const std::string& pid_file() const
  {
    return _pid_file;
  }

  const std::string& base_command() const
  {
    return _base_command;
  }

  const std::string& log_file() const
  {
    return _log_file;
  }

  const std::string& hostname() const
  {
    return _hostname;
  }

  bool cycle();

  virtual bool ping()= 0;

  virtual pid_t get_pid()= 0;

  virtual bool build(int argc, const char *argv[])= 0;

  in_port_t port() const
  {
    return _port;
  }

  bool has_port() const
  {
    return (_port != 0);
  }

  // Reset a server if another process has killed the server
  void reset()
  {
    _pid= -1;
    _pid_file.clear();
    _log_file.clear();
  }

  void set_extra_args(const std::string &arg);

  bool args(std::string& options);

  pid_t pid();

  pid_t pid() const
  {
    return _pid;
  }

  bool has_pid() const
  {
    return (_pid > 1);
  }

  bool is_socket() const
  {
    return _hostname[0] == '/';
  }

  const std::string running() const
  {
    return _running;
  }

  std::string log_and_pid();

  bool kill();
  bool start();
  bool command(std::string& command_arg);

protected:
  void sleep();

private:
  bool is_valgrind() const;
  bool is_debug() const;
  bool set_log_file();
  bool set_pid_file();
  void rebuild_base_command();
  void reset_pid();
};

std::ostream& operator<<(std::ostream& output, const libtest::Server &arg);

struct server_startup_st
{
  uint8_t udp;
  std::string server_list;
  std::vector<Server *> servers;

  server_startup_st() :
    udp(0)
  { }

  size_t count() const
  {
    return servers.size();
  }

  bool is_debug() const;
  bool is_valgrind() const;

  void shutdown();
  void push_server(Server *);
  Server *pop_server();

  ~server_startup_st();
};

bool server_startup(server_startup_st&, in_port_t try_port, int argc, const char *argv[]);

} // namespace libtest


