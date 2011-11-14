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

#include <libtest/test.hpp>

#include <cassert>
#include <cstring>
#include <memory>
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

#include <tests/start_worker.h>
#include <util/instance.hpp>

using namespace libtest;
using namespace datadifferential;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

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
  volatile bool failed_startup;

  context_st(worker_handle_st* handle_arg, gearman_function_t& arg) :
    port(handle_arg->port()),
    handle(handle_arg),
    options(gearman_worker_options_t()),
    worker_fn(arg),
    namespace_key(NULL),
    context(0),
    failed_startup(false)
  {
    sem_init(&lock, 0, 0);
  }

  const std::string& shutdown_function() const
  {
    return _shutdown_function;
  }

  void fail(void)
  {
    failed_startup= true;
    sem_post(&lock);
  }

  ~context_st()
  {
    sem_destroy(&lock);
  }
};

class Worker {
public:
  Worker()
  {
    _worker= gearman_worker_create(NULL);

    if (_worker == NULL)
    {
      throw "gearman_worker_create() failed";
    }
  }

  gearman_worker_st* operator&() const
  { 
    return _worker;
  }

  gearman_worker_st* operator->() const
  { 
    return _worker;
  }

  ~Worker()
  {
    gearman_worker_free(_worker);
  }

private:
  gearman_worker_st *_worker;

};

extern "C" {

  static void *thread_runner(void *con)
  {
    context_st *context= (context_st *)con;

    assert(context);

    Worker worker;
    assert(&worker);

    assert(context->handle);
    context->handle->worker_id= gearman_worker_id(&worker);

    if (context->namespace_key)
    {
      gearman_worker_set_namespace(&worker, context->namespace_key, strlen(context->namespace_key));
    }

    if (gearman_failed(gearman_worker_add_server(&worker, NULL, context->port)))
    {
      Error << "gearman_worker_add_server()";
      context->fail();
      pthread_exit(0);
    }

    // Check for a working server by "asking" it for an option
    {
      size_t count= 5;
      bool success= false;
      while (--count and success == false)
      {
        success= gearman_worker_set_server_option(&worker, test_literal_param("exceptions"));
      }

      if (success == false)
      {
        Error << "gearman_worker_set_server_option() failed";
        context->fail();
        pthread_exit(0);
      }
    }

    if (gearman_failed(gearman_worker_define_function(&worker,
                                                      context->function_name, strlen(context->function_name), 
                                                      context->worker_fn,
                                                      0, 
                                                      context->context)))
    {
      Error << "Failed to add function " << context->function_name << "(" << gearman_worker_error(&worker) << ")";
      context->fail();
      pthread_exit(0);
    }

    if (context->options != gearman_worker_options_t())
    {
      gearman_worker_add_options(&worker, context->options);
    }

    sem_post(&context->lock);
    gearman_return_t ret= GEARMAN_SUCCESS;
    while (context->handle->is_shutdown() == false or ret != GEARMAN_SHUTDOWN)
    {
      ret= gearman_worker_work(&worker);
    }

    pthread_exit(context);
  }
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

  if (pthread_create(&handle->thread, NULL, thread_runner, context) != 0)
  {
    Error << "pthread_create(" << strerror(errno) << ")";
    delete context;
    delete handle;
    abort();
  }

  sem_wait(&context->lock);

  if (context->failed_startup)
  {
    void *unused;
    pthread_join(handle->thread, &unused);
    delete context;
    delete handle;
    Error << "Now aborting from failed worker startup";
    abort(); // @todo test all code for failure case
  }

  return handle;
}

worker_handle_st::worker_handle_st(const char *, const std::string& name_arg, in_port_t port_arg) :
  _shutdown(false),
  _name(name_arg),
  worker_id(gearman_id_t()),
  _port(port_arg)
{
  pthread_mutex_init(&_shutdown_lock, NULL);
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
  {
    return true;
  }

  set_shutdown();

  gearman_return_t rc;
  if (gearman_failed(rc=  gearman_kill(worker_id, GEARMAN_KILL)))
  {
    Error << "failed to shutdown " << rc;
    return false;
  }

  void *ret;
  pthread_join(thread, &ret);
  context_st *con= (context_st *)ret;

  delete con;
  
  return true;
}

worker_handle_st::~worker_handle_st()
{
  shutdown();
}
