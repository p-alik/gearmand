/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef __GEARMAN_ARGUMENTS_H__
#define __GEARMAN_ARGUMENTS_H__

namespace gearman_client
{

typedef std::vector<char> Bytes;

class Function {
  bool _used_task;
  Bytes _name;
  gearman_task_st _task;
  Bytes _buffer;

public:
  typedef std::vector<Function> vector;

  Function(const char *name_arg) :
    _used_task(false)
  {
    memset(&_task, 0, sizeof(gearman_task_st));

    // copy the name into the _name vector
    size_t length= strlen(name_arg);
    _name.resize(length +1);
    memcpy(&_name[0], name_arg, length);
  }

  ~Function()
  {
    if (_used_task)
      gearman_task_free(&_task);
  }

  gearman_task_st *task()
  {
    _used_task= true;
    return &_task;
  }

  const char *name()
  {
    return &_name[0];
  }

  Bytes &buffer()
  {
    return _buffer;
  }

  char *buffer_ptr()
  {
    return &_buffer[0];
  }
};

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
    if (argv)
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

#endif /* __GEARMAN_ARGUMENTS_H__ */
