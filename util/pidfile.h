/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef __GEARMAN_UTIL_PIDFILE_H__
#define __GEARMAN_UTIL_PIDFILE_H__

#include <string>

namespace gearman_util
{

class Pidfile
{
public:
  Pidfile(const std::string &arg);

  ~Pidfile();

  const std::string &error_message()
  {
    return _error_message;
  }

  bool create();

private:
  int _last_errno;
  std::string _filename;
  std::string _error_message;
};

} // namespace gearman_util

#endif /* __GEARMAN_UTIL_PIDFILE_H__ */
