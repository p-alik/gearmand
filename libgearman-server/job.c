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



/**
 * @file
 * @brief Server job definitions
 */

#include <libgearman-server/common.h>
#include <string.h>

#include <libgearman-server/list.h>
#include <libgearman-server/hash.h>

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

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-fpermissive"


/*
 * Public definitions
 */

gearman_server_job_st *
gearman_server_job_add(gearman_server_st *server,
                       const char *function_name, size_t function_name_size,
                       const char *unique, size_t unique_size,
                       const void *data, size_t data_size,
                       gearmand_job_priority_t priority,
                       gearman_server_client_st *server_client,
                       gearmand_error_t *ret_ptr,
                       int64_t when)
{
  gearman_server_function_st *server_function= gearman_server_function_get(server, function_name,
                                                                           function_name_size);
  if (server_function == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  uint32_t key;
  gearman_server_job_st *server_job;
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

    server_job= gearman_server_job_create(server);
    if (server_job == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    server_job->priority= priority;

    server_job->function= server_function;
    server_function->job_total++;

    int checked_length;
    checked_length= snprintf(server_job->job_handle, GEARMAND_JOB_HANDLE_SIZE, "%s:%u",
                             server->job_handle_prefix, server->job_handle_count);

    if (checked_length >= GEARMAND_JOB_HANDLE_SIZE || checked_length < 0)
    {
      gearmand_log_error("Job handle plus handle count beyond GEARMAND_JOB_HANDLE_SIZE: %s:%u",
                         server->job_handle_prefix, server->job_handle_count);
    }

    checked_length= snprintf(server_job->unique, GEARMAN_UNIQUE_SIZE, "%.*s",
                             (int)unique_size, unique);
    if (checked_length >= GEARMAN_UNIQUE_SIZE || checked_length < 0)
    {
      gearmand_log_error("We recieved a unique beyond GEARMAN_UNIQUE_SIZE: %.*s", (int)unique_size, unique);
    }

    server->job_handle_count++;
    server_job->data= data;
    server_job->data_size= data_size;
		server_job->when= when; 
		
    server_job->unique_key= key;
    key= key % GEARMAND_JOB_HASH_SIZE;
    GEARMAN_HASH_ADD(server->unique, key, server_job, unique_);

    key= _server_job_hash(server_job->job_handle,
                          strlen(server_job->job_handle));
    server_job->job_handle_key= key;
    key= key % GEARMAND_JOB_HASH_SIZE;
    gearmand_hash_server_add(server, key, server_job);

    if (server->state.queue_startup)
    {
      server_job->job_queued= true;
    }
    else if (server_client == NULL && server->queue._add_fn != NULL)
    {
      *ret_ptr= (*(server->queue._add_fn))(server,
                                         (void *)server->queue._context,
                                          server_job->unique,
                                          unique_size,
                                          function_name,
                                          function_name_size,
                                          data, data_size, priority, 
                                          when);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        server_job->data= NULL;
        gearman_server_job_free(server_job);
        return NULL;
      }

      if (server->queue._flush_fn != NULL)
      {
        *ret_ptr= (*(server->queue._flush_fn))(server,
                                              (void *)server->queue._context);
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
      if (server_client == NULL && server->queue._done_fn != NULL)
      {
        /* Do our best to remove the job from the queue. */
        (void)(*(server->queue._done_fn))(server,
                                      (void *)server->queue._context,
                                      server_job->unique, unique_size,
                                      server_job->function->function_name,
                                      server_job->function->function_name_size);
      }

      gearman_server_job_free(server_job);
      return NULL;
    }
  }
  else
  {
    *ret_ptr= GEARMAN_JOB_EXISTS;
  }

  if (server_client != NULL)
  {
    server_client->job= server_job;
    GEARMAN_LIST_ADD(server_job->client, server_client, job_)
  }

  return server_job;
}

gearman_server_job_st *
gearman_server_job_create(gearman_server_st *server)
{
  gearman_server_job_st *server_job;

  if (server->free_job_count > 0)
  {
    server_job= server->free_job_list;
    gearmand_server_free_job_list_free(server, server_job);
  }
  else
  {
    server_job= (gearman_server_job_st *)malloc(sizeof(gearman_server_job_st));
    if (server_job == NULL)
      return NULL;
  }

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

  key= server_job->unique_key % GEARMAND_JOB_HASH_SIZE;
  GEARMAN_HASH_DEL(Server->unique, key, server_job, unique_);

  key= server_job->job_handle_key % GEARMAND_JOB_HASH_SIZE;
  gearmand_hash_server_free(Server, key, server_job);

  if (Server->free_job_count < GEARMAN_MAX_FREE_SERVER_JOB)
  {
    gearmand_server_job_list_add(Server, server_job);
  }
  else
  {
    free(server_job);
  }
}

gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              const char *job_handle,
                                              gearman_server_con_st *worker_con)
{
  uint32_t key;

  key= _server_job_hash(job_handle, strlen(job_handle));

  for (gearman_server_job_st *server_job= server->job_hash[key % GEARMAND_JOB_HASH_SIZE];
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
  for (gearman_server_worker_st *server_worker= server_con->worker_list;
       server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
    {
      for (gearmand_job_priority_t priority= GEARMAND_JOB_PRIORITY_HIGH;
           priority != GEARMAND_JOB_PRIORITY_MAX; priority++)
      {
        gearman_server_job_st *server_job;
        server_job= server_worker->function->job_list[priority];

        int64_t current_time= (int64_t)time(NULL);

        while(server_job != NULL && 
             server_job->when != 0 && 
             server_job->when > current_time)
        {
          server_job = server_job->function_next;  
        }
        
        if (server_job != NULL)
        {

          if (server_job->ignore_job)
          {
            /* This is only happens when a client disconnects from a foreground
              job. We do this because we don't want to run the job anymore. */
            server_job->ignore_job= false;

            gearman_server_job_free(gearman_server_job_take(server_con));

            return gearman_server_job_peek(server_con);
          }
        
          return server_job;
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
  for (gearman_server_worker_st *server_worker= server_con->worker_list; server_worker != NULL; server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
    {
      if (server_worker == NULL)
        return NULL;

      if (Server->flags.round_robin)
      {
        GEARMAN_LIST_DEL(server_con->worker, server_worker, con_)
        _server_con_worker_list_append(server_con->worker_list, server_worker);
        ++server_con->worker_count;
        if (server_con->worker_list == NULL)
        {
          server_con->worker_list= server_worker;
        }
      }

      gearmand_job_priority_t priority;
      for (priority= GEARMAND_JOB_PRIORITY_HIGH; priority < GEARMAND_JOB_PRIORITY_MAX; priority++)
      {
        if (server_worker->function->job_list[priority] != NULL)
          break;
      }

      gearman_server_job_st *server_job= server_job= server_worker->function->job_list[priority];
      gearman_server_job_st *previous_job= server_job;
  
      int64_t current_time= (int64_t)time(NULL);
  
      while (server_job != NULL && server_job->when != 0 && server_job->when > current_time)
      {
        previous_job= server_job;
        server_job= server_job->function_next;  
      }
  
      if (server_job != NULL)
      { 
        if (server_job->function->job_list[priority] == server_job)
        {
          // If it's the head of the list, advance it
          server_job->function->job_list[priority]= server_job->function_next;
        }
        else
        {
          // Otherwise, just remove the item from the list
          previous_job->function_next = server_job->function_next;
        }
        
        // If it's the tail of the list, move the tail back
        if (server_job->function->job_end[priority] == server_job)
        {
          server_job->function->job_end[priority]= previous_job;
        }
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
    }
  }
  
  return NULL;
}

gearmand_error_t gearman_server_job_queue(gearman_server_job_st *job)
{
  gearman_server_client_st *client;
  gearman_server_worker_st *worker;
  uint32_t noop_sent;
  gearmand_error_t ret;

  if (job->worker != NULL)
  {
    job->retries++;
    if (Server->job_retries == job->retries)
    {
      gearmand_log_error("Dropped job due to max retry count: %s %s",
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
        {
          return ret;
        }
      }

      /* Remove from persistent queue if one exists. */
      if (job->job_queued && Server->queue._done_fn != NULL)
      {
	ret= (*(Server->queue._done_fn))(Server,
					 (void *)Server->queue._context,
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
        {
          gearmand_gerror("gearman_server_io_packet_add", ret);
          return ret;
        }

        worker->con->is_noop_sent= true;
        noop_sent++;
      }

      worker= worker->function_next;
    }
    while (worker != job->function->worker_list &&
           (Server->worker_wakeup == 0 ||
           noop_sent < Server->worker_wakeup));

    job->function->worker_list= worker;
  }

  /* Queue the job to be run. */
  if (job->function->job_list[job->priority] == NULL)
  {
    job->function->job_list[job->priority]= job;
  }
  else
  {
    job->function->job_end[job->priority]->function_next= job;
  }

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

  for (server_job= server->unique_hash[unique_key % GEARMAND_JOB_HASH_SIZE];
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
