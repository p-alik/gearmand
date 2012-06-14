/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server function definitions
 */

#include <config.h>
#include <libgearman-server/common.h>

#include <cstring>
#include <memory>

#include <libgearman-server/list.h>

/*
 * Public definitions
 */

static gearman_server_function_st* gearman_server_function_create(gearman_server_st *server)
{
  gearman_server_function_st* function= new (std::nothrow) gearman_server_function_st;

  if (function == NULL)
  {
    gearmand_merror("new gearman_server_function_st", gearman_server_function_st, 0);
    return NULL;
  }

  function->worker_count= 0;
  function->job_count= 0;
  function->job_total= 0;
  function->job_running= 0;
  memset(function->max_queue_size, GEARMAN_DEFAULT_MAX_QUEUE_SIZE,
         sizeof(uint32_t) * GEARMAN_JOB_PRIORITY_MAX);
  function->function_name_size= 0;
  gearmand_server_list_add(server, function);
  function->function_name= NULL;
  function->worker_list= NULL;
  memset(function->job_list, 0,
         sizeof(gearman_server_job_st *) * GEARMAN_JOB_PRIORITY_MAX);
  memset(function->job_end, 0,
         sizeof(gearman_server_job_st *) * GEARMAN_JOB_PRIORITY_MAX);

  return function;
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

gearman_server_function_st *
gearman_server_function_get(gearman_server_st *server,
                            const char *function_name,
                            size_t function_name_size)
{
  gearman_server_function_st *function;

  for (function= server->function_list; function != NULL;
       function= function->next)
  {
    if (function->function_name_size == function_name_size and
        memcmp(function->function_name, function_name, function_name_size) == 0)
    {
      return function;
    }
  }

  function= gearman_server_function_create(server);
  if (function == NULL)
  {
    return NULL;
  }

  function->function_name= new char[function_name_size +1];
  if (function->function_name == NULL)
  {
    gearmand_merror("new[]", char,  function_name_size +1);
    gearman_server_function_free(server, function);
    return NULL;
  }

  memcpy(function->function_name, function_name, function_name_size);
  function->function_name[function_name_size]= 0;
  function->function_name_size= function_name_size;

  return function;
}

void gearman_server_function_free(gearman_server_st *server, gearman_server_function_st *function)
{
  delete function->function_name;

  gearmand_server_list_free(server, function);

  delete function;
}
