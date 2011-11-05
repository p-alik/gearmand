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

#include <libgearman-server/common.h>
#include <libgearman-server/byte.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <libgearman-server/plugins/queue/drizzle/queue.h>
#include <libgearman-server/plugins/queue/base.h>
#include <libdrizzle-1.0/drizzle_client.h>

using namespace gearmand_internal;
using namespace gearmand;

namespace gearmand { namespace plugins { namespace queue { class Drizzle; }}}

static gearmand_error_t gearman_server_queue_libdrizzle_init(plugins::queue::Drizzle *queue, gearman_server_st *server);

/**
 * @addtogroup plugins::queue::Drizzleatic Static libdrizzle Queue Storage Definitions
 * @ingroup gearman_queue_libdrizzle
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE "gearman"
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE "queue"
#define GEARMAN_QUEUE_QUERY_BUFFER 256

#define libdrizzle_failed(X) bool(X != DRIZZLE_RETURN_OK)

namespace gearmand {
namespace plugins {
namespace queue {

class Drizzle : public gearmand::plugins::Queue {
public:
  Drizzle();
  ~Drizzle();

  gearmand_error_t initialize();

  drizzle_result_st *result()
  {
    return &_result;
  }

  void resize_query(size_t arg)
  {
    _query.resize(arg);
  }

  char *query_ptr()
  {
    return &_query[0];
  }

  void set_epoch_support(bool arg)
  {
    _epoch_support= arg;
  }

  bool epoch_support()
  {
    return _epoch_support;
  }

  drizzle_st *drizzle;
  drizzle_con_st *con;
  drizzle_result_st _result;
  std::vector<char> _query;
  std::string host;
  std::string username;
  std::string password;
  std::string uds;
  std::string user;
  std::string schema;
  std::string table;
  bool mysql_protocol;
  in_port_t port;

private:
  bool _epoch_support;
};


Drizzle::Drizzle () :
  Queue("libdrizzle"),
  drizzle(NULL),
  con(NULL),
  _result(),
  _query(),
  username(""),
  _epoch_support(true)
{
  command_line_options().add_options()
  ("libdrizzle-host", boost::program_options::value(&host)->default_value("localhost"), "Host of server.")
  ("libdrizzle-port", boost::program_options::value(&port)->default_value(4427), "Port of server. (by default Drizzle)")
  ("libdrizzle-uds", boost::program_options::value(&uds), "Unix domain socket for server.")
  ("libdrizzle-user", boost::program_options::value(&user)->default_value("root"), "User name for authentication.")
  ("libdrizzle-password", boost::program_options::value(&password), "Password for authentication.")
  ("libdrizzle-db", boost::program_options::value(&schema)->default_value(GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE), "Database to use.")
  ("libdrizzle-table", boost::program_options::value(&table)->default_value(GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE), "Table to use.")
  ("libdrizzle-mysql", boost::program_options::bool_switch(&mysql_protocol)->default_value(false), "Use MySQL protocol.")
  ;

  drizzle =drizzle_create(NULL);
  con= drizzle_con_create(drizzle, NULL);
}

Drizzle::~Drizzle()
{
  drizzle_con_free(con);
  drizzle_free(drizzle);
}

gearmand_error_t Drizzle::initialize()
{
  return gearman_server_queue_libdrizzle_init(this, &Gearmand()->server);
}

void initialize_drizzle()
{
  static Drizzle local_instance;
}

} // namespace queue
} // namespace plugins
} // namespace gearmand

/**
 * Query error handling function.
 */
static drizzle_return_t _libdrizzle_query(plugins::queue::Drizzle *queue,
                                          const char *query, size_t query_size);

/* Queue callback functions. */
static gearmand_error_t _libdrizzle_add(gearman_server_st *server,
                                        void *context,
                                        const char *unique, size_t unique_size,
                                        const char *function_name, size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearmand_job_priority_t priority,
                                        int64_t when);

static gearmand_error_t _libdrizzle_flush(gearman_server_st *gearman,
                                          void *context);

static gearmand_error_t _libdrizzle_done(gearman_server_st *gearman,
                                          void *context, const char *unique, size_t unique_size,
                                         const char *function_name, size_t function_name_size);

static gearmand_error_t _libdrizzle_replay(gearman_server_st *gearman,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context);

/** @} */

/*
 * Public definitions
 */

#pragma GCC diagnostic ignored "-Wold-style-cast"

gearmand_error_t gearman_server_queue_libdrizzle_init(plugins::queue::Drizzle *queue,  gearman_server_st *server)
{
  gearmand_info("Initializing libdrizzle module");

  if (drizzle_create(queue->drizzle) == NULL)
  {
    gearmand_error("drizzle_create");
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_con_create(queue->drizzle, queue->con) == NULL)
  {
    gearmand_error("drizzle_con_create");
    return GEARMAN_QUEUE_ERROR;
  }

  drizzle_con_set_db(queue->con, queue->schema.c_str());

  if (queue->mysql_protocol)
    drizzle_con_set_options(queue->con, DRIZZLE_CON_MYSQL);

  if (queue->uds.empty())
  {
    drizzle_con_set_tcp(queue->con, queue->host.c_str(), queue->port);
  }
  else
  {
    drizzle_con_set_uds(queue->con, queue->uds.c_str());
  }

  drizzle_con_set_auth(queue->con, queue->user.c_str(), queue->password.c_str());
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Using '%s' as the username", queue->user.c_str());

  std::string query;

  query+= "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = \"";
  query+= queue->schema;
  query+= "\"";
  if (libdrizzle_failed(_libdrizzle_query(queue, query.c_str(), query.size())))
  {
    return gearmand_gerror("Error occurred while searching for gearman queue schema", GEARMAN_QUEUE_ERROR);
  }

  if (libdrizzle_failed(drizzle_result_buffer(queue->result())))
  {
    return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
  }

  if (not drizzle_result_row_count(queue->result()))
  {
    return gearmand_gerror("Error occurred while search for gearman queue schema", GEARMAN_QUEUE_ERROR);
  }
  drizzle_result_free(queue->result());

  // We need to check and see if the tables exists, and if not create it
  query.clear();
  query+= "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = \"" +queue->table +"\"";
  if (libdrizzle_failed(_libdrizzle_query(queue, query.c_str(), query.size())))
  {
    return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
  }

  if (libdrizzle_failed(drizzle_result_buffer(queue->result())))
  {
    return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
  }
  bool create_table= drizzle_result_row_count(queue->result());
  drizzle_result_free(queue->result());

  if (not create_table)
  {
    gearmand_log_info("libdrizzle module creating table '%s.%s'",
                      drizzle_con_db(queue->con), queue->table.c_str());

    query.clear();

    query+= "CREATE TABLE " +queue->table + "( unique_key VARCHAR(" + TOSTRING(GEARMAN_UNIQUE_SIZE) + "),";
    query+= "function_name VARCHAR(255), priority INT, data LONGBLOB, when_to_run BIGINT, unique key (unique_key, function_name))";

    if (libdrizzle_failed(_libdrizzle_query(queue, query.c_str(), query.size())))
    {
      return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
    }

    if (libdrizzle_failed(drizzle_column_skip(queue->result())))
    {
      drizzle_result_free(queue->result());
      return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
    }
    drizzle_result_free(queue->result());
  }
  else
  {
    gearmand_log_info("libdrizzle module using table '%s.%s'",
                      drizzle_con_db(queue->con), queue->table.c_str());

    query.clear();
    query+= "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = \"" +queue->schema +"\" ";
    query+= "AND TABLE_NAME = \"" +queue->table +"\" AND COLUMN_NAME = \"when_to_run\"";
    if (libdrizzle_failed(_libdrizzle_query(queue, query.c_str(), query.size())))
    {
      return GEARMAN_QUEUE_ERROR;
    }

    if (libdrizzle_failed(drizzle_result_buffer(queue->result())))
    {
      return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
    }

    if (not drizzle_result_row_count(queue->result()))
    {
      gearmand_info("Current schema does not have when_to_run column");
      queue->set_epoch_support(false);
    }
    drizzle_result_free(queue->result());
  }

  gearman_server_set_queue(server, queue, _libdrizzle_add, _libdrizzle_flush, _libdrizzle_done, _libdrizzle_replay);

  return GEARMAN_SUCCESS;
}

/*
 * Static definitions
 */

static drizzle_return_t _libdrizzle_query(plugins::queue::Drizzle *queue,
                                          const char *query, size_t query_size)
{
  drizzle_return_t ret;

  gearmand_log_crazy(GEARMAN_DEFAULT_LOG_PARAM, "libdrizzle query: %.*s", (uint32_t)query_size, query);

  drizzle_result_st *result= drizzle_query(queue->con, queue->result(), query, query_size, &ret);
  if (ret != DRIZZLE_RETURN_OK)
  {

    if (ret == DRIZZLE_RETURN_COULD_NOT_CONNECT)
    {
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
			 "Failed to connect to database instance. host: %s:%d user: %s schema: %s (%s)", 
                         drizzle_con_host(queue->con),
                         (int)(drizzle_con_port(queue->con)),
                         drizzle_con_user(queue->con),
                         drizzle_con_db(queue->con),
                         drizzle_error(queue->drizzle));
    }
    else
    {
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM,
			 "libdrizled error '%s' executing '%.*s'",
                         drizzle_error(queue->drizzle),
                         query_size, query);
    }

    return ret;
  }
  (void)result;

  return DRIZZLE_RETURN_OK;
}

static gearmand_error_t _libdrizzle_add(gearman_server_st *,
                                        void *context,
                                        const char *unique, size_t unique_size,
                                        const char *function_name, size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearmand_job_priority_t priority,
                                        int64_t when)
{
  plugins::queue::Drizzle *queue= (plugins::queue::Drizzle *)context;
  Byte query;

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libdrizzle add: %.*s", (uint32_t)unique_size, (char *)unique);

  if (when and not queue->epoch_support())
  {
    return gearmand_gerror("Table lacks when_to_run field", GEARMAN_QUEUE_ERROR);
  }

  // @todo test for overallocation failure
  query.resize(((unique_size + function_name_size + data_size) * 2) + GEARMAN_QUEUE_QUERY_BUFFER);

  size_t query_size;

  if (queue->epoch_support())
  {
    query_size= (size_t)snprintf(query.c_str(), query.size(),
                                 "INSERT INTO %s SET priority=%u,when_to_run=%lld,unique_key='",
                                 queue->table.c_str(), (uint32_t)priority,
                                 (long long unsigned int)when);
  }
  else
  {
    query_size= (size_t)snprintf(query.c_str(), query.size(),
                                 "INSERT INTO %s SET priority=%u,unique_key='",
                                 queue->table.c_str(), (uint32_t)priority);
  }

  query_size+= (size_t)drizzle_escape_string((char *)(query.c_str() + query_size), (const char *)unique, unique_size);
  memcpy(query.c_str() + query_size, "',function_name='", 17);
  query_size+= 17;

  query_size+= (size_t)drizzle_escape_string(query.c_str() + query_size, (const char *)function_name,
                                             function_name_size);
  memcpy(query.c_str() + query_size, "',data='", 8);
  query_size+= 8;

  query_size+= (size_t)drizzle_escape_string(query.c_str() + query_size, (const char *)data,
                                             data_size);
  memcpy(query.c_str() + query_size, "'", 1);
  query_size+= 1;

  gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "%.*s", query_size, query.c_str());



  if (_libdrizzle_query(queue, query.c_str(), query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(queue->result());

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libdrizzle_flush(gearman_server_st *, void *)
{
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libdrizzle flush");

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libdrizzle_done(gearman_server_st *,
                                         void *context,
                                         const char *unique, size_t unique_size,
                                         const char *function_name, size_t function_name_size)
{
  plugins::queue::Drizzle *queue= (plugins::queue::Drizzle *)context;
  Byte escaped_function_name(function_name_size*2);
  Byte escaped_unique_name(unique_size*2);
  Byte query(function_name_size*2 + unique_size*2 +queue->table.size() +GEARMAN_QUEUE_QUERY_BUFFER);

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
		     "libdrizzle done: %.*s(%.*s)", 
                     uint32_t(function_name_size), (char *)function_name,
                     uint32_t(unique_size), (char *)unique);

  (void)drizzle_escape_string(escaped_unique_name.c_str(), (const char *)unique, unique_size);
  (void)drizzle_escape_string(escaped_function_name.c_str(), (const char *)function_name, function_name_size);

  ssize_t query_size= (ssize_t)snprintf(query.c_str(), query.size(),
                                       "DELETE FROM %s WHERE unique_key='%s' and function_name= '%s'",
                                       queue->table.c_str(),
                                       escaped_unique_name.c_str(),
                                       escaped_function_name.c_str());

  if (query_size < 0 || size_t(query_size) > query.size())
  {
    return gearmand_gerror("snprintf(DELETE)", GEARMAN_MEMORY_ALLOCATION_FAILURE);
  }

  gearmand_log_crazy(GEARMAN_DEFAULT_LOG_PARAM, "%.*", size_t(query_size), query.c_str());
  
  if (_libdrizzle_query(queue, query.c_str(), (size_t)query_size) != DRIZZLE_RETURN_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  drizzle_result_free(queue->result());

  return GEARMAN_SUCCESS;
}

#if 0
static gearmand_error_t _dump_queue(plugins::queue::Drizzle *queue)
{
  char query[DRIZZLE_MAX_TABLE_SIZE + GEARMAN_QUEUE_QUERY_BUFFER];

  size_t query_size;
  query_size= (size_t)snprintf(query, sizeof(query),
                               "SELECT unique_key,function_name FROM %s",
                               queue->table.c_str());

  if (_libdrizzle_query(NULL, queue, query, query_size) != DRIZZLE_RETURN_OK)
  {
    gearmand_log_error("drizzle_column_skip:%s", drizzle_error(queue->drizzle));
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_column_skip(queue->result()) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(queue->result());
    gearmand_log_error("drizzle_column_skip:%s", drizzle_error(queue->drizzle));

    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_debug("Shutting down with the following items left to be processed.");
  while (1)
  {
    drizzle_return_t ret;
    drizzle_row_t row= drizzle_row_buffer(queue->result(), &ret);

    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(queue->result());
      gearmand_log_error("drizzle_row_buffer:%s", drizzle_error(queue->drizzle));

      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    size_t *field_sizes;
    field_sizes= drizzle_row_field_sizes(queue->result());

    gearmand_log_debug("\t unique: %.*s function: %.*s",
                       (uint32_t)field_sizes[0], row[0],
                       (uint32_t)field_sizes[1], row[1]
                       );

    drizzle_row_free(queue->result(), row);
  }

  drizzle_result_free(queue->result());

  return GEARMAN_SUCCESS;
}
#endif

static gearmand_error_t _libdrizzle_replay(gearman_server_st *server,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context)
{
  plugins::queue::Drizzle *queue= (plugins::queue::Drizzle *)context;
  size_t *field_sizes;
  gearmand_error_t gret;

  gearmand_info("libdrizzle replay start");

  std::string query;
  if (queue->epoch_support())
  {
    query+= "SELECT unique_key,function_name,priority,data,when_to_run FROM " + queue->table;
  }
  else
  {
    query+= "SELECT unique_key,function_name,priority,data FROM " +queue->table;
  }

  if (libdrizzle_failed(_libdrizzle_query(queue, query.c_str(), query.size())))
  {
    return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
  }

  if (drizzle_column_skip(queue->result()) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(queue->result());
    return gearmand_gerror(drizzle_error(queue->drizzle), GEARMAN_QUEUE_ERROR);
  }

  while (1)
  {
    drizzle_return_t ret;
    drizzle_row_t row= drizzle_row_buffer(queue->result(), &ret);

    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(queue->result());
      gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "drizzle_row_buffer:%s", drizzle_error(queue->drizzle));

      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    field_sizes= drizzle_row_field_sizes(queue->result());

    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libdrizzle replay: %.*s", (uint32_t)field_sizes[0], row[0]);

    size_t data_size= field_sizes[3];
    /* need to make a copy here ... gearman_server_job_free will free it later */
    char *data= (char *)malloc(data_size);
    if (data == NULL)
    {
      gearmand_perror("Failed to allocate data while replaying the queue");
      drizzle_row_free(queue->result(), row);
      drizzle_result_free(queue->result());
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }
    memcpy(data, row[3], data_size);

    int64_t when;
    if (not queue->epoch_support())
    {
      when= 0;
    }
    else
    {
      when= atoi(row[4]);
    }

    gret= (*add_fn)(server, add_context, row[0], field_sizes[0],
                    row[1], field_sizes[1],
                    data, data_size, (gearmand_job_priority_t)atoi(row[2]), when);

    if (gret != GEARMAN_SUCCESS)
    {
      drizzle_row_free(queue->result(), row);
      drizzle_result_free(queue->result());
      return gret;
    }

    drizzle_row_free(queue->result(), row);
  }

  drizzle_result_free(queue->result());

  return GEARMAN_SUCCESS;
}
