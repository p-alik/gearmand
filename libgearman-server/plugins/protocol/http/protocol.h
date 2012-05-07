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
#include <libgearman-server/plugins/protocol/http/method.h>
#include <libgearman-server/plugins/protocol/http/response_codes.h>

struct gearmand_st;

namespace gearmand {
namespace protocol {

class HTTP : public gearmand::Plugin {

public:

  HTTP();
  ~HTTP();

  gearmand_error_t start(gearmand_st *gearmand);

private:
  std::string _port;
};

} // namespace protocol
} // namespace gearmand
