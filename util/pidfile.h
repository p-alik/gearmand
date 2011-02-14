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

private:
  std::string _filename;

  void init();
};

} // namespace gearman_util

#endif /* __GEARMAN_UTIL_PIDFILE_H__ */
