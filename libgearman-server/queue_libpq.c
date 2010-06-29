/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libpq Queue Storage Definitions
 */

#include "common.h"

#include <libgearman-server/queue_libpq.h>

#if defined(HAVE_LIBPQ_FE_H)
# include <libpq-fe.h>
# include <pg_config_manual.h>
#else
# include <postgresql/libpq-fe.h>
# include <postgresql/pg_config_manual.h>
#endif

/**
 * @addtogroup gearman_queue_libpq_static Static libpq Queue Storage Definitions
 * @ingroup gearman_queue_libpq
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBPQ_DEFAULT_TABLE "queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

/**
 * Structure for libpq specific data.
 */
typedef struct
{
  PGconn *con;
  char table[NAMEDATALEN];
  char *query;
  size_t query_size;
} gearman_queue_libpq_st;

/**
 * PostgreSQL notification callback.
 */
static void _libpq_notice_processor(void *arg, const char *message);

/* Queue callback functions. */
static gearman_return_t _libpq_add(gearman_server_st *server, void *context,
                                   const void *unique, size_t unique_size,
                                   const void *function_name,
                                   size_t function_name_size,
                                   const void *data, size_t data_size,
                                   gearman_job_priority_t priority);
static gearman_return_t _libpq_flush(gearman_server_st *server, void *context);
static gearman_return_t _libpq_done(gearman_server_st *server, void *context,
                                    const void *unique,
                                    size_t unique_size,
                                    const void *function_name,
                                    size_t function_name_size);
static gearman_return_t _libpq_replay(gearman_server_st *server, void *context,
                                      gearman_queue_add_fn *add_fn,
                                      void *add_context);

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearman_server_queue_libpq_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "libpq");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

#define MCO(__name, __value, __help) \
  gearman_conf_module_add_option(module, __name, 0, __value, __help);

  MCO("conninfo", "STRING", "PostgreSQL connection information string.")
  MCO("table", "TABLE", "Table to use.")

  return gearman_conf_return(conf);
}

gearman_return_t gearman_server_queue_libpq_init(gearman_server_st *server,
                                                 gearman_conf_st *conf)
{
  gearman_queue_libpq_st *queue;
  gearman_conf_module_st *module;
  const char *name;
  const char *value;
  const char *conninfo= "";
  char create[1024];
  PGresult *result;

  gearman_log_info(server->gearman, "Initializing libpq module");

  queue= (gearman_queue_libpq_st *)malloc(sizeof(gearman_queue_libpq_st));
  if (queue == NULL)
  {
    gearman_log_error(server->gearman, "gearman_queue_libpq_init", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(queue, 0, sizeof(gearman_queue_libpq_st));
  snprintf(queue->table, NAMEDATALEN, GEARMAN_QUEUE_LIBPQ_DEFAULT_TABLE);

  gearman_server_set_queue_context(server, queue);

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libpq");
  if (module == NULL)
  {
    gearman_log_error(server->gearman, "gearman_queue_libpq_init", "gearman_conf_module_find:NULL");
    return GEARMAN_QUEUE_ERROR;
  }

  while (gearman_conf_module_value(module, &name, &value))
  {
    if (!strcmp(name, "conninfo"))
      conninfo= value;
    else if (!strcmp(name, "table"))
      snprintf(queue->table, NAMEDATALEN, "%s", value);
    else
    {
      gearman_server_queue_libpq_deinit(server);
      gearman_log_error(server->gearman, "gearman_queue_libpq_init", "Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  queue->con= PQconnectdb(conninfo);

  if (queue->con == NULL || PQstatus(queue->con) != CONNECTION_OK)
  {
    gearman_log_error(server->gearman, "gearman_queue_libpq_init",
                             "PQconnectdb: %s", PQerrorMessage(queue->con));
    gearman_server_queue_libpq_deinit(server);
    return GEARMAN_QUEUE_ERROR;
  }

  (void)PQsetNoticeProcessor(queue->con, _libpq_notice_processor, server);

  snprintf(create, 1024, "SELECT tablename FROM pg_tables WHERE tablename='%s'",
           queue->table);

  result= PQexec(queue->con, create);
  if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    gearman_log_error(server->gearman, "gearman_queue_libpq_init", "PQexec:%s",
                             PQerrorMessage(queue->con));
    PQclear(result);
    gearman_server_queue_libpq_deinit(server);
    return GEARMAN_QUEUE_ERROR;
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);

    snprintf(create, 1024,
             "CREATE TABLE %s"
             "("
               "unique_key VARCHAR(%d) PRIMARY KEY,"
               "function_name VARCHAR(255),"
               "priority INTEGER,"
               "data BYTEA"
             ")",
             queue->table, GEARMAN_UNIQUE_SIZE);

    gearman_log_info(server->gearman, "libpq module creating table '%s'", queue->table);

    result= PQexec(queue->con, create);
    if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
      gearman_log_error(server->gearman, "gearman_queue_libpq_init", "PQexec:%s",
                               PQerrorMessage(queue->con));
      PQclear(result);
      gearman_server_queue_libpq_deinit(server);
      return GEARMAN_QUEUE_ERROR;
    }

    PQclear(result);
  }
  else
    PQclear(result);

  gearman_server_set_queue_add_fn(server, _libpq_add);
  gearman_server_set_queue_flush_fn(server, _libpq_flush);
  gearman_server_set_queue_done_fn(server, _libpq_done);
  gearman_server_set_queue_replay_fn(server, _libpq_replay);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_server_queue_libpq_deinit(gearman_server_st *server)
{
  gearman_queue_libpq_st *queue;

  gearman_log_info(server->gearman, "Shutting down libpq queue module");

  queue= (gearman_queue_libpq_st *)gearman_server_queue_context(server);
  gearman_server_set_queue_context(server, NULL);

  if (queue->con != NULL)
    PQfinish(queue->con);

  if (queue->query != NULL)
    free(queue->query);

  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libpq_init(gearmand_st *gearmand,
                                                gearman_conf_st *conf)
{
  return gearman_server_queue_libpq_init(&(gearmand->server), conf);
}

gearman_return_t gearmand_queue_libpq_deinit(gearmand_st *gearmand)
{
  return gearman_server_queue_libpq_deinit(&(gearmand->server));
}

/*
 * Static definitions
 */

static void _libpq_notice_processor(void *arg, const char *message)
{
  gearman_server_st *server= (gearman_server_st *)arg;
  gearman_log_info(server->gearman, "PostgreSQL %s", message);
}

static gearman_return_t _libpq_add(gearman_server_st *server, void *context,
                                        const void *unique, size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearman_job_priority_t priority)
{
  gearman_queue_libpq_st *queue= (gearman_queue_libpq_st *)context;
  char *query;
  size_t query_size;
  PGresult *result;

  const char *param_values[3]= { (char *)unique,
                                 (char *)function_name,
                                 (char *)data };
  int param_lengths[3]= { (int)unique_size,
                          (int)function_name_size,
                          (int)data_size };
  int param_formats[3]= { 0, 0, 1 };

  gearman_log_debug(server->gearman, "libpq add: %.*s", (uint32_t)unique_size, (char *)unique);

  /* This is not used currently, it will be once batch writes are supported
     inside of the Gearman job server. */
#if 0
  if (!not started)
  {
    if (_query(pq, "BEGIN", 5) != PQ_RETURN_OK)
      return REPQ_RETURN_EXTERNAL;

    pq_result_free(&(pq->result));
  }
#endif

  query_size= GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= (char *)realloc(queue->query, query_size);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libpq_add", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  (void)snprintf(query, query_size,
                 "INSERT INTO %s (priority,unique_key,function_name,data) "
                 "VALUES(%u,$1,$2,$3)", queue->table, (uint32_t)priority);
  result= PQexecParams(queue->con, query, 3, NULL, param_values, param_lengths,
                       param_formats, 0);
  if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    gearman_log_error(server->gearman, "_libpq_command", "PQexec:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libpq_flush(gearman_server_st *server,
                                     void *context __attribute__((unused)))
{
  gearman_log_debug(server->gearman, "libpq flush");

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libpq_done(gearman_server_st *server, void *context,
                                    const void *unique,
                                    size_t unique_size,
                                    const void *function_name __attribute__((unused)),
                                    size_t function_name_size __attribute__((unused)))
{
  gearman_queue_libpq_st *queue= (gearman_queue_libpq_st *)context;
  char *query;
  size_t query_size;
  PGresult *result;

  gearman_log_debug(server->gearman, "libpq done: %.*s", (uint32_t)unique_size, (char *)unique);

  query_size= (unique_size * 2) + GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= (char *)realloc(queue->query, query_size);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libpq_add", "realloc");
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

  query_size+= PQescapeStringConn(queue->con, query + query_size, unique,
                                  unique_size, NULL);
  memcpy(query + query_size, "'", 1);
  query_size+= 1;
  query[query_size]= 0;

  result= PQexec(queue->con, query);
  if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    gearman_log_error(server->gearman, "_libpq_add", "PQexec:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libpq_replay(gearman_server_st *server, void *context,
                                      gearman_queue_add_fn *add_fn,
                                      void *add_context)
{
  gearman_queue_libpq_st *queue= (gearman_queue_libpq_st *)context;
  char *query;
  gearman_return_t ret;
  PGresult *result;
  int row;
  void *data;

  gearman_log_info(server->gearman, "libpq replay start");

  if (GEARMAN_QUEUE_QUERY_BUFFER > queue->query_size)
  {
    query= (char *)realloc(queue->query, GEARMAN_QUEUE_QUERY_BUFFER);
    if (query == NULL)
    {
      gearman_log_error(server->gearman, "_libpq_replay", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= GEARMAN_QUEUE_QUERY_BUFFER;
  }
  else
    query= queue->query;

  (void)snprintf(query, GEARMAN_QUEUE_QUERY_BUFFER,
                 "SELECT unique_key,function_name,priority,data FROM %s",
                 queue->table);

  result= PQexecParams(queue->con, query, 0, NULL, NULL, NULL, NULL, 1);
  if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    gearman_log_error(server->gearman, "_libpq_replay", "PQexecParams:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  for (row= 0; row < PQntuples(result); row++)
  {
    gearman_log_debug(server->gearman, "libpq replay: %.*s",
                      PQgetlength(result, row, 0),
                      PQgetvalue(result, row, 0));

    if (PQgetlength(result, row, 3) == 0)
    {
      data= NULL;
    }
    else
    {
      data= (void *)malloc((size_t)PQgetlength(result, row, 3));
      if (query == NULL)
      {
        PQclear(result);
        gearman_log_error(server->gearman, "_libpq_replay", "malloc");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }

      memcpy(data, PQgetvalue(result, row, 3),
             (size_t)PQgetlength(result, row, 3));
    }

    ret= (*add_fn)(server, add_context, PQgetvalue(result, row, 0),
                   (size_t)PQgetlength(result, row, 0),
                   PQgetvalue(result, row, 1),
                   (size_t)PQgetlength(result, row, 1),
                   data, (size_t)PQgetlength(result, row, 3),
                   atoi(PQgetvalue(result, row, 2)));
    if (ret != GEARMAN_SUCCESS)
    {
      PQclear(result);
      return ret;
    }
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}
