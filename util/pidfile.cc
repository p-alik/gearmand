/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include "pidfile.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>

namespace gearman_util
{

Pidfile::Pidfile(const std::string &arg) :
  _filename(arg)
{
  init();
}


Pidfile::~Pidfile()
{
  if (not _filename.empty() and (unlink(_filename.c_str()) == -1))
  {
    std::cerr << "gearman: Could not remove the pid file: " << _filename.c_str() << "(" << strerror(errno) << ")" << std::endl;
  }
}

void Pidfile::init()
{
  if (_filename.empty())
    return;

  FILE *f;

  f= fopen(_filename.c_str(), "w");
  if (f == NULL)
  {
    fprintf(stderr, "gearmand: Could not open pid file for writing: %s (%d)\n",
            _filename.c_str(), errno);
    return;
  }

  unsigned long temp= static_cast<unsigned long>(getpid());
  fprintf(f, "%lu\n", temp);

  if (fclose(f) == -1)
  {
    fprintf(stderr, "gearmand: Could not close the pid file: %s (%d)\n",
            _filename.c_str(), errno);
  }
}

} // namespace gearman_util
