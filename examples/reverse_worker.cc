/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
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



#include "config.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>

#include <libgearman/gearman.h>
#include <boost/program_options.hpp>

struct reverse_worker_options_t
{
  bool chunk;
  bool status;
  bool unique;

  reverse_worker_options_t():
    chunk(false),
    status(false),
    unique(false)
  { }
};

#pragma GCC diagnostic ignored "-Wold-style-cast"

static void *reverse(gearman_job_st *job, void *context,
                     size_t *result_size, gearman_return_t *ret_ptr);

int main(int args, char *argv[])
{
  uint32_t count;
  reverse_worker_options_t options;
  int timeout;

  in_port_t port;
  std::string host;
  boost::program_options::options_description desc("Options");
  desc.add_options()
    ("help", "Options related to the program.")
    ("host,h", boost::program_options::value<std::string>(&host)->default_value("localhost"),"Connect to the host")
    ("port,p", boost::program_options::value<in_port_t>(&port)->default_value(GEARMAN_DEFAULT_TCP_PORT), "Port number use for connection")
    ("count,c", boost::program_options::value<uint32_t>(&count)->default_value(0), "Number of jobs to run before exiting")
    ("timeout,u", boost::program_options::value<int>(&timeout)->default_value(-1), "Timeout in milliseconds")
    ("chunk,d", boost::program_options::bool_switch(&options.chunk)->default_value(false), "Send result back in data chunks")
    ("status,s", boost::program_options::bool_switch(&options.status)->default_value(false), "Send status updates and sleep while running job")
    ("unique,u", boost::program_options::bool_switch(&options.unique)->default_value(false), "When grabbing jobs, grab the uniqie id")
            ;

  boost::program_options::variables_map vm;
  try
  {
    boost::program_options::store(boost::program_options::parse_command_line(args, argv, desc), vm);
    boost::program_options::notify(vm);
  }
  catch(std::exception &e)
  { 
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    std::cerr << "signal:" << strerror(errno) << std::endl;
    return EXIT_FAILURE;
  }

  gearman_worker_st worker;
  if (gearman_worker_create(&worker) == NULL)
  {
    std::cerr << "Memory allocation failure on worker creation." << std::endl;
    return EXIT_FAILURE;
  }

  if (options.unique)
    gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);

  if (timeout >= 0)
    gearman_worker_set_timeout(&worker, timeout);

  gearman_return_t ret;
  ret= gearman_worker_add_server(&worker, host.c_str(), port);
  if (ret != GEARMAN_SUCCESS)
  {
    std::cerr << gearman_worker_error(&worker) << std::endl;
    return EXIT_FAILURE;
  }

  ret= gearman_worker_add_function(&worker, "reverse", 0, reverse,
                                   &options);
  if (ret != GEARMAN_SUCCESS)
  {
    std::cerr << gearman_worker_error(&worker) << std::endl;
    return EXIT_FAILURE;
  }

  while (1)
  {
    ret= gearman_worker_work(&worker);
    if (ret != GEARMAN_SUCCESS)
    {
      std::cerr << gearman_worker_error(&worker) << std::endl;
      break;
    }

    if (count > 0)
    {
      count--;
      if (count == 0)
        break;
    }
  }

  gearman_worker_free(&worker);

  return EXIT_SUCCESS;
}

static void *reverse(gearman_job_st *job, void *context,
                     size_t *result_size, gearman_return_t *ret_ptr)
{
  reverse_worker_options_t options= *((reverse_worker_options_t *)context);


  const char *workload;
  workload= (const char *)gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  char *result;
  result= (char *)malloc(*result_size);
  if (result == NULL)
  {
    perror("malloc");
    *ret_ptr= GEARMAN_WORK_FAIL;
    return NULL;
  }

  size_t x;
  size_t y;
  for (y= 0, x= *result_size; x; x--, y++)
  {
    result[y]= ((uint8_t *)workload)[x - 1];

    if (options.chunk)
    {
      *ret_ptr= gearman_job_send_data(job, &(result[y]), 1);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        free(result);
        return NULL;
      }
    }

    if (options.status)
    {
      *ret_ptr= gearman_job_send_status(job, (uint32_t)y,
                                        (uint32_t)*result_size);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        free(result);
        return NULL;
      }

      sleep(1);
    }
  }

  std::cout << "Job=" << gearman_job_handle(job);

  if (options.unique)
  {
    std::cout << "Unique=" << gearman_job_unique(job);
  }


  std::cout << "  Workload=";
  std::cout.write(workload, *result_size);

  std::cout << "  Result=";
  std::cout.write(result, *result_size);

  std::cout << std::endl;

  *ret_ptr= GEARMAN_SUCCESS;

  if (options.chunk)
  {
    *result_size= 0;
    return NULL;
  }

  return result;
}
