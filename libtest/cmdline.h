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

#pragma once

#include <spawn.h>

namespace libtest {

class Application {
public:

  enum error_t {
    SUCCESS= EXIT_SUCCESS,
    FAILURE= EXIT_FAILURE,
    INVALID= 127
  };

  class Pipe {
  public:
    Pipe();
    ~Pipe();

    int* fd()
    {
      return _fd;
    }

    enum close_t {
      READ,
      WRITE
    };

    void reset();
    void close(const close_t& arg);
    void dup_for_spawn(const close_t& arg,
                       posix_spawn_file_actions_t& file_actions,
                       const int newfildes);

  private:
    int _fd[2];
    bool _open[2];
  };

public:
  Application(const std::string& arg, const bool _use_libtool_arg= false);

  virtual ~Application();

  void add_option(const std::string&);
  void add_option(const std::string&, const std::string&);
  error_t run(const char *args[]);
  error_t wait();

private:
  const bool _use_libtool;
  int argc;
  std::string _exectuble;
  std::string _exectuble_with_path;
  std::vector< std::pair<std::string, std::string> > _options;
  Pipe stdin_fd;
  Pipe stdout_fd;
  Pipe stderr_fd;
  pid_t _pid;
};

int exec_cmdline(const std::string& executable, const char *args[], bool use_libtool= false);

const char *gearmand_binary(); 

}
