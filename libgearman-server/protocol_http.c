/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief HTTP Protocol Definitions
 */

#include "common.h"
#include "gearmand.h"

#include <libgearman-server/protocol_http.h>

/**
 * @addtogroup gearman_protocol_http_static Static HTTP Protocol Definitions
 * @ingroup gearman_protocol_http
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT "8080"

/**
 * Structure for HTTP specific data.
 */
typedef struct
{
  bool background;
  bool keep_alive;
} gearman_protocol_http_st;

/* Protocol callback functions. */
static gearman_return_t _http_con_add(gearman_server_con_st *connection);
static void _http_free(gearman_server_con_st *connection, void *context);
static size_t _http_pack(const gearmand_packet_st *packet, gearman_server_con_st *connection,
                         void *data, size_t data_size,
                         gearman_return_t *ret_ptr);
static size_t _http_unpack(gearmand_packet_st *packet, gearman_server_con_st *connection,
                           const void *data, size_t data_size,
                           gearman_return_t *ret_ptr);

/* Line parsing helper function. */
static const char *_http_line(const void *data, size_t data_size,
                              size_t *line_size, size_t *offset);

/** @} */

/*
 * Public definitions
 */

gearman_return_t gearmand_protocol_http_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "http");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  gearman_conf_module_add_option(module, "port", 0, "PORT",
                                 "Port to listen on.");

  return gearman_conf_return(conf);
}

gearman_return_t gearmand_protocol_http_init(gearmand_st *gearmand,
                                             gearman_conf_st *conf)
{
  gearman_conf_module_st *module;
  const char *name;
  const char *value;

  char port[NI_MAXSERV];
  strncpy(port, GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT, NI_MAXSERV);

  gearmand_log_info("Initializing http module");

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "http");
  if (module == NULL)
  {
    gearmand_fatal("gearman_conf_module_find(NULL)");
    return GEARMAN_QUEUE_ERROR;
  }

  while (gearman_conf_module_value(module, &name, &value))
  {
    if (!strcmp(name, "port"))
    {
      strncpy(port, value, NI_MAXSERV);
    }
    else
    {
      gearmand_protocol_http_deinit(gearmand);
      gearmand_log_fatal("gearman_protocol_http_init:Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }

  return gearmand_port_add(gearmand, port, _http_con_add);
}

gearman_return_t gearmand_protocol_http_deinit(gearmand_st *gearmand __attribute__ ((unused)))
{
  return GEARMAN_SUCCESS;
}

/*
 * Static definitions
 */

static gearman_return_t _http_con_add(gearman_server_con_st *connection)
{
  gearman_protocol_http_st *http;

  http= (gearman_protocol_http_st *)malloc(sizeof(gearman_protocol_http_st));
  if (http == NULL)
  {
    gearmand_log_error("_http_con_add", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  http->background= false;
  http->keep_alive= false;

  gearmand_connection_set_protocol(connection, http, _http_free, _http_pack, _http_unpack);

  return GEARMAN_SUCCESS;
}

static void _http_free(gearman_server_con_st *connection __attribute__ ((unused)),
                       void *context)
{
  free((gearman_protocol_http_st *)context);
}

static size_t _http_pack(const gearmand_packet_st *packet, gearman_server_con_st *connection,
                         void *data, size_t data_size,
                         gearman_return_t *ret_ptr)
{
  size_t pack_size;
  gearman_protocol_http_st *http;

  http= (gearman_protocol_http_st *)gearmand_connection_protocol_context(connection);

  if (packet->command != GEARMAN_COMMAND_WORK_COMPLETE &&
      packet->command != GEARMAN_COMMAND_WORK_FAIL &&
      (http->background == false ||
       packet->command != GEARMAN_COMMAND_JOB_CREATED))
  {
    *ret_ptr= GEARMAN_IGNORE_PACKET;
    return 0;
  }

  pack_size= (size_t)snprintf((char *)data, data_size,
                              "HTTP/1.0 200 OK\r\n"
                              "X-Gearman-Job-Handle: %.*s\r\n"
                              "Content-Length: %"PRIu64"\r\n"
                              "Server: Gearman/" PACKAGE_VERSION "\r\n"
                              "\r\n",
                              packet->command == GEARMAN_COMMAND_JOB_CREATED ?
                              (uint32_t)packet->arg_size[0] :
                              (uint32_t)packet->arg_size[0] - 1,
                              packet->arg[0],
                              (uint64_t)packet->data_size);

  if (pack_size > data_size)
  {
    *ret_ptr= GEARMAN_FLUSH_DATA;
    return 0;
  }

  if (! (http->keep_alive))
  {
    gearman_io_set_option(&connection->con, GEARMAN_CON_CLOSE_AFTER_FLUSH, true);
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return pack_size;
}

static size_t _http_unpack(gearmand_packet_st *packet, gearman_server_con_st *connection,
                           const void *data, size_t data_size,
                           gearman_return_t *ret_ptr)
{
  gearman_protocol_http_st *http;
  size_t offset= 0;
  const char *request;
  size_t request_size;
  const char *method;
  ptrdiff_t method_size;
  const char *uri;
  ptrdiff_t uri_size;
  const char *version;
  size_t version_size;
  const char *header;
  size_t header_size;
  char content_length[11]; /* 11 bytes to fit max display length of uint32_t */
  const char *unique= "-";
  size_t unique_size= 2;
  gearman_job_priority_t priority= GEARMAN_JOB_PRIORITY_NORMAL;

  /* Get the request line first. */
  request= _http_line(data, data_size, &request_size, &offset);
  if (request == NULL || request_size == 0)
  {
    *ret_ptr= GEARMAN_IO_WAIT;
    return offset;
  }

  http= (gearman_protocol_http_st *)gearmand_connection_protocol_context(connection);
  http->background= false;
  http->keep_alive= false;

  /* Parse out the method, URI, and HTTP version from the request line. */
  method= request;
  uri= memchr(request, ' ', request_size);
  if (uri == NULL)
  {
    gearmand_log_error("_http_unpack", "bad request line: %.*s", (uint32_t)request_size, request);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  method_size= uri - request;
  if ((method_size != 3 ||
       (strncasecmp(method, "GET", 3) && strncasecmp(method, "PUT", 3))) &&
      (method_size != 4 || strncasecmp(method, "POST", 4)))
  {
    gearmand_log_error("_http_unpack", "bad method: %.*s", (uint32_t)method_size, method);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  while (*uri == ' ')
    uri++;

  while (*uri == '/')
    uri++;

  version= memchr(uri, ' ', request_size - (size_t)(uri - request));
  if (version == NULL)
  {
    gearmand_log_error("_http_unpack", "bad request line: %.*s",
                      (uint32_t)request_size, request);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  uri_size= version - uri;
  if (uri_size == 0)
  {
    gearmand_log_error("_http_unpack",
                      "must give function name in URI");
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  while (*version == ' ')
    version++;

  version_size= request_size - (size_t)(version - request);

  if (version_size == 8 && !strncasecmp(version, "HTTP/1.1", 8))
    http->keep_alive= true;
  else if (version_size != 8 || strncasecmp(version, "HTTP/1.0", 8))
  {
    gearmand_log_error("_http_unpack", "bad version: %.*s",
                      (uint32_t)version_size, version);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  /* Loop through all the headers looking for ones of interest. */
  while ((header= _http_line(data, data_size, &header_size, &offset)) != NULL)
  {
    if (header_size == 0)
      break;

    if (header_size > 16 && !strncasecmp(header, "Content-Length: ", 16))
    {
      if ((method_size == 4 && !strncasecmp(method, "POST", 4)) ||
          (method_size == 3 && !strncasecmp(method, "PUT", 3)))
      {
        snprintf(content_length, 11, "%.*s", (int)header_size - 16,
                 header + 16);
        packet->data_size= (size_t)atoi(content_length);
      }
    }
    else if (header_size == 22 &&
             !strncasecmp(header, "Connection: Keep-Alive", 22))
    {
      http->keep_alive= true;
    }
    else if (header_size > 18 && !strncasecmp(header, "X-Gearman-Unique: ", 18))
    {
      unique= header + 18;
      unique_size= header_size - 18;
    }
    else if (header_size == 26 &&
             !strncasecmp(header, "X-Gearman-Background: true", 26))
    {
      http->background= true;
    }
    else if (header_size == 24 &&
             !strncasecmp(header, "X-Gearman-Priority: high", 24))
    {
      priority= GEARMAN_JOB_PRIORITY_HIGH;
    }
    else if (header_size == 23 &&
             !strncasecmp(header, "X-Gearman-Priority: low", 23))
    {
      priority= GEARMAN_JOB_PRIORITY_LOW;
    }
  }

  /* Make sure we received the end of headers. */
  if (header == NULL)
  {
    *ret_ptr= GEARMAN_IO_WAIT;
    return 0;
  }

  /* Request and all headers complete, build a packet based on HTTP request. */
  packet->magic= GEARMAN_MAGIC_REQUEST;

  if (http->background)
  {
    if (priority == GEARMAN_JOB_PRIORITY_NORMAL)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_BG;
    else if (priority == GEARMAN_JOB_PRIORITY_HIGH)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG;
    else
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG;
  }
  else
  {
    if (priority == GEARMAN_JOB_PRIORITY_NORMAL)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB;
    else if (priority == GEARMAN_JOB_PRIORITY_HIGH)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_HIGH;
    else
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_LOW;
  }

  *ret_ptr= gearmand_packet_pack_header(packet);
  if (*ret_ptr != GEARMAN_SUCCESS)
    return 0;

  *ret_ptr= gearmand_packet_create(packet, uri, (size_t)uri_size + 1);
  if (*ret_ptr != GEARMAN_SUCCESS)
    return 0;

  *ret_ptr= gearmand_packet_create(packet, unique, unique_size + 1);
  if (*ret_ptr != GEARMAN_SUCCESS)
    return 0;

  /* Make sure function and unique are NULL terminated. */
  packet->arg[0][uri_size]= 0;
  packet->arg[1][unique_size]= 0;

  *ret_ptr= GEARMAN_SUCCESS;
  return offset;
}

static const char *_http_line(const void *data, size_t data_size,
                              size_t *line_size, size_t *offset)
{
  const char *start= (const char *)data + *offset;
  const char *end;

  end= memchr(start, '\n', data_size - *offset);
  if (end == NULL)
    return NULL;

  *offset+= (size_t)(end - start) + 1;

  if (end != start && *(end - 1) == '\r')
    end--;

  *line_size= (size_t)(end - start);

  return start;
}
