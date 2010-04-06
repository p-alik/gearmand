/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server job definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_server_job_private Private Server Job Functions
 * @ingroup gearman_server_job
 * @{
 */

/**
 * Generate hash key for job handles and unique IDs.
 */
static uint32_t _server_job_hash(const char *key, size_t key_size);

/**
 * Appends a worker onto the end of a list of workers.
 */
static inline void _server_con_worker_list_append(gearman_server_worker_st *list,
                                                  gearman_server_worker_st *worker);

/**
 * Get a server job structure from the unique ID. If data_size is non-zero,
 * then unique points to the workload data and not a real unique key.
 */
static gearman_server_job_st *
_server_job_get_unique(gearman_server_st *server, uint32_t unique_key,
                       gearman_server_function_st *server_function,
                       const char *unique, size_t data_size);

/** @} */

/*
 * Public definitions
 */

gearman_server_job_st *
gearman_server_job_add(gearman_server_st *server, const char *function_name,
                       size_t function_name_size, const char *unique,
                       size_t unique_size, const void *data, size_t data_size,
                       gearman_job_priority_t priority,
                       gearman_server_client_st *server_client,
                       gearman_return_t *ret_ptr)
{
  gearman_server_job_st *server_job;
  gearman_server_function_st *server_function;
  uint32_t key;

  server_function= gearman_server_function_get(server, function_name,
                                               function_name_size);
  if (server_function == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  if (unique_size == 0)
  {
    server_job= NULL;
    key= 0;
  }
  else
  {
    if (unique_size == 1 && *unique ==  '-')
    {
      if (data_size == 0)
      {
        key= 0;
        server_job= NULL;
      }
      else
      {
        /* Look up job via unique data when unique = '-'. */
        key= _server_job_hash(data, data_size);
        server_job= _server_job_get_unique(server, key, server_function, data,
                                           data_size);
      }
    }
    else
    {
      /* Look up job via unique ID first to make sure it's not a duplicate. */
      key= _server_job_hash(unique, unique_size);
      server_job= _server_job_get_unique(server, key, server_function, unique,
                                         0);
    }
  }

  if (server_job == NULL)
  {
    if (server_function->max_queue_size > 0 &&
        server_function->job_total >= server_function->max_queue_size)
    {
      *ret_ptr= GEARMAN_JOB_QUEUE_FULL;
      return NULL;
    }

    server_job= gearman_server_job_create(server, NULL);
    if (server_job == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    server_job->priority= priority;

    server_job->function= server_function;
    server_function->job_total++;

    snprintf(server_job->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s:%u",
             server->job_handle_prefix, server->job_handle_count);
    snprintf(server_job->unique, GEARMAN_UNIQUE_SIZE, "%.*s",
             (uint32_t)unique_size, unique);
    server->job_handle_count++;
    server_job->data= data;
    server_job->data_size= data_size;

    server_job->unique_key= key;
    key= key % GEARMAN_JOB_HASH_SIZE;
    GEARMAN_HASH_ADD(server->unique, key, server_job, unique_);

    key= _server_job_hash(server_job->job_handle,
                          strlen(server_job->job_handle));
    server_job->job_handle_key= key;
    key= key % GEARMAN_JOB_HASH_SIZE;
    GEARMAN_HASH_ADD(server->job, key, server_job,);

    if (server->state.queue_startup)
    {
      server_job->job_queued= true;
    }
    else if (server_client == NULL && server->queue_add_fn != NULL)
    {
      *ret_ptr= (*(server->queue_add_fn))(server,
                                          (void *)server->queue_context,
                                          server_job->unique,
                                          unique_size,
                                          function_name,
                                          function_name_size,
                                          data, data_size, priority);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        server_job->data= NULL;
        gearman_server_job_free(server_job);
        return NULL;
      }

      if (server->queue_flush_fn != NULL)
      {
        *ret_ptr= (*(server->queue_flush_fn))(server,
                                              (void *)server->queue_context);
        if (*ret_ptr != GEARMAN_SUCCESS)
        {
          server_job->data= NULL;
          gearman_server_job_free(server_job);
          return NULL;
        }
      }

      server_job->job_queued= true;
    }

    *ret_ptr= gearman_server_job_queue(server_job);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      if (server_client == NULL && server->queue_done_fn != NULL)
      {
        /* Do our best to remove the job from the queue. */
        (void)(*(server->queue_done_fn))(server,
                                      (void *)server->queue_context,
                                      server_job->unique, unique_size,
                                      server_job->function->function_name,
                                      server_job->function->function_name_size);
      }

      gearman_server_job_free(server_job);
      return NULL;
    }
  }
  else
    *ret_ptr= GEARMAN_JOB_EXISTS;

  if (server_client != NULL)
  {
    server_client->job= server_job;
    GEARMAN_LIST_ADD(server_job->client, server_client, job_)
  }

  return server_job;
}

gearman_server_job_st *
gearman_server_job_create(gearman_server_st *server,
                          gearman_server_job_st *server_job)
{
  if (server_job == NULL)
  {
    if (server->free_job_count > 0)
    {
      server_job= server->free_job_list;
      GEARMAN_LIST_DEL(server->free_job, server_job,)
    }
    else
    {
      server_job= (gearman_server_job_st *)malloc(sizeof(gearman_server_job_st));
      if (server_job == NULL)
        return NULL;
    }

    server_job->options.allocated= true;
  }
  else
    server_job->options.allocated= false;

  server_job->ignore_job= false;
  server_job->job_queued= false;
  server_job->retries= 0;
  server_job->priority= 0;
  server_job->job_handle_key= 0;
  server_job->unique_key= 0;
  server_job->client_count= 0;
  server_job->numerator= 0;
  server_job->denominator= 0;
  server_job->data_size= 0;
  server_job->server= server;
  server_job->next= NULL;
  server_job->prev= NULL;
  server_job->unique_next= NULL;
  server_job->unique_prev= NULL;
  server_job->worker_next= NULL;
  server_job->worker_prev= NULL;
  server_job->function= NULL;
  server_job->function_next= NULL;
  server_job->data= NULL;
  server_job->client_list= NULL;
  server_job->worker= NULL;
  server_job->job_handle[0]= 0;
  server_job->unique[0]= 0;

  return server_job;
}

void gearman_server_job_free(gearman_server_job_st *server_job)
{
  uint32_t key;

  if (server_job->worker != NULL)
    server_job->function->job_running--;

  server_job->function->job_total--;

  if (server_job->data != NULL)
    free((void *)(server_job->data));

  while (server_job->client_list != NULL)
    gearman_server_client_free(server_job->client_list);

  if (server_job->worker != NULL)
    GEARMAN_LIST_DEL(server_job->worker->job, server_job, worker_)

  key= server_job->unique_key % GEARMAN_JOB_HASH_SIZE;
  GEARMAN_HASH_DEL(server_job->server->unique, key, server_job, unique_);

  key= server_job->job_handle_key % GEARMAN_JOB_HASH_SIZE;
  GEARMAN_HASH_DEL(server_job->server->job, key, server_job,);

  if (server_job->options.allocated)
  {
    if (server_job->server->free_job_count < GEARMAN_MAX_FREE_SERVER_JOB)
      GEARMAN_LIST_ADD(server_job->server->free_job, server_job,)
    else
      free(server_job);
  }
}

gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              const char *job_handle,
                                              gearman_server_con_st *worker_con)
{
  uint32_t key;

  key= _server_job_hash(job_handle, strlen(job_handle));

  for (gearman_server_job_st *server_job= server->job_hash[key % GEARMAN_JOB_HASH_SIZE];
       server_job != NULL; server_job= server_job->next)
  {
    if (server_job->job_handle_key == key &&
        !strcmp(server_job->job_handle, job_handle))
    {
      /* Check to make sure the worker asking for the job still owns the job. */
      if (worker_con != NULL &&
          (server_job->worker == NULL || server_job->worker->con != worker_con))
      {
        return NULL;
      }

      return server_job;
    }
  }

  return NULL;
}

gearman_server_job_st *
gearman_server_job_peek(gearman_server_con_st *server_con)
{
  gearman_server_worker_st *server_worker;
  gearman_job_priority_t priority;

  for (server_worker= server_con->worker_list; server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
    {
      for (priority= GEARMAN_JOB_PRIORITY_HIGH;
           priority != GEARMAN_JOB_PRIORITY_MAX; priority++)
      {
        if (server_worker->function->job_list[priority] != NULL)
        {
          if (server_worker->function->job_list[priority]->ignore_job)
          {
            /* This is only happens when a client disconnects from a foreground
              job. We do this because we don't want to run the job anymore. */
            server_worker->function->job_list[priority]->ignore_job= false;

            gearman_server_job_free(gearman_server_job_take(server_con));

            return gearman_server_job_peek(server_con);
          }
          return server_worker->function->job_list[priority];
        }
      }
    }
  }

  return NULL;
}

static inline void _server_con_worker_list_append(gearman_server_worker_st *list,
                                                  gearman_server_worker_st *worker)
{
  worker->con_prev= NULL;
  worker->con_next= list;
  while (worker->con_next != NULL)
  {
    worker->con_prev= worker->con_next;
    worker->con_next= worker->con_next->con_next;
  }
  if (worker->con_prev)
    worker->con_prev->con_next= worker;
}

gearman_server_job_st *
gearman_server_job_take(gearman_server_con_st *server_con)
{
  gearman_server_worker_st *server_worker;
  gearman_server_job_st *server_job;
  gearman_job_priority_t priority;

  for (server_worker= server_con->worker_list; server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
      break;
  }

  if (server_worker == NULL)
    return NULL;

  if (server_con->thread->server->flags.round_robin)
  {
    GEARMAN_LIST_DEL(server_con->worker, server_worker, con_)
    _server_con_worker_list_append(server_con->worker_list, server_worker);
    ++server_con->worker_count;
    if (server_con->worker_list == NULL) {
      server_con->worker_list= server_worker;
    }
  }

  for (priority= GEARMAN_JOB_PRIORITY_HIGH;
       priority != GEARMAN_JOB_PRIORITY_MAX; priority++)
  {
    if (server_worker->function->job_list[priority] != NULL)
      break;
  }

  server_job= server_worker->function->job_list[priority];
  server_job->function->job_list[priority]= server_job->function_next;
  if (server_job->function->job_end[priority] == server_job)
    server_job->function->job_end[priority]= NULL;
  server_job->function->job_count--;

  server_job->worker= server_worker;
  GEARMAN_LIST_ADD(server_worker->job, server_job, worker_)
  server_job->function->job_running++;

  if (server_job->ignore_job)
  {
    gearman_server_job_free(server_job);
    return gearman_server_job_take(server_con);
  }

  return server_job;
}

gearman_return_t gearman_server_job_queue(gearman_server_job_st *job)
{
  gearman_server_client_st *client;
  gearman_server_worker_st *worker;
  uint32_t noop_sent;
  gearman_return_t ret;

  if (job->worker != NULL)
  {
    job->retries++;
    if (job->server->job_retries == job->retries)
    {
       gearman_log_error(job->server->gearman,
                            "Dropped job due to max retry count: %s %s",
                            job->job_handle, job->unique);
       for (client= job->client_list; client != NULL; client= client->job_next)
       {
         ret= gearman_server_io_packet_add(client->con, false,
                                           GEARMAN_MAGIC_RESPONSE,
                                           GEARMAN_COMMAND_WORK_FAIL,
                                           job->job_handle,
                                           (size_t)strlen(job->job_handle),
                                           NULL);
         if (ret != GEARMAN_SUCCESS)
           return ret;
      }

      /* Remove from persistent queue if one exists. */
      if (job->job_queued && job->server->queue_done_fn != NULL)
      {
        ret= (*(job->server->queue_done_fn))(job->server,
                                             (void *)job->server->queue_context,
                                             job->unique,
                                             (size_t)strlen(job->unique),
                                             job->function->function_name,
                                             job->function->function_name_size);
        if (ret != GEARMAN_SUCCESS)
          return ret;
      }

      gearman_server_job_free(job);
      return GEARMAN_SUCCESS;
    }

    GEARMAN_LIST_DEL(job->worker->job, job, worker_)
    job->worker= NULL;
    job->function->job_running--;
    job->function_next= NULL;
    job->numerator= 0;
    job->denominator= 0;
  }

  /* Queue NOOP for possible sleeping workers. */
  if (job->function->worker_list != NULL)
  {
    worker= job->function->worker_list;
    noop_sent= 0;
    do
    {
      if (worker->con->is_sleeping && ! (worker->con->is_noop_sent))
      {
        ret= gearman_server_io_packet_add(worker->con, false,
                                          GEARMAN_MAGIC_RESPONSE,
                                          GEARMAN_COMMAND_NOOP, NULL);
        if (ret != GEARMAN_SUCCESS)
          return ret;

        worker->con->is_noop_sent= true;
        noop_sent++;
      }

      worker= worker->function_next;
    }
    while (worker != job->function->worker_list &&
           (job->server->worker_wakeup == 0 ||
           noop_sent < job->server->worker_wakeup));

    job->function->worker_list= worker;
  }

  /* Queue the job to be run. */
  if (job->function->job_list[job->priority] == NULL)
    job->function->job_list[job->priority]= job;
  else
    job->function->job_end[job->priority]->function_next= job;
  job->function->job_end[job->priority]= job;
  job->function->job_count++;

  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static uint32_t _server_job_hash(const char *key, size_t key_size)
{
  const char *ptr= key;
  int32_t value= 0;

  while (key_size--)
  {
    value += (int32_t)*ptr++;
    value += (value << 10);
    value ^= (value >> 6);
  }
  value += (value << 3);
  value ^= (value >> 11);
  value += (value << 15);

  return (uint32_t)(value == 0 ? 1 : value);
}

static gearman_server_job_st *
_server_job_get_unique(gearman_server_st *server, uint32_t unique_key,
                       gearman_server_function_st *server_function,
                       const char *unique, size_t data_size)
{
  gearman_server_job_st *server_job;

  for (server_job= server->unique_hash[unique_key % GEARMAN_JOB_HASH_SIZE];
       server_job != NULL; server_job= server_job->unique_next)
  {
    if (data_size == 0)
    {
      if (server_job->function == server_function &&
          server_job->unique_key == unique_key &&
          !strcmp(server_job->unique, unique))
      {
        return server_job;
      }
    }
    else
    {
      if (server_job->function == server_function &&
          server_job->unique_key == unique_key &&
          server_job->data_size == data_size &&
          !memcmp(server_job->data, unique, data_size))
      {
        return server_job;
      }
    }
  }

  return NULL;
}
