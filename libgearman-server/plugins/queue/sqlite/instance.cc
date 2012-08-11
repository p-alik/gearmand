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

#include "libgearman-server/plugins/base.h"
#include "libgearman-server/plugins/queue/sqlite/instance.hpp"

namespace gearmand {
namespace queue {

Instance::Instance(const std::string& schema_, const std::string& table_):
  _epoch_support(true),
  _in_trans(0),
  _db(NULL),
  _schema(schema_),
  _table(table_)
  { 
    _delete_query+= "DELETE FROM ";
    _delete_query+= _table;
    _delete_query+= " WHERE unique_key=? and function_name=?";

    if (_epoch_support)
    {
      _insert_query+= "INSERT OR REPLACE INTO ";
      _insert_query+= _table;
      _insert_query+= " (priority, unique_key, function_name, data, when_to_run) VALUES (?,?,?,?,?)";
    }
    else
    {
      _insert_query+= "INSERT OR REPLACE INTO ";
      _insert_query+= _table;
      _insert_query+= " (priority, unique_key, function_name, data) VALUES (?,?,?,?,?)";
    }
  }

Instance::~Instance()
{
  if (_db)
  {
    sqlite3_close(_db);
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite shutdown database");
    return;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite shutdown");
}

int Instance::_sqlite_query(const char *query, size_t query_size,
                            sqlite3_stmt ** sth)
{
  if (query_size > UINT32_MAX)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "query size too big [%u]", (uint32_t)query_size);
    return SQLITE_ERROR;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite query: %s", query);
  int ret= sqlite3_prepare(_db, query, (int)query_size, sth, NULL);
  if (ret  != SQLITE_OK)
  {
    if (*sth)
    {
      sqlite3_finalize(*sth);
    }
    *sth= NULL;
    gearmand_log_error(AT, "sqlite_prepare:%s", sqlite3_errmsg(_db));
  }

  return ret;
}

int Instance::_sqlite_lock()
{
  if (_in_trans)
  {
    /* already in transaction */
    return SQLITE_OK;
  }

  sqlite3_stmt* sth;
  int ret= _sqlite_query(gearman_literal_param("BEGIN TRANSACTION"), &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error(AT, "failed to begin transaction: %s", sqlite3_errmsg(_db));
    if (sth)
    {
      sqlite3_finalize(sth);
    }

    return ret;
  }

  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    gearmand_log_error(AT, "lock error: %s", sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return ret;
  }

  sqlite3_finalize(sth);
  _in_trans++;

  return SQLITE_OK;
}

int Instance::_sqlite_commit()
{
  if (_in_trans == 0)
  {
    /* not in transaction */
    return SQLITE_OK;
  }

  sqlite3_stmt* sth;
  int ret= _sqlite_query(gearman_literal_param("COMMIT"), &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error("_sqlite_commit",
                       "failed to commit transaction: %s",
                       sqlite3_errmsg(_db));
    if (sth)
    {
      sqlite3_finalize(sth);
    }

    return ret;
  }

  ret= sqlite3_step(sth);
  if (ret != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_commit", "commit error: %s", sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  _in_trans= 0;

  return SQLITE_OK;
}

gearmand_error_t Instance::init()
{
  gearmand_info("Initializing libsqlite3 module");

  if (_schema.empty())
  {
    return gearmand_gerror("missing required --libsqlite3-db=<dbfile> argument", GEARMAN_QUEUE_ERROR);
  }


  if (sqlite3_open(_schema.c_str(), &_db) != SQLITE_OK)
  {
    _error_string= "sqlite3_open failed with: ";
    _error_string+= sqlite3_errmsg(_db);
    return gearmand_gerror(_error_string.c_str(), GEARMAN_QUEUE_ERROR);
  }

  if (_db == NULL)
  {
    _error_string= "Unknown error while opening up sqlite file";
    return gearmand_gerror(_error_string.c_str(), GEARMAN_QUEUE_ERROR);
  }

  sqlite3_stmt* sth;
  if (_sqlite_query(gearman_literal_param("SELECT name FROM sqlite_master WHERE type='table'"), &sth) != SQLITE_OK)
  {
    _error_string= "Unknown error while calling SELECT on sqlite file ";
    _error_string+= _schema;
    _error_string+= " :";
    _error_string+= sqlite3_errmsg(_db);

    return gearmand_gerror(_error_string.c_str(), GEARMAN_QUEUE_ERROR);
  }

  bool found= false;
  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    char *table= NULL;

    if (sqlite3_column_type(sth, 0) == SQLITE_TEXT)
    {
      table= (char*)sqlite3_column_text(sth, 0);
    }
    else
    {
      std::string error_string("Column `name` from sqlite_master is not type TEXT");
      sqlite3_finalize(sth);
      return gearmand_gerror(error_string.c_str(), GEARMAN_QUEUE_ERROR);
    }

    if (_table.compare(table) == 0)
    {
      gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "sqlite module using table '%s'", table);
      found= true;
      break;
    }
  }

  if (sqlite3_finalize(sth) != SQLITE_OK)
  {
    return gearmand_gerror(sqlite3_errmsg(_db), GEARMAN_QUEUE_ERROR);
  }

  if (found == false)
  {
    std::string query("CREATE TABLE ");
    query+= _table;
    query+= " ( unique_key TEXT, function_name TEXT, priority INTEGER, data BLOB, when_to_run INTEGER, PRIMARY KEY (unique_key, function_name))";

    gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "sqlite module creating table '%s'", _table.c_str());

    if (_sqlite_query(query.c_str(), query.size(), &sth) != SQLITE_OK)
    {
      return gearmand_gerror(sqlite3_errmsg(_db), GEARMAN_QUEUE_ERROR);
    }

    if (sqlite3_step(sth) != SQLITE_DONE)
    {
      gearmand_gerror(sqlite3_errmsg(_db), GEARMAN_QUEUE_ERROR);
      sqlite3_finalize(sth);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_finalize(sth) != SQLITE_OK)
    {
      return gearmand_gerror(sqlite3_errmsg(_db), GEARMAN_QUEUE_ERROR);
    }
  }
  else
  {
    std::string query("SELECT when_to_run FROM ");
    query+= _table;

    if (_sqlite_query(query.c_str(), query.size(), &sth) != SQLITE_OK)
    {
      gearmand_info("No epoch support in sqlite queue");
      _epoch_support= false;
    }
  }

  return GEARMAN_SUCCESS;
}

int Instance::_sqlite_rollback()
{
  if (_in_trans == 0)
  {
    /* not in transaction */
    return SQLITE_OK;
  }

  sqlite3_stmt* sth;
  int ret= _sqlite_query(gearman_literal_param("ROLLBACK"), &sth);
  if (ret != SQLITE_OK)
  {
    gearmand_log_error("_sqlite_rollback",
                       "failed to rollback transaction: %s",
                       sqlite3_errmsg(_db));
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
                       sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return ret;
  }
  sqlite3_finalize(sth);
  _in_trans= 0;

  return SQLITE_OK;
}

gearmand_error_t Instance::add(gearman_server_st *server,
                                  const char *unique, size_t unique_size,
                                  const char *function_name,
                                  size_t function_name_size,
                                  const void *data, size_t data_size,
                                  gearman_job_priority_t priority,
                                  int64_t when)
{
  (void)server;
  sqlite3_stmt* sth;

  if (when and _epoch_support == false)
  {
    return gearmand_gerror("Table lacks when_to_run field", GEARMAN_QUEUE_ERROR);
  }

  if (unique_size > UINT32_MAX || function_name_size > UINT32_MAX ||
      data_size > UINT32_MAX)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "size too big [%u]", (uint32_t)unique_size);
    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite add: %.*s at %ld", (uint32_t)unique_size, (char *)unique, (long int)when);

  if (_sqlite_lock() !=  SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  if (_sqlite_query(insert_query().c_str(), insert_query().size(), &sth) != SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_int(sth,  1, priority) != SQLITE_OK)
  {
    _sqlite_rollback();
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "failed to bind int [%d]: %s", priority, sqlite3_errmsg(_db));
    sqlite3_finalize(sth);

    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 2, (const char *)unique, (int)unique_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback();
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "failed to bind text [%.*s]: %s", (uint32_t)unique_size, (char*)unique, sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_text(sth, 3, (const char *)function_name, (int)function_name_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback();
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "failed to bind text [%.*s]: %s", (uint32_t)function_name_size, (char*)function_name, sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_bind_blob(sth, 4, data, (int)data_size,
                        SQLITE_TRANSIENT) != SQLITE_OK)
  {
    _sqlite_rollback();
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "failed to bind blob: %s", sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  // epoch data
  if (sqlite3_bind_int64(sth,  5, when) != SQLITE_OK)
  {
    _sqlite_rollback();
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "failed to bind int64_t(%ld): %s", (long int)when, sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "insert error: %s", sqlite3_errmsg(_db));
    if (sqlite3_finalize(sth) != SQLITE_OK )
    {
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "finalize error: %s", sqlite3_errmsg(_db));
    }

    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                     "sqlite data: priority: %d, unique_key: %s, function_name: %s",
                     priority, (char*)unique, (char*)function_name);

  sqlite3_finalize(sth);

  if (_sqlite_commit() !=  SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  return GEARMAN_SUCCESS;
}

gearmand_error_t Instance::flush(gearman_server_st *server)
{
  (void)server;
  gearmand_debug("sqlite flush");

  return GEARMAN_SUCCESS;
}

gearmand_error_t Instance::done(gearman_server_st *server,
                                   const char *unique,
                                   size_t unique_size,
                                   const char *function_name,
                                   size_t function_name_size)
{
  (void)server;
  sqlite3_stmt* sth;

  if (unique_size > UINT32_MAX)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
                       "unique key size too big [%u]", (uint32_t)unique_size);
    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite done: %.*s", (uint32_t)unique_size, (char *)unique);

  if (_sqlite_lock() !=  SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  if (_sqlite_query(delete_query().c_str(), delete_query().size(), &sth) != SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  sqlite3_bind_text(sth, 1, (const char *)unique, (int)unique_size, SQLITE_TRANSIENT);
  sqlite3_bind_text(sth, 2, (const char *)function_name, (int)function_name_size, SQLITE_TRANSIENT);

  if (sqlite3_step(sth) != SQLITE_DONE)
  {
    gearmand_log_error("_sqlite_done", "delete error: %s",
                       sqlite3_errmsg(_db));
    sqlite3_finalize(sth);
    return GEARMAN_QUEUE_ERROR;
  }

  sqlite3_finalize(sth);

  if (_sqlite_commit() !=  SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  return GEARMAN_SUCCESS;
}

gearmand_error_t Instance::replay(gearman_server_st *server)
{
  gearmand_info("sqlite replay start");

  std::string query;
  if (_epoch_support)
  {
    query+= "SELECT unique_key,function_name,priority,data,when_to_run FROM ";
  }
  else
  {
    query+= "SELECT unique_key,function_name,priority,data FROM ";
  }
  query+= _table;

  sqlite3_stmt* sth;
  if (_sqlite_query(query.c_str(), query.size(), &sth) != SQLITE_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  while (sqlite3_step(sth) == SQLITE_ROW)
  {
    const char *unique, *function_name;
    std::vector<char> data;
    size_t unique_size, function_name_size;

    if (sqlite3_column_type(sth,0) == SQLITE_TEXT)
    {
      unique= (char *)sqlite3_column_text(sth,0);
      unique_size= (size_t) sqlite3_column_bytes(sth,0);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
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
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
                         "column %d is not type TEXT", 1);
      return GEARMAN_QUEUE_ERROR;
    }

    gearman_job_priority_t priority;
    if (sqlite3_column_type(sth,2) == SQLITE_INTEGER)
    {
      priority= (gearman_job_priority_t)sqlite3_column_int64(sth,2);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
                         "column %d is not type INTEGER", 2);
      return GEARMAN_QUEUE_ERROR;
    }

    if (sqlite3_column_type(sth,3) == SQLITE_BLOB)
    {
      size_t data_size= (size_t)sqlite3_column_bytes(sth,3);
      /* need to make a copy here ... gearman_server_job_free will free it later */
      try {
        data.resize(data_size);
      }
      catch (...)
      {
        sqlite3_finalize(sth);
        gearmand_perror("malloc");
        return GEARMAN_MEMORY_ALLOCATION_FAILURE;
      }
      memcpy(&data[0], sqlite3_column_blob(sth,3), data_size);
    }
    else
    {
      sqlite3_finalize(sth);
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "column %d is not type TEXT", 3);
      return GEARMAN_QUEUE_ERROR;
    }
    
    int64_t when;
    if (_epoch_support)
    {
      if (sqlite3_column_type(sth, 4) == SQLITE_INTEGER)
      {
        when= (int64_t)sqlite3_column_int64(sth, 4);
      }
      else
      {
        sqlite3_finalize(sth);
        gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "column %d is not type INTEGER", 3);
        return GEARMAN_QUEUE_ERROR;
      }
    }
    else
    {
      when= 0;
    }

    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "sqlite replay: %s", (char*)function_name);

    gearmand_error_t gret= add(server,
                               unique, unique_size,
                               function_name, function_name_size,
                               &data[0], data.size(),
                               priority, when);
    if (gearmand_failed(gret))
    {
      sqlite3_finalize(sth);
      return gret;
    }
  }

  sqlite3_finalize(sth);

  return GEARMAN_SUCCESS;
}

} // namespace queue
} // namespace gearmand


