/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libtest library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <config.h>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <iostream>

#include <libtest/server.h>
#include <libtest/stream.h>
#include <libtest/killpid.h>

namespace libtest {

std::ostream& operator<<(std::ostream& output, const Server &arg)
{
  if (arg.is_socket())
  {
    output << arg.hostname();
  }
  else
  {
    output << arg.hostname() << ":" << arg.port();
  }

  if (arg.has_pid())
  {
    output << " Pid:" <<  arg.pid();
  }

  if (not arg.running().empty())
  {
    output << " Exec:" <<  arg.running();
  }


  return output;  // for multiple << operators
}

void Server::sleep(void)
{
#ifdef WIN32
  sleep(1);
#else
  struct timespec global_sleep_value= { 0, 50000 };
  nanosleep(&global_sleep_value, NULL);
#endif
}

Server::Server(const std::string& host_arg, in_port_t port_arg) :
  _pid(-1),
  _port(port_arg),
  _hostname(host_arg)
{
}

Server::Server(const std::string& socket_file) :
  _pid(-1),
  _port(0),
  _hostname(socket_file)
{
}

Server::~Server()
{
  if (has_pid() and not kill())
  {
    Error << "Unable to kill:" << *this;
  }
}


// If the server exists, kill it
bool Server::cycle()
{
  uint32_t limit= 3;

  // Try to ping, and kill the server #limit number of times
  pid_t current_pid;
  while (--limit and (current_pid= get_pid()) != -1)
  {
    if (kill())
    {
      Log << "Killed existing server," << *this << " with pid:" << current_pid;
      sleep();
      continue;
    }
  }

  // For whatever reason we could not kill it, and we reached limit
  if (limit == 0)
  {
    Error << "Reached limit, could not kill server pid:" << current_pid;
    return false;
  }

  return true;
}

// Grab a one off command
bool Server::command(std::string& command_arg)
{
  rebuild_base_command();

  command_arg+= _base_command;

  if (args(command_arg))
  {
    return true;
  }

  return false;
}

bool Server::start()
{
  // If we find that we already have a pid then kill it.
  if (has_pid() and not kill())
  {
    Error << "Could not kill() existing server during start()";
    return false;
  }
  assert(not has_pid());

  _running.clear();
  if (not command(_running))
  {
    Error << "Could not build command()";
    return false;
  }

  if (system(_running.c_str()) == -1)
  {
    Error << " failed:" << strerror(errno);
    _running.clear();
    return false;
  }

  int count= 30;
  while (not ping() and --count)
  {
    sleep();
  }

  if (count == 0)
  {
    Error << "Failed to ping() server once started:" << *this;
    _running.clear();
    return false;
  }

  _pid= get_pid();

  return has_pid();
}

void Server::reset_pid()
{
  _running.clear();
  _pid_file.clear();
  _pid= -1;
}

pid_t Server::pid()
{
  return _pid;
}

bool Server::set_pid_file()
{
  char file_buffer[FILENAME_MAX];
  file_buffer[0]= 0;

  snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/%s.pidXXXXXX", name());
  int fd;
  if ((fd= mkstemp(file_buffer)) == -1)
  {
    perror(file_buffer);
    return false;
  }
  close(fd);

  _pid_file= file_buffer;

  return true;
}

bool Server::set_log_file()
{
  char file_buffer[FILENAME_MAX];
  file_buffer[0]= 0;

  snprintf(file_buffer, sizeof(file_buffer), "tests/var/tmp/%s.logXXXXXX", name());
  int fd;
  if ((fd= mkstemp(file_buffer)) == -1)
  {
    perror(file_buffer);
    return false;
  }
  close(fd);

  _log_file= file_buffer;

  return true;
}

void Server::rebuild_base_command()
{
  _base_command.clear();
  if (is_libtool())
  {
    _base_command+= "./libtool --mode=execute ";
  }

  if (is_debug())
  {
    _base_command+= "gdb ";
  }
  else if (is_valgrind())
  {
    _base_command+= "valgrind --log-file=tests/var/tmp/valgrind.out --leak-check=full  --show-reachable=yes ";
  }

  _base_command+= executable();
}

void Server::set_extra_args(const std::string &arg)
{
  _extra_args= arg;
}

bool Server::args(std::string& options)
{
  std::stringstream arg_buffer;

  // Set a log file if it was requested (and we can)
  if (getenv("LIBTEST_LOG") and log_file_option())
  {
    if (not set_log_file())
      return false;

    arg_buffer << " " << log_file_option() << _log_file;
  }

  // Update pid_file
  if (pid_file_option())
  {
    if (not set_pid_file())
      return false;

    arg_buffer << " " << pid_file_option() << pid_file(); 
  }

  assert(daemon_file_option());
  if (daemon_file_option())
  {
    arg_buffer << " " << daemon_file_option();
  }

  assert(port_option());
  if (port_option())
  {
    arg_buffer << " " << port_option() << _port;
  }

  options+= arg_buffer.str();

  if (not _extra_args.empty())
    options+= _extra_args;

  return true;
}

bool Server::is_debug() const
{
  return bool(getenv("LIBTEST_MANUAL_GDB"));
}

bool Server::is_valgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_VALGRIND"));
}

bool Server::kill()
{
  if (has_pid() and kill_pid(_pid)) // If we kill it, reset
  {
    reset_pid();

    return true;
  }

  return false;
}

void server_startup_st::push_server(Server *arg)
{
  servers.push_back(arg);
}

Server* server_startup_st::pop_server()
{
  Server *tmp= servers.back();
  servers.pop_back();
  return tmp;
}

void server_startup_st::shutdown()
{
  for (std::vector<Server *>::iterator iter= servers.begin(); iter != servers.end(); iter++)
  {
    if ((*iter)->has_pid() and not (*iter)->kill())
    {
      Error << "Unable to kill:" <<  *(*iter);
    }
  }
}

server_startup_st::~server_startup_st()
{
  for (std::vector<Server *>::iterator iter= servers.begin(); iter != servers.end(); iter++)
  {
    delete *iter;
  }
  servers.clear();
}

bool server_startup_st::is_debug() const
{
  return bool(getenv("LIBTEST_MANUAL_GDB"));
}

bool server_startup_st::is_valgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_VALGRIND"));
}

} // namespace libtest
