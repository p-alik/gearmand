/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011-2012 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
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


/**
 * @file
 * @brief HTTP Protocol Definitions
 */

#include <config.h>
#include <libgearman-server/common.h>
#include <libgearman/strcommand.h>

#include <cstdio>
#include <cstdlib>

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

static const char *_http_line(const void *data, size_t data_size,
                              size_t& line_size, size_t& offset)
{
  const char *start= (const char *)data +offset;

  const char *end= (const char *)memchr(start, '\n', data_size -offset);
  if (end == NULL)
  {
    return NULL;
  }

  offset+= size_t(end - start) +1;

  if (end != start && *(end - 1) == '\r')
  {
    end--;
  }

  line_size= size_t(end - start);

  return start;
}

static void _http_free(gearman_server_con_st *, void *context)
{
  gearmand_debug("HTTP connection disconnected");
  gearmand::protocol::HTTP *http= (gearmand::protocol::HTTP *)context;
  delete http;
}

static size_t _http_pack(const gearmand_packet_st *packet, gearman_server_con_st *connection,
                         void *data, size_t data_size,
                         gearmand_error_t *ret_ptr)
{
  gearmand::protocol::HTTP *http= (gearmand::protocol::HTTP *)gearmand_connection_protocol_context(connection);

  if (packet->command != GEARMAN_COMMAND_WORK_COMPLETE and
      packet->command != GEARMAN_COMMAND_WORK_FAIL and
      packet->command != GEARMAN_COMMAND_ECHO_RES and
      (http->background() == false or
       packet->command != GEARMAN_COMMAND_JOB_CREATED))
  {
    gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                       "Sending HTTP told to ignore packet: gearmand_command_t:%s", 
                       gearman_strcommand(packet->command));
    *ret_ptr= GEARMAN_IGNORE_PACKET;
    return 0;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "Sending HTTP response: Content-length:%"PRIu64" gearmand_command_t:%s response:%s", 
                     uint64_t(packet->data_size), gearman_strcommand(packet->command),
                     gearmand::protocol::httpd::response(http->response()));

  size_t pack_size;
  if (http->response() != gearmand::protocol::httpd::HTTP_OK)
  {
    pack_size= (size_t)snprintf((char *)data, data_size,
                                "HTTP/1.0 %u %s\r\n"
                                "Server: Gearman/" PACKAGE_VERSION "\r\n"
                                "Content-Length: 0\r\n"
                                "\r\n",
                                int(http->response()), gearmand::protocol::httpd::response(http->response()));
  }
  else if (http->method() == gearmand::protocol::httpd::HEAD)
  {
    pack_size= (size_t)snprintf((char *)data, data_size,
                                "HTTP/1.0 200 OK\r\n"
                                "X-Gearman-Job-Handle: %.*s\r\n"
                                "Content-Length: %"PRIu64"\r\n"
                                "Server: Gearman/" PACKAGE_VERSION "\r\n"
                                "\r\n",
                                packet->command == GEARMAN_COMMAND_JOB_CREATED ?  (int)packet->arg_size[0] : (int)packet->arg_size[0] - 1,
                                (const char *)packet->arg[0],
                                (uint64_t)packet->data_size);
  }
  else if (http->method() == gearmand::protocol::httpd::TRACE)
  {
    pack_size= (size_t)snprintf((char *)data, data_size,
                                "HTTP/1.0 200 OK\r\n"
                                "Server: Gearman/" PACKAGE_VERSION "\r\n"
                                "Connection: close\r\n"
                                "Content-Type: message/http\r\n"
                                "\r\n");
  }
  else
  {
    pack_size= (size_t)snprintf((char *)data, data_size,
                                "HTTP/1.0 200 OK\r\n"
                                "X-Gearman-Job-Handle: %.*s\r\n"
                                "X-Gearman-Command: %s\r\n"
                                "Content-Length: %"PRIu64"\r\n"
                                "Server: Gearman/" PACKAGE_VERSION "\r\n"
                                "\r\n",
                                packet->command == GEARMAN_COMMAND_JOB_CREATED ?  int(packet->arg_size[0]) : int(packet->arg_size[0] - 1), (const char *)packet->arg[0],
                                gearman_strcommand(packet->command),
                                uint64_t(packet->data_size));
  }

  if (pack_size > data_size)
  {
    gearmand_debug("Sending HTTP had to flush");
    *ret_ptr= GEARMAN_FLUSH_DATA;
    return 0;
  }

  if (http->keep_alive() == false)
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
  const char *unique= "-";
  size_t unique_size= 2;
  gearmand_job_priority_t priority= GEARMAND_JOB_PRIORITY_NORMAL;

  gearmand_info("Receiving HTTP response");

  /* Get the request line first. */
  size_t request_size;
  size_t offset= 0;
  const char *request= _http_line(data, data_size, request_size, offset);
  if (request == NULL or request_size == 0)
  {
    gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "Zero length request made");
    *ret_ptr= GEARMAN_IO_WAIT;
    return offset;
  }

  gearmand::protocol::HTTP *http= (gearmand::protocol::HTTP *)gearmand_connection_protocol_context(connection);
  http->reset();

  /* Parse out the method, URI, and HTTP version from the request line. */
  const char *method= request;
  const char *uri= (const char *)memchr(request, ' ', request_size);
  if (uri == NULL)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "bad request line: %.*s", (uint32_t)request_size, request);
    http->set_response(gearmand::protocol::httpd::HTTP_NOT_FOUND);
    *ret_ptr= GEARMAN_SUCCESS;
    return 0;
  }

  ptrdiff_t method_size= uri - request;

  if (method_size == 3 and strncmp(method, "GET", 3) == 0)
  {
    http->set_method(gearmand::protocol::httpd::GET);
  }
  else if (method_size == 3 and strncmp(method, "PUT", 3) == 0)
  {
    http->set_method(gearmand::protocol::httpd::PUT);
  }
  else if (method_size == 4 and strncmp(method, "POST", 4) == 0)
  {
    http->set_method(gearmand::protocol::httpd::POST);
  }
  else if (method_size == 4 and strncmp(method, "HEAD", 4) == 0)
  {
    http->set_method(gearmand::protocol::httpd::HEAD);
  }
  else if (method_size == 5 and strncmp(method, "TRACE", 5) == 0)
  {
    http->set_method(gearmand::protocol::httpd::TRACE);
  }
  else
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "bad method: %.*s", (uint32_t)method_size, method);
    http->set_response(gearmand::protocol::httpd::HTTP_METHOD_NOT_ALLOWED);
    *ret_ptr= GEARMAN_SUCCESS;
    return 0;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "METHOD: %s", str_method(http->method()));

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "URI: %s", uri);

  while (*uri == ' ')
  {
    uri++;
  }
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "URI: %s", uri);

  while (*uri == '/')
  {
    uri++;
  }
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "URI: %s", uri);

  const char *version= (const char *)memchr(uri, ' ', request_size - (size_t)(uri - request));
  if (version == NULL)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "bad request line: %.*s",
                      (uint32_t)request_size, request);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  ptrdiff_t uri_size= version -uri;
  switch (http->method())
  {
  case gearmand::protocol::httpd::POST:
  case gearmand::protocol::httpd::PUT:
  case gearmand::protocol::httpd::GET:
    if (uri_size == 0)
    {
      gearmand_error("must give function name in URI");
      http->set_response(gearmand::protocol::httpd::HTTP_NOT_FOUND);
    }

  case gearmand::protocol::httpd::TRACE:
  case gearmand::protocol::httpd::HEAD:
    break;
  }

  while (*version == ' ')
  {
    version++;
  }

  size_t version_size= request_size - (size_t)(version - request);
  if (false and version_size == 8 and strncasecmp(version, "HTTP/1.1", 8) == 0)
  {
    http->set_keep_alive(true);
  }
  else if (version_size == 8 and strncasecmp(version, "HTTP/1.1", 8) == 0)
  {
  }
  else
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "bad version: %.*s", (uint32_t)version_size, version);
    *ret_ptr= GEARMAN_INVALID_PACKET;
    return 0;
  }

  /* Loop through all the headers looking for ones of interest. */
  const char *header;
  size_t header_size;
  while ((header= _http_line(data, data_size, header_size, offset)) != NULL)
  {
    if (header_size == 0)
    {
      break;
    }

    if (header_size > 16 && !strncasecmp(header, "Content-Length: ", 16))
    {
      if ((method_size == 4 && !strncasecmp(method, "POST", 4)) ||
          (method_size == 3 && !strncasecmp(method, "PUT", 3)))
      {
        char content_length[11]; /* 11 bytes to fit max display length of uint32_t */
        snprintf(content_length, sizeof(content_length), "%.*s", (int)header_size - 16,
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
  if (header == NULL and http->response() == gearmand::protocol::httpd::HTTP_OK)
  {
    gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, "No headers were found");
    *ret_ptr= GEARMAN_IO_WAIT;
    return 0;
  }

  /* Request and all headers complete, build a packet based on HTTP request. */
  packet->magic= GEARMAN_MAGIC_REQUEST;

  if (http->response() != gearmand::protocol::httpd::HTTP_OK)
  {
    packet->command= GEARMAN_COMMAND_ECHO_REQ;

    *ret_ptr= gearmand_packet_pack_header(packet);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      return 0;
    }

    packet->data_size= 0;
    packet->data= NULL;
  }
  else if (http->method() == gearmand::protocol::httpd::TRACE)
  {
    packet->command= GEARMAN_COMMAND_ECHO_REQ;

    *ret_ptr= gearmand_packet_pack_header(packet);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      return 0;
    }

    packet->data_size= data_size;
    packet->data= (const char*)data;
  }
  else if (http->method() == gearmand::protocol::httpd::HEAD and uri_size == 0)
  {
    packet->command= GEARMAN_COMMAND_ECHO_REQ;

    *ret_ptr= gearmand_packet_pack_header(packet);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      return 0;
    }
  }
  else
  {
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
    {
      return 0;
    }

    *ret_ptr= gearmand_packet_create(packet, uri, (size_t)uri_size + 1);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      return 0;
    }

    *ret_ptr= gearmand_packet_create(packet, unique, unique_size + 1);
    if (*ret_ptr != GEARMAN_SUCCESS)
    {
      return 0;
    }

    /* Make sure function and unique are NULL terminated. */
    packet->arg[0][uri_size]= 0;
    packet->arg[1][unique_size]= 0;

    *ret_ptr= GEARMAN_SUCCESS;
  }

  gearmand_info("Receiving HTTP response(finished)");

  return offset;
}

static gearmand_error_t _http_con_add(gearman_server_con_st *connection)
{
  gearmand_info("HTTP connection made");

  gearmand::protocol::HTTP *http= new (std::nothrow) gearmand::protocol::HTTP;
  if (http == NULL)
  {
    gearmand_error("new");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearmand_connection_set_protocol(connection, http, _http_free, _http_pack, _http_unpack);

  return GEARMAN_SUCCESS;
}

namespace gearmand {
namespace protocol {

HTTP::HTTP() :
  Plugin("HTTP"),
  _method(gearmand::protocol::httpd::TRACE),
  _background(false),
  _keep_alive(false),
  _http_response(gearmand::protocol::httpd::HTTP_OK)
{
  command_line_options().add_options()
    ("http-port", boost::program_options::value(&global_port)->default_value(GEARMAN_PROTOCOL_HTTP_DEFAULT_PORT), "Port to listen on.");
}

HTTP::~HTTP()
{
}

gearmand_error_t HTTP::start(gearmand_st *gearmand)
{
  gearmand_info("Initializing HTTP");
  return gearmand_port_add(gearmand, global_port.c_str(), _http_con_add);
}

void HTTP::reset()
{
  _background= false;
  _keep_alive= false;
  _method= httpd::TRACE;
  _http_response= gearmand::protocol::httpd::HTTP_OK;
}

} // namespace protocol
} // namespace gearmand

/** @} */
