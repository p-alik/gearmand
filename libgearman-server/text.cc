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

#include "gear_config.h"

#include "libgearman-server/common.h"
#include "libgearman-server/log.h"

#include <cassert>
#include <cerrno>
#include <cstring>

#define TEXT_SUCCESS "OK\r\n"
#define TEXT_ERROR_ARGS "ERR INVALID_ARGUMENTS An+incomplete+set+of+arguments+was+sent+to+this+command+%.*s\r\n"
#define TEXT_ERROR_CREATE_FUNCTION "ERR CREATE_FUNCTION %.*s\r\n"
#define TEXT_ERROR_UNKNOWN_COMMAND "ERR UNKNOWN_COMMAND Unknown+server+command%.*s\r\n"
#define TEXT_ERROR_INTERNAL_ERROR "ERR UNKNOWN_ERROR\r\n"

gearmand_error_t server_run_text(gearman_server_con_st *server_con,
                                 gearmand_packet_st *packet)
{
  size_t total;

  char *data= (char *)malloc(GEARMAN_TEXT_RESPONSE_SIZE);
  if (data == NULL)
  {
    return gearmand_perror(errno, "malloc");
  }
  total= GEARMAN_TEXT_RESPONSE_SIZE;
  data[0]= 0;

  if (packet->argc)
  {
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "text command %.*s", packet->arg_size[0],  packet->arg[0]);
  }

  if (packet->argc == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_UNKNOWN_COMMAND, 4, "NULL");
  }
  else if (strcasecmp("workers", (char *)(packet->arg[0])) == 0)
  {
    size_t size= 0;

    for (gearman_server_thread_st *thread= Server->thread_list;
         thread != NULL;
         thread= thread->next)
    {
      int error;
      if ((error= pthread_mutex_lock(&thread->lock)) == 0)
      {
        for (gearman_server_con_st *con= thread->con_list; con != NULL; con= con->next)
        {
          if (con->_host == NULL)
          {
            continue;
          }

          if (size > total)
          {
            size= total;
          }

          /* Make sure we have at least GEARMAN_TEXT_RESPONSE_SIZE bytes. */
          if (size + GEARMAN_TEXT_RESPONSE_SIZE > total)
          {
            char *new_data= (char *)realloc(data, total + GEARMAN_TEXT_RESPONSE_SIZE);
            if (new_data == NULL)
            {
              if ((error= pthread_mutex_unlock(&(thread->lock))))
              {
                gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
              }
              free(data);
              return gearmand_perror(ENOMEM, "realloc");
            }

            data= new_data;
            total+= GEARMAN_TEXT_RESPONSE_SIZE;
          }

          int sn_checked_length= snprintf(data + size, total - size, "%d %s %s :",
                                          con->con.fd, con->_host, con->id);

          if ((size_t)sn_checked_length > total - size || sn_checked_length < 0)
          {
            if ((error= pthread_mutex_unlock(&(thread->lock))))
            {
              gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
            }

            free(data);
            return gearmand_perror(ENOMEM, "snprintf");
          }

          size+= (size_t)sn_checked_length;
          if (size > total)
          {
            continue;
          }

          for (gearman_server_worker_st *worker= con->worker_list; worker != NULL; worker= worker->con_next)
          {
            int checked_length= snprintf(data + size, total - size, " %.*s",
                                     (int)(worker->function->function_name_size),
                                     worker->function->function_name);

            if ((size_t)checked_length > total - size || checked_length < 0)
            {
              if ((error= (pthread_mutex_unlock(&(thread->lock)))))
              {
                gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
              }

              free(data);
              return gearmand_perror(ENOMEM, "snprintf");
            }

            size+= (size_t)checked_length;
            if (size > total)
              break;
          }

          if (size > total)
          {
            continue;
          }

          int checked_length= snprintf(data + size, total - size, "\n");
          if ((size_t)checked_length > total - size || checked_length < 0)
          {
            if ((error= (pthread_mutex_unlock(&(thread->lock)))))
            {
              gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
            }

            free(data);
            return gearmand_perror(ENOMEM, "snprintf");
          }
          size+= (size_t)checked_length;
        }

        if ((error= (pthread_mutex_unlock(&(thread->lock)))) != 0)
        {
          gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
        }
      }
      else
      {
        gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_lock");
      }
    }

    if (size < total)
    {
      int checked_length= snprintf(data + size, total - size, ".\n");
      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        return gearmand_perror(ENOMEM, "snprintf");
      }
    }
  }
  else if (strcasecmp("status", (char *)(packet->arg[0])) == 0)
  {
    size_t size= 0;

    gearman_server_function_st *function;
    for (function= Server->function_list; function != NULL;
         function= function->next)
    {
      if (size + GEARMAN_TEXT_RESPONSE_SIZE > total)
      {
        char *new_data= (char *)realloc(data, total + GEARMAN_TEXT_RESPONSE_SIZE);
        if (new_data == NULL)
        {
          free(data);
          return gearmand_perror(errno, "realloc");
        }

        data= new_data;
        total+= GEARMAN_TEXT_RESPONSE_SIZE;
      }

      int checked_length= snprintf(data + size, total - size, "%.*s\t%u\t%u\t%u\n",
                                   (int)(function->function_name_size),
                                   function->function_name, function->job_total,
                                   function->job_running, function->worker_count);

      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        free(data);
        return gearmand_perror(ENOMEM, "snprintf");
      }

      size+= (size_t)checked_length;
      if (size > total)
      {
        size= total;
      }
    }

    if (size < total)
    {
      int checked_length= snprintf(data + size, total - size, ".\n");
      if ((size_t)checked_length > total - size || checked_length < 0)
      {
        free(data);
        return gearmand_perror(ENOMEM, "snprintf");
      }
    }
  }
  else if (strcasecmp("create", (char *)(packet->arg[0])) == 0)
  {
    if (packet->argc == 3 && !strcasecmp("function", (char *)(packet->arg[1])))
    {
      gearman_server_function_st *function;
      function= gearman_server_function_get(Server, (char *)(packet->arg[2]), packet->arg_size[2] -2);

      if (function)
      {
        snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_SUCCESS);
      }
      else
      {
        snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_CREATE_FUNCTION,
                 (int)packet->arg_size[2], (char *)(packet->arg[2]));
      }
    }
    else
    {
      // create
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_ARGS, (int)packet->arg_size[0], (char *)(packet->arg[0]));
    }
  }
  else if (strcasecmp("drop", (char *)(packet->arg[0])) == 0)
  {
    if (packet->argc == 3 && !strcasecmp("function", (char *)(packet->arg[1])))
    {
      bool success= false;
      for (gearman_server_function_st *function= Server->function_list; function != NULL; function= function->next)
      {
        if (strcasecmp(function->function_name, (char *)(packet->arg[2])) == 0)
        {
          success= true;
          if (function->worker_count == 0 && function->job_running == 0)
          {
            gearman_server_function_free(Server, function);
            snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_SUCCESS);
          }
          else
          {
            snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "ERR there are still connected workers or executing clients\r\n");
          }
          break;
        }
      }

      if (success == false)
      {
        snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "ERR function not found\r\n");
        gearmand_debug(data);
      }
    }
    else
    {
      // drop
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_ARGS, (int)packet->arg_size[0], (char *)(packet->arg[0]));
    }
  }
  else if (strcasecmp("maxqueue", (char *)(packet->arg[0])) == 0)
  {
    if (packet->argc == 1)
    {
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_ARGS, (int)packet->arg_size[0], (char *)(packet->arg[0]));
    }
    else
    {
      uint32_t max_queue_size[GEARMAN_JOB_PRIORITY_MAX];

      for (int priority= 0; priority < GEARMAN_JOB_PRIORITY_MAX; ++priority)
      {
        const int argc= priority +2;
        if (packet->argc > argc)
        {
          const int parameter= atoi((char *)(packet->arg[argc]));
          if (parameter < 0)
          {
            max_queue_size[priority]= 0;
          }
          else
          {
            max_queue_size[priority]= (uint32_t)parameter;
          }
        }
        else
        {
          max_queue_size[priority]= GEARMAN_DEFAULT_MAX_QUEUE_SIZE;
        }
      }

      /* 
        To preserve the existing behavior of maxqueue, ensure that the
         one-parameter invocation is applied to all priorities.
      */
      if (packet->argc <= 3)
      {
        for (int priority= 1; priority < GEARMAN_JOB_PRIORITY_MAX; ++priority)
        {
          max_queue_size[priority]= max_queue_size[0];
        }
      }
       
      for (gearman_server_function_st *function= Server->function_list; function != NULL; function= function->next)
      {
        if (strlen((char *)(packet->arg[1])) == function->function_name_size &&
            (memcmp(packet->arg[1], function->function_name, function->function_name_size) == 0))
        {
          gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Applying queue limits to %s", function->function_name);
          memcpy(function->max_queue_size, max_queue_size, sizeof(uint32_t) * GEARMAN_JOB_PRIORITY_MAX);
        }
      }

      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_SUCCESS);
    }
  }
  else if (strcasecmp("getpid", (char *)(packet->arg[0])) == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK %d\n", (int)getpid());
  }
  else if (strcasecmp("shutdown", (char *)(packet->arg[0])) == 0)
  {
    if (packet->argc == 1)
    {
      Server->shutdown= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_SUCCESS);
    }
    else if (packet->argc == 2 &&
             strcasecmp("graceful", (char *)(packet->arg[1])) == 0)
    {
      Server->shutdown_graceful= true;
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_SUCCESS);
    }
    else
    {
      // shutdown
      snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_ARGS, (int)packet->arg_size[0], (char *)(packet->arg[0]));
    }
  }
  else if (strcasecmp("verbose", (char *)(packet->arg[0])) == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK %s\n", gearmand_verbose_name(Gearmand()->verbose));
  }
  else if (strcasecmp("version", (char *)(packet->arg[0])) == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, "OK %s\n", PACKAGE_VERSION);
  }
  else
  {
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Failed to find command %.*s(%" PRIu64 ")",
                       packet->arg_size[0], packet->arg[0], 
                       packet->arg_size[0]);
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_UNKNOWN_COMMAND, (int)packet->arg_size[0], (char *)(packet->arg[0]));
  }

  gearman_server_packet_st *server_packet= gearman_server_packet_create(server_con->thread, false);
  if (server_packet == NULL)
  {
    gearmand_debug("free");
    free(data);
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearmand_packet_init(&(server_packet->packet), GEARMAN_MAGIC_TEXT, GEARMAN_COMMAND_TEXT);

  server_packet->packet.magic= GEARMAN_MAGIC_TEXT;
  server_packet->packet.command= GEARMAN_COMMAND_TEXT;
  server_packet->packet.options.complete= true;
  server_packet->packet.options.free_data= true;

  if (data[0] == 0)
  {
    snprintf(data, GEARMAN_TEXT_RESPONSE_SIZE, TEXT_ERROR_INTERNAL_ERROR);
  }
  server_packet->packet.data= data;
  server_packet->packet.data_size= strlen(data);

  int error;
  if ((error= pthread_mutex_lock(&server_con->thread->lock)) == 0)
  {
    GEARMAN_FIFO__ADD(server_con->io_packet, server_packet);
    if ((error= pthread_mutex_unlock(&(server_con->thread->lock))))
    {
      gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_unlock");
    }
  }
  else
  {
    gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, error, "pthread_mutex_lock");
  }

  gearman_server_con_io_add(server_con);

  return GEARMAN_SUCCESS;
}
