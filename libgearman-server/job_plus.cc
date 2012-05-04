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

#include <cstring>

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

gearman_server_job_st * gearman_server_job_peek(gearman_server_con_st *server_con)
{
  for (gearman_server_worker_st *server_worker= server_con->worker_list;
       server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_count != 0)
    {
      for (gearmand_job_priority_t priority= GEARMAND_JOB_PRIORITY_HIGH;
           priority != GEARMAND_JOB_PRIORITY_MAX;
           priority= gearmand_job_priority_t(int(priority) +1))
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
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Jobs available %lu", (unsigned long)(server_worker->function->job_count));
    if (server_worker->function->job_count)
    {
      if (server_worker == NULL)
      {
        return NULL;
      }

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
      for (priority= GEARMAND_JOB_PRIORITY_HIGH; priority < GEARMAND_JOB_PRIORITY_LOW;
           priority= gearmand_job_priority_t(int(priority) +1))
      {
        if (server_worker->function->job_list[priority])
        {
          break;
        }
      }

      gearman_server_job_st *server_job= server_job= server_worker->function->job_list[priority];
      gearman_server_job_st *previous_job= server_job;
  
      int64_t current_time= (int64_t)time(NULL);
  
      while (server_job && server_job->when != 0 && server_job->when > current_time)
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

  gearmand_initialize_thread_logging("[ proc ]");

  while (1)
  {
    (void) pthread_mutex_lock(&(server->proc_lock));
    while (server->proc_wakeup == false)
    {
      if (server->proc_shutdown)
      {
        (void) pthread_mutex_unlock(&(server->proc_lock));
        return NULL;
      }

      (void) pthread_cond_wait(&(server->proc_cond), &(server->proc_lock));
    }
    server->proc_wakeup= false;
    (void) pthread_mutex_unlock(&(server->proc_lock));

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
