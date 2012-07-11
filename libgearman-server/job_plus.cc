/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
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

#include <libgearman-server/common.h>
#include <libgearman-server/list.h>

#include <cstring>
#include <memory>
#include <cassert>

/**
 * Generate hash key for job handles and unique IDs.
 */
uint32_t _server_job_hash(const char *key, size_t key_size)
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

void _server_con_worker_list_append(gearman_server_worker_st *list,
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
  {
    worker->con_prev->con_next= worker;
  }
}

gearman_server_job_st* build_gearman_server_job_st(void)
{
  return new (std::nothrow) gearman_server_job_st;
}

void destroy_gearman_server_job_st(gearman_server_job_st* arg)
{
  gearmand_debug("delete gearman_server_con_st");
  delete arg;
}

gearman_server_job_st *gearman_server_job_get_by_unique(gearman_server_st *server,
                                                        const char *unique,
                                                        const size_t unique_length,
                                                        gearman_server_con_st *worker_con)
{
  (void)unique_length;
  for (size_t x= 0; x < GEARMAND_JOB_HASH_SIZE; x++)
  {
    for (gearman_server_job_st *server_job= server->job_hash[x];
         server_job != NULL;
         server_job= server_job->next)
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "COMPARE unique \"%s\"(%u) == \"%s\"(%u)",
                         bool(server_job->unique[0]) ? server_job->unique :  "<null>", uint32_t(strlen(server_job->unique)),
                         unique, uint32_t(unique_length));

      if (bool(server_job->unique[0]) and
          (strcmp(server_job->unique, unique) == 0))
      {
        /* Check to make sure the worker asking for the job still owns the job. */
        if (worker_con != NULL and
            (server_job->worker == NULL or server_job->worker->con != worker_con))
        {
          return NULL;
        }

        return server_job;
      }
    }
  }

  return NULL;
}

gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              const char *job_handle,
                                              const size_t job_handle_length,
                                              gearman_server_con_st *worker_con)
{
  uint32_t key= _server_job_hash(job_handle, job_handle_length);

  for (gearman_server_job_st *server_job= server->job_hash[key % GEARMAND_JOB_HASH_SIZE];
       server_job != NULL; server_job= server_job->next)
  {
    if (server_job->job_handle_key == key and
        strncmp(server_job->job_handle, job_handle, GEARMAND_JOB_HANDLE_SIZE) == 0)
    {
      /* Check to make sure the worker asking for the job still owns the job. */
      if (worker_con != NULL and
          (server_job->worker == NULL or server_job->worker->con != worker_con))
      {
        return NULL;
      }

      return server_job;
    }
  }

  return NULL;
}

gearman_server_job_st * gearman_server_job_peek(gearman_server_con_st *server_con)
{
  for (gearman_server_worker_st *server_worker= server_con->worker_list;
       server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
    {
      for (gearman_job_priority_t priority= GEARMAN_JOB_PRIORITY_HIGH;
           priority != GEARMAN_JOB_PRIORITY_MAX;
           priority= gearman_job_priority_t(int(priority) +1))
      {
        gearman_server_job_st *server_job;
        server_job= server_worker->function->job_list[priority];

        int64_t current_time= (int64_t)time(NULL);

        while(server_job && 
              server_job->when != 0 && 
              server_job->when > current_time)
        {
          server_job= server_job->function_next;  
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

gearman_server_job_st *gearman_server_job_take(gearman_server_con_st *server_con)
{
  for (gearman_server_worker_st *server_worker= server_con->worker_list; server_worker; server_worker= server_worker->con_next)
  {
    if (server_worker->function and server_worker->function->job_count)
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Jobs available for %.*s: %lu",
                         (int)server_worker->function->function_name_size, server_worker->function->function_name,
                         (unsigned long)(server_worker->function->job_count));

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

      gearman_job_priority_t priority;
      for (priority= GEARMAN_JOB_PRIORITY_HIGH; priority < GEARMAN_JOB_PRIORITY_LOW;
           priority= gearman_job_priority_t(int(priority) +1))
      {
        if (server_worker->function->job_list[priority])
        {
          break;
        }
      }

      gearman_server_job_st *server_job= server_worker->function->job_list[priority];
      gearman_server_job_st *previous_job= server_job;
  
      int64_t current_time= (int64_t)time(NULL);
  
      while (server_job and server_job->when != 0 and server_job->when > current_time)
      {
        previous_job= server_job;
        server_job= server_job->function_next;  
      }
  
      if (server_job)
      { 
        if (server_job->function->job_list[priority] == server_job)
        {
          // If it's the head of the list, advance it
          server_job->function->job_list[priority]= server_job->function_next;
        }
        else
        {
          // Otherwise, just remove the item from the list
          previous_job->function_next= server_job->function_next;
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

void *_proc(void *data)
{
  gearman_server_st *server= (gearman_server_st *)data;

  gearmand_initialize_thread_logging("[  proc ]");

  while (1)
  {
    if (pthread_mutex_lock(&(server->proc_lock)) == -1)
    {
      gearmand_fatal("pthread_mutex_lock()");
      return NULL;
    }

    while (server->proc_wakeup == false)
    {
      if (server->proc_shutdown)
      {
        if (pthread_mutex_unlock(&(server->proc_lock)) == -1)
        {
          gearmand_fatal("pthread_mutex_unlock()");
          assert(!"pthread_mutex_lock");
        }
        return NULL;
      }

      (void) pthread_cond_wait(&(server->proc_cond), &(server->proc_lock));
    }
    server->proc_wakeup= false;
    if (pthread_mutex_unlock(&(server->proc_lock)) == -1)
    {
      gearmand_fatal("pthread_mutex_unlock()");
    }

    for (gearman_server_thread_st *thread= server->thread_list; thread != NULL; thread= thread->next)
    {
      gearman_server_con_st *con;
      while ((con= gearman_server_con_proc_next(thread)) != NULL)
      {
        bool packet_sent = false;
        while (1)
        {
          gearman_server_packet_st *packet= gearman_server_proc_packet_remove(con);
          if (packet == NULL)
          {
            break;
          }

          con->ret= gearman_server_run_command(con, &(packet->packet));
          packet_sent = true;
          gearmand_packet_free(&(packet->packet));
          gearman_server_packet_free(packet, con->thread, false);
        }

        // if a packet was sent in above block, and connection is dead,
        // queue up into io thread so it comes back to the PROC queue for
        // marking proc_removed. this prevents leaking any connection objects
        if (packet_sent)
        {
          if (con->is_dead)
          {
            gearman_server_con_io_add(con);
          }
        }
        else if (con->is_dead)
        {
          gearman_server_con_free_workers(con);

          while (con->client_list != NULL)
            gearman_server_client_free(con->client_list);

          con->proc_removed= true;
          gearman_server_con_to_be_freed_add(con);
        }
      }
    }
  }
}

gearman_server_job_st * gearman_server_job_create(gearman_server_st *server)
{
  gearman_server_job_st *server_job;

  if (server->free_job_count > 0)
  {
    server_job= server->free_job_list;
    gearmand_server_free_job_list_free(server, server_job);
  }
  else
  {
    server_job= new (std::nothrow) gearman_server_job_st;
    if (server_job == NULL)
    {
      return NULL;
    }
  }

  server_job->ignore_job= false;
  server_job->job_queued= false;
  server_job->retries= 0;
  server_job->priority= GEARMAN_JOB_PRIORITY_NORMAL;
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
  server_job->unique_length= 0;

  return server_job;
}
