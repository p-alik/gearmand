/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libtest/common.h>

using namespace libtest;

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <spawn.h>

extern "C" {
  static int exited_successfully(int status)
  {
    if (status == 0)
    {
      return EXIT_SUCCESS;
    }

    if (WIFEXITED(status) == true)
    {
      return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status) == true)
    {
      return WTERMSIG(status);
    }

    return EXIT_FAILURE;
  }
}

namespace {

  void print_argv(char * * & built_argv, const size_t& argc, const pid_t& pid)
  {
    std::stringstream arg_buffer;

    for (size_t x= 1; x < argc; x++)
    {
      arg_buffer << built_argv[x] << " ";
    }
  }

  void create_argv(const std::string& command, char * * & built_argv, size_t& argc, const char *args[], const bool use_libtool)
  {
    argc= 2 + use_libtool ? 2 : 0; // +1 for the command, +2 for libtool/mode=execute, +1 for the NULL

    if (use_libtool)
    {
      argc+= 2; // +2 for libtool --mode=execute
    }

    for (const char **ptr= args; *ptr; ++ptr)
    {
      argc++;
    }
    built_argv= new char * [argc];

    size_t x= 0;
    if (use_libtool)
    {
      assert(libtool());
      built_argv[x++]= strdup(libtool());
      built_argv[x++]= strdup("--mode=execute");
    }
    built_argv[x++]= strdup(command.c_str());

    for (const char **ptr= args; *ptr; ++ptr)
    {
      built_argv[x++]= strdup(*ptr);
    }
    built_argv[argc -1]= NULL;
  }

  void delete_argv(char * * & built_argv, const size_t& argc)
  {
    for (size_t x= 0; x < argc; x++)
    {
      ::free(built_argv[x]);
    }
    delete[] built_argv;
    built_argv= NULL;
  }

}

namespace libtest {

Application::Application(const std::string& arg, const bool _use_libtool_arg) :
  _use_libtool(_use_libtool_arg),
  argc(0),
  _exectuble(arg),
  _pid(-1)
  { 
    if (_use_libtool)
    {
      if (libtool() == NULL)
      {
        throw "libtool requested, but know libtool was found";
      }
    }

    if (_use_libtool and getenv("PWD"))
    {
      _exectuble_with_path+= getenv("PWD");
      _exectuble_with_path+= "/";
    }
    _exectuble_with_path+= _exectuble;
  }

Application::~Application()
{
}

Application::error_t Application::run(const char *args[])
{
  stdin_fd.reset();
  stdout_fd.reset();
  stderr_fd.reset();
  _stdout_buffer.clear();

  posix_spawn_file_actions_t file_actions;
  posix_spawn_file_actions_init(&file_actions);

  stdin_fd.dup_for_spawn(Application::Pipe::READ, file_actions, STDIN_FILENO);
  stdout_fd.dup_for_spawn(Application::Pipe::WRITE, file_actions, STDOUT_FILENO);
  stderr_fd.dup_for_spawn(Application::Pipe::WRITE, file_actions, STDERR_FILENO);
  
  char * * built_argv;
  size_t argc= 0;
  create_argv(_exectuble_with_path, built_argv, argc, args, _use_libtool);

  int spawn_ret;
  if (_use_libtool)
  {
    spawn_ret= posix_spawn(&_pid, built_argv[0], &file_actions, NULL, built_argv, NULL);
  }
  else
  {
    spawn_ret= posix_spawnp(&_pid, built_argv[0], &file_actions, NULL, built_argv, NULL);
  }

  posix_spawn_file_actions_destroy(&file_actions);
  delete_argv(built_argv, argc);

  stdin_fd.close(Application::Pipe::READ);
  stdout_fd.close(Application::Pipe::WRITE);
  stderr_fd.close(Application::Pipe::WRITE);

  if (spawn_ret)
  {
    return Application::INVALID;
  }

  return Application::SUCCESS;
}

Application::error_t Application::wait()
{
  if (_pid == -1)
  {
    return Application::INVALID;
  }

  ssize_t read_length;
  char buffer[1024]= { 0 };
  while ((read_length= ::read(stdout_fd.fd()[0], buffer, sizeof(buffer))) >= 1)
  {
    Error << "Going to resize to " << _stdout_buffer.size() << " " << read_length;
    _stdout_buffer.resize(_stdout_buffer.size() +read_length);
    for (size_t x= 0; x < read_length; x++)
    {
      _stdout_buffer.push_back(buffer[x]);
    }
    // @todo Suck up all output code here
  }

  error_t exit_code= FAILURE;
  {
    int status= 0;
    pid_t waited_pid;
    if ((waited_pid= waitpid(_pid, &status, 0)) == -1)
    {
      Error << "Error occured while waitpid(" << strerror(errno) << ") on pid " << int(_pid);
    }
    else
    {
      assert(waited_pid == _pid);
      exit_code= error_t(exited_successfully(status));
    }
  }

  return exit_code;
}

void Application::add_option(const std::string& arg)
{
  _options.push_back(std::make_pair(arg, std::string()));
}

void Application::add_option(const std::string& name, const std::string& value)
{
  _options.push_back(std::make_pair(name, value));
}

Application::Pipe::Pipe()
{
  _fd[0]= -1;
  _fd[1]= -1;
  _open[0]= false;
  _open[1]= false;
}

void Application::Pipe::reset()
{
  close(READ);
  close(WRITE);

  int ret;
  if ((ret= pipe(_fd)) < 0)
  {
    throw strerror(ret);
  }
  _open[0]= true;
  _open[1]= true;

  {
    ret= fcntl(_fd[0], F_GETFL, 0);
    if (ret == -1)
    {
      Error << "fcntl(F_GETFL) " << strerror(ret);
      throw strerror(ret);
    }

    ret= fcntl(_fd[0], F_SETFL, ret | O_NONBLOCK);
    if (ret == -1)
    {
      Error << "fcntl(F_SETFL) " << strerror(ret);
      throw strerror(ret);
    }
  }
}

Application::Pipe::~Pipe()
{
  close(READ);
  close(WRITE);
}

void Application::Pipe::dup_for_spawn(const close_t& arg, posix_spawn_file_actions_t& file_actions, const int newfildes)
{
  int type= int(arg);

  int ret;
  if ((ret= posix_spawn_file_actions_adddup2(&file_actions, _fd[type], newfildes )) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    throw strerror(ret);
  }

  if ((ret= posix_spawn_file_actions_addclose(&file_actions, _fd[type])) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    throw strerror(ret);
  }
}

void Application::Pipe::close(const close_t& arg)
{
  int type= int(arg);

  if (_open[type])
  {
    int ret;
    if ((ret= ::close(_fd[type])) < 0)
    {
      Error << "close(" << strerror(ret) << ")";
    }
    _open[type]= false;
  }
}

int exec_cmdline(const std::string& command, const char *args[], bool use_libtool)
{
  Application app(command, use_libtool);

  Application::error_t ret= app.run(args);

  if (ret != Application::SUCCESS)
  {
    return int(ret);
  }
  else
  {
    ret= app.wait();
  }

  return int(ret);
}

const char *gearmand_binary() 
{
  return GEARMAND_BINARY;
}

} // namespace exec_cmdline
