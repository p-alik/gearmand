/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab: 
 * 
 * Gearman server and library 
 * Copyright (C) 2011 DataDifferential All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <bin/function.h>

namespace gearman_client
{

/**
 * Data structure for arguments and state.
 */
class Args
{
public:
  Args(int p_argc, char *p_argv[]);

  ~Args();

  bool usage()
  {
    return _usage;
  }

  bool prefix() const
  {
    return _prefix;
  }

  bool background() const
  {
    return _background;
  }

  bool daemon() const
  {
    return _daemon;
  }

  gearman_job_priority_t priority() const
  {
    return _priority;
  }

  int timeout() const
  {
    return _timeout;
  }

  const char *unique() const
  {
    return _unique;
  }

  bool job_per_newline() const
  {
    return _job_per_newline;
  }

  Function::vector::iterator begin()
  {
    return _functions.begin();
  }

  Function::vector::iterator end()
  {
    return _functions.end();
  }
  bool strip_newline() const
  {
    return _strip_newline;
  }

  bool worker() const
  {
    return _worker;
  }

  const std::string &pid_file() const
  {
    return _pid_file;
  }

  int error() const
  {
    return _error;
  }

  void set_error() const
  {
    _error= 1;
  }

  uint32_t &count()
  {
    return _count;
  }

  const char *host() const
  {
    return _host;
  }

  in_port_t port() const
  {
    return _port;
  }

  bool arguments() const
  {
    if (argv[0])
      return true;

    return false;
  }

  bool suppress_input() const
  {
    return _suppress_input;
  }

  const char *argument(size_t arg)
  {
    return argv[arg];
  }

  char **argumentv()
  {
    return argv;
  }

  bool is_error() const
  {
    if (_functions.empty())
    {
      return true;
    }

    return _is_error;
  }

  bool is_valid() const
  {
    return _functions.size();
  }

private:
  Function::vector _functions;
  char *_host;
  in_port_t _port;
  uint32_t _count;
  char *_unique;
  bool _job_per_newline;
  bool _strip_newline;
  bool _worker;
  bool _suppress_input;

  bool _prefix;
  bool _background;
  bool _daemon;
  bool _usage;
  bool _is_error;
  gearman_job_priority_t _priority;
  int _timeout;
  char **argv;
  mutable int _error;
  std::string _pid_file;

  void init(int argc);

  void add(const char *name)
  {
    _functions.push_back(Function(name));
  }
};

} // namespace gearman_client
