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

#include <config.h>

#include <iostream>

#include <boost/program_options.hpp>

#include <libgearman-server/plugins/queue/base.h>
#include <libgearman-server/queue.hpp>
#include <libgearman-server/log.h>


namespace gearmand {

namespace queue {

plugins::Queue::vector all_queue_modules;

void add(plugins::Queue* arg)
{
  all_queue_modules.push_back(arg);
}

void load_options(boost::program_options::options_description &all)
{
  for (plugins::Queue::vector::iterator iter= all_queue_modules.begin();
       iter != all_queue_modules.end();
       iter++)
  {
    all.add((*iter)->command_line_options());
  }
}

gearmand_error_t initialize(gearmand_st *, const std::string &name)
{
  bool launched= false;

  if (name.empty())
  {
    return GEARMAN_SUCCESS;
  }

  for (plugins::Queue::vector::iterator iter= all_queue_modules.begin();
       iter != all_queue_modules.end();
       iter++)
  {
    if (not name.compare((*iter)->name()))
    {
      if (launched)
      {
        return gearmand_gerror("Attempt to initialize multiple queues", GEARMAN_UNKNOWN_OPTION);
      }

      gearmand_error_t rc;
      if (gearmand_failed(rc= (*iter)->initialize()))
      {
        std::string error_string("Failed to initialize ");
        error_string+= name;

        return gearmand_gerror(error_string.c_str(), rc);
      }

      launched= true;
    }
  }

  if (launched == false)
  {
    std::string error_string("Unknown queue ");
    error_string+= name;
    return gearmand_gerror(error_string.c_str(), GEARMAN_UNKNOWN_OPTION);
  }

  return GEARMAN_SUCCESS;
}

} // namespace queue
} // namespace gearmand
