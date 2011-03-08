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

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <libgearman-server/plugins/queue/drizzle/queue.h>
#include <libdrizzle/drizzle_client.h>


/**
 * @addtogroup gearman_queue_libdrizzle_static Static libdrizzle Queue Storage Definitions
 * @ingroup gearman_queue_libdrizzle
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE "gearman"
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
static gearmand_error_t _libdrizzle_add(gearman_server_st *server,
                                        void *context, const void *unique,
                                        size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearmand_job_priority_t priority);
static gearmand_error_t _libdrizzle_flush(gearman_server_st *gearman,
                                          void *context);
static gearmand_error_t _libdrizzle_done(gearman_server_st *gearman,
                                          void *context, const void *unique,
                                         size_t unique_size,
                                         const void *function_name,
                                         size_t function_name_size);
static gearmand_error_t _libdrizzle_replay(gearman_server_st *gearman,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context);

static gearmand_error_t _dump_queue(gearman_queue_libdrizzle_st *queue);

/** @} */

/*
 * Public definitions
 */

gearmand_error_t gearman_server_queue_libdrizzle_conf(gearman_conf_st *conf)
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

gearmand_error_t gearman_server_queue_libdrizzle_init(gearman_server_st *server,
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

  gearmand_log_info("Initializing libdrizzle module");

  queue= (gearman_queue_libdrizzle_st *)malloc(sizeof(gearman_queue_libdrizzle_st));
  if (queue == NULL)
  {
    gearmand_perror("malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  memset(queue, 0, sizeof(gearman_queue_libdrizzle_st));
  snprintf(queue->table, DRIZZLE_MAX_TABLE_SIZE,
           GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_TABLE);

  if (drizzle_create(&(queue->drizzle)) == NULL)
  {
    free(queue);
    gearmand_error("drizzle_create");
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_con_create(&(queue->drizzle), &(queue->con)) == NULL)
  {
    drizzle_free(&(queue->drizzle));
    free(queue);
    gearmand_error("drizzle_con_create");
    return GEARMAN_QUEUE_ERROR;
  }

  gearman_server_set_queue(server, queue, _libdrizzle_add, _libdrizzle_flush, _libdrizzle_done, _libdrizzle_replay);

  drizzle_con_set_db(&(queue->con), GEARMAN_QUEUE_LIBDRIZZLE_DEFAULT_DATABASE);

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libdrizzle");
  if (module == NULL)
  {
    gearmand_error("gearman_conf_module_find(NULL)");
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
      gearmand_log_error("gearman_queue_libdrizzle_init", "Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  if (uds == NULL)
  {
    drizzle_con_set_tcp(&(queue->con), host, port);
  }
  else
  {
    drizzle_con_set_uds(&(queue->con), uds);
  }

  struct passwd password_buffer, *pwd;
  if (! user)
  {
    long buffer_length= sysconf(_SC_GETPW_R_SIZE_MAX);

    if (buffer_length)
    {
      uid_t user_id= getuid();
      char buffer[buffer_length]; 

      getpwuid_r(user_id, &password_buffer, buffer, (size_t)buffer_length, &pwd);

      if (pwd == NULL)
      {
        gearmand_error("Failed to find the username of the current process, please specify database user");
        return GEARMAN_QUEUE_ERROR;
      }
    }
    else
    {
      gearmand_error("Failed to find the username of the current process, please specify database user");
      return GEARMAN_QUEUE_ERROR;
    }

    user= pwd->pw_name;
  }
  drizzle_con_set_auth(&(queue->con), user, password);
  gearmand_log_debug("Using '%s' as the username", user);

  /* Overwrite password string so it does not appear in 'ps' output. */
  if (password != NULL)
  {
    memset((void *)password, 'x', strlen(password));
  }

  if (_libdrizzle_query(server, queue, STRING_WITH_LEN("SHOW TABLES")) != DRIZZLE_RETURN_OK)
  {
    gearman_server_queue_libdrizzle_deinit(server);
    gearmand_error("SHOW TABLES failed");
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_result_buffer(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    gearman_server_queue_libdrizzle_deinit(server);
    gearmand_error(drizzle_error(&(queue->drizzle)));
    return GEARMAN_QUEUE_ERROR;
  }

  while ((row= drizzle_row_next(&(queue->result))) != NULL)
  {
    if (!strcasecmp(queue->table, row[0]))
    {
      gearmand_log_info("libdrizzle module using table '%s.%s'", drizzle_con_db(&queue->con), row[0]);

      break;
    }
  }

  drizzle_result_free(&(queue->result));

  if (row == NULL)
  {
    snprintf(create, sizeof(create),
             "CREATE TABLE %s"
             "("
               "unique_key VARCHAR(%d),"
               "function_name VARCHAR(255),"
               "priority INT,"
               "data LONGBLOB,"
               "unique key (unique_key, function_name)"
             ")",
             queue->table, GEARMAN_UNIQUE_SIZE);

    gearmand_log_info("libdrizzle module creating table '%s.%s'",
                      drizzle_con_db(&queue->con), queue->table);

    if (_libdrizzle_query(server, queue, create, strlen(create))
        != DRIZZLE_RETURN_OK)
    {
      gearman_server_queue_libdrizzle_deinit(server);
      return GEARMAN_QUEUE_ERROR;
    }

    drizzle_result_free(&(queue->result));
  }

  return GEARMAN_SUCCESS;
}

gearmand_error_t
gearman_server_queue_libdrizzle_deinit(gearman_server_st *server)
{
  gearman_queue_libdrizzle_st *queue;
  queue= (gearman_queue_libdrizzle_st *)gearman_server_queue_context(server);

  gearmand_log_info("Shutting down libdrizzle queue module");

  _dump_queue(queue);

  gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
  drizzle_con_free(&(queue->con));
  drizzle_free(&(queue->drizzle));
  if (queue->query != NULL)
    free(queue->query);
  free(queue);

  return GEARMAN_SUCCESS;
}

gearmand_error_t gearmand_queue_libdrizzle_init(gearmand_st *gearmand,
                                                gearman_conf_st *conf)
{
  return gearman_server_queue_libdrizzle_init(gearmand_server(gearmand), conf);
}

gearmand_error_t gearmand_queue_libdrizzle_deinit(gearmand_st *gearmand)
{
  return gearman_server_queue_libdrizzle_deinit(gearmand_server(gearmand));
}

/*
 * Static definitions
 */

static drizzle_return_t _libdrizzle_query(gearman_server_st *server,
                                          gearman_queue_libdrizzle_st *queue,
                                          const char *query, size_t query_size)
{
  (void)server;
  drizzle_return_t ret;

  gearmand_log_crazy("libdrizzle query: %.*s", (uint32_t)query_size, query);

  (void)drizzle_query(&(queue->con), &(queue->result), query, query_size, &ret);
  if (ret != DRIZZLE_RETURN_OK)
  {

    if (ret == DRIZZLE_RETURN_COULD_NOT_CONNECT)
    {
      gearmand_log_error("Failed to connect to database instance. host: %s:%d user: %s schema: %s", 
                         drizzle_con_host(&(queue->con)),
                         (int)(drizzle_con_port(&(queue->con))),
                         drizzle_con_user(&(queue->con)),
                         drizzle_con_db(&(queue->con)));
    }
    else
    {
      gearmand_log_error("libdrizled error '%s' executing '%.*s'",
                         drizzle_error(&(queue->drizzle)),
                         query_size, query);
    }

    return ret;
  }

  return DRIZZLE_RETURN_OK;
}

static gearmand_error_t _libdrizzle_add(gearman_server_st *server,
                                        void *context, const void *unique,
                                        size_t unique_size,
                                        const void *function_name,
                                        size_t function_name_size,
                                        const void *data, size_t data_size,
                                        gearmand_job_priority_t priority)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char *query;
  size_t query_size;

  gearmand_log_debug("libdrizzle add: %.*s", (uint32_t)unique_size, (char *)unique);

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
      gearmand_log_error("_libdrizzle_add", "realloc");
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

  gearmand_log_info("%.*s", query_size, query);



  if (_libdrizzle_query(server, queue, query, query_size) != DRIZZLE_RETURN_OK)
    return GEARMAN_QUEUE_ERROR;

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libdrizzle_flush(gearman_server_st *server,
                                          void *context __attribute__((unused)))
{
  (void)server;
  gearmand_log_debug("libdrizzle flush");

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libdrizzle_done(gearman_server_st *server,
                                         void *context, const void *unique,
                                         size_t unique_size,
                                         const void *function_name,
                                         size_t function_name_size)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char escaped_function_name[function_name_size*2];
  char escaped_unique_name[unique_size*2];
  char query[unique_size*2 + unique_size*2 + GEARMAN_QUEUE_QUERY_BUFFER];

  gearmand_log_debug("libdrizzle done: %.*s", (uint32_t)unique_size, (char *)unique);

  size_t escaped_unique_name_size= drizzle_escape_string(escaped_unique_name, unique, unique_size);
  size_t escaped_function_name_size= drizzle_escape_string(escaped_function_name, function_name, function_name_size);
  (void)escaped_unique_name_size;
  (void)escaped_function_name_size;

  ssize_t query_size= (ssize_t)snprintf(query, sizeof(query),
                                       "DELETE FROM %s WHERE unique_key='%s' and function_name= '%s'",
                                       queue->table,
                                       escaped_unique_name,
                                       escaped_function_name);

  if (query_size < 0 || query_size > (ssize_t)sizeof(query))
  {
    gearmand_perror("snprintf(DELETE)");
    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_log_info("%.*", (size_t)query_size, query);
  
  if (_libdrizzle_query(server, queue, query, (size_t)query_size) != DRIZZLE_RETURN_OK)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _dump_queue(gearman_queue_libdrizzle_st *queue)
{
  char query[DRIZZLE_MAX_TABLE_SIZE + GEARMAN_QUEUE_QUERY_BUFFER];

  size_t query_size;
  query_size= (size_t)snprintf(query, sizeof(query),
                               "SELECT unique_key,function_name FROM %s",
                               queue->table);

  if (_libdrizzle_query(NULL, queue, query, query_size) != DRIZZLE_RETURN_OK)
  {
    gearmand_log_error("drizzle_column_skip:%s", drizzle_error(&(queue->drizzle)));
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_column_skip(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    gearmand_log_error("drizzle_column_skip:%s", drizzle_error(&(queue->drizzle)));

    return GEARMAN_QUEUE_ERROR;
  }

  gearmand_debug("Shutting down with the following items left to be processed.");
  while (1)
  {
    drizzle_return_t ret;
    drizzle_row_t row= drizzle_row_buffer(&(queue->result), &ret);

    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(&(queue->result));
      gearmand_log_error("drizzle_row_buffer:%s", drizzle_error(&(queue->drizzle)));

      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    size_t *field_sizes;
    field_sizes= drizzle_row_field_sizes(&(queue->result));

    gearmand_log_debug("\t unique: %.*s function: %.*s",
                       (uint32_t)field_sizes[0], row[0],
                       (uint32_t)field_sizes[1], row[1]
                       );

    drizzle_row_free(&(queue->result), row);
  }

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libdrizzle_replay(gearman_server_st *server,
                                           void *context,
                                           gearman_queue_add_fn *add_fn,
                                           void *add_context)
{
  gearman_queue_libdrizzle_st *queue= (gearman_queue_libdrizzle_st *)context;
  char query[DRIZZLE_MAX_TABLE_SIZE + GEARMAN_QUEUE_QUERY_BUFFER];
  size_t query_size;
  drizzle_return_t ret;
  drizzle_row_t row;
  size_t *field_sizes;
  gearmand_error_t gret;

  gearmand_log_info("libdrizzle replay start");

  query_size= (size_t)snprintf(query, sizeof(query),
                               "SELECT unique_key,function_name,priority,data FROM %s",
                               queue->table);

  if (_libdrizzle_query(server, queue, query, query_size) != DRIZZLE_RETURN_OK)
  {
    gearmand_log_error("_libdrizzle_replay", "drizzle_column_skip:%s", drizzle_error(&(queue->drizzle)));
    return GEARMAN_QUEUE_ERROR;
  }

  if (drizzle_column_skip(&(queue->result)) != DRIZZLE_RETURN_OK)
  {
    drizzle_result_free(&(queue->result));
    gearmand_log_error("_libdrizzle_replay", "drizzle_column_skip:%s", drizzle_error(&(queue->drizzle)));

    return GEARMAN_QUEUE_ERROR;
  }

  while (1)
  {
    row= drizzle_row_buffer(&(queue->result), &ret);
    if (ret != DRIZZLE_RETURN_OK)
    {
      drizzle_result_free(&(queue->result));
      gearmand_log_error("_libdrizzle_replay", "drizzle_row_buffer:%s", drizzle_error(&(queue->drizzle)));

      return GEARMAN_QUEUE_ERROR;
    }

    if (row == NULL)
      break;

    field_sizes= drizzle_row_field_sizes(&(queue->result));

    gearmand_log_debug("libdrizzle replay: %.*s", (uint32_t)field_sizes[0], row[0]);

    gret= (*add_fn)(server, add_context, row[0], field_sizes[0], row[1],
                    field_sizes[1], row[3], field_sizes[3], atoi(row[2]));
    if (gret != GEARMAN_SUCCESS)
    {
      drizzle_row_free(&(queue->result), row);
      drizzle_result_free(&(queue->result));
      return gret;
    }

    drizzle_row_free(&(queue->result), row);
  }

  drizzle_result_free(&(queue->result));

  return GEARMAN_SUCCESS;
}
