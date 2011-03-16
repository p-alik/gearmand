/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief HTTP Protocol Declarations
 */

#pragma once

#include <libgearman-server/plugins/base.h>

struct gearmand_st;

namespace gearmand {
namespace protocol {

class HTTP : public gearmand::Plugin {
  bool _background;
  bool _keep_alive;
  std::string global_port;

public:

  HTTP();
  ~HTTP();

  gearmand_error_t start(gearmand_st *gearmand);

  bool background()
  {
    return _background;
  }

  bool keep_alive()
  {
    return _keep_alive;
  }

  void set_background(bool arg)
  {
    _background= arg;
  }

  void set_keep_alive(bool arg)
  {
    _keep_alive= arg;
  }

  void reset()
  {
    _background= false;
    _keep_alive= false;
  }
};

} // namespace protocol
} // namespace gearmand
