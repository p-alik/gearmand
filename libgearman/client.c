/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

/* Task state machine. */
static gearman_return _client_run_task(gearman_client_st *client,
                                       gearman_task_st *task,
                                       gearman_workload_function workload_cb,
                                       gearman_created_function created_cb,
                                       gearman_data_function data_cb,
                                       gearman_status_function status_cb,
                                       gearman_complete_function complete_cb,
                                       gearman_fail_function fail_cb);

/* Callbacks for gearman_client_do* functions. */
static gearman_return _client_do_created(gearman_task_st *task, void *cb_arg);
static gearman_return _client_do_data(gearman_task_st *task, void *cb_arg,
                                      const void *data, size_t data_size);
static gearman_return _client_do_status(gearman_task_st *task, void *cb_arg,
                                        uint32_t numerator,
                                        uint32_t denominator);
static gearman_return _client_do_complete(gearman_task_st *task, void *cb_arg,
                                          const void *result,
                                          size_t result_size);
static gearman_return _client_do_fail(gearman_task_st *task, void *cb_arg);

/* Add a task. */
static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *cb_arg,
                                         gearman_command command,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return *ret_ptr);

/* Initialize a client structure. */
gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  if (client == NULL)
  {
    client= malloc(sizeof(gearman_client_st));
    if (client == NULL)
      return NULL;

    memset(client, 0, sizeof(gearman_client_st));
    client->options|= GEARMAN_CLIENT_ALLOCATED;
  }
  else
    memset(client, 0, sizeof(gearman_client_st));

  (void)gearman_create(&(client->gearman));

  return client;
}

/* Clone a client structure using 'from' as the source. */
gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        gearman_client_st *from)
{
  client= gearman_client_create(client);
  if (client == NULL)
    return NULL;

  client->options|= (from->options & ~GEARMAN_CLIENT_ALLOCATED);

  if (gearman_clone(&(client->gearman), &(from->gearman)) == NULL)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

/* Free a client structure. */
void gearman_client_free(gearman_client_st *client)
{
  if (client->options & GEARMAN_CLIENT_TASK_IN_USE)
    gearman_task_free(&(client->do_task));

  if (client->options & GEARMAN_CLIENT_ALLOCATED)
    free(client);
}

/* Return an error string for last error encountered. */
char *gearman_client_error(gearman_client_st *client)
{
  return gearman_error(&(client->gearman));
}

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_client_errno(gearman_client_st *client)
{
  return gearman_errno(&(client->gearman));
}

/* Set options for a client structure. */
void gearman_client_set_options(gearman_client_st *client,
                                gearman_options options, uint32_t data)
{
  gearman_set_options(&(client->gearman), options, data);
}

/* Add a job server to a client. */
gearman_return gearman_client_server_add(gearman_client_st *client, char *host,
                                         in_port_t port)
{
  if (gearman_con_add(&(client->gearman), NULL, host, port) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  return GEARMAN_SUCCESS;
}

/* Run a task, returns allocated result. */
void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return *ret_ptr)
{
  if (!(client->options & GEARMAN_CLIENT_TASK_IN_USE))
  {
    (void)_client_add_task(client, &(client->do_task), client,
                           GEARMAN_COMMAND_SUBMIT_JOB, function_name, workload,
                           workload_size, ret_ptr);
    if (*ret_ptr != GEARMAN_SUCCESS)
      return NULL;

    client->options|= GEARMAN_CLIENT_TASK_IN_USE;
  }

  *ret_ptr= gearman_client_run_tasks(client, NULL, NULL, _client_do_data,
                                     _client_do_status, _client_do_complete,
                                     _client_do_fail);

  if (*ret_ptr != GEARMAN_IO_WAIT && *ret_ptr != GEARMAN_WORK_DATA &&
      *ret_ptr != GEARMAN_WORK_STATUS)
  {
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  workload= client->do_data;
  client->do_data= NULL;
  *result_size= client->do_data_size;

  return (void *)workload;
}

/* Run a high priority task, returns allocated result. */
void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const void *workload,
                             size_t workload_size, size_t *result_size,
                             gearman_return *ret_ptr)
{
  if (!(client->options & GEARMAN_CLIENT_TASK_IN_USE))
  {
    (void)_client_add_task(client, &(client->do_task), client,
                           GEARMAN_COMMAND_SUBMIT_JOB_HIGH, function_name,
                           workload, workload_size, ret_ptr);
    if (*ret_ptr != GEARMAN_SUCCESS)
      return NULL;

    client->options|= GEARMAN_CLIENT_TASK_IN_USE;
  }

  *ret_ptr= gearman_client_run_tasks(client, NULL, NULL, _client_do_data,
                                     _client_do_status, _client_do_complete,
                                     _client_do_fail);

  if (*ret_ptr != GEARMAN_IO_WAIT && *ret_ptr != GEARMAN_WORK_DATA &&
      *ret_ptr != GEARMAN_WORK_STATUS)
  {
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  workload= client->do_data;
  client->do_data= NULL;
  *result_size= client->do_data_size;

  return (void *)workload;
}

/* Run a task in the background. The job_handle_buffer must be at least
   GEARMAN_JOB_HANDLE_SIZE bytes. */
gearman_return gearman_client_do_bg(gearman_client_st *client,
                                    const char *function_name,
                                    const void *workload, size_t workload_size,
                                    char *job_handle_buffer)
{
  gearman_return ret;

  if (!(client->options & GEARMAN_CLIENT_TASK_IN_USE))
  {
    (void)_client_add_task(client, &(client->do_task), client,
                           GEARMAN_COMMAND_SUBMIT_JOB_BG, function_name,
                           workload, workload_size, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    client->options|= GEARMAN_CLIENT_TASK_IN_USE;
    client->do_data= job_handle_buffer;
  }

  ret= gearman_client_run_tasks(client, NULL, _client_do_created, NULL, NULL,
                                NULL, NULL);

  if (ret != GEARMAN_IO_WAIT)
  {
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  return ret;
}

#if 0
/* Get the status for a backgound task. */
gearman_return gearman_client_task_status(gearman_client_st *client,
                                          const char *job_handle,
                                          bool *is_known, bool *is_running,
                                          long *numerator,
                                          long *denominator)
{
}

/* Send data to all job servers to see if they echo it back. */
gearman_return gearman_client_echo(gearman_client_st *client,
                                   const void *workload, size_t workload_size)
{
}
#endif

/* Add a task to be run in parallel. */
gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *cb_arg,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return *ret_ptr)
{
  return _client_add_task(client, task, cb_arg, GEARMAN_COMMAND_SUBMIT_JOB,
                          function_name, workload, workload_size, ret_ptr);
}

/* Add a high priority task to be run in parallel. */
gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              const void *cb_arg,  
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return *ret_ptr)
{
  return _client_add_task(client, task, cb_arg, GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                          function_name, workload, workload_size, ret_ptr);
}

/* Add a background task to be run in parallel. */
gearman_task_st *gearman_client_add_task_bg(gearman_client_st *client,
                                            gearman_task_st *task,
                                            const void *cb_arg,  
                                            const char *function_name,
                                            const void *workload,
                                            size_t workload_size,
                                            gearman_return *ret_ptr)
{
  return _client_add_task(client, task, cb_arg, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                          function_name, workload, workload_size, ret_ptr);
}

/* Run tasks that have been added in parallel. */
gearman_return gearman_client_run_tasks(gearman_client_st *client,
                                        gearman_workload_function workload_cb,
                                        gearman_created_function created_cb,
                                        gearman_data_function data_cb,
                                        gearman_status_function status_cb,
                                        gearman_complete_function complete_cb,
                                        gearman_fail_function fail_cb)
{
  gearman_options options;
  gearman_con_st *con;
  gearman_return ret;
  bool start_new= false;

  options= client->gearman.options;
  client->gearman.options|= GEARMAN_NON_BLOCKING;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    while (1)
    {
      /* Start any new tasks. */
      for (client->task= client->gearman.task_list; client->task != NULL;
           client->task= client->task->next)
      {
        if (client->task->state != GEARMAN_TASK_STATE_NEW)
          continue;

  case GEARMAN_CLIENT_STATE_NEW:
        ret= _client_run_task(client, client->task, workload_cb, created_cb,
                              data_cb, status_cb, complete_cb, fail_cb);
        if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
        {
          client->state= GEARMAN_CLIENT_STATE_NEW;
          client->gearman.options= options;
          return ret;
        }
      }

      /* See if there are any connections ready for I/O. */
      while ((con= gearman_con_ready(&(client->gearman))) != NULL)
      {
        if (con->revents & POLLOUT)
        {
          /* Socket is ready for writing, continue submitting jobs. */
          for (client->task= client->gearman.task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->task->con != con ||
                client->task->state != GEARMAN_TASK_STATE_SUBMIT)
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            ret= _client_run_task(client, client->task, workload_cb, created_cb,
                                  data_cb, status_cb, complete_cb, fail_cb);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              client->gearman.options= options;
              return ret;
            }
          }

          /* A connection may now be idle to start a new job, set this. */
          start_new= true;
        }

        /* Try reading even if POLLIN is not set, we may not have asked yet. */
        while (1)
        {
          /* Read packet on connection and find which task it belongs to. */
          (void)gearman_con_recv(con, &(client->packet), &ret, true);
          if (ret != GEARMAN_SUCCESS)
          {
            if (ret == GEARMAN_IO_WAIT)
              break;

            client->state= GEARMAN_CLIENT_STATE_IDLE;
            client->gearman.options= options;
            return ret;
          }

          for (client->task= client->gearman.task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->packet.command == GEARMAN_COMMAND_JOB_CREATED)
            {
              if (client->task->con != con ||
                  client->task->created_id != con->created_id)
              {
                continue;
              }

              con->created_id++;
            }
            else if (strcmp(client->task->job_handle,
                            (char *)client->packet.arg[0]))
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_PACKET:
            ret= _client_run_task(client, client->task, workload_cb, created_cb,
                                  data_cb, status_cb, complete_cb, fail_cb);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_PACKET;
              client->gearman.options= options;
              return ret;
            }

            break;
          }

          gearman_packet_free(&(client->packet));
        }
      }

      /* If all tasks are done, return. */
      if (client->running == 0)
        break;

      if (client->new > 0 && start_new)
      {
        start_new= false;
        continue;
      }

      if (options & GEARMAN_NON_BLOCKING)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;
        client->gearman.options= options;
        return GEARMAN_IO_WAIT;
      }

      ret= gearman_con_wait(&(client->gearman), true);
      if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;
        client->gearman.options= options;
        return ret;
      }
    }
  }

  client->state= GEARMAN_CLIENT_STATE_IDLE;
  client->gearman.options= options;
  return GEARMAN_SUCCESS;
}

/* Task state machine. */
gearman_return _client_run_task(gearman_client_st *client,
                                gearman_task_st *task,
                                gearman_workload_function workload_cb,
                                gearman_created_function created_cb,
                                gearman_data_function data_cb,
                                gearman_status_function status_cb,
                                gearman_complete_function complete_cb,
                                gearman_fail_function fail_cb)
{
  gearman_return ret;
  char status_buffer[11]; /* Max string size to hold a uint32_t. */
  (void)workload_cb;

  switch(task->state)
  {
  case GEARMAN_TASK_STATE_NEW:
    if (task->gearman->con_list == NULL)
      return GEARMAN_NO_SERVERS;

    for (task->con= task->gearman->con_list; task->con != NULL;
         task->con= task->con->next)
    {
      if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
        break;
    }

    if (task->con == NULL)
      return GEARMAN_IO_WAIT;

    client->new--;
    task->created_id= task->con->created_id_next;
    task->con->created_id_next++;

  case GEARMAN_TASK_STATE_SUBMIT:
    ret= gearman_con_send(task->con, &(task->packet), true);
    if (ret != GEARMAN_SUCCESS)
    {
      task->state= GEARMAN_TASK_STATE_SUBMIT;
      return ret;
    }

    task->state= GEARMAN_TASK_STATE_WORK;
    break;

  case GEARMAN_TASK_STATE_WORK:
    if (client->packet.command == GEARMAN_COMMAND_JOB_CREATED)
    {
      strncpy(task->job_handle, (char *)client->packet.arg[0],
              GEARMAN_JOB_HANDLE_SIZE);

  case GEARMAN_TASK_STATE_CREATED:
      if (created_cb != NULL)
      {
        ret= created_cb(task, (void *)task->cb_arg);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_CREATED;
          return ret;
        }
      }

      if (task->packet.command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
      {
        client->running--;
        task->state= GEARMAN_TASK_STATE_FINISHED;
        break;
      }
    }
    else if (client->packet.command == GEARMAN_COMMAND_WORK_DATA)
    {
  case GEARMAN_TASK_STATE_DATA:
      if (data_cb != NULL)
      {
        ret= data_cb(task, (void *)task->cb_arg, client->packet.data,
                     client->packet.data_size);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_DATA;
          return ret;
        }
      }
    }
    else if (client->packet.command == GEARMAN_COMMAND_WORK_STATUS)
    {
      task->numerator= atoi((char *)client->packet.arg[1]);
      strncpy(status_buffer, (char *)client->packet.arg[2], 11);
      task->denominator= atoi(status_buffer);

  case GEARMAN_TASK_STATE_STATUS:
      if (status_cb != NULL)
      {
        ret= status_cb(task, (void *)task->cb_arg, task->numerator,
                       task->denominator);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_STATUS;
          return ret;
        }
      }
    }
    else if (client->packet.command == GEARMAN_COMMAND_WORK_COMPLETE)
    {
  case GEARMAN_TASK_STATE_COMPLETE:
      if (complete_cb != NULL)
      {
        ret= complete_cb(task, (void *)task->cb_arg, client->packet.data,
                         client->packet.data_size);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_COMPLETE;
          return ret;
        }
      }

      client->running--;
      task->state= GEARMAN_TASK_STATE_FINISHED;
      break;
    }
    else if (client->packet.command == GEARMAN_COMMAND_WORK_FAIL)
    {
  case GEARMAN_TASK_STATE_FAIL:
      if (fail_cb != NULL)
      {
        ret= fail_cb(task, (void *)task->cb_arg);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_FAIL;
          return ret;
        }
      }

      client->running--;
      task->state= GEARMAN_TASK_STATE_FINISHED;
      break;
    }

    task->state= GEARMAN_TASK_STATE_WORK;
    break;

  case GEARMAN_TASK_STATE_FINISHED:
    break;
  }

  return GEARMAN_SUCCESS;
}

/* Callbacks for gearman_client_do* functions. */
static gearman_return _client_do_created(gearman_task_st *task, void *cb_arg)
{
  gearman_client_st *client= (gearman_client_st *)cb_arg;

  strcpy((char *)(client->do_data), task->job_handle);
  return GEARMAN_SUCCESS;
}

static gearman_return _client_do_data(gearman_task_st *task, void *cb_arg,
                                      const void *data, size_t data_size)
{
  gearman_client_st *client= (gearman_client_st *)cb_arg;
  (void)task;
  (void)data;

  client->do_data_size= data_size;

  if (data_size == 0)
    client->do_data= NULL;
  else
  {
    client->do_data= client->packet.data;
    client->packet.data= NULL;
    client->data_buffer= NULL;
    client->data_buffer_size= 0;
  }

  return GEARMAN_WORK_DATA;
}

static gearman_return _client_do_status(gearman_task_st *task, void *cb_arg,
                                        uint32_t numerator,
                                        uint32_t denominator)
{
  (void)cb_arg;

  task->numerator= numerator;
  task->denominator= denominator;

  return GEARMAN_WORK_STATUS;
}

static gearman_return _client_do_complete(gearman_task_st *task, void *cb_arg,
                                          const void *result,
                                          size_t result_size)
{
  gearman_client_st *client= (gearman_client_st *)cb_arg;
  (void)task;
  (void)result;

  client->do_data_size= result_size;

  if (result_size == 0)
    client->do_data= NULL;
  else
  {
    client->do_data= client->packet.data;
    client->packet.data= NULL;
    client->data_buffer= NULL;
    client->data_buffer_size= 0;
  }

  return GEARMAN_SUCCESS;
}

static gearman_return _client_do_fail(gearman_task_st *task, void *cb_arg)
{
  (void)task;
  (void)cb_arg;

  return GEARMAN_WORK_FAIL;
}

/* Add a task. */
static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *cb_arg,
                                         gearman_command command,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return *ret_ptr)
{
  uuid_t uuid;
  char uuid_string[37];

  task= gearman_task_create(&(client->gearman), task);
  if (task == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->cb_arg= cb_arg;

  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_string);

  *ret_ptr= gearman_packet_add(&(client->gearman), &(task->packet),
                               GEARMAN_MAGIC_REQUEST, command,
                               (uint8_t *)function_name,
                               strlen(function_name) + 1,
                               (uint8_t *)uuid_string, (size_t)37, workload,
                               workload_size, NULL);

  if (*ret_ptr == GEARMAN_SUCCESS)
  {
    client->new++;
    client->running++;
  }

  return task;
}
