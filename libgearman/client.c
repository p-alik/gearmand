/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Client Definitions
 */

#include "common.h"

/**
 * @addtogroup gearman_client_static Static Client Declarations
 * @ingroup gearman_client
 * @{
 */

/**
 * Allocate a client structure.
 */
static gearman_client_st *_client_allocate(gearman_client_st *client, bool is_clone);

/**
 * Callback function used when parsing server lists.
 */
static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context);

/**
 * Add a task.
 */
static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         void *context,
                                         gearman_command_t command,
                                         const char *function_name,
                                         size_t function_name_length,
                                         const char *unique,
                                         size_t unique_name_length,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr);

/**
 * Task state machine.
 */
static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task);

/**
 * Real do function.
 */
static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name, size_t functiona_name_length,
                        const char *unique, size_t unique_length,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr);

/**
 * Real background do function.
 */
static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              const char *function_name,
                                              size_t functiona_name_length,
                                              const char *unique,
                                              size_t unique_length,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle);

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
 * Public Definitions
 */

gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  return _client_allocate(client, false);
}

gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        const gearman_client_st *from)
{
  gearman_universal_st *check;

  if (! from)
  {
    return _client_allocate(client, false);
  }

  client= _client_allocate(client, true);

  if (client == NULL)
  {
    return client;
  }

  client->options.non_blocking= from->options.non_blocking;
  client->options.task_in_use= from->options.task_in_use;
  client->options.unbuffered_result= from->options.unbuffered_result;
  client->options.no_new= from->options.no_new;
  client->options.free_tasks= from->options.free_tasks;

  check= gearman_universal_clone((&client->universal), &(from->universal));
  if (! check)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  if (client->options.task_in_use)
    gearman_task_free(&(client->do_task));

  gearman_client_task_free_all(client);

  gearman_universal_free(&client->universal);

  if (client->options.allocated)
    free(client);
}

const char *gearman_client_error(const gearman_client_st *client)
{
  return gearman_universal_error(&client->universal);
}

int gearman_client_errno(const gearman_client_st *client)
{
  return gearman_universal_errno(&client->universal);
}

gearman_client_options_t gearman_client_options(const gearman_client_st *client)
{
  gearman_client_options_t options;
  memset(&options, 0, sizeof(gearman_client_options_t));

  if (client->options.allocated)
    options|= GEARMAN_CLIENT_ALLOCATED;
  if (client->options.non_blocking)
    options|= GEARMAN_CLIENT_NON_BLOCKING;
  if (client->options.task_in_use)
    options|= GEARMAN_CLIENT_TASK_IN_USE;
  if (client->options.unbuffered_result)
    options|= GEARMAN_CLIENT_UNBUFFERED_RESULT;
  if (client->options.no_new)
    options|= GEARMAN_CLIENT_NO_NEW;
  if (client->options.free_tasks)
    options|= GEARMAN_CLIENT_FREE_TASKS;

  return options;
}

void gearman_client_set_options(gearman_client_st *client,
                                gearman_client_options_t options)
{
  gearman_client_options_t usable_options[]= {
    GEARMAN_CLIENT_NON_BLOCKING,
    GEARMAN_CLIENT_UNBUFFERED_RESULT,
    GEARMAN_CLIENT_FREE_TASKS,
    GEARMAN_CLIENT_MAX
  };

  gearman_client_options_t *ptr;


  for (ptr= usable_options; *ptr != GEARMAN_CLIENT_MAX ; ptr++)
  {
    if (options & *ptr)
    {
      gearman_client_add_options(client, *ptr);
    }
    else
    {
      gearman_client_remove_options(client, *ptr);
    }
  }
}

void gearman_client_add_options(gearman_client_st *client,
                                gearman_client_options_t options)
{
  if (options & GEARMAN_CLIENT_NON_BLOCKING)
  {
    gearman_universal_add_options(&client->universal, GEARMAN_NON_BLOCKING);
    client->options.non_blocking= true;
  }

  if (options & GEARMAN_CLIENT_UNBUFFERED_RESULT)
  {
    client->options.unbuffered_result= true;
  }

  if (options & GEARMAN_CLIENT_FREE_TASKS)
  {
    client->options.free_tasks= true;
  }
}

void gearman_client_remove_options(gearman_client_st *client,
                                   gearman_client_options_t options)
{
  if (options & GEARMAN_CLIENT_NON_BLOCKING)
  {
    gearman_universal_remove_options(&client->universal, GEARMAN_NON_BLOCKING);
    client->options.non_blocking= false;
  }

  if (options & GEARMAN_CLIENT_UNBUFFERED_RESULT)
  {
    client->options.unbuffered_result= false;
  }

  if (options & GEARMAN_CLIENT_FREE_TASKS)
  {
    client->options.free_tasks= false;
  }
}

int gearman_client_timeout(gearman_client_st *client)
{
  return gearman_universal_timeout(&client->universal);
}

void gearman_client_set_timeout(gearman_client_st *client, int timeout)
{
  gearman_universal_set_timeout(&client->universal, timeout);
}

void *gearman_client_context(const gearman_client_st *client)
{
  return (void *)(client->context);
}

void gearman_client_set_context(gearman_client_st *client, void *context)
{
  client->context= context;
}

void gearman_client_set_log_fn(gearman_client_st *client,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  gearman_set_log_fn(&client->universal, function, context, verbose);
}

void gearman_client_set_workload_malloc_fn(gearman_client_st *client,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  gearman_set_workload_malloc_fn(&client->universal, function, context);
}

void gearman_client_set_workload_free_fn(gearman_client_st *client,
                                         gearman_free_fn *function,
                                         void *context)
{
  gearman_set_workload_free_fn(&client->universal, function, context);
}

gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port)
{
  if (gearman_connection_create_args(&client->universal, NULL, host, port) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_client_add_servers(gearman_client_st *client,
                                            const char *servers)
{
  return gearman_parse_servers(servers, _client_add_server, client);
}

void gearman_client_remove_servers(gearman_client_st *client)
{
  gearman_free_all_cons(&client->universal);
}

gearman_return_t gearman_client_wait(gearman_client_st *client)
{
  return gearman_wait(&client->universal);
}

void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const char *unique, const void *workload,
                        size_t workload_size, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const char *unique,
                             const void *workload, size_t workload_size,
                             size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

void *gearman_client_do_low(gearman_client_st *client,
                            const char *function_name, const char *unique,
                            const void *workload, size_t workload_size,
                            size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

const char *gearman_client_do_job_handle(const gearman_client_st *client)
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
                                              const char *unique,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
                               job_handle);
}

gearman_return_t gearman_client_do_high_background(gearman_client_st *client,
                                                   const char *function_name,
                                                   const char *unique,
                                                   const void *workload,
                                                   size_t workload_size,
                                                   char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
                               job_handle);
}

gearman_return_t gearman_client_do_low_background(gearman_client_st *client,
                                                  const char *function_name,
                                                  const char *unique,
                                                  const void *workload,
                                                  size_t workload_size,
                                                  char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
                               job_handle);
}

gearman_return_t gearman_client_job_status(gearman_client_st *client,
                                           const char *job_handle,
                                           bool *is_known, bool *is_running,
                                           uint32_t *numerator,
                                           uint32_t *denominator)
{
  gearman_return_t ret;

  if (! (client->options.task_in_use))
  {
    (void)gearman_client_add_task_status(client, &(client->do_task), client,
                                         job_handle, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    client->options.task_in_use= true;
  }

  gearman_client_clear_fn(client);

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (is_known != NULL)
      *is_known= client->do_task.options.is_known;
    if (is_running != NULL)
      *is_running= client->do_task.options.is_running;
    if (numerator != NULL)
      *numerator= client->do_task.numerator;
    if (denominator != NULL)
      *denominator= client->do_task.denominator;

    gearman_task_free(&(client->do_task));
    client->options.task_in_use= false;
  }

  return ret;
}

gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size)
{
  return gearman_echo(&client->universal, workload, workload_size);
}

void gearman_client_task_free_all(gearman_client_st *client)
{
  while (client->task_list != NULL)
    gearman_task_free(client->task_list);
}

void gearman_client_set_task_context_free_fn(gearman_client_st *client,
                                        gearman_task_context_free_fn *function)
{
  client->task_context_free_fn= function;
}

gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         void *context,
                                         const char *function_name,
                                         const char *unique,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size,
                          ret_ptr);
}

gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              void *context,
                                              const char *function_name,
                                              const char *unique,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context,
                          GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size, ret_ptr);
}

gearman_task_st *gearman_client_add_task_low(gearman_client_st *client,
                                             gearman_task_st *task,
                                             void *context,
                                             const char *function_name,
                                             const char *unique,
                                             const void *workload,
                                             size_t workload_size,
                                             gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size,
                          ret_ptr);
}

gearman_task_st *gearman_client_add_task_background(gearman_client_st *client,
                                                    gearman_task_st *task,
                                                    void *context,
                                                    const char *function_name,
                                                    const char *unique,
                                                    const void *workload,
                                                    size_t workload_size,
                                                    gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size,
                          ret_ptr);
}

gearman_task_st *
gearman_client_add_task_high_background(gearman_client_st *client,
                                        gearman_task_st *task,
                                        void *context,
                                        const char *function_name,
                                        const char *unique,
                                        const void *workload,
                                        size_t workload_size,
                                        gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context,
                          GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size, ret_ptr);
}

gearman_task_st *
gearman_client_add_task_low_background(gearman_client_st *client,
                                       gearman_task_st *task,
                                       void *context,
                                       const char *function_name,
                                       const char *unique,
                                       const void *workload,
                                       size_t workload_size,
                                       gearman_return_t *ret_ptr)
{
  return _client_add_task(client, task, context,
                          GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                          function_name, strlen(function_name),
                          unique, unique ? strlen(unique) : 0,
                          workload, workload_size, ret_ptr);
}

gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                void *context,
                                                const char *job_handle,
                                                gearman_return_t *ret_ptr)
{
  const void *args[1];
  size_t args_size[1];

  task= gearman_task_create(client, task);
  if (task == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->context= context;
  snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s", job_handle);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle);
  *ret_ptr= gearman_packet_create_args(&client->universal, &(task->send),
                                       GEARMAN_MAGIC_REQUEST,
                                       GEARMAN_COMMAND_GET_STATUS,
                                       args, args_size, 1);
  if (*ret_ptr == GEARMAN_SUCCESS)
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }

  return task;
}

void gearman_client_set_workload_fn(gearman_client_st *client,
                                    gearman_workload_fn *function)
{
  client->workload_fn= function;
}

void gearman_client_set_created_fn(gearman_client_st *client,
                                   gearman_created_fn *function)
{
  client->created_fn= function;
}

void gearman_client_set_data_fn(gearman_client_st *client,
                                gearman_data_fn *function)
{
  client->data_fn= function;
}

void gearman_client_set_warning_fn(gearman_client_st *client,
                                   gearman_warning_fn *function)
{
  client->warning_fn= function;
}

void gearman_client_set_status_fn(gearman_client_st *client,
                                  gearman_universal_status_fn *function)
{
  client->status_fn= function;
}

void gearman_client_set_complete_fn(gearman_client_st *client,
                                    gearman_complete_fn *function)
{
  client->complete_fn= function;
}

void gearman_client_set_exception_fn(gearman_client_st *client,
                                     gearman_exception_fn *function)
{
  client->exception_fn= function;
}

void gearman_client_set_fail_fn(gearman_client_st *client,
                                gearman_fail_fn *function)
{
  client->fail_fn= function;
}

void gearman_client_clear_fn(gearman_client_st *client)
{
  client->workload_fn= NULL;
  client->created_fn= NULL;
  client->data_fn= NULL;
  client->warning_fn= NULL;
  client->status_fn= NULL;
  client->complete_fn= NULL;
  client->exception_fn= NULL;
  client->fail_fn= NULL;
}

static inline void _push_non_blocking(gearman_client_st *client)
{
  client->universal.options.stored_non_blocking= client->universal.options.non_blocking;
  client->universal.options.non_blocking= true;
}

static inline void _pop_non_blocking(gearman_client_st *client)
{
  client->universal.options.non_blocking= client->options.non_blocking;
  assert(client->universal.options.stored_non_blocking == client->options.non_blocking);
}

static inline gearman_return_t _client_run_tasks(gearman_client_st *client)
{
  gearman_return_t ret;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    while (1)
    {
      /* Start any new tasks. */
      if (client->new_tasks > 0 && ! (client->options.no_new))
      {
        for (client->task= client->task_list; client->task != NULL;
             client->task= client->task->next)
        {
          if (client->task->state != GEARMAN_TASK_STATE_NEW)
            continue;

  case GEARMAN_CLIENT_STATE_NEW:
          ret= _client_run_task(client, client->task);
          if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
          {
            client->state= GEARMAN_CLIENT_STATE_NEW;

            return ret;
          }
        }

        if (client->new_tasks == 0)
        {
          ret= gearman_flush_all(&client->universal);
          if (ret != GEARMAN_SUCCESS)
          {
            return ret;
          }
        }
      }

      /* See if there are any connections ready for I/O. */
      while ((client->con= gearman_ready(&client->universal)) != NULL)
      {
        if (client->con->revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL))
        {
          /* Socket is ready for writing, continue submitting jobs. */
          for (client->task= client->task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->task->con != client->con ||
                (client->task->state != GEARMAN_TASK_STATE_SUBMIT &&
                 client->task->state != GEARMAN_TASK_STATE_WORKLOAD))
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            ret= _client_run_task(client, client->task);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              return ret;
            }
          }
        }

        if (! (client->con->revents & POLLIN))
          continue;

        /* Socket is ready for reading. */
        while (1)
        {
          /* Read packet on connection and find which task it belongs to. */
          if (client->options.unbuffered_result)
          {
            /* If client is handling the data read, make sure it's complete. */
            if (client->con->recv_state == GEARMAN_CON_RECV_STATE_READ_DATA)
            {
              for (client->task= client->task_list; client->task != NULL;
                   client->task= client->task->next)
              {
                if (client->task->con == client->con &&
                    (client->task->state == GEARMAN_TASK_STATE_DATA ||
                     client->task->state == GEARMAN_TASK_STATE_COMPLETE))
                {
                  break;
                }
              }

              assert(client->task != NULL);
            }
            else
            {
              /* Read the next packet, without buffering the data part. */
              client->task= NULL;
              (void)gearman_connection_recv(client->con, &(client->con->packet), &ret,
                                     false);
            }
          }
          else
          {
            /* Read the next packet, buffering the data part. */
            client->task= NULL;
            (void)gearman_connection_recv(client->con, &(client->con->packet), &ret, true);
          }

          if (client->task == NULL)
          {
            /* Check the return of the gearman_connection_recv() calls above. */
            if (ret != GEARMAN_SUCCESS)
            {
              if (ret == GEARMAN_IO_WAIT)
                break;

              client->state= GEARMAN_CLIENT_STATE_IDLE;
              return ret;
            }

            client->con->options.packet_in_use= true;

            /* We have a packet, see which task it belongs to. */
            for (client->task= client->task_list; client->task != NULL;
                 client->task= client->task->next)
            {
              if (client->task->con != client->con)
                continue;

              if (client->con->packet.command == GEARMAN_COMMAND_JOB_CREATED)
              {
                if (client->task->created_id != client->con->created_id)
                  continue;

                /* New job created, drop through below and notify task. */
                client->con->created_id++;
              }
              else if (client->con->packet.command == GEARMAN_COMMAND_ERROR)
              {
		gearman_universal_set_error(&client->universal, "gearman_client_run_tasks",
				  "%s:%.*s",
				  (char *)(client->con->packet.arg[0]),
				  (int)(client->con->packet.arg_size[1]),
				  (char *)(client->con->packet.arg[1]));

                return GEARMAN_SERVER_ERROR;
              }
              else if (strncmp(client->task->job_handle,
                               (char *)(client->con->packet.arg[0]),
                               client->con->packet.arg_size[0]) ||
                       (client->con->packet.command != GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->packet.arg_size[0] - 1) ||
                       (client->con->packet.command == GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->packet.arg_size[0]))
              {
                continue;
              }

              /* Else, we have a matching result packet of some kind. */

              break;
            }

            if (client->task == NULL)
            {
              /* The client has stopped waiting for the response, ignore it. */
              gearman_packet_free(&(client->con->packet));
              client->con->options.packet_in_use= false;
              continue;
            }

            client->task->recv= &(client->con->packet);
          }

  case GEARMAN_CLIENT_STATE_PACKET:
          /* Let task process job created or result packet. */
          ret= _client_run_task(client, client->task);
          if (ret == GEARMAN_IO_WAIT)
            break;
          if (ret != GEARMAN_SUCCESS)
          {
            client->state= GEARMAN_CLIENT_STATE_PACKET;
            return ret;
          }

          /* Clean up the packet. */
          gearman_packet_free(&(client->con->packet));
          client->con->options.packet_in_use= false;

          /* If all tasks are done, return. */
          if (client->running_tasks == 0)
            break;
        }
      }

      /* If all tasks are done, return. */
      if (client->running_tasks == 0)
        break;

      if (client->new_tasks > 0 && ! (client->options.no_new))
        continue;

      if (client->options.non_blocking)
      {
        /* Let the caller wait for activity. */
        client->state= GEARMAN_CLIENT_STATE_IDLE;

        return GEARMAN_IO_WAIT;
      }

      /* Wait for activity on one of the connections. */
      ret= gearman_wait(&client->universal);
      if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;

        return ret;
      }
    }

    break;

  default:
    gearman_universal_set_error(&client->universal, "gearman_client_run_tasks",
                                "unknown state: %u", client->state);

    return GEARMAN_UNKNOWN_STATE;
  }

  client->state= GEARMAN_CLIENT_STATE_IDLE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_client_run_tasks(gearman_client_st *client)
{
  gearman_return_t rc;

  _push_non_blocking(client);

  rc= _client_run_tasks(client);

  _pop_non_blocking(client);

  return rc;
}

/*
 * Static Definitions
 */

static gearman_client_st *_client_allocate(gearman_client_st *client, bool is_clone)
{
  if (client == NULL)
  {
    client= malloc(sizeof(gearman_client_st));
    if (client == NULL)
      return NULL;

    client->options.allocated= true;
  }
  else
  {
    client->options.allocated= false;
  }

  client->options.non_blocking= false;
  client->options.task_in_use= false;
  client->options.unbuffered_result= false;
  client->options.no_new= false;
  client->options.free_tasks= false;

  client->state= 0;
  client->do_ret= 0;
  client->new_tasks= 0;
  client->running_tasks= 0;
  client->task_count= 0;
  client->do_data_size= 0;
  client->context= NULL;
  client->con= NULL;
  client->task= NULL;
  client->task_list= NULL;
  client->task_context_free_fn= NULL;
  client->do_data= NULL;
  client->workload_fn= NULL;
  client->created_fn= NULL;
  client->data_fn= NULL;
  client->warning_fn= NULL;
  client->status_fn= NULL;
  client->complete_fn= NULL;
  client->exception_fn= NULL;
  client->fail_fn= NULL;

  if (! is_clone)
  {
    gearman_universal_st *check;

    check= gearman_universal_create(&client->universal, NULL);
    if (check == NULL)
    {
      gearman_client_free(client);
      return NULL;
    }
  }

  return client;
}

static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_client_add_server((gearman_client_st *)context, host, port);
}

static gearman_task_st *_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         void *context,
                                         gearman_command_t command,
                                         const char *function_name,
                                         size_t function_name_length,
                                         const char *unique,
                                         size_t unique_length,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  uuid_t uuid;
  char uuid_string[37];
  const void *args[3];
  size_t args_size[3];

  task= gearman_task_create(client, task);
  if (task == NULL)
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->context= context;

  if (unique == NULL)
  {
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_string);
    uuid_string[36]= 0;
    unique= uuid_string;
    unique_length= 36; // @note This comes from uuid/uuid.h (which does not define a number)
  }

  /**
    @todo fix it so that NULL is done by default by the API not by happenstance.
  */

  args[0]= function_name;
  args_size[0]= function_name_length + 1;
  args[1]= unique;
  args_size[1]= unique_length + 1;
  args[2]= workload;
  args_size[2]= workload_size;

  *ret_ptr= gearman_packet_create_args(&client->universal, &(task->send),
                                       GEARMAN_MAGIC_REQUEST, command,
                                       args, args_size, 3);
  if (*ret_ptr == GEARMAN_SUCCESS)
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }

  return task;
}

static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task)
{
  gearman_return_t ret;
  char status_buffer[11]; /* Max string size to hold a uint32_t. */
  uint8_t x;

  switch(task->state)
  {
  case GEARMAN_TASK_STATE_NEW:
    if (task->client->universal.con_list == NULL)
    {
      client->new_tasks--;
      client->running_tasks--;
      gearman_universal_set_error(&client->universal, "_client_run_task",
                        "no servers added");
      return GEARMAN_NO_SERVERS;
    }

    for (task->con= task->client->universal.con_list; task->con != NULL;
         task->con= task->con->next)
    {
      if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
        break;
    }

    if (task->con == NULL)
    {
      client->options.no_new= true;
      return GEARMAN_IO_WAIT;
    }

    client->new_tasks--;

    if (task->send.command != GEARMAN_COMMAND_GET_STATUS)
    {
      task->created_id= task->con->created_id_next;
      task->con->created_id_next++;
    }

  case GEARMAN_TASK_STATE_SUBMIT:
    while (1)
    {
      ret= gearman_connection_send(task->con, &(task->send),
                            client->new_tasks == 0 ? true : false);
      if (ret == GEARMAN_SUCCESS)
        break;
      else if (ret == GEARMAN_IO_WAIT)
      {
        task->state= GEARMAN_TASK_STATE_SUBMIT;
        return GEARMAN_IO_WAIT;
      }
      else if (ret != GEARMAN_SUCCESS)
      {
        /* Increment this since the job submission failed. */
        task->con->created_id++;

        if (ret == GEARMAN_COULD_NOT_CONNECT)
        {
          for (task->con= task->con->next; task->con != NULL;
               task->con= task->con->next)
          {
            if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
              break;
          }
        }
        else
          task->con= NULL;

        if (task->con == NULL)
        {
          client->running_tasks--;
          return ret;
        }

        if (task->send.command != GEARMAN_COMMAND_GET_STATUS)
        {
          task->created_id= task->con->created_id_next;
          task->con->created_id_next++;
        }
      }
    }

    if (task->send.data_size > 0 && task->send.data == NULL)
    {
      if (client->workload_fn == NULL)
      {
        gearman_universal_set_error(&client->universal, "_client_run_task",
                                    "workload size > 0, but no data pointer or workload_fn was given");
        return GEARMAN_NEED_WORKLOAD_FN;
      }

  case GEARMAN_TASK_STATE_WORKLOAD:
      ret= client->workload_fn(task);
      if (ret != GEARMAN_SUCCESS)
      {
        task->state= GEARMAN_TASK_STATE_WORKLOAD;
        return ret;
      }
    }

    client->options.no_new= false;
    task->state= GEARMAN_TASK_STATE_WORK;
    return gearman_connection_set_events(task->con, POLLIN);

  case GEARMAN_TASK_STATE_WORK:
    if (task->recv->command == GEARMAN_COMMAND_JOB_CREATED)
    {
      snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
               (uint32_t)(task->recv->arg_size[0]),
               (char *)(task->recv->arg[0]));

  case GEARMAN_TASK_STATE_CREATED:
      if (client->created_fn != NULL)
      {
        ret= client->created_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_CREATED;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG)
      {
        break;
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    {
  case GEARMAN_TASK_STATE_DATA:
      if (client->data_fn != NULL)
      {
        ret= client->data_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_DATA;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_WARNING)
    {
  case GEARMAN_TASK_STATE_WARNING:
      if (client->warning_fn != NULL)
      {
        ret= client->warning_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_WARNING;
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
          task->options.is_known= false;
        else
          task->options.is_known= true;

        if (atoi((char *)task->recv->arg[2]) == 0)
          task->options.is_running= false;
        else
          task->options.is_running= true;

        x= 3;
      }
      else
        x= 1;

      task->numerator= (uint32_t)atoi((char *)task->recv->arg[x]);
      snprintf(status_buffer, 11, "%.*s",
               (uint32_t)(task->recv->arg_size[x + 1]),
               (char *)(task->recv->arg[x + 1]));
      task->denominator= (uint32_t)atoi(status_buffer);

  case GEARMAN_TASK_STATE_STATUS:
      if (client->status_fn != NULL)
      {
        ret= client->status_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_STATUS;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_GET_STATUS)
        break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_COMPLETE)
    {
  case GEARMAN_TASK_STATE_COMPLETE:
      if (client->complete_fn != NULL)
      {
        ret= client->complete_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_COMPLETE;
          return ret;
        }
      }

      break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_EXCEPTION)
    {
  case GEARMAN_TASK_STATE_EXCEPTION:
      if (client->exception_fn != NULL)
      {
        ret= client->exception_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_EXCEPTION;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_FAIL)
    {
  case GEARMAN_TASK_STATE_FAIL:
      if (client->fail_fn != NULL)
      {
        ret= client->fail_fn(task);
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

  default:
    gearman_universal_set_error(&client->universal, "_client_run_task", "unknown state: %u",
                                task->state);
    return GEARMAN_UNKNOWN_STATE;
  }

  client->running_tasks--;
  task->state= GEARMAN_TASK_STATE_FINISHED;

  if (client->options.free_tasks)
    gearman_task_free(task);

  return GEARMAN_SUCCESS;
}

static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name, size_t function_name_length,
                        const char *unique, size_t unique_length,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr)
{
  if (! client->options.task_in_use)
  {
    (void)_client_add_task(client, &(client->do_task), client, command,
                           function_name, function_name_length,
                           unique, unique_length,
                           workload, workload_size,
                           ret_ptr);
    if (*ret_ptr != GEARMAN_SUCCESS)
      return NULL;

    client->options.task_in_use= true;
  }

  client->workload_fn= NULL;
  client->created_fn= NULL;
  client->data_fn= _client_do_data;
  client->warning_fn= _client_do_data;
  client->status_fn= _client_do_status;
  client->complete_fn= _client_do_data;
  client->exception_fn= _client_do_data;
  client->fail_fn= _client_do_fail;

  *ret_ptr= gearman_client_run_tasks(client);
  if (*ret_ptr != GEARMAN_IO_WAIT && (*ret_ptr != GEARMAN_PAUSE ||
      client->do_ret == GEARMAN_WORK_FAIL))
  {
    gearman_task_free(&(client->do_task));
    client->options.task_in_use= false;
    client->new_tasks= 0;
    client->running_tasks= 0;
  }

  workload= NULL;

  if (*ret_ptr == GEARMAN_SUCCESS || *ret_ptr == GEARMAN_PAUSE)
  {
    *ret_ptr= client->do_ret;
    workload= client->do_data;
    *result_size= client->do_data_size;
    client->do_data= NULL;
    client->do_data_size= 0;
  }

  return (void *)workload;
}

static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              const char *function_name,
                                              size_t function_name_length,
                                              const char *unique,
                                              size_t unique_length,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle)
{
  gearman_return_t ret;

  if (! client->options.task_in_use)
  {
    (void)_client_add_task(client, &(client->do_task), client, command,
                           function_name, function_name_length,
                           unique, unique_length,
                           workload, workload_size,
                           &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    client->options.task_in_use= true;
  }

  gearman_client_clear_fn(client);

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (job_handle)
    {
      strncpy(job_handle, client->do_task.job_handle, GEARMAN_JOB_HANDLE_SIZE);
    }

    gearman_task_free(&(client->do_task));
    client->options.task_in_use= false;
    client->new_tasks= 0;
    client->running_tasks= 0;
  }

  return ret;
}

static gearman_return_t _client_do_data(gearman_task_st *task)
{
  gearman_client_st *client= (gearman_client_st *)gearman_task_context(task);

  if (client->do_ret != GEARMAN_SUCCESS)
  {
    client->do_ret= GEARMAN_SUCCESS;
    return GEARMAN_SUCCESS;
  }

  client->do_data= gearman_task_take_data(task, &(client->do_data_size));

  if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    client->do_ret= GEARMAN_WORK_DATA;
  else if (task->recv->command == GEARMAN_COMMAND_WORK_WARNING)
    client->do_ret= GEARMAN_WORK_WARNING;
  else if (task->recv->command == GEARMAN_COMMAND_WORK_EXCEPTION)
    client->do_ret= GEARMAN_WORK_EXCEPTION;
  else
    return GEARMAN_SUCCESS;

  return GEARMAN_PAUSE;
}

static gearman_return_t _client_do_status(gearman_task_st *task)
{
  gearman_client_st *client= (gearman_client_st *)gearman_task_context(task);

  if (client->do_ret != GEARMAN_SUCCESS)
  {
    client->do_ret= GEARMAN_SUCCESS;
    return GEARMAN_SUCCESS;
  }

  client->do_ret= GEARMAN_WORK_STATUS;
  return GEARMAN_PAUSE;
}

static gearman_return_t _client_do_fail(gearman_task_st *task)
{
  gearman_client_st *client= (gearman_client_st *)gearman_task_context(task);

  client->do_ret= GEARMAN_WORK_FAIL;
  return GEARMAN_SUCCESS;
}
