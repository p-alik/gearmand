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

#include <libgearman-server/common.h>

#include <libgearman-server/plugins/protocol/http/protocol.h>

/**
 * @addtogroup gearmand::protocol::HTTPatic Static HTTP Protocol Definitions
 * @ingroup gearman_protocol_http
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT "8080"

#pragma GCC diagnostic ignored "-Wold-style-cast"

/* Protocol callback functions. */
static gearmand_error_t _http_con_add(gearman_server_con_st *connection);
static void _http_free(gearman_server_con_st *connection, void *context);
static size_t _http_pack(const gearmand_packet_st *packet, gearman_server_con_st *connection,
                         void *data, size_t data_size,
                         gearmand_error_t *ret_ptr);
static size_t _http_unpack(gearmand_packet_st *packet, gearman_server_con_st *connection,
                           const void *data, size_t data_size,
                           gearmand_error_t *ret_ptr);

namespace gearmand {
namespace protocol {

HTTP::HTTP() :
  Plugin("HTTP"),
  _background(false),
  _keep_alive(false)
{
  command_line_options().add_options()
    ("http-port", boost::program_options::value(&global_port)->default_value(GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT), "Port to listen on.");
}

HTTP::~HTTP()
{
}

gearmand_error_t HTTP::start(gearmand_st *gearmand)
{
  return gearmand_port_add(gearmand, global_port.c_str(), _http_con_add);
}

} // namespace protocol
} // namespace gearmand

/* Line parsing helper function. */
static const char *_http_line(const void *data, size_t data_size,
                              size_t *line_size, size_t *offset);

/** @} */

/*
 * Public definitions
 */


/*
 * Static definitions
 */

static gearmand_error_t _http_con_add(gearman_server_con_st *connection)
{
  gearmand::protocol::HTTP *http;

  http= new gearmand::protocol::HTTP;
  if (http == NULL)
  {
    gearmand_log_error("_http_con_add", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearmand_connection_set_protocol(connection, http, _http_free, _http_pack, _http_unpack);

  return GEARMAN_SUCCESS;
}

static void _http_free(gearman_server_con_st *connection __attribute__ ((unused)),
                       void *context)
{
  gearmand::protocol::HTTP *http;
  http= (gearmand::protocol::HTTP *)context;
  delete http;
}

static size_t _http_pack(const gearmand_packet_st *packet, gearman_server_con_st *connection,
                         void *data, size_t data_size,
                         gearmand_error_t *ret_ptr)
{
  size_t pack_size;
  gearmand::protocol::HTTP *http;

  http= (gearmand::protocol::HTTP *)gearmand_connection_protocol_context(connection);

  if (packet->command != GEARMAN_COMMAND_WORK_COMPLETE &&
      packet->command != GEARMAN_COMMAND_WORK_FAIL &&
      (http->background() == false ||
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
                              packet->command == GEARMAN_COMMAND_JOB_CREATED ?  (int)packet->arg_size[0] : (int)packet->arg_size[0] - 1,
                              (const char *)packet->arg[0],
                              (uint64_t)packet->data_size);

  if (pack_size > data_size)
  {
    *ret_ptr= GEARMAN_FLUSH_DATA;
    return 0;
  }

  if (! (http->keep_alive()))
  {
    gearman_io_set_option(&connection->con, GEARMAND_CON_CLOSE_AFTER_FLUSH, true);
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return pack_size;
}

static size_t _http_unpack(gearmand_packet_st *packet, gearman_server_con_st *connection,
                           const void *data, size_t data_size,
                           gearmand_error_t *ret_ptr)
{
  gearmand::protocol::HTTP *http;
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
  gearmand_job_priority_t priority= GEARMAND_JOB_PRIORITY_NORMAL;

  /* Get the request line first. */
  request= _http_line(data, data_size, &request_size, &offset);
  if (request == NULL || request_size == 0)
  {
    *ret_ptr= GEARMAN_IO_WAIT;
    return offset;
  }

  http= (gearmand::protocol::HTTP *)gearmand_connection_protocol_context(connection);
  http->reset();

  /* Parse out the method, URI, and HTTP version from the request line. */
  method= request;
  uri= (const char *)memchr(request, ' ', request_size);
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

  version= (const char *)memchr(uri, ' ', request_size - (size_t)(uri - request));
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
  {
    http->set_keep_alive(true);
  }
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
      http->set_keep_alive(true);
    }
    else if (header_size > 18 && !strncasecmp(header, "X-Gearman-Unique: ", 18))
    {
      unique= header + 18;
      unique_size= header_size - 18;
    }
    else if (header_size == 26 &&
             !strncasecmp(header, "X-Gearman-Background: true", 26))
    {
      http->set_background(true);
    }
    else if (header_size == 24 &&
             !strncasecmp(header, "X-Gearman-Priority: high", 24))
    {
      priority= GEARMAND_JOB_PRIORITY_HIGH;
    }
    else if (header_size == 23 &&
             !strncasecmp(header, "X-Gearman-Priority: low", 23))
    {
      priority= GEARMAND_JOB_PRIORITY_LOW;
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

  if (http->background())
  {
    if (priority == GEARMAND_JOB_PRIORITY_NORMAL)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_BG;
    else if (priority == GEARMAND_JOB_PRIORITY_HIGH)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG;
    else
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG;
  }
  else
  {
    if (priority == GEARMAND_JOB_PRIORITY_NORMAL)
      packet->command= GEARMAN_COMMAND_SUBMIT_JOB;
    else if (priority == GEARMAND_JOB_PRIORITY_HIGH)
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

  end= (const char *)memchr(start, '\n', data_size - *offset);
  if (end == NULL)
    return NULL;

  *offset+= (size_t)(end - start) + 1;

  if (end != start && *(end - 1) == '\r')
    end--;

  *line_size= (size_t)(end - start);

  return start;
}
