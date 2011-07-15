/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <cstring>
#include <iostream>

#include <bin/function.h>

namespace gearman_client
{

Function::Function(const char *name_arg) :
  _name(),
  _buffer()
{
  // copy the name into the _name vector
  size_t length= strlen(name_arg);
  _name.resize(length +1);
  memcpy(&_name[0], name_arg, length);
}

Function::~Function()
{
}

} // namespace gearman_client
