/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include "pidfile.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>


namespace gearman_util
{

Pidfile::Pidfile(const std::string &arg) :
  _last_errno(0),
  _filename(arg)
{
}


Pidfile::~Pidfile()
{
  if (not _filename.empty() and (unlink(_filename.c_str()) == -1))
  {
    _error_message+= "Could not remove the pid file: ";
    _error_message+= _filename;
  }
}

bool Pidfile::create()
{
  if (_filename.empty())
    return true;

  int file;
  if ((file = open(_filename.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRGRP|S_IROTH)) < 0)
  {
    _error_message+= "Could not open pid file for writing: "; 
    _error_message+= _filename;
    return false;
  }

  char buffer[BUFSIZ];

  unsigned long temp= static_cast<unsigned long>(getpid());
  int length= snprintf(buffer, sizeof(buffer), "%lu\n", temp);

  if (write(file, buffer, length) != length)
  { 
    _error_message+= "Could not write pid to file: "; 
    _error_message+= _filename;

    return false;
  }

  if (close(file < 0))
  {
    _error_message+= "Could not close() file after writing pid to it: "; 
    _error_message+= _filename;
    return false;
  }

  return true;
}

} // namespace gearman_util
