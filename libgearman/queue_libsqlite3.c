/* Gearman server and library
 * Copyright (C) 2009 Cory Bennett
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Sqlite Queue Storage Definitions
 */

#include "common.h"

#include <libgearman/queue_libsqlite3.h>
#include <sqlite3.h>

/**
 * @addtogroup gearman_queue_sqlite sqlite Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE "gearman_queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

/*
 * Private declarations
 */
#define SQLITE_MAX_TABLE_SIZE 256
#define SQLITE_MAX_CREATE_TABLE_SIZE 1024

/**
 * Structure for sqlite specific data.
 */
typedef struct
{
  sqlite3* db;
  char table[SQLITE_MAX_TABLE_SIZE];
  char *query;
  size_t query_size;
  int in_trans;
} gearman_queue_sqlite_st;

/**
 * Query error handling function.
 */
static int _sqlite_query(gearman_st *gearman,
                         gearman_queue_sqlite_st *queue,
                         const char *query, size_t query_size,
                         sqlite3_stmt ** sth);
static int _sqlite_lock(gearman_st *gearman,
                        gearman_queue_sqlite_st *queue);
static int _sqlite_commit(gearman_st *gearman,
                          gearman_queue_sqlite_st *queue);
static int _sqlite_rollback(gearman_st *gearman,
                            gearman_queue_sqlite_st *queue);

/* Queue callback functions. */
static gearman_return_t _sqlite_add(gearman_st *gearman, void *fn_arg,
                                    const void *unique, size_t unique_size,
                                    const void *function_name,
                                    size_t function_name_size,
                                    const void *data, size_t data_size,
                                    gearman_job_priority_t priority);
static gearman_return_t _sqlite_flush(gearman_st *gearman, void *fn_arg);
static gearman_return_t _sqlite_done(gearman_st *gearman, void *fn_arg,
                                     const void *unique,
                                     size_t unique_size,
                                     const void *function_name,
                                     size_t function_name_size);
static gearman_return_t _sqlite_replay(gearman_st *gearman, void *fn_arg,
                                       gearman_queue_add_fn *add_fn,
                                       void *add_fn_arg);

/*
 * Public definitions
 */

gearman_return_t gearman_queue_libsqlite3_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "libsqlite3");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

#define MCO(__name, __value, __help) \
  gearman_conf_module_add_option(module, __name, 0, __value, __help);

  MCO("db", "DB", "Database file to use.")
    MCO("table", "TABLE", "Table to use.")

    return gearman_conf_return(conf);
}

gearman_return_t gearman_queue_libsqlite3_init(gearman_st *gearman,
                                               gearman_conf_st *conf)
{
  gearman_queue_sqlite_st *queue;
  gearman_conf_module_st *module;
  const char *name;
  const char *value;
  char *table= NULL;
  const char *query;
  sqlite3_stmt* sth;
  char create[SQLITE_MAX_CREATE_TABLE_SIZE];

  GEARMAN_INFO(gearman, "Initializing libsqlite3 module");

  queue= malloc(sizeof(gearman_queue_sqlite_st));
  if (queue == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init", "malloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(queue, 0, sizeof(gearman_queue_sqlite_st));
  snprintf(queue->table, SQLITE_MAX_TABLE_SIZE,
           GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE);

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libsqlite3");
  if (module == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                      "gearman_conf_module_find:NULL");
    free(queue);
    return GEARMAN_QUEUE_ERROR;
  }

  gearman_set_queue_fn_arg(gearman, queue);

  while (gearman_conf_module_value(module, &name, &value))
  {
    if (!strcmp(name, "db"))
    {
      if (sqlite3_open(value, &(queue->db)) != SQLITE_OK)
      {
        gearman_queue_libsqlite3_deinit(gearman);        
        GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                          "Can't open database: %s\n", sqlite3_errmsg(queue->db));
        free(queue);
        return GEARMAN_QUEUE_ERROR;
      }
    }
    else if (!strcmp(name, "table"))
      snprintf(queue->table, SQLITE_MAX_TABLE_SIZE, "%s", value);
    else
    {
      gearman_queue_libsqlite3_deinit(gearman);
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                        "Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  if (!queue->db)
  {
    gearman_queue_libsqlite3_deinit(gearman);
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                      "missing required --libsqlite3-db=<dbfile> argument");
    return GEARMAN_QUEUE_ERROR;
  }    

  query= "SELECT name FROM sqlite_master WHERE type='table'";
  if (_sqlite_query(gearman, queue, query, strlen(query), &sth) != SQLITE_OK)
  {
    gearman_queue_libsqlite3_deinit(gearman);
    return GEARMAN_QUEUE_ERROR;
  }

  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    if (sqlite3_column_type(sth,0) == SQLITE_TEXT)
    {
      table= (char*)sqlite3_column_text(sth, 0);
    }
    else
    {
      sqlite3_finalize(sth);
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                        "column %d is not type TEXT", 0);
      return GEARMAN_QUEUE_ERROR;
    }

    if (!strcasecmp(queue->table, table))
    {
      GEARMAN_INFO(gearman, "sqlite module using table '%s'", table);
      break;
    }
  }

  if (sqlite3_finalize(sth) != SQLITE_OK)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                      "sqlite_finalize:%s", sqlite3_errmsg(queue->db));
    gearman_queue_libsqlite3_deinit(gearman);
    return GEARMAN_QUEUE_ERROR;
  }

  if (table == NULL)
  {
    snprintf(create, SQLITE_MAX_CREATE_TABLE_SIZE,
             "CREATE TABLE %s"
             "("
             "unique_key TEXT PRIMARY KEY,"
             "function_name TEXT,"
             "priority INTEGER,"
             "data BLOB"
             ")",
             queue->table);

    GEARMAN_INFO(gearman, "sqlite module creating table '%s'", queue->table);

    if (_sqlite_query(gearman, queue, create, strlen(create), &sth)
        != SQLITE_OK)
    {
      gearman_queue_libsqlite3_deinit(gearman);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_step(sth) != SQLITE_DONE)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                        "create table error: %s", sqlite3_errmsg(queue->db));
      sqlite3_finalize(sth);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_finalize(sth) != SQLITE_OK)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libsqlite3_init",
                        "sqlite_finalize:%s", sqlite3_errmsg(queue->db));
      gearman_queue_libsqlite3_deinit(gearman);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  gearman_set_queue_add(gearman, _sqlite_add);
  gearman_set_queue_flush(gearman, _sqlite_flush);
  gearman_set_queue_done(gearman, _sqlite_done);
  gearman_set_queue_replay(gearman, _sqlite_replay);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_queue_libsqlite3_deinit(gearman_st *gearman)
{
  gearman_queue_sqlite_st *queue;

  GEARMAN_INFO(gearman, "Shutting down sqlite queue module");

  queue= (gearman_queue_sqlite_st *)gearman_queue_fn_arg(gearman);
  gearman_set_queue_fn_arg(gearman, NULL);
  sqlite3_close(queue->db);
  if (queue->query != NULL)
    free(queue->query);
  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libsqlite3_init(gearmand_st *gearmand,
                                                gearman_conf_st *conf)
{
  return gearman_queue_libsqlite3_init(gearmand->server.gearman, conf);
}

gearman_return_t gearmand_queue_libsqlite3_deinit(gearmand_st *gearmand)
{
  return gearman_queue_libsqlite3_deinit(gearmand->server.gearman);
}

/*
 * Private definitions
 */

int _sqlite_query(gearman_st *gearman,
                  gearman_queue_sqlite_st *queue,
                  const char *query, size_t query_size,
                  sqlite3_stmt ** sth)
{
  int ret;

  if (query_size > UINT32_MAX)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_query", "query size too big [%u]",
                      (uint32_t)query_size);
    return SQLITE_ERROR;
  }

  GEARMAN_CRAZY(gearman, "sqlite query: %s", query);
  ret= sqlite3_prepare(queue->db, query, (int)query_size, sth, NULL);
  if (ret  != SQLITE_OK)
  {
    if (*sth)
      sqlite3_finalize(*sth);
    *sth= NULL;
    GEARMAN_ERROR_SET(gearman, "_sqlite_query", "sqlite_prepare:%s", 
                      sqlite3_errmsg(queue->db));
  }

  return ret;
}

int _sqlite_lock(gearman_st *gearman,
                 gearman_queue_sqlite_st *queue)
{
  sqlite3_stmt* sth;
  int ret;
  if (queue->in_trans)
  {
    /* already in transaction */
    return SQLITE_OK;
  }

  ret= _sqlite_query(gearman, queue, "BEGIN TRANSACTION", 
                     sizeof("BEGIN TRANSACTION") - 1, &sth);
  if (ret != SQLITE_OK)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_lock",
                      "failed to begin transaction: %s", 
                      sqlite3_errmsg(queue->db));
    if(sth)
      sqlite3_finalize(sth);

    return ret;
  }

  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_lock", "lock error: %s",
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }

  sqlite3_finalize(sth);
  queue->in_trans++;

  return SQLITE_OK;
}

int _sqlite_commit(gearman_st *gearman,
                   gearman_queue_sqlite_st *queue)
{
  sqlite3_stmt* sth;
  int ret;

  if (! queue->in_trans)
  {
    /* not in transaction */
    return SQLITE_OK;
  }

  ret= _sqlite_query(gearman, queue, "COMMIT", sizeof("COMMIT") - 1, &sth);
  if (ret != SQLITE_OK)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_commit",
                      "failed to commit transaction: %s", 
                      sqlite3_errmsg(queue->db));
    if(sth)
      sqlite3_finalize(sth);
    return ret;
  }
  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_commit", "commit error: %s",
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  queue->in_trans= 0;
  return SQLITE_OK;
}

int _sqlite_rollback(gearman_st *gearman,
                     gearman_queue_sqlite_st *queue)
{
  sqlite3_stmt* sth;
  int ret;
  const char* query;

  if (! queue->in_trans)
  {
    /* not in transaction */
    return SQLITE_OK;
  }

  query= "ROLLBACK";
  ret= _sqlite_query(gearman, queue, query, strlen(query), &sth);
  if (ret != SQLITE_OK)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_rollback",
                      "failed to rollback transaction: %s",
                      sqlite3_errmsg(queue->db));
    if(sth)
      sqlite3_finalize(sth);
    return ret;
  }
  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_rollback", "rollback error: %s",
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  queue->in_trans= 0;
  return SQLITE_OK;
}

static gearman_return_t _sqlite_add(gearman_st *gearman, void *fn_arg,
                                    const void *unique, size_t unique_size,
                                    const void *function_name,
                                    size_t function_name_size,
                                    const void *data, size_t data_size,
                                    gearman_job_priority_t priority)
{
  gearman_queue_sqlite_st *queue= (gearman_queue_sqlite_st *)fn_arg;
  char *query;
  size_t query_size;
  sqlite3_stmt* sth;

  if (unique_size > UINT32_MAX || function_name_size > UINT32_MAX ||
      data_size > UINT32_MAX)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "size too big [%u]",
                      (uint32_t)unique_size);
    return SQLITE_ERROR;
  }

  GEARMAN_DEBUG(gearman, "sqlite add: %.*s", (uint32_t)unique_size,
                (char *)unique);

  if (_sqlite_lock(gearman, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  query_size= ((unique_size + function_name_size + data_size) * 2) +
    GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= realloc(queue->query, query_size);
    if (query == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "_sqlite_add", "realloc")
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  query_size= (size_t)snprintf(query, query_size,
                               "INSERT INTO %s (priority,unique_key,"
                               "function_name,data) VALUES (?,?,?,?)",
                               queue->table);

  if (_sqlite_query(gearman, queue, query, query_size, &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  if (sqlite3_bind_int(sth,  1, priority) != SQLITE_OK)
  {
    _sqlite_rollback(gearman, queue);
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "failed to bind int [%d]: %s",
                      priority, sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 2, unique, (int)unique_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(gearman, queue);
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "failed to bind text [%.*s]: %s", 
                      (uint32_t)unique_size, (char*)unique,
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 3, function_name, (int)function_name_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(gearman, queue);
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "failed to bind text [%.*s]: %s", 
                      (uint32_t)function_name_size, (char*)function_name,
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_blob(sth, 4, data, (int)data_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(gearman, queue);
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "failed to bind blob: %s",
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_add", "insert error: %s",
                      sqlite3_errmsg(queue->db));
    if (sqlite3_finalize(sth) != SQLITE_OK )
      GEARMAN_ERROR_SET(gearman, "_sqlite_add", "finalize error: %s",
                        sqlite3_errmsg(queue->db));

    return GEARMAN_QUEUE_ERROR;
  }

  sqlite3_finalize(sth);

  if (_sqlite_commit(gearman, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _sqlite_flush(gearman_st *gearman,
                                      void *fn_arg __attribute__((unused)))
{
  GEARMAN_DEBUG(gearman, "sqlite flush");

  return GEARMAN_SUCCESS;
}

static gearman_return_t _sqlite_done(gearman_st *gearman, void *fn_arg,
                                     const void *unique,
                                     size_t unique_size,
                                     const void *function_name __attribute__((unused)),
                                     size_t function_name_size __attribute__((unused)))
{
  gearman_queue_sqlite_st *queue= (gearman_queue_sqlite_st *)fn_arg;
  char *query;
  size_t query_size;
  sqlite3_stmt* sth;

  if (unique_size > UINT32_MAX)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_query", "unique key size too big [%u]",
                      (uint32_t)unique_size);
    return SQLITE_ERROR;
  }

  GEARMAN_DEBUG(gearman, "sqlite done: %.*s", (uint32_t)unique_size,
                (char *)unique);

    if (_sqlite_lock(gearman, queue) !=  SQLITE_OK)
      return GEARMAN_QUEUE_ERROR;

  query_size= (unique_size * 2) + GEARMAN_QUEUE_QUERY_BUFFER;
  if (query_size > queue->query_size)
  {
    query= realloc(queue->query, query_size);
    if (query == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "_sqlite_add", "realloc")
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    queue->query= query;
    queue->query_size= query_size;
  }
  else
    query= queue->query;

  query_size= (size_t)snprintf(query, query_size,
                               "DELETE FROM %s WHERE unique_key=?",
                               queue->table);

  if (_sqlite_query(gearman, queue, query, query_size, &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  sqlite3_bind_text(sth, 1, unique, (int)unique_size, SQLITE_TRANSIENT);

  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    GEARMAN_ERROR_SET(gearman, "_sqlite_done", "delete error: %s",
                      sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  sqlite3_finalize(sth);

  if (_sqlite_commit(gearman, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _sqlite_replay(gearman_st *gearman, void *fn_arg,
                                       gearman_queue_add_fn *add_fn,
                                       void *add_fn_arg)
{
  gearman_queue_sqlite_st *queue= (gearman_queue_sqlite_st *)fn_arg;
  char *query;
  size_t query_size;
  sqlite3_stmt* sth;
  gearman_return_t gret;

  GEARMAN_INFO(gearman, "sqlite replay start")

    if (GEARMAN_QUEUE_QUERY_BUFFER > queue->query_size)
    {
      query= realloc(queue->query, GEARMAN_QUEUE_QUERY_BUFFER);
      if (query == NULL)
      {
        GEARMAN_ERROR_SET(gearman, "_sqlite_replay", "realloc")
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

  if (_sqlite_query(gearman, queue, query, query_size, &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;
  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    const void *unique, *function_name;
    void *data;
    size_t unique_size, function_name_size, data_size;
    gearman_job_priority_t priority;

    if (sqlite3_column_type(sth,0) == SQLITE_TEXT)
    {
      unique= sqlite3_column_text(sth,0);
      unique_size= (size_t) sqlite3_column_bytes(sth,0);
    }
    else
    {
      sqlite3_finalize(sth);
      GEARMAN_ERROR_SET(gearman, "_sqlite_replay",
                        "column %d is not type TEXT", 0);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,1) == SQLITE_TEXT)
    {
      function_name= sqlite3_column_text(sth,1);
      function_name_size= (size_t)sqlite3_column_bytes(sth,1);
    }
    else
    {
      sqlite3_finalize(sth);
      GEARMAN_ERROR_SET(gearman, "_sqlite_replay",
                        "column %d is not type TEXT", 1);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,2) == SQLITE_INTEGER)
    {
      priority= (double)sqlite3_column_int64(sth,2);
    }
    else
    {
      sqlite3_finalize(sth);
      GEARMAN_ERROR_SET(gearman, "_sqlite_replay",
                        "column %d is not type INTEGER", 2);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,3) == SQLITE_BLOB)
    {
      data_size= (size_t)sqlite3_column_bytes(sth,3);
      /* need to make a copy here ... gearman_server_job_free will free it later */
      data= malloc(data_size);
      if (data == NULL)
      {
        sqlite3_finalize(sth);
        GEARMAN_ERROR_SET(gearman, "_sqlite_replay", "malloc");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
      memcpy(data, sqlite3_column_blob(sth,3), data_size);
    }
    else
    {
      sqlite3_finalize(sth);
      GEARMAN_ERROR_SET(gearman, "_sqlite_replay",
                        "column %d is not type TEXT", 3);
      return GEARMAN_QUEUE_ERROR;
    }

    GEARMAN_DEBUG(gearman, "sqlite replay: %s", (char*)function_name);

    gret= (*add_fn)(gearman, add_fn_arg,
                    unique, unique_size,
                    function_name, function_name_size,
                    data, data_size,
                    priority);
    if (gret != GEARMAN_SUCCESS)
    {
      sqlite3_finalize(sth);
      return gret;
    }
  }

  sqlite3_finalize(sth);

  return GEARMAN_SUCCESS;
}
