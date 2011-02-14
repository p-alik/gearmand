/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>

#include <libgearman/gearman.h>
#include <util/error.h>

namespace gearman_util
{

namespace error {

void perror(const char *message)
{
  char buffer[BUFSIZ];

  strerror_r(errno, buffer, sizeof(buffer));
  std::cerr << "gearman: " << message << " (" << buffer << ")" << std::endl;
}

void message(const char *arg)
{
  std::cerr << "gearman: " << arg << std::endl;
}

void message(const char *arg, const char *arg2)
{
  std::cerr << "gearman: " << arg << " : " << arg2 << std::endl;
}

void message(const char *arg, const gearman_client_st &client)
{
  std::cerr << "gearman: " << arg << " : " << gearman_client_error(&client) << std::endl;
}

void message(const char *arg, const gearman_worker_st &worker)
{
  std::cerr << "gearman: " << arg << " : " << gearman_worker_error(&worker) << std::endl;
}

} // namespace error

} // namespace gearman_util
