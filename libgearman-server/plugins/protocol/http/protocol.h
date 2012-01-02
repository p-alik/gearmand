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
public:
  enum method_t 
  {
    HEAD,
    GET,
    PUT,
    POST,
    TRACE,
  };

private:
  HTTP::method_t _method;
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

  void set_background(bool arg)
  {
    _background= arg;
  }

  bool keep_alive()
  {
    return _keep_alive;
  }

  void set_keep_alive(bool arg)
  {
    _keep_alive= arg;
  }

  HTTP::method_t method()
  {
    return _method;
  }

  void set_method(HTTP::method_t arg)
  {
    _method= arg;
  }

  void reset()
  {
    _background= false;
    _keep_alive= false;
    _method= HTTP::TRACE;
  }
};

} // namespace protocol
} // namespace gearmand
