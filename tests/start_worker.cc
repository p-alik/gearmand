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

#include <cassert>
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

#include <cstdio>

#include <libtest/test.hpp>
#include <tests/start_worker.h>
#include <util/instance.hpp>

using namespace libtest;
using namespace datadifferential;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/*
  Print to debug the output of what workers a server might have.
*/
class Finish : public util::Instance::Finish
{
public:
  bool call(bool success, const std::string &response)
  {
    if (success)
    {
      if (response.empty())
      {
        std::cout << "OK" << std::endl;
      }
      else
      {
        std::cout << response;
      }
    }
    else if (not response.empty())
    {
      std::cerr << "Error: " << response;
    }
    else
    {
      std::cerr << "Error" << std::endl;
    }

    return true;
  }
};

class Status : public util::Instance::Finish
{
  bool _dropped;

public:

  Status() :
    _dropped(false)
  { }

  bool call(bool success, const std::string &)
  {
    _dropped= success;

    return true;
  }

  bool dropped() const
  {
    return _dropped;
  }
};

static gearman_return_t shutdown_fn(gearman_job_st*, void* /* context */)
{
  return GEARMAN_SHUTDOWN;
}

struct context_st {
  in_port_t port;
  const char *function_name;
  struct worker_handle_st *handle;
  gearman_worker_options_t options;
  gearman_function_t& worker_fn;
  const char *namespace_key;
  std::string _shutdown_function;
  void *context;
  sem_t lock;

  context_st(worker_handle_st* handle_arg, gearman_function_t& arg) :
    port(handle_arg->port()),
    handle(handle_arg),
    options(gearman_worker_options_t()),
    worker_fn(arg),
    namespace_key(NULL),
    _shutdown_function(handle_arg->shutdown_function()),
    context(0)
  {
    sem_init(&lock, 0, 0);
  }

  const std::string& shutdown_function() const
  {
    return _shutdown_function;
  }

  ~context_st()
  {
    sem_destroy(&lock);
  }
};

static void *thread_runner(void *con)
{
  context_st *context= (context_st *)con;

  assert(context);

  gearman_worker_st *worker= gearman_worker_create(NULL);
  assert(worker);
  context->handle->_worker_ptr= worker;

  if (context->namespace_key)
  {
    gearman_worker_set_namespace(worker, context->namespace_key, strlen(context->namespace_key));
  }

  gearman_return_t rc= gearman_worker_add_server(worker, NULL, context->port);
  assert(rc == GEARMAN_SUCCESS);

  bool success= gearman_worker_set_server_option(worker, test_literal_param("exceptions"));
  assert(success);

  if (gearman_failed(gearman_worker_define_function(worker,
                                                    context->function_name, strlen(context->function_name), 
                                                    context->worker_fn,
                                                    0, 
                                                    context->context)))
  {
    Error << "Failed to add function " << context->function_name << "(" << gearman_worker_error(worker) << ")";
    pthread_exit(0);
  }

  gearman_function_t shutdown_function= gearman_function_create(shutdown_fn);
  if (gearman_failed(gearman_worker_define_function(worker,
                                                    context->shutdown_function().c_str(), context->shutdown_function().size(),
                                                    shutdown_function,
                                                    0, 0)))
  {
    Error << "Failed to add function shutdown(" << gearman_worker_error(worker) << ")";
    pthread_exit(0);
  }

  if (context->options != gearman_worker_options_t())
  {
    gearman_worker_add_options(worker, context->options);
  }

  assert(context->handle);
  sem_post(&context->lock);
  while (context->handle->is_shutdown() == false)
  {
    gearman_return_t ret= gearman_worker_work(worker);
    (void)ret;
  }

  gearman_worker_free(worker);

  delete context;

  pthread_exit(0);
}

struct worker_handle_st *test_worker_start(in_port_t port, 
					   const char *namespace_key,
					   const char *function_name,
					   gearman_function_t &worker_fn,
					   void *context_arg,
					   gearman_worker_options_t options)
{
  worker_handle_st *handle= new worker_handle_st(namespace_key, function_name, port);
  assert(handle);

  context_st *context= new context_st(handle, worker_fn);

  context->port= port;
  context->function_name= function_name;
  context->context= context_arg;
  context->handle= handle;
  context->options= options;
  context->namespace_key= namespace_key;

  test_assert_errno(pthread_create(&handle->thread, NULL, thread_runner, context));

  sem_wait(&context->lock);

  return handle;
}

worker_handle_st::worker_handle_st(const char *namespace_key, const std::string& name_arg, in_port_t port_arg) :
  _shutdown(false),
  _name(name_arg),
  _port(port_arg),
  _worker_ptr(0)
{
  pthread_mutex_init(&_shutdown_lock, NULL);

  uuid_t uuid;
  char uuid_string[37];
  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_string);
  uuid_string[36]= 0;

  _shutdown_function.append(uuid_string);
  _shutdown_function.append("_SHUTDOWN");

  if (namespace_key)
  {
    _fully_shutdown_function.append(namespace_key);
  }
  _fully_shutdown_function+= _shutdown_function;
}

bool worker_handle_st::is_shutdown()
{
  bool tmp;
  pthread_mutex_lock(&_shutdown_lock);
  tmp= _shutdown;
  pthread_mutex_unlock(&_shutdown_lock);

  return tmp;
}


bool worker_handle_st::shutdown()
{
  if (is_shutdown())
    return true;

  set_shutdown();

  gearman_client_st *client= gearman_client_create(NULL);

  if (client == NULL)
  {
    Error << "gearman_client_create(" << gearman_client_error(client) << ")";
    gearman_client_free(client);
    return false;
  }

  if (gearman_failed(gearman_client_add_server(client, NULL, port())))
  {
    Error << "gearman_client_add_server(" << gearman_client_error(client) << ")";
    gearman_client_free(client);
    return false;
  }

  // If the worker is non-responsive this will allow us to not get stuck in
  // gearman_wait().
  gearman_client_set_timeout(client, 1000);

  gearman_return_t rc;
  (void)gearman_client_do(client, shutdown_function(true).c_str(), NULL, NULL, 0, 0, &rc);
  gearman_client_free(client);

  if (gearman_failed(rc))
  {
    if (0)
    {
      Error << "Trying to see what workers are registered:" << port();
      util::Instance instance("localhost", port());
      instance.set_finish(new Finish);

      instance.push(new util::Operation(test_literal_param("workers\r\n")));

      instance.run();
    }

    pthread_cancel(thread);

    return false;
  }
  else
  {
    Status *status;
    util::Instance instance("localhost", port());
    instance.set_finish(status= new Status);

    std::string execute(test_literal_param("drop function "));
    execute.append(shutdown_function(true));
    execute.append("\r\n");
    instance.push(new util::Operation(execute.c_str(), execute.size()));

    instance.run();

    if (not status->dropped())
    {
      Error << "Was unable to drop function " << shutdown_function(true);
    }
  }

  void *unused;
  pthread_join(thread, &unused);
  
  return true;
}

worker_handle_st::~worker_handle_st()
{
  shutdown();
}
