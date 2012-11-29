/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "gear_config.h"
#include <iostream>

#include <libgearman-server/plugins.h>

#include <libgearman-server/queue.hpp>
#include <libgearman-server/plugins/queue.h>

namespace gearmand {
namespace plugins {

void initialize(boost::program_options::options_description &all)
{
  queue::initialize_default();

  if (HAVE_LIBDRIZZLE)
  {
    queue::initialize_drizzle();
  }

  if (HAVE_LIBMEMCACHED)
  {
    queue::initialize_libmemcached();
  }

  if (HAVE_LIBSQLITE3)
  {
    queue::initialize_sqlite();
  }

  if (HAVE_LIBPQ)
  {
    queue::initialize_postgres();
  }


  if (HAVE_HIREDIS)
  {
#ifdef HAVE_LIBHIREDIS
    queue::initialize_redis();
#endif
  }


  if (HAVE_TOKYOCABINET)
  {
    queue::initialize_tokyocabinet();
  }

  if (HAVE_LIBMYSQL_BUILD)
  {
    queue::initialize_mysql();
  }

  gearmand::queue::load_options(all);
}

} //namespace plugins
} //namespace gearmand
