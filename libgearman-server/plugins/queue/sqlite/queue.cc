/* Gearman server and library
 * Copyright (C) 2009 Cory Bennett
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libsqlite3 Queue Storage Definitions
 */

#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/sqlite/queue.h>
#include <libgearman-server/plugins/queue/base.h>
#include <sqlite3.h>

/**
 * @addtogroup gearman_queue_libsqlite3_static Static libsqlite3 Queue Storage Definitions
 * @ingroup gearman_queue_libsqlite3
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE "gearman_queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256
#define SQLITE_MAX_TABLE_SIZE 256
#define SQLITE_MAX_CREATE_TABLE_SIZE 1024

namespace gearmand { namespace plugins { namespace queue { class Sqlite; }}}

static gearmand_error_t _initialize(gearman_server_st *server,
                                    gearmand::plugins::queue::Sqlite *queue);

namespace gearmand {
namespace plugins {
namespace queue {

class Sqlite : public gearmand::plugins::Queue {
public:
  Sqlite();
  ~Sqlite();

  gearmand_error_t initialize();

  std::string schema;
  std::string table;
  sqlite3 *db;
  int in_trans;

  const std::string &insert_query() const
  {
    return _insert_query;
  }

  const std::string &delete_query() const
  {
    return _delete_query;
  }

private:
  std::string _insert_query;
  std::string _delete_query;
};

Sqlite::Sqlite() :
  Queue("sqlite")
{
  command_line_options().add_options()
    ("libsqlite3-db", boost::program_options::value(&schema), "Database file to use.")
    ("libsqlite3-table", boost::program_options::value(&table)->default_value(GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE), "Table to use.")
    ;
}

Sqlite::~Sqlite()
{
  if (db)
    sqlite3_close(db);
}

gearmand_error_t Sqlite::initialize()
{
  _insert_query+= "INSERT OR REPLACE INTO ";
  _insert_query+= table;
  _insert_query+= " (priority, unique_key, function_name, data, when_to_run) VALUES (?,?,?,?,?)";

  _delete_query+= "DELETE FROM ";
  _delete_query+= table;
  _delete_query+= " WHERE unique_key=? and function_name=?";

  return _initialize(&Gearmand()->server, this);
}

void initialize_sqlite()
{
  static Sqlite local_instance;
}

} // namespace queue
} // namespace plugin
} // namespace gearmand

/**
 * Query error handling function.
 */
static int _sqlite_query(gearman_server_st *server,
                         gearmand::plugins::queue::Sqlite *queue,
                         const char *query, size_t query_size,
                         sqlite3_stmt ** sth);
static int _sqlite_lock(gearman_server_st *server,
                        gearmand::plugins::queue::Sqlite *queue);
static int _sqlite_commit(gearman_server_st *server,
                          gearmand::plugins::queue::Sqlite *queue);
static int _sqlite_rollback(gearman_server_st *server,
                            gearmand::plugins::queue::Sqlite *queue);

/* Queue callback functions. */
static gearmand_error_t _sqlite_add(gearman_server_st *server, void *context,
                                    const char *unique, size_t unique_size,
                                    const char *function_name,
                                    size_t function_name_size,
                                    const void *data, size_t data_size,
                                    gearmand_job_priority_t priority,
                                    int64_t when);

static gearmand_error_t _sqlite_flush(gearman_server_st *server, void *context);

static gearmand_error_t _sqlite_done(gearman_server_st *server, void *context,
                                     const char *unique,
                                     size_t unique_size,
                                     const char *function_name,
                                     size_t function_name_size);

static gearmand_error_t _sqlite_replay(gearman_server_st *server, void *context,
                                       gearman_queue_add_fn *add_fn,
                                       void *add_context);

/** @} */

/*
 * Public definitions
 */

#pragma GCC diagnostic ignored "-Wold-style-cast"

gearmand_error_t _initialize(gearman_server_st *server,
                             gearmand::plugins::queue::Sqlite *queue)
{
  char *table= NULL;
  const char *query;
  sqlite3_stmt* sth;

  gearmand_log_info("Initializing libsqlite3 module");

  queue= (gearmand::plugins::queue::Sqlite *)calloc(1, sizeof(gearmand::plugins::queue::Sqlite));
  if (queue == NULL)
  {
    gearmand_log_error("gearman_queue_libsqlite3_init", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearman_server_set_queue(server, queue, _sqlite_add, _sqlite_flush, _sqlite_done, _sqlite_replay);

  if (sqlite3_open(queue->schema.c_str(), &(queue->db)) != SQLITE_OK)
  {
    gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
    gearmand_log_error("gearman_queue_libsqlite3_init",
                       "Can't open database: %s\n",
                       sqlite3_errmsg(queue->db));
    return GEARMAN_QUEUE_ERROR;
  }

  if (not queue->db)
  {
    gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
    gearmand_log_error("gearman_queue_libsqlite3_init",
                       "missing required --libsqlite3-db=<dbfile> argument");
    return GEARMAN_QUEUE_ERROR;
  }

  query= "SELECT name FROM sqlite_master WHERE type='table'";
  if (_sqlite_query(server, queue, query, strlen(query), &sth) != SQLITE_OK)
  {
    gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
    return GEARMAN_QUEUE_ERROR;
  }

  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    if (sqlite3_column_type(sth,0) == SQLITE_TEXT)
      table= (char*)sqlite3_column_text(sth, 0);
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("gearman_queue_libsqlite3_init",
                         "column %d is not type TEXT", 0);
      return GEARMAN_QUEUE_ERROR;
    }

    if (! strcasecmp(queue->table.c_str(), table))
    {
      gearmand_log_info("sqlite module using table '%s'", table);
      break;
    }
  }

  if (sqlite3_finalize(sth) != SQLITE_OK)
  {
    gearmand_log_error("gearman_queue_libsqlite3_init",
                       "sqlite_finalize:%s", sqlite3_errmsg(queue->db));
    gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
    return GEARMAN_QUEUE_ERROR;
  }

  if (table == NULL)
  {
    char create[SQLITE_MAX_CREATE_TABLE_SIZE];
    snprintf(create, SQLITE_MAX_CREATE_TABLE_SIZE,
             "CREATE TABLE %s"
             "("
             "unique_key TEXT,"
             "function_name TEXT,"
             "priority INTEGER,"
             "data BLOB,"
             "PRIMARY KEY (unique_key, function_name), when_to_run INTEGER"
             ")",
             queue->table.c_str());

    gearmand_log_info("sqlite module creating table '%s'", queue->table.c_str());

    if (_sqlite_query(server, queue, create, strlen(create), &sth)
        != SQLITE_OK)
    {
      gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_step(sth) != SQLITE_DONE)
    {
      gearmand_log_error("gearman_queue_libsqlite3_init",
                         "create table error: %s",
                         sqlite3_errmsg(queue->db));
      sqlite3_finalize(sth);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_finalize(sth) != SQLITE_OK)
    {
      gearmand_log_error("gearman_queue_libsqlite3_init",
                         "sqlite_finalize:%s",
                         sqlite3_errmsg(queue->db));
      gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  return GEARMAN_SUCCESS;
}

/*
 * Static definitions
 */

int _sqlite_query(gearman_server_st *server,
                  gearmand::plugins::queue::Sqlite *queue,
                  const char *query, size_t query_size,
                  sqlite3_stmt ** sth)
{
  (void)server;
  int ret;

  if (query_size > UINT32_MAX)
  {
    gearmand_log_error("_sqlite_query", "query size too big [%u]",
                       (uint32_t)query_size);
    return SQLITE_ERROR;
  }

  gearmand_log_crazy("sqlite query: %s", query);
  ret= sqlite3_prepare(queue->db, query, (int)query_size, sth, NULL);
  if (ret  != SQLITE_OK)
  {
    if (*sth)
    {
      sqlite3_finalize(*sth);
    }
    *sth= NULL;
    gearmand_log_error("_sqlite_query", "sqlite_prepare:%s",
                       sqlite3_errmsg(queue->db));
  }

  return ret;
}

int _sqlite_lock(gearman_server_st *server,
                 gearmand::plugins::queue::Sqlite *queue)
{
  sqlite3_stmt* sth;
  int ret;
  if (queue->in_trans)
  {
    /* already in transaction */
    return SQLITE_OK;
  }

  ret= _sqlite_query(server, queue, "BEGIN TRANSACTION",
                     sizeof("BEGIN TRANSACTION") - 1, &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error("_sqlite_lock",
                       "failed to begin transaction: %s",
                       sqlite3_errmsg(queue->db));
    if (sth)
    {
      sqlite3_finalize(sth);
    }

    return ret;
  }

  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_lock", "lock error: %s",
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }

  sqlite3_finalize(sth);
  queue->in_trans++;

  return SQLITE_OK;
}

int _sqlite_commit(gearman_server_st *server,
                   gearmand::plugins::queue::Sqlite *queue)
{
  sqlite3_stmt* sth;
  int ret;

  if (! queue->in_trans)
  {
    /* not in transaction */
    return SQLITE_OK;
  }

  ret= _sqlite_query(server, queue, "COMMIT", sizeof("COMMIT") - 1, &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error("_sqlite_commit",
                       "failed to commit transaction: %s",
                       sqlite3_errmsg(queue->db));
    if (sth)
    {
      sqlite3_finalize(sth);
    }

    return ret;
  }
  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_commit", "commit error: %s",
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  queue->in_trans= 0;
  return SQLITE_OK;
}

int _sqlite_rollback(gearman_server_st *server,
                     gearmand::plugins::queue::Sqlite *queue)
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
  ret= _sqlite_query(server, queue, query, strlen(query), &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error("_sqlite_rollback",
                       "failed to rollback transaction: %s",
                       sqlite3_errmsg(queue->db));
    if (sth)
    {
      sqlite3_finalize(sth);
    }

    return ret;
  }
  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_rollback", "rollback error: %s",
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  queue->in_trans= 0;

  return SQLITE_OK;
}

static gearmand_error_t _sqlite_add(gearman_server_st *server, void *context,
                                    const char *unique, size_t unique_size,
                                    const char *function_name,
                                    size_t function_name_size,
                                    const void *data, size_t data_size,
                                    gearmand_job_priority_t priority,
                                    int64_t when)
{
  gearmand::plugins::queue::Sqlite *queue= (gearmand::plugins::queue::Sqlite *)context;
  sqlite3_stmt* sth;

  if (unique_size > UINT32_MAX || function_name_size > UINT32_MAX ||
      data_size > UINT32_MAX)
  {
    gearmand_log_error("_sqlite_add", "size too big [%u]",
                       (uint32_t)unique_size);
    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_debug("sqlite add: %.*s at %ld", (uint32_t)unique_size, (char *)unique, (long int)when);

  if (_sqlite_lock(server, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  if (_sqlite_query(server, queue, queue->insert_query().c_str(), queue->insert_query().size(), &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  if (sqlite3_bind_int(sth,  1, priority) != SQLITE_OK)
  {
    _sqlite_rollback(server, queue);
    gearmand_log_error("_sqlite_add", "failed to bind int [%d]: %s", priority, sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);

    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 2, (const char *)unique, (int)unique_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(server, queue);
    gearmand_log_error("_sqlite_add",
                       "failed to bind text [%.*s]: %s",
                       (uint32_t)unique_size, (char*)unique,
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 3, (const char *)function_name, (int)function_name_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(server, queue);
    gearmand_log_error("_sqlite_add",
                       "failed to bind text [%.*s]: %s",
                       (uint32_t)function_name_size, (char*)function_name,
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_blob(sth, 4, data, (int)data_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback(server, queue);
    gearmand_log_error("_sqlite_add", "failed to bind blob: %s",
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  // epoch data
  if (sqlite3_bind_int64(sth,  5, when) != SQLITE_OK)
  {
    _sqlite_rollback(server, queue);
    gearmand_log_error("_sqlite_add", "failed to bind int64_t(%ld): %s",
                       (long int)when,
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }
  
  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_add", "insert error: %s",
                       sqlite3_errmsg(queue->db));
    if (sqlite3_finalize(sth) != SQLITE_OK )
    {
      gearmand_log_error("_sqlite_add", "finalize error: %s",
                         sqlite3_errmsg(queue->db));
    }

    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_crazy(
                     "sqlite data: priority: %d, unique_key: %s, function_name: %s",
                     priority, (char*)unique, (char*)function_name);

  sqlite3_finalize(sth);

  if (_sqlite_commit(server, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _sqlite_flush(gearman_server_st *server,
                                      void *context __attribute__((unused)))
{
  (void)server;
  gearmand_log_debug("sqlite flush");

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _sqlite_done(gearman_server_st *server, void *context,
                                     const char *unique,
                                     size_t unique_size,
                                     const char *function_name __attribute__((unused)),
                                     size_t function_name_size __attribute__((unused)))
{
  gearmand::plugins::queue::Sqlite *queue= (gearmand::plugins::queue::Sqlite *)context;
  sqlite3_stmt* sth;

  if (unique_size > UINT32_MAX)
  {
    gearmand_log_error(
                       "_sqlite_query", "unique key size too big [%u]",
                       (uint32_t)unique_size);
    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_debug("sqlite done: %.*s", (uint32_t)unique_size, (char *)unique);

  if (_sqlite_lock(server, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  if (_sqlite_query(server, queue, queue->delete_query().c_str(), queue->delete_query().size(), &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  sqlite3_bind_text(sth, 1, (const char *)unique, (int)unique_size, SQLITE_TRANSIENT);
  sqlite3_bind_text(sth, 2, (const char *)function_name, (int)function_name_size, SQLITE_TRANSIENT);

  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_done", "delete error: %s",
                       sqlite3_errmsg(queue->db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  sqlite3_finalize(sth);

  if (_sqlite_commit(server, queue) !=  SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _sqlite_replay(gearman_server_st *server, void *context,
                                       gearman_queue_add_fn *add_fn,
                                       void *add_context)
{
  gearmand::plugins::queue::Sqlite *queue= (gearmand::plugins::queue::Sqlite *)context;
  sqlite3_stmt* sth;
  gearmand_error_t gret;

  gearmand_log_info("sqlite replay start");

  std::string query;
  query+= "SELECT unique_key,function_name,priority,data,when_to_run FROM ";
  query+= queue->table;

  if (_sqlite_query(server, queue, query.c_str(), query.size(), &sth) != SQLITE_OK)
    return GEARMAN_QUEUE_ERROR;

  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    const char *unique, *function_name;
    void *data;
    size_t unique_size, function_name_size, data_size;
    gearmand_job_priority_t priority;
    int64_t when;

    if (sqlite3_column_type(sth,0) == SQLITE_TEXT)
    {
      unique= (char *)sqlite3_column_text(sth,0);
      unique_size= (size_t) sqlite3_column_bytes(sth,0);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("_sqlite_replay",
                         "column %d is not type TEXT", 0);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,1) == SQLITE_TEXT)
    {
      function_name= (char *)sqlite3_column_text(sth,1);
      function_name_size= (size_t)sqlite3_column_bytes(sth,1);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("_sqlite_replay",
                         "column %d is not type TEXT", 1);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,2) == SQLITE_INTEGER)
    {
      priority= (gearmand_job_priority_t)sqlite3_column_int64(sth,2);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("_sqlite_replay",
                         "column %d is not type INTEGER", 2);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,3) == SQLITE_BLOB)
    {
      data_size= (size_t)sqlite3_column_bytes(sth,3);
      /* need to make a copy here ... gearman_server_job_free will free it later */
      data= (void *)malloc(data_size);
      if (data == NULL)
      {
        sqlite3_finalize(sth);
        gearmand_perror("malloc");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
      memcpy(data, sqlite3_column_blob(sth,3), data_size);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("_sqlite_replay", "column %d is not type TEXT", 3);
      return GEARMAN_QUEUE_ERROR;
    }
    
    if (sqlite3_column_type(sth,4) == SQLITE_INTEGER)
    {
      when= (int64_t)sqlite3_column_int64(sth,4);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error("_sqlite_replay", "column %d is not type INTEGER", 3);
      return GEARMAN_QUEUE_ERROR;
    }

    gearmand_log_debug("sqlite replay: %s", (char*)function_name);

    gret= (*add_fn)(server, add_context,
                    unique, unique_size,
                    function_name, function_name_size,
                    data, data_size,
                    priority, when);
    if (gret != GEARMAN_SUCCESS)
    {
      sqlite3_finalize(sth);
      return gret;
    }
  }

  sqlite3_finalize(sth);

  return GEARMAN_SUCCESS;
}
