/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <iostream>
#include <vector>

#include <libgearman/gearman.h>

namespace gearman_client
{

typedef std::vector<char> Bytes;

class Function {
  Bytes _name;
  gearman_task_st _task;
  Bytes _buffer;

public:
  typedef std::vector<Function> vector;

  Function(const char *name_arg);

  ~Function();

  gearman_task_st *task();

  const char *name() const
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

} // namespace gearman_client

inline std::ostream& operator<<(std::ostream& output, const gearman_client::Function& arg) 
{
  output << arg.name();
  output << "()";

  return output;
}
