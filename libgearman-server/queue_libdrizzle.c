/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libdrizzle Queue Storage Definitions
 */

#include "common.h"

#include <libgearman-server/queue_libdrizzle.h>
#include <libdrizzle/drizzle_client.h>

/**
 * @addtogroup gearman_queue_libdrizzle_static Static libdrizzle Queue Storage Definitions
 * @ingroup gearman_queue_libdrizzle
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE "test"
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE "queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

/**
 * Structure for libdrizzle specific data.
 */
typedef struct
{
  drizzle_st drizzle;
  drizzle_con_st con;
  drizzle_result_st result;
  char table[DRIZZLE_MAX_TABLE_SIZE];
  char *query;
  size_t query_size;
} gearman_queue_libdrizzle_st;

/**
 * Query error handling function.
 */
static drizzle_return_t _libdrizzle_query(gearman_server_st *server,
                                          gearman_queue_libdrizzle_st *queue,
                                          const char *query, size_t query_size);

/* Queue callback functions. */
static gearman_return_t _libdrizzle_add(gearman_server_st *server,
                                        void *context, const void *unique,
                                        size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearman_job_priority_t priority);
static gearman_return_t _libdrizzle_flush(gearman_server_st *gearman,
                                          void *context);
static gearman_return_t _libdrizzle_done(gearman_server_st *gearman,
                                          void *context, const void *unique,
                                         size_t unique_size,
                                         const void *function_name,
                                         size_t function_name_size);
static gearman_return_t _libdrizzle_replay(gearman_server_st *gearman,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context);

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearman_server_queue_libdrizzle_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "libdrizzle");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

#define MCO(__name, __value, __help) \
  gearman_conf_module_add_option(module, __name, 0, __value, __help);

  MCO("host", "HOST", "Host of server.")
  MCO("port", "PORT", "Port of server.")
  MCO("uds", "UDS", "Unix domain socket for server.")
  MCO("user", "USER", "User name for authentication.")
  MCO("password", "PASSWORD", "Password for authentication.")
  MCO("db", "DB", "Database to use.")
  MCO("table", "TABLE", "Table to use.")
  MCO("mysql", NULL, "Use MySQL protocol.")

  return gearman_conf_return(conf);
}

gearman_return_t gearman_server_queue_libdrizzle_init(gearman_server_st *server,
                                                      gearman_conf_st *conf)
{
  gearman_queue_libdrizzle_st *queue;
  gearman_conf_module_st *module;
  const char *name;
  const char *value;
  const char *host= NULL;
  in_port_t port= 0;
  const char *uds= NULL;
  const char *user= NULL;
  const char *password= NULL;
  drizzle_row_t row;
  char create[1024];

  gearman_log_info(server->gearman, "Initializing libdrizzle module");

  queue= (gearman_queue_libdrizzle_st *)malloc(sizeof(gearman_queue_libdrizzle_st));
  if (queue == NULL)
  {
    gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(queue, 0, sizeof(gearman_queue_libdrizzle_st));
  snprintf(queue->table, DRIZZLE_MAX_TABLE_SIZE,
           GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE);

  if (drizzle_create(&(queue->drizzle)) == NULL)
  {
    free(queue);
    gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "drizzle_create");
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_con_create(&(queue->drizzle), &(queue->con)) == NULL)
  {
    drizzle_free(&(queue->drizzle));
    free(queue);
    gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "drizzle_con_create");
    return GEARMAN_QUEUE_ERROR;
  }

  gearman_server_set_queue_context(server, queue);

  drizzle_con_set_db(&(queue->con), GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE);

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libdrizzle");
  if (module == NULL)
  {
    gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "gearman_conf_module_find:NULL");
    return GEARMAN_QUEUE_ERROR;
  }

  while (gearman_conf_module_value(module, &name, &value))
  {
    if (!strcmp(name, "host"))
      host= value;
    else if (!strcmp(name, "port"))
      port= (in_port_t)atoi(value);
    else if (!strcmp(name, "uds"))
      uds= value;
    else if (!strcmp(name, "user"))
      user= value;
    else if (!strcmp(name, "password"))
      password= value;
    else if (!strcmp(name, "db"))
      drizzle_con_set_db(&(queue->con), value);
    else if (!strcmp(name, "table"))
      snprintf(queue->table, DRIZZLE_MAX_TABLE_SIZE, "%s", value);
    else if (!strcmp(name, "mysql"))
      drizzle_con_set_options(&(queue->con), DRIZZLE_CON_MYSQL);
    else
    {
      gearman_server_queue_libdrizzle_deinit(server);
      gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  if (uds == NULL)
    drizzle_con_set_tcp(&(queue->con), host, port);
  else
    drizzle_con_set_uds(&(queue->con), uds);

  drizzle_con_set_auth(&(queue->con), user, password);

  /* Overwrite password string so it does not appear in 'ps' output. */
  if (password != NULL)
    memset((void *)password, 'x', strlen(password));

  if (_libdrizzle_query(server, queue, "SHOW TABLES", 11) != DRIZZLE_RETURN_OK)
  {
    gearman_server_queue_libdrizzle_deinit(server);
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_result_buffer(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    gearman_server_queue_libdrizzle_deinit(server);
    gearman_log_error(server->gearman, "gearman_queue_libdrizzle_init", "drizzle_result_buffer:%s", drizzle_error(&(queue->drizzle)));
    return GEARMAN_QUEUE_ERROR;
  }

  while ((row= drizzle_row_next(&(queue->result))) != NULL)
  {
    if (!strcasecmp(queue->table, row[0]))
    {
      gearman_log_info(server->gearman, "libdrizzle module using table '%s.%s'", drizzle_con_db(&queue->con), row[0]);

      break;
    }
  }

  drizzle_result_free(&(queue->result));

  if (row == NULL)
  {
    snprintf(create, 1024,
             "CREATE TABLE %s"
             "("
               "unique_key VARCHAR(%d) PRIMARY KEY,"
               "function_name VARCHAR(255),"
               "priority INT,"
               "data LONGBLOB"
             ")",
             queue->table, GEARMAN_UNIQUE_SIZE);

    gearman_log_info(server->gearman, "libdrizzle module creating table '%s.%s'",
                     drizzle_con_db(&queue->con), queue->table);

    if (_libdrizzle_query(server, queue, create, strlen(create))
        != DRIZZLE_RETURN_OK)
    {
      gearman_server_queue_libdrizzle_deinit(server);
      return GEARMAN_QUEUE_ERROR;
    }

    drizzle_result_free(&(queue->result));
  }

  gearman_server_set_queue_add_fn(server, _libdrizzle_add);
  gearman_server_set_queue_flush_fn(server, _libdrizzle_flush);
  gearman_server_set_queue_done_fn(server, _libdrizzle_done);
  gearman_server_set_queue_replay_fn(server, _libdrizzle_replay);

  return GEARMAN_SUCCESS;
}

gearman_return_t
gearman_server_queue_libdrizzle_deinit(gearman_server_st *server)
{
  gearman_queue_libdrizzle_st *queue;

  gearman_log_info(server->gearman, "Shutting down libdrizzle queue module");

  queue= (gearman_queue_libdrizzle_st *)gearman_server_queue_context(server);
  gearman_server_set_queue_context(server, NULL);
  drizzle_con_free(&(queue->con));
  drizzle_free(&(queue->drizzle));
  if (queue->query != NULL)
    free(queue->query);
  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libdrizzle_init(gearmand_st *gearmand,
                                                gearman_conf_st *conf)
{
  return gearman_server_queue_libdrizzle_init(&(gearmand->server), conf);
}

gearman_return_t gearmand_queue_libdrizzle_deinit(gearmand_st *gearmand)
{
  return gearman_server_queue_libdrizzle_deinit(&(gearmand->server));
}

/*
 * Static definitions
 */

static drizzle_return_t _libdrizzle_query(gearman_server_st *server,
                                          gearman_queue_libdrizzle_st *queue,
                                          const char *query, size_t query_size)
{
  drizzle_return_t ret;

  gearman_log_crazy(server->gearman, "libdrizzle query: %.*s", (uint32_t)query_size, query);

  (void)drizzle_query(&(queue->con), &(queue->result), query, query_size, &ret);
  if (ret != DRIZZLE_RETURN_OK)
  {
    /* If we lost the connection, try one more time before exiting. */
    if (ret == DRIZZLE_RETURN_LOST_CONNECTION)
    {
      (void)drizzle_query(&(queue->con), &(queue->result), query, query_size,
                          &ret);
    }

    if (ret != DRIZZLE_RETURN_OK)
    {
      gearman_log_error(server->gearman, "_libdrizzle_query", "drizzle_query:%s",
                               drizzle_error(&(queue->drizzle)));
      return ret;
    }
  }

  return DRIZZLE_RETURN_OK;
}

static gearman_return_t _libdrizzle_add(gearman_server_st *server,
                                        void *context, const void *unique,
                                        size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearman_job_priority_t priority)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char *query;
  size_t query_size;

  gearman_log_debug(server->gearman, "libdrizzle add: %.*s", (uint32_t)unique_size, (char *)unique);

  /* This is not used currently, it will be once batch writes are supported
     inside of the Gearman job server. */
#if 0
  if (!not started)
  {
    if (_query(drizzle, "BEGIN", 5) != DRIZZLE_RETURN_OK)
      return REPQ_RETURN_EXTERNAL;

    drizzle_result_free(&(drizzle->result));
  }
#endif

  query_size= ((unique_size + function_name_size + data_size) * 2) +
              GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= (char *)realloc(queue->query, query_size);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libdrizzle_add", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
  {
    query= queue->query;
  }

  query_size= (size_t)snprintf(query, query_size,
                               "INSERT INTO %s SET priority=%u,unique_key='",
                               queue->table, (uint32_t)priority);

  query_size+= (size_t)drizzle_escape_string(query + query_size, unique,
                                             unique_size);
  memcpy(query + query_size, "',function_name='", 17);
  query_size+= 17;

  query_size+= (size_t)drizzle_escape_string(query + query_size, function_name,
                                             function_name_size);
  memcpy(query + query_size, "',data='", 8);
  query_size+= 8;

  query_size+= (size_t)drizzle_escape_string(query + query_size, data,
                                             data_size);
  memcpy(query + query_size, "'", 1);
  query_size+= 1;

  if (_libdrizzle_query(server, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_flush(gearman_server_st *server,
                                          void *context __attribute__((unused)))
{
  gearman_log_debug(server->gearman, "libdrizzle flush");

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_done(gearman_server_st *server,
                                         void *context, const void *unique,
                                         size_t unique_size,
                                         const void *function_name __attribute__((unused)),
                                         size_t function_name_size __attribute__((unused)))
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char *query;
  size_t query_size;

  gearman_log_debug(server->gearman, "libdrizzle done: %.*s", (uint32_t)unique_size, (char *)unique);

  query_size= (unique_size * 2) + GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= (char *)realloc(queue->query, query_size);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libdrizzle_add", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  query_size= (size_t)snprintf(query, query_size,
                               "DELETE FROM %s WHERE unique_key='",
                               queue->table);

  query_size+= (size_t)drizzle_escape_string(query + query_size, unique,
                                             unique_size);
  memcpy(query + query_size, "'", 1);
  query_size+= 1;

  if (_libdrizzle_query(server, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_replay(gearman_server_st *server,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char *query;
  size_t query_size;
  drizzle_return_t ret;
  drizzle_row_t row;
  size_t *field_sizes;
  gearman_return_t gret;

  gearman_log_info(server->gearman, "libdrizzle replay start");

  if (GEARMAN_QUEUE_QUERY_BUFFER > queue->query_size)
  {
    query= (char *)realloc(queue->query, GEARMAN_QUEUE_QUERY_BUFFER);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libdrizzle_add", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= GEARMAN_QUEUE_QUERY_BUFFER;
  }
  else
    query= queue->query;

  query_size= (size_t)snprintf(query, GEARMAN_QUEUE_QUERY_BUFFER,
                               "SELECT unique_key,function_name,priority,data "
                               "FROM %s",
                               queue->table);

  if (_libdrizzle_query(server, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  if (drizzle_column_skip(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    gearman_log_error(server->gearman, "_libdrizzle_replay", "drizzle_column_skip:%s", drizzle_error(&(queue->drizzle)));

    return GEARMAN_QUEUE_ERROR;
  }

  while (1)
  {
    row= drizzle_row_buffer(&(queue->result), &ret);
    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(&(queue->result));
      gearman_log_error(server->gearman, "_libdrizzle_replay", "drizzle_row_buffer:%s", drizzle_error(&(queue->drizzle)));

      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    field_sizes= drizzle_row_field_sizes(&(queue->result));

    gearman_log_debug(server->gearman, "libdrizzle replay: %.*s", (uint32_t)field_sizes[0], row[1]);

    gret= (*add_fn)(server, add_context, row[0], field_sizes[0], row[1],
                    field_sizes[1], row[3], field_sizes[3], atoi(row[2]));
    if (gret != GEARMAN_SUCCESS)
    {
      drizzle_row_free(&(queue->result), row);
      drizzle_result_free(&(queue->result));
      return gret;
    }

    row[3]= NULL;
    drizzle_row_free(&(queue->result), row);
  }

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}
