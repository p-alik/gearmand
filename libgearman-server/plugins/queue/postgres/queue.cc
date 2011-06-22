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

#include <libgearman-server/common.h>
#include <libgearman-server/byte.h>

#include <libgearman-server/plugins/queue/postgres/queue.h>
#include <libgearman-server/plugins/queue/base.h>

#if defined(HAVE_LIBPQ_FE_H)
# include <libpq-fe.h>
# include <pg_config_manual.h>
#else
# include <postgresql/libpq-fe.h>
# include <postgresql/pg_config_manual.h>
#endif

/**
 * @addtogroup plugins::queue::Postgresatic Static libpq Queue Storage Definitions
 * @ingroup gearman_queue_libpq
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBPQ_DEFAULT_TABLE "queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

namespace gearmand { namespace plugins { namespace  queue { class Postgres; }}}

static gearmand_error_t _initialize(gearman_server_st *server, gearmand::plugins::queue::Postgres *queue);

namespace gearmand {
namespace plugins {
namespace queue {

class Postgres : public gearmand::plugins::Queue {
public:
  Postgres();
  ~Postgres();

  gearmand_error_t initialize();

  const std::string &insert()
  {
    return _insert_query;
  }

  const std::string &select()
  {
    return _select_query;
  }

  const std::string &create()
  {
    return _create_query;
  }

  PGconn *con;
  std::string postgres_connect_string;
  std::string table;
  std::vector<char> query_buffer;

public:
  std::string _insert_query;
  std::string _select_query;
  std::string _create_query;
};

Postgres::Postgres() :
  Queue("Postgres"),
  con(NULL),
  postgres_connect_string(""),
  table(""),
  query_buffer()
{
  command_line_options().add_options()
    ("libpq-conninfo", boost::program_options::value(&postgres_connect_string)->default_value(""), "PostgreSQL connection information string.")
    ("libpq-table", boost::program_options::value(&table)->default_value(GEARMAN_QUEUE_LIBPQ_DEFAULT_TABLE), "Table to use.");
}

Postgres::~Postgres ()
{
  if (con)
    PQfinish(con);
}

gearmand_error_t Postgres::initialize()
{
  _create_query+= "CREATE TABLE " +table +" (unique_key VARCHAR" +"(" + TOSTRING(GEARMAN_UNIQUE_SIZE) +"), ";
  _create_query+= "function_name VARCHAR(255), priority INTEGER, data BYTEA, when_to_run INTEGER, UNIQUE (unique_key, function_name))";

  gearmand_error_t ret= _initialize(&Gearmand()->server, this);

  _insert_query+= "INSERT INTO " +table +" (priority, unique_key, function_name, data, when_to_run) VALUES($1,$2,$3,$4::BYTEA,$5)";

  _select_query+= "SELECT unique_key,function_name,priority,data,when_to_run FROM " +table;

  return ret;
}

void initialize_postgres()
{
  static Postgres local_instance;
}

} // namespace queue
} // namespace plugins
} // namespace gearmand

/**
 * PostgreSQL notification callback.
 */
static void _libpq_notice_processor(void *arg, const char *message);

/* Queue callback functions. */
static gearmand_error_t _libpq_add(gearman_server_st *server, void *context,
                                   const char *unique, size_t unique_size,
                                   const char *function_name,
                                   size_t function_name_size,
                                   const void *data, size_t data_size,
                                   gearmand_job_priority_t priority,
                                   int64_t when);

static gearmand_error_t _libpq_flush(gearman_server_st *server, void *context);

static gearmand_error_t _libpq_done(gearman_server_st *server, void *context,
                                    const char *unique,
                                    size_t unique_size,
                                    const char *function_name,
                                    size_t function_name_size);

static gearmand_error_t _libpq_replay(gearman_server_st *server, void *context,
                                      gearman_queue_add_fn *add_fn,
                                      void *add_context);

/** @} */

/*
 * Public definitions
 */

#pragma GCC diagnostic ignored "-Wold-style-cast"

gearmand_error_t _initialize(gearman_server_st *server,
                             gearmand::plugins::queue::Postgres *queue)
{
  PGresult *result;

  gearmand_info("Initializing libpq module");

  gearman_server_set_queue(server, queue, _libpq_add, _libpq_flush, _libpq_done, _libpq_replay);

  queue->con= PQconnectdb(queue->postgres_connect_string.c_str());

  if (queue->con == NULL || PQstatus(queue->con) != CONNECTION_OK)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "PQconnectdb: %s", PQerrorMessage(queue->con));
    gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
    return GEARMAN_QUEUE_ERROR;
  }

  (void)PQsetNoticeProcessor(queue->con, _libpq_notice_processor, server);

  std::string query("SELECT tablename FROM pg_tables WHERE tablename='" +queue->table + "'");

  result= PQexec(queue->con, query.c_str());
  if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    std::string error_string= "PQexec:";
    error_string+= PQerrorMessage(queue->con);
    gearmand_gerror(error_string.c_str(), GEARMAN_QUEUE_ERROR);
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  if (PQntuples(result) == 0)
  {
    PQclear(result);

    gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "libpq module creating table '%s'", queue->table.c_str());

    result= PQexec(queue->con, queue->create().c_str());
    if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, 
			 "PQexec:%s", PQerrorMessage(queue->con));
      PQclear(result);
      gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
      return GEARMAN_QUEUE_ERROR;
    }

    PQclear(result);
  }
  else
  {
    PQclear(result);
  }

  return GEARMAN_SUCCESS;
}

/*
 * Static definitions
 */

static void _libpq_notice_processor(void *arg, const char *message)
{
  (void)arg;
  gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "PostgreSQL %s", message);
}

static gearmand_error_t _libpq_add(gearman_server_st *server, void *context,
                                        const char *unique, size_t unique_size,
                                        const char *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearmand_job_priority_t priority,
                                        int64_t when)
{
  (void)server;
  (void)when;
  gearmand::plugins::queue::Postgres *queue= (gearmand::plugins::queue::Postgres *)context;

  char buffer[22];
  snprintf(buffer, sizeof(buffer), "%u", static_cast<uint32_t>(priority));

  const char *param_values[]= {
    (char *)buffer,
    (char *)unique,
    (char *)function_name,
    (char *)data,
    (char *)when };

  int param_lengths[]= { 
    (int)strlen(buffer),
    (int)unique_size,
    (int)function_name_size,
    (int)data_size,
    (int)when };

  int param_formats[] = { 0, 0, 0, 1, 0 };

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libpq add: %.*s", (uint32_t)unique_size, (char *)unique);

  PGresult *result= PQexecParams(queue->con, queue->insert().c_str(), 
                                 gearmand_array_size(param_lengths),
                                 NULL, param_values, param_lengths, param_formats, 0);
  if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "PQexec:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libpq_flush(gearman_server_st *, void *)
{
  gearmand_debug("libpq flush");

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libpq_done(gearman_server_st *server, void *context,
                                    const char *unique,
                                    size_t unique_size,
                                    const char *function_name,
                                    size_t function_name_size)
{
  (void)server;
  (void)function_name_size;
  gearmand::plugins::queue::Postgres *queue= (gearmand::plugins::queue::Postgres *)context;
  PGresult *result;

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libpq done: %.*s", (uint32_t)unique_size, (char *)unique);

  std::string query;
  query+= "DELETE FROM ";
  query+= queue->table;
  query+= " WHERE unique_key='";
  query+= (const char *)unique;
  query+= "' AND function_name='";
  query+= (const char *)function_name;
  query+= "'";

  result= PQexec(queue->con, query.c_str());
  if (result == NULL || PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "PQexec:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libpq_replay(gearman_server_st *server, void *context,
                                      gearman_queue_add_fn *add_fn,
                                      void *add_context)
{
  gearmand::plugins::queue::Postgres *queue= (gearmand::plugins::queue::Postgres *)context;

  gearmand_info("libpq replay start");

  std::string query("SELECT unique_key,function_name,priority,data,when_to_run FROM " + queue->table);

  PGresult *result= PQexecParams(queue->con, query.c_str(), 0, NULL, NULL, NULL, NULL, 1);
  if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "PQexecParams:%s", PQerrorMessage(queue->con));
    PQclear(result);
    return GEARMAN_QUEUE_ERROR;
  }

  for (int row= 0; row < PQntuples(result); row++)
  {
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                       "libpq replay: %.*s",
                       PQgetlength(result, row, 0),
                       PQgetvalue(result, row, 0));

    size_t data_length;
    char *data;
    if (PQgetlength(result, row, 3) == 0)
    {
      data= NULL;
      data_length= 0;
    }
    else
    {
      data_length= size_t(PQgetlength(result, row, 3));
      data= (char *)malloc(data_length);
      if (not data)
      {
        PQclear(result);
        return gearmand_perror("malloc");
      }

      memcpy(data, PQgetvalue(result, row, 3), data_length);
    }

    gearmand_error_t ret;
    ret= (*add_fn)(server, add_context, PQgetvalue(result, row, 0),
                   (size_t)PQgetlength(result, row, 0),
                   PQgetvalue(result, row, 1),
                   (size_t)PQgetlength(result, row, 1),
                   data, data_length,
                   (gearmand_job_priority_t)atoi(PQgetvalue(result, row, 2)),
                   atoll(PQgetvalue(result, row, 4)));
    if (gearmand_failed(ret))
    {
      PQclear(result);
      return ret;
    }
  }

  PQclear(result);

  return GEARMAN_SUCCESS;
}
