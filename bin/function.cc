/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <cstring>

#include <bin/function.h>

namespace gearman_client
{
Function::Function(const char *name_arg)
{
  memset(&_task, 0, sizeof(gearman_task_st));

  // copy the name into the _name vector
  size_t length= strlen(name_arg);
  _name.resize(length +1);
  memcpy(&_name[0], name_arg, length);
}

Function::~Function()
{
  gearman_task_free(&_task);
}

gearman_task_st * Function::task()
{
  return &_task;
}

} // namespace gearman_client
