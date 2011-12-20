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

int exec_cmdline(const std::string& command, const char *args[], bool use_libtool)
{
  int stdin_fd[2];
  int stdout_fd[2];
  int stderr_fd[2];

  int ret;
  if ((ret= pipe(stdin_fd)) < 0)
  {
    Error << "posix_spawn(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= pipe(stdout_fd)) < 0)
  {
    Error << "posix_spawn(" << strerror(ret) << ")";
    close(stdin_fd[0]);
    close(stdin_fd[1]);

    return EXIT_FAILURE;
  }

  if ((ret= pipe(stderr_fd)) < 0)
  {
    Error << "posix_spawn(" << strerror(ret) << ")";
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[0]);
    close(stderr_fd[1]);

    return EXIT_FAILURE;
  }

  {
    ret= fcntl(stdin_fd[0], F_GETFL, 0);
    if (ret == -1)
    {
      Error << "fcntl(F_GETFL) " << strerror(ret);
      return EXIT_FAILURE;
    }

    ret= fcntl(stdin_fd[0], F_SETFL, ret | O_NONBLOCK);
    if (ret == -1)
    {
      Error << "fcntl(F_SETFL) " << strerror(ret);
      return EXIT_FAILURE;
    }
  }

  posix_spawn_file_actions_t file_actions;
  posix_spawn_file_actions_init(&file_actions);

  if ((ret= posix_spawn_file_actions_adddup2(&file_actions, stdin_fd[0], STDIN_FILENO)) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= posix_spawn_file_actions_adddup2(&file_actions, stdout_fd[1], STDOUT_FILENO)) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= posix_spawn_file_actions_adddup2(&file_actions, stderr_fd[1], STDERR_FILENO)) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= posix_spawn_file_actions_addclose(&file_actions, stdin_fd[1])) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= posix_spawn_file_actions_addclose(&file_actions, stdout_fd[0])) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  if ((ret= posix_spawn_file_actions_addclose(&file_actions, stderr_fd[0])) < 0)
  {
    Error << "posix_spawn_file_actions_adddup2(" << strerror(ret) << ")";
    return EXIT_FAILURE;
  }

  std::string command_with_path;
  if (use_libtool and getenv("PWD"))
  {
    command_with_path+= getenv("PWD");
    command_with_path+= "/";
  }
  command_with_path+= command;
  
  char * * built_argv;
  size_t argc= 0;
  if (use_libtool and libtool())
  {
    create_argv(command_with_path, built_argv, argc, args, true);
  }
  else
  {
    create_argv(command_with_path, built_argv, argc, args, false);
  }

  pid_t pid;
  int spawn_ret;

  if (use_libtool)
  {
    spawn_ret= posix_spawn(&pid, built_argv[0], &file_actions, NULL, built_argv, NULL);
  }
  else
  {
    spawn_ret= posix_spawnp(&pid, built_argv[0], &file_actions, NULL, built_argv, NULL);
  }

  posix_spawn_file_actions_destroy(&file_actions);
  delete_argv(built_argv, argc);

  if ((ret= close(stdin_fd[0])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  if ((ret= close(stdout_fd[1])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  if ((ret= close(stderr_fd[1])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  ssize_t read_length;
  char buffer[1024]= { 0 };
  while ((read_length= ::read(stdout_fd[0], buffer, sizeof(buffer))) != 0)
  {
    // @todo Suck up all output code here
  }

  int exit_code= EXIT_FAILURE;
  if (spawn_ret == 0)
  {
    int status= 0;
    pid_t waited_pid;
    if ((waited_pid= waitpid(pid, &status, 0)) == -1)
    {
      Error << "Error occured while waitpid(" << strerror(errno) << ") on pid " << int(pid);
    }
    else
    {
      assert(waited_pid == pid);
      exit_code= exited_successfully(status);
    }
  }

  if ((ret= close(stdin_fd[1])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  if ((ret= close(stdout_fd[0])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  if ((ret= close(stderr_fd[0])) < 0)
  {
    Error << "close(" << strerror(ret) << ")";
  }

  return exit_code;
}

const char *gearmand_binary() 
{
  return GEARMAND_BINARY;
}

} // namespace exec_cmdline
