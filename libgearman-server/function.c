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

#include "common.h"

/*
 * Public definitions
 */

gearman_server_function_st *
gearman_server_function_get(gearman_server_st *server,
                            const char *function_name,
                            size_t function_name_size)
{
  gearman_server_function_st *function;

  for (function= server->function_list; function != NULL;
       function= function->next)
  {
    if (function->function_name_size == function_name_size &&
        !memcmp(function->function_name, function_name, function_name_size))
    {
      return function;
    }
  }

  function= gearman_server_function_create(server, NULL);
  if (function == NULL)
    return NULL;

  function->function_name= (char *)malloc(function_name_size + 1);
  if (function->function_name == NULL)
  {
    gearman_server_function_free(function);
    return NULL;
  }

  memcpy(function->function_name, function_name, function_name_size);
  function->function_name[function_name_size]= 0;
  function->function_name_size= function_name_size;

  return function;
}

gearman_server_function_st *
gearman_server_function_create(gearman_server_st *server,
                               gearman_server_function_st *function)
{
  if (function == NULL)
  {
    function= (gearman_server_function_st *)malloc(sizeof(gearman_server_function_st));
    if (function == NULL)
      return NULL;

    function->options.allocated= true;
  }
  else
  {
    function->options.allocated= false;
  }

  function->worker_count= 0;
  function->job_count= 0;
  function->job_total= 0;
  function->job_running= 0;
  function->max_queue_size= GEARMAN_DEFAULT_MAX_QUEUE_SIZE;
  function->function_name_size= 0;
  function->server= server;
  GEARMAN_LIST_ADD(server->function, function,)
  function->function_name= NULL;
  function->worker_list= NULL;
  memset(function->job_list, 0,
         sizeof(gearman_server_job_st *) * GEARMAN_JOB_PRIORITY_MAX);
  memset(function->job_end, 0,
         sizeof(gearman_server_job_st *) * GEARMAN_JOB_PRIORITY_MAX);

  return function;
}

void gearman_server_function_free(gearman_server_function_st *function)
{
  if (function->function_name != NULL)
    free(function->function_name);

  GEARMAN_LIST_DEL(function->server->function, function,)

  if (function->options.allocated)
    free(function);
}
