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

#include <libgearman-server/common.h>
#include <libgearman-server/queue.h>
#include <libgearman-server/plugins/queue/base.h>
#include <libgearman-server/queue.hpp>
#include <libgearman-server/log.h>

gearmand_error_t gearman_queue_add(gearman_server_st *server,
                                   const char *unique,
                                   size_t unique_size,
                                   const char *function_name,
                                   size_t function_name_size,
                                   const void *data,
                                   size_t data_size,
                                   gearman_job_priority_t priority,
                                   int64_t when)
{
  if (server->queue_version == QUEUE_VERSION_FUNCTION)
  {
    assert(server->queue.functions._add_fn);
    return (*(server->queue.functions._add_fn))(server,
                                                (void *)server->queue.functions._context,
                                                unique, unique_size,
                                                function_name,
                                                function_name_size,
                                                data, data_size, priority, 
                                                when);
  }

  return server->queue.object->add(server,
                                   unique, unique_size,
                                   function_name,
                                   function_name_size,
                                   data, data_size, priority, 
                                   when);
}

gearmand_error_t gearman_queue_flush(gearman_server_st *server)
{
  if (server->queue_version == QUEUE_VERSION_FUNCTION)
  {
    assert(server->queue.functions._flush_fn);
    return (*(server->queue.functions._flush_fn))(server, (void *)server->queue.functions._context);
  }

  return server->queue.object->flush(server);
}

gearmand_error_t gearman_queue_done(gearman_server_st *server,
                                    const char *unique,
                                    size_t unique_size,
                                    const char *function_name,
                                    size_t function_name_size)
{
  if (server->queue_version == QUEUE_VERSION_FUNCTION)
  {
    assert(server->queue.functions._done_fn);
    return (*(server->queue.functions._done_fn))(server,
                                                  (void *)server->queue.functions._context,
                                                  unique, unique_size,
                                                  function_name,
                                                  function_name_size);
  }

  return server->queue.object->done(server,
                                    unique, unique_size,
                                    function_name,
                                    function_name_size);
}

gearmand_error_t gearman_queue_replay(gearman_server_st *server,
                                      gearman_queue_add_fn *add_fn,
                                      void *add_context)
{
  if (server->queue_version == QUEUE_VERSION_FUNCTION)
  {
    assert(server->queue.functions._replay_fn);
    return (*(server->queue.functions._replay_fn))(server,
                                                    (void *)server->queue.functions._context,
                                                    add_fn,
                                                    server);
  }

  return server->queue.object->replay(server);
}

void gearman_server_set_queue(gearman_server_st *server,
                              void *context,
                              gearman_queue_add_fn *add,
                              gearman_queue_flush_fn *flush,
                              gearman_queue_done_fn *done,
                              gearman_queue_replay_fn *replay)
{
  server->queue_version= QUEUE_VERSION_FUNCTION;
  server->queue.functions._context= context;
  server->queue.functions._add_fn= add;
  server->queue.functions._flush_fn= flush;
  server->queue.functions._done_fn= done;
  server->queue.functions._replay_fn= replay;
}

void gearman_server_set_queue(gearman_server_st *server,
                              gearmand::queue::Context* context)
{
  server->queue_version= QUEUE_VERSION_CLASS;
  server->queue.object= context;
}

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
       ++iter)
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
       ++iter)
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
