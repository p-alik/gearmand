/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Drizzle Queue Storage Definitions
 */

#include "common.h"

#include <libgearman/queue_libdrizzle.h>
#include <libdrizzle/drizzle_client.h>

/**
 * @addtogroup gearman_queue_libdrizzle libdrizzle Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE "test"
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE "queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

/*
 * Private declarations
 */

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
static drizzle_return_t _libdrizzle_query(gearman_st *gearman,
                                          gearman_queue_libdrizzle_st *queue,
                                          const char *query, size_t query_size);

/* Queue callback functions. */
static gearman_return_t _libdrizzle_add(gearman_st *gearman, void *fn_arg,
                                        const void *unique, size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearman_job_priority_t priority);
static gearman_return_t _libdrizzle_flush(gearman_st *gearman, void *fn_arg);
static gearman_return_t _libdrizzle_done(gearman_st *gearman, void *fn_arg,
                                         const void *unique,
                                         size_t unique_size);
static gearman_return_t _libdrizzle_replay(gearman_st *gearman, void *fn_arg,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_fn_arg);

/*
 * Public definitions
 */

void gearman_queue_libdrizzle_usage(void)
{
  printf("\thost=<host>         - Host of server.\n");
  printf("\tport=<port>         - Port of server.\n");
  printf("\tuds=<uds>           - Unix domain socket for server.\n");
  printf("\tuser=<user>         - User name for authentication.\n");
  printf("\tpassword=<password> - Password for authentication.\n");
  printf("\tdb=<db>             - Database to use.\n");
  printf("\ttable=<table>       - Table to use.\n");
  printf("\tmysql               - Use MySQL protocol.\n");
}

gearman_return_t gearman_queue_libdrizzle_init(gearman_st *gearman, int argc,
                                               char *argv[])
{
  gearman_queue_libdrizzle_st *queue;
  int x;
  char *host= NULL;
  in_port_t port= 0;
  char *uds= NULL;
  char *user= NULL;
  char *password= NULL;
  drizzle_row_t row;
  char create[1024];

  queue= malloc(sizeof(gearman_queue_libdrizzle_st));
  if (queue == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libdrizzle_init", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(queue, 0, sizeof(gearman_queue_libdrizzle_st));
  snprintf(queue->table, DRIZZLE_MAX_TABLE_SIZE,
           GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE);

  if (drizzle_create(&(queue->drizzle)) == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libdrizzle_init",
                      "drizzle_create")
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_con_create(&(queue->drizzle), &(queue->con)) == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libdrizzle_init",
                      "drizzle_con_create")
    return GEARMAN_QUEUE_ERROR;
  }

  drizzle_con_set_db(&(queue->con), GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE);

  for (x= 0; x < argc; x++)
  {
    if (!strncmp(argv[x], "host=", 5))
      host= argv[x] + 5;
    else if (!strncmp(argv[x], "port=", 5))
      port= atoi(argv[x] + 5);
    else if (!strncmp(argv[x], "uds=", 4))
      uds= argv[x] + 4;
    else if (!strncmp(argv[x], "user=", 5))
      user= argv[x] + 5;
    else if (!strncmp(argv[x], "password=", 9))
      password= argv[x] + 9;
    else if (!strncmp(argv[x], "db=", 3))
      drizzle_con_set_db(&(queue->con), argv[x] + 3);
    else if (!strncmp(argv[x], "table=", 6))
      snprintf(queue->table, DRIZZLE_MAX_TABLE_SIZE, argv[x] + 6);
    else if (!strcmp(argv[x], "mysql"))
      drizzle_con_set_options(&(queue->con), DRIZZLE_CON_MYSQL);
    else
    {
      drizzle_con_free(&(queue->con));
      drizzle_free(&(queue->drizzle));
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libdrizzle_init",
                        "Unknown argument: %s", argv[x])
      return GEARMAN_QUEUE_ERROR;
    }
  }

  if (uds == NULL)
    drizzle_con_set_tcp(&(queue->con), host, port);
  else
    drizzle_con_set_uds(&(queue->con), uds);

  drizzle_con_set_auth(&(queue->con), user, password);

  if (_libdrizzle_query(gearman, queue, "SHOW TABLES", 11) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  if (drizzle_result_buffer(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libdrizzle_init",
                      "drizzle_result_buffer:%s",
                      drizzle_error(&(queue->drizzle)))
    return GEARMAN_QUEUE_ERROR;
  }

  while ((row= drizzle_row_next(&(queue->result))) != NULL)
  {
    if (!strcasecmp(queue->table, row[0]))
      break;
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

    if (_libdrizzle_query(gearman, queue, create, strlen(create))
        != DRIZZLE_RETURN_OK)
    {
      return GEARMAN_QUEUE_ERROR;
    }

    drizzle_result_free(&(queue->result));
  }

  gearman_set_queue_add(gearman, _libdrizzle_add);
  gearman_set_queue_flush(gearman, _libdrizzle_flush);
  gearman_set_queue_done(gearman, _libdrizzle_done);
  gearman_set_queue_replay(gearman, _libdrizzle_replay);
  gearman_set_queue_fn_arg(gearman, queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_queue_libdrizzle_deinit(gearman_st *gearman)
{
  gearman_queue_libdrizzle_st *queue;

  queue= (gearman_queue_libdrizzle_st *)gearman_queue_fn_arg(gearman);
  drizzle_con_free(&(queue->con));
  drizzle_free(&(queue->drizzle));
  if (queue->query != NULL)
    free(queue->query);
  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libdrizzle_init(gearmand_st *gearmand,
                                                int argc, char *argv[])
{
  return gearman_queue_libdrizzle_init(gearmand->server.gearman, argc, argv);
}

gearman_return_t gearmand_queue_libdrizzle_deinit(gearmand_st *gearmand)
{
  return gearman_queue_libdrizzle_deinit(gearmand->server.gearman);
}

/*
 * Private definitions
 */

static drizzle_return_t _libdrizzle_query(gearman_st *gearman,
                                          gearman_queue_libdrizzle_st *queue,
                                          const char *query, size_t query_size)
{
  drizzle_return_t ret;

  (void)drizzle_query(&(queue->con), &(queue->result), query, query_size, &ret);
  if (ret != DRIZZLE_RETURN_OK)
  {
    if (ret == DRIZZLE_RETURN_ERROR_CODE)
    {
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_query", "drizzle_query:%s",
                        drizzle_result_error(&(queue->result)))
      drizzle_result_free(&(queue->result));
    }
    else
    {
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_query", "drizzle_query:%s",
                        drizzle_error(&(queue->drizzle)))
    }

    return ret;
  }

  return DRIZZLE_RETURN_OK;
}

static gearman_return_t _libdrizzle_add(gearman_st *gearman, void *fn_arg,
                                        const void *unique, size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearman_job_priority_t priority)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)fn_arg;
  char *query;
  size_t query_size;

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
    query= realloc(queue->query, query_size);
    if (query == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_add", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  query_size= snprintf(query, query_size,
                       "INSERT INTO %s SET priority=%u,unique_key='",
                       queue->table, (uint32_t)priority);

  query_size+= drizzle_escape_string(query + query_size, unique, unique_size);
  memcpy(query + query_size, "',function_name='", 17);
  query_size+= 17;

  query_size+= drizzle_escape_string(query + query_size, function_name,
                                     function_name_size);
  memcpy(query + query_size, "',data='", 8);
  query_size+= 8;

  query_size+= drizzle_escape_string(query + query_size, data, data_size);
  memcpy(query + query_size, "'", 1);
  query_size+= 1;

  if (_libdrizzle_query(gearman, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_flush(gearman_st *gearman, void *fn_arg)
{
  (void) gearman;
  (void) fn_arg;

  /* This is not used currently, it will be once batch writes are supported
     inside of the Gearman job server. */

#if 0
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)fn_arg;

  if (_query(gearman, queue, "COMMIT", 6) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));
#endif

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_done(gearman_st *gearman, void *fn_arg,
                                         const void *unique,
                                         size_t unique_size)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)fn_arg;
  char *query;
  size_t query_size;

  query_size= (unique_size * 2) + GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= realloc(queue->query, query_size);
    if (query == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_add", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  query_size= snprintf(query, query_size, "DELETE FROM %s WHERE unique_key='",
                       queue->table);

  query_size+= drizzle_escape_string(query + query_size, unique, unique_size);
  memcpy(query + query_size, "'", 1);
  query_size+= 1;

  if (_libdrizzle_query(gearman, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libdrizzle_replay(gearman_st *gearman, void *fn_arg,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_fn_arg)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)fn_arg;
  char *query;
  size_t query_size;
  drizzle_return_t ret;
  drizzle_row_t row;
  size_t *field_sizes;
  gearman_return_t gret;

  if (GEARMAN_QUEUE_QUERY_BUFFER > queue->query_size)
  {
    query= realloc(queue->query, GEARMAN_QUEUE_QUERY_BUFFER);
    if (query == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_add", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= GEARMAN_QUEUE_QUERY_BUFFER;
  }
  else
    query= queue->query;

  query_size= snprintf(query, GEARMAN_QUEUE_QUERY_BUFFER,
                       "SELECT unique_key,function_name,priority,data FROM %s",
                       queue->table);

  if (_libdrizzle_query(gearman, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  if (drizzle_column_skip(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    GEARMAN_ERROR_SET(gearman, "_libdrizzle_replay", "drizzle_column_skip:%s",
                      drizzle_error(&(queue->drizzle)))
    return GEARMAN_QUEUE_ERROR;
  }

  while (1)
  {
    row= drizzle_row_buffer(&(queue->result), &ret);
    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(&(queue->result));
      GEARMAN_ERROR_SET(gearman, "_libdrizzle_replay", "drizzle_row_buffer:%s",
                        drizzle_error(&(queue->drizzle)))
      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    field_sizes= drizzle_row_field_sizes(&(queue->result));

    gret= (*add_fn)(gearman, add_fn_arg, row[0], field_sizes[0], row[1],
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
