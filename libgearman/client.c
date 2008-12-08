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

/**
 * @file
 * @brief Client definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_client_private Private Client Functions
 * @ingroup gearman_client
 * @{
 */

/**
 * Allocate a client structure.
 */
static gearman_client_st *_client_allocate(gearman_client_st *client);

/**
 * Add a task.
 */
static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *fn_arg,
                                         gearman_command_t command,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr);

/**
 * Task state machine.
 */
static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         gearman_workload_fn *workload_fn,
                                         gearman_created_fn *created_fn,
                                         gearman_data_fn *data_fn,
                                         gearman_status_fn *status_fn,
                                         gearman_complete_fn *complete_fn,
                                         gearman_fail_fn *fail_fn);

/**
 * Data and complete function for gearman_client_do* functions.
 */
static gearman_return_t _client_do_data(gearman_task_st *task);

/**
 * Status function for gearman_client_do* functions.
 */
static gearman_return_t _client_do_status(gearman_task_st *task);

/**
 * Fail function for gearman_client_do* functions.
 */
static gearman_return_t _client_do_fail(gearman_task_st *task);

/** @} */

/*
 * Public definitions
 */

gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  client= _client_allocate(client);
  if (client == NULL)
    return NULL;

  client->gearman= gearman_create(&(client->gearman_static));
  if (client->gearman == NULL)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        gearman_client_st *from)
{
  client= _client_allocate(client);
  if (client == NULL)
    return NULL;

  client->options|= (from->options & ~GEARMAN_CLIENT_ALLOCATED);

  client->gearman= gearman_clone(&(client->gearman_static), from->gearman);
  if (client->gearman == NULL)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  if (client->options & GEARMAN_CLIENT_TASK_IN_USE)
    gearman_task_free(&(client->do_task));

  gearman_free(client->gearman);

  if (client->options & GEARMAN_CLIENT_ALLOCATED)
    free(client);
}

const char *gearman_client_error(gearman_client_st *client)
{
  return gearman_error(client->gearman);
}

int gearman_client_errno(gearman_client_st *client)
{
  return gearman_errno(client->gearman);
}

void gearman_client_set_options(gearman_client_st *client,
                                gearman_client_options_t options,
                                uint32_t data)
{
  if (options & GEARMAN_CLIENT_NON_BLOCKING)
    gearman_set_options(client->gearman, GEARMAN_NON_BLOCKING, data);

  if (data)
    client->options |= options;
  else
    client->options &= ~options;
}

gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port)
{
  if (gearman_con_add(client->gearman, NULL, host, port) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  return GEARMAN_SUCCESS;
}

void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr)
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
                                     _client_do_status, _client_do_data,
                                     _client_do_fail);
  if (*ret_ptr != GEARMAN_IO_WAIT && *ret_ptr != GEARMAN_WORK_DATA &&
      *ret_ptr != GEARMAN_WORK_STATUS)
  {
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  workload= NULL;

  if (*ret_ptr == GEARMAN_WORK_DATA || *ret_ptr == GEARMAN_SUCCESS)
  {
    if (client->do_fail)
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }

    workload= client->do_data;
    *result_size= client->do_data_size;
    client->do_data= NULL;
    client->do_data_size= 0;
  }

  return (void *)workload;
}

void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const void *workload,
                             size_t workload_size, size_t *result_size,
                             gearman_return_t *ret_ptr)
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
                                     _client_do_status, _client_do_data,
                                     _client_do_fail);
  if (*ret_ptr != GEARMAN_IO_WAIT && *ret_ptr != GEARMAN_WORK_DATA &&
      *ret_ptr != GEARMAN_WORK_STATUS)
  {
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  workload= NULL;

  if (*ret_ptr == GEARMAN_WORK_DATA || *ret_ptr == GEARMAN_SUCCESS)
  {
    if (client->do_fail)
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }

    workload= client->do_data;
    *result_size= client->do_data_size;
    client->do_data= NULL;
    client->do_data_size= 0;
  }

  return (void *)workload;
}

const char *gearman_client_do_job_handle(gearman_client_st *client)
{
  return client->do_task.job_handle;
}

void gearman_client_do_status(gearman_client_st *client, uint32_t *numerator,
                              uint32_t *denominator)
{
  if (numerator != NULL)
    *numerator= client->do_task.numerator;

  if (denominator != NULL)
    *denominator= client->do_task.denominator;
}

gearman_return_t gearman_client_do_background(gearman_client_st *client,
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_job_handle_t job_handle)
{
  gearman_return_t ret;

  if (!(client->options & GEARMAN_CLIENT_TASK_IN_USE))
  {
    (void)_client_add_task(client, &(client->do_task), client,
                           GEARMAN_COMMAND_SUBMIT_JOB_BG, function_name,
                           workload, workload_size, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    client->options|= GEARMAN_CLIENT_TASK_IN_USE;
  }

  ret= gearman_client_run_tasks(client, NULL, NULL, NULL, NULL, NULL, NULL);
  if (ret != GEARMAN_IO_WAIT)
  {
    strcpy(job_handle, client->do_task.job_handle);
    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  return ret;
}

gearman_return_t gearman_client_task_status(gearman_client_st *client,
                                          const gearman_job_handle_t job_handle,
                                            bool *is_known, bool *is_running,
                                            uint32_t *numerator,
                                            uint32_t *denominator)
{
  gearman_return_t ret;

  if (!(client->options & GEARMAN_CLIENT_TASK_IN_USE))
  {
    (void)gearman_client_add_task_status(client,  &(client->do_task), client,
                                         job_handle, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    client->options|= GEARMAN_CLIENT_TASK_IN_USE;
  }

  ret= gearman_client_run_tasks(client, NULL, NULL, NULL, NULL, NULL, NULL);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (is_known != NULL)
      *is_known= client->do_task.is_known;
    if (is_running != NULL)
      *is_running= client->do_task.is_running;
    if (numerator != NULL)
      *numerator= client->do_task.numerator;
    if (denominator != NULL)
      *denominator= client->do_task.denominator;

    gearman_task_free(&(client->do_task));
    client->options&= ~GEARMAN_CLIENT_TASK_IN_USE;
  }

  return ret;
}

#ifdef NOT_DONE
/* Send data to all job servers to see if they echo it back. */
gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size)
{
}
#endif

gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *fn_arg,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, fn_arg, GEARMAN_COMMAND_SUBMIT_JOB,
                          function_name, workload, workload_size, ret_ptr);
}

gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              const void *fn_arg,  
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, fn_arg, GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                          function_name, workload, workload_size, ret_ptr);
}

gearman_task_st *gearman_client_add_task_background(gearman_client_st *client,
                                                    gearman_task_st *task,
                                                    const void *fn_arg,  
                                                    const char *function_name,
                                                    const void *workload,
                                                    size_t workload_size,
                                                    gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, fn_arg, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                          function_name, workload, workload_size, ret_ptr);
}

gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                const void *fn_arg,
                                          const gearman_job_handle_t job_handle,
                                                gearman_return_t *ret_ptr)
{
  task= gearman_task_create(client->gearman, task);
  if (task == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->fn_arg= fn_arg;
  strcpy(task->job_handle, job_handle);

  *ret_ptr= gearman_packet_add(client->gearman, &(task->send),
                               GEARMAN_MAGIC_REQUEST,
                               GEARMAN_COMMAND_GET_STATUS,
                               (uint8_t *)job_handle, strlen(job_handle), NULL);
  if (*ret_ptr == GEARMAN_SUCCESS)
  {
    client->new++;
    client->running++;
    task->options|= GEARMAN_TASK_SEND_IN_USE;
  }

  return task;
}

gearman_return_t gearman_client_run_tasks(gearman_client_st *client,
                                          gearman_workload_fn *workload_fn,
                                          gearman_created_fn *created_fn,
                                          gearman_data_fn *data_fn,
                                          gearman_status_fn *status_fn,
                                          gearman_complete_fn *complete_fn,
                                          gearman_fail_fn *fail_fn)
{
  gearman_options_t options;
  gearman_return_t ret;
  bool start_new= false;

  options= client->gearman->options;
  client->gearman->options|= GEARMAN_NON_BLOCKING;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    while (1)
    {
      /* Start any new tasks. */
      if (client->new > 0)
      {
        for (client->task= client->gearman->task_list; client->task != NULL;
             client->task= client->task->next)
        {
          if (client->task->state != GEARMAN_TASK_STATE_NEW)
            continue;

  case GEARMAN_CLIENT_STATE_NEW:
          ret= _client_run_task(client, client->task, workload_fn, created_fn,
                                data_fn, status_fn, complete_fn, fail_fn);
          if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
          {
            client->state= GEARMAN_CLIENT_STATE_NEW;
            client->gearman->options= options;
            return ret;
          }
        }
      }

      /* See if there are any connections ready for I/O. */
      while ((client->con= gearman_con_ready(client->gearman)) != NULL)
      {
        if (client->con->revents & POLLOUT)
        {
          /* Socket is ready for writing, continue submitting jobs. */
          for (client->task= client->gearman->task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->task->con != client->con ||
                client->task->state != GEARMAN_TASK_STATE_SUBMIT)
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            ret= _client_run_task(client, client->task, workload_fn, created_fn,
                                  data_fn, status_fn, complete_fn, fail_fn);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              client->gearman->options= options;
              return ret;
            }
          }

          /* A connection may now be idle to start a new job, set this. */
          start_new= true;
        }

        /* Try reading even if POLLIN is not set, we may not have asked yet. */
        while (1)
        {
          assert(client->con);

          /* Read packet on connection and find which task it belongs to. */
          (void)gearman_con_recv(client->con, &(client->con->packet), &ret,
                                 client->options &
                                 GEARMAN_CLIENT_BUFFER_RESULT ? true : false);
          if (ret != GEARMAN_SUCCESS)
          {
            if (ret == GEARMAN_IO_WAIT)
              break;

            client->state= GEARMAN_CLIENT_STATE_IDLE;
            client->gearman->options= options;
            return ret;
          }

          for (client->task= client->gearman->task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->con->packet.command == GEARMAN_COMMAND_JOB_CREATED)
            {
              if (client->task->con != client->con ||
                  client->task->created_id != client->con->created_id)
              {
                continue;
              }

              client->con->created_id++;
            }
            else if (strcmp(client->task->job_handle,
                            (char *)client->con->packet.arg[0]))
            {
              continue;
            }

            client->task->recv= &(client->con->packet);

  case GEARMAN_CLIENT_STATE_PACKET:
            ret= _client_run_task(client, client->task, workload_fn, created_fn,
                                  data_fn, status_fn, complete_fn, fail_fn);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_PACKET;
              client->gearman->options= options;
              return ret;
            }

            break;
          }

          gearman_packet_free(&(client->con->packet));

          /* If all tasks are done, return. */
          if (client->running == 0)
            break;
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
        client->gearman->options= options;
        return GEARMAN_IO_WAIT;
      }

      ret= gearman_con_wait(client->gearman, true);
      if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;
        client->gearman->options= options;
        return ret;
      }
    }
  }

  client->state= GEARMAN_CLIENT_STATE_IDLE;
  client->gearman->options= options;
  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearman_client_st *_client_allocate(gearman_client_st *client)
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

  return client;
}

static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *fn_arg,
                                         gearman_command_t command,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  uuid_t uuid;
  char uuid_string[37];

  task= gearman_task_create(client->gearman, task);
  if (task == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->fn_arg= fn_arg;

  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_string);

  *ret_ptr= gearman_packet_add(client->gearman, &(task->send),
                               GEARMAN_MAGIC_REQUEST, command,
                               (uint8_t *)function_name,
                               strlen(function_name) + 1,
                               (uint8_t *)uuid_string, (size_t)37, workload,
                               workload_size, NULL);
  if (*ret_ptr == GEARMAN_SUCCESS)
  {
    client->new++;
    client->running++;
    task->options|= GEARMAN_TASK_SEND_IN_USE;
  }

  return task;
}

static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         gearman_workload_fn *workload_fn,
                                         gearman_created_fn *created_fn,
                                         gearman_data_fn *data_fn,
                                         gearman_status_fn *status_fn,
                                         gearman_complete_fn *complete_fn,
                                         gearman_fail_fn *fail_fn)
{
  gearman_return_t ret;
  char status_buffer[11]; /* Max string size to hold a uint32_t. */
  uint8_t x;

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

    if (task->send.command != GEARMAN_COMMAND_GET_STATUS)
    {
      task->created_id= task->con->created_id_next;
      task->con->created_id_next++;
    }

  case GEARMAN_TASK_STATE_SUBMIT:
    ret= gearman_con_send(task->con, &(task->send), true);
    if (ret != GEARMAN_SUCCESS)
    {
      task->state= GEARMAN_TASK_STATE_SUBMIT;
      return ret;
    }

    if (task->send.data_size > 0 && task->send.data == NULL)
    {
  case GEARMAN_TASK_STATE_WORKLOAD:
      ret= (*workload_fn)(task);
      if (ret != GEARMAN_SUCCESS)
      {
        task->state= GEARMAN_TASK_STATE_WORKLOAD;
        return ret;
      }
    }

    task->state= GEARMAN_TASK_STATE_WORK;
    return GEARMAN_SUCCESS;

  case GEARMAN_TASK_STATE_WORK:
    if (task->recv->command == GEARMAN_COMMAND_JOB_CREATED)
    {
      strncpy(task->job_handle, (char *)task->recv->arg[0],
              GEARMAN_JOB_HANDLE_SIZE);
      task->job_handle[GEARMAN_JOB_HANDLE_SIZE - 1]= 0;

  case GEARMAN_TASK_STATE_CREATED:
      if (created_fn != NULL)
      {
        ret= (*created_fn)(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_CREATED;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_BG)
        break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    {
  case GEARMAN_TASK_STATE_DATA:
      if (data_fn != NULL)
      {
        ret= (*data_fn)(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_DATA;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_STATUS ||
             task->recv->command == GEARMAN_COMMAND_STATUS_RES)
    {
      if (task->recv->command == GEARMAN_COMMAND_STATUS_RES)
      {
        if (atoi((char *)task->recv->arg[1]) == 0)
          task->is_known= false;
        else
          task->is_known= true;

        if (atoi((char *)task->recv->arg[2]) == 0)
          task->is_running= false;
        else
          task->is_running= true;

        x= 3;
      }
      else
        x= 1;

      task->numerator= atoi((char *)task->recv->arg[x]);
      strncpy(status_buffer, (char *)task->recv->arg[x + 1], 11);
      status_buffer[10]= 0;
      task->denominator= atoi(status_buffer);

  case GEARMAN_TASK_STATE_STATUS:
      if (status_fn != NULL)
      {
        ret= (*status_fn)(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_STATUS;
          return ret;
        }
      }

      if (task->recv->command == GEARMAN_COMMAND_STATUS_RES)
        break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_COMPLETE)
    {
  case GEARMAN_TASK_STATE_COMPLETE:
      if (complete_fn != NULL)
      {
        ret= (*complete_fn)(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_COMPLETE;
          return ret;
        }
      }

      break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_FAIL)
    {
  case GEARMAN_TASK_STATE_FAIL:
      if (fail_fn != NULL)
      {
        ret= (*fail_fn)(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_FAIL;
          return ret;
        }
      }

      break;
    }

    task->state= GEARMAN_TASK_STATE_WORK;
    return GEARMAN_SUCCESS;

  case GEARMAN_TASK_STATE_FINISHED:
    break;
  }

  client->running--;
  task->state= GEARMAN_TASK_STATE_FINISHED;
  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_do_data(gearman_task_st *task)
{
  gearman_client_st *client= (gearman_client_st *)gearman_task_fn_arg(task);
  gearman_return_t ret;

  if (task->recv == NULL)
    return GEARMAN_SUCCESS;

  if (task->recv->data_size == 0)
    return GEARMAN_WORK_DATA;

  if (client->do_data == NULL)
  {
    client->do_data= malloc(task->recv->data_size);
    if (client->do_data == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    client->do_data_size= task->recv->data_size;
  }

  client->do_data_offset+= gearman_con_recv_data(task->con,
                                                 (uint8_t *)(client->do_data) +
                                                 client->do_data_offset,
                                                 client->do_data_size -
                                                 client->do_data_offset, &ret);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    ret= GEARMAN_WORK_DATA;
  else
    ret= GEARMAN_SUCCESS;

  if (client->do_data_offset == client->do_data_size)
  {
    client->do_data_offset= 0;
    task->recv= NULL;
  }

  return ret;
}

static gearman_return_t _client_do_status(gearman_task_st *task)
{
  (void)task;
  return GEARMAN_WORK_STATUS;
}

static gearman_return_t _client_do_fail(gearman_task_st *task)
{
  gearman_client_st *client= (gearman_client_st *)gearman_task_fn_arg(task);

  client->do_fail= true;
  return GEARMAN_SUCCESS;
}
