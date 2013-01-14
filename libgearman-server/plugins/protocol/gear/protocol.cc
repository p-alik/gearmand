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


/**
 * @file
 * @brief Gear Protocol Definitions
 */

#include "gear_config.h"

#include <libgearman-server/common.h>
#include <libgearman/strcommand.h>
#include <libgearman-server/packet.h>

#include <cstdio>
#include <cstdlib>

#include <libgearman-server/plugins/protocol/gear/protocol.h>
#include "libgearman/command.h"

static gearmand_error_t gearmand_packet_unpack_header(gearmand_packet_st *packet)
{
  uint32_t tmp;

  if (memcmp(packet->args, "\0REQ", 4) == 0)
  {
    packet->magic= GEARMAN_MAGIC_REQUEST;
  }
  else if (memcmp(packet->args, "\0RES", 4) == 0)
  {
    packet->magic= GEARMAN_MAGIC_RESPONSE;
  }
  else
  {
    gearmand_warning("invalid magic value");
    return GEARMAN_INVALID_MAGIC;
  }

  memcpy(&tmp, packet->args + 4, 4);
  packet->command= static_cast<gearman_command_t>(ntohl(tmp));

  if (packet->command == GEARMAN_COMMAND_TEXT ||
      packet->command >= GEARMAN_COMMAND_MAX)
  {
    gearmand_error("invalid command value");
    return GEARMAN_INVALID_COMMAND;
  }

  memcpy(&tmp, packet->args + 8, 4);
  packet->data_size= ntohl(tmp);

  return GEARMAN_SUCCESS;
}

class Geartext : public gearmand::protocol::Context {

public:
  ~Geartext()
  { }

  bool is_owner()
  {
    return false;
  }

  void notify(gearman_server_con_st *)
  {
    gearmand_info("Gear connection disconnected");
  }

  size_t unpack(gearmand_packet_st *packet,
                gearman_server_con_st *,
                const void *data, const size_t data_size,
                gearmand_error_t& ret_ptr)
  {
    size_t used_size;
    gearmand_info("Gear unpack");

    if (packet->args_size == 0)
    {
      if (data_size > 0 && ((uint8_t *)data)[0] != 0)
      {
        /* Try to parse a text-based command. */
        uint8_t* ptr= (uint8_t *)memchr(data, '\n', data_size);
        if (ptr == NULL)
        {
          ret_ptr= GEARMAN_IO_WAIT;
          return 0;
        }

        packet->magic= GEARMAN_MAGIC_TEXT;
        packet->command= GEARMAN_COMMAND_TEXT;

        used_size= size_t(ptr - ((uint8_t *)data)) +1;
        *ptr= 0;
        if (used_size > 1 && *(ptr - 1) == '\r')
        {
          *(ptr - 1)= 0;
        }

        size_t arg_size;
        for (arg_size= used_size, ptr= (uint8_t *)data; ptr != NULL; data= ptr)
        {
          ptr= (uint8_t *)memchr(data, ' ', arg_size);
          if (ptr != NULL)
          {
            *ptr= 0;
            ptr++;
            while (*ptr == ' ')
            {
              ptr++;
            }

            arg_size-= size_t(ptr - ((uint8_t *)data));
          }

          ret_ptr= gearmand_packet_create(packet, data, ptr == NULL ? arg_size :
                                          size_t(ptr - ((uint8_t *)data)));
          if (ret_ptr != GEARMAN_SUCCESS)
          {
            return used_size;
          }
        }

        return used_size;
      }
      else if (data_size < GEARMAN_PACKET_HEADER_SIZE)
      {
        ret_ptr= GEARMAN_IO_WAIT;
        return 0;
      }

      packet->args= packet->args_buffer;
      packet->args_size= GEARMAN_PACKET_HEADER_SIZE;
      memcpy(packet->args, data, GEARMAN_PACKET_HEADER_SIZE);

      if (gearmand_failed(ret_ptr= gearmand_packet_unpack_header(packet)))
      {
        return 0;
      }

      used_size= GEARMAN_PACKET_HEADER_SIZE;
    }
    else
    {
      used_size= 0;
    }

    while (packet->argc != gearman_command_info(packet->command)->argc)
    {
      if (packet->argc != (gearman_command_info(packet->command)->argc - 1) or
          gearman_command_info(packet->command)->data)
      {
        uint8_t* ptr= (uint8_t *)memchr(((uint8_t *)data) +used_size, 0,
                               data_size -used_size);
        if (ptr == NULL)
        {
          gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                             "Possible protocol error for %s, received only %u args",
                             gearman_command_info(packet->command)->name, packet->argc);
          ret_ptr= GEARMAN_IO_WAIT;
          return used_size;
        }

        size_t arg_size= size_t(ptr - (((uint8_t *)data) + used_size)) +1;
        if (gearmand_failed((ret_ptr= gearmand_packet_create(packet, ((uint8_t *)data) + used_size, arg_size))))
        {
          return used_size;
        }

        packet->data_size-= arg_size;
        used_size+= arg_size;
      }
      else
      {
        if ((data_size - used_size) < packet->data_size)
        {
          ret_ptr= GEARMAN_IO_WAIT;
          return used_size;
        }

        ret_ptr= gearmand_packet_create(packet, ((uint8_t *)data) + used_size, packet->data_size);
        if (gearmand_failed(ret_ptr))
        {
          return used_size;
        }

        used_size+= packet->data_size;
        packet->data_size= 0;
      }
    }

    if (packet->command == GEARMAN_COMMAND_ECHO_RES)
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                         "GEAR length: %" PRIu64 " gearmand_command_t: %s echo: %.*s",
                         uint64_t(packet->data_size),
                         gearman_strcommand(packet->command),
                         int(packet->data_size),
                         packet->data);
    }
    else if (packet->command == GEARMAN_COMMAND_TEXT)
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                         "GEAR length: %" PRIu64 " gearmand_command_t: %s text: %.*s",
                         uint64_t(packet->data_size),
                         gearman_strcommand(packet->command),
                         int(packet->data_size),
                         packet->data);
    }
    else
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                         "GEAR length: %" PRIu64 " gearmand_command_t: %s",
                         uint64_t(packet->data_size),
                         gearman_strcommand(packet->command));
    }

    ret_ptr= GEARMAN_SUCCESS;
    return used_size;
  }

  size_t pack(const gearmand_packet_st *packet,
              gearman_server_con_st*,
              void *data, const size_t data_size,
              gearmand_error_t& ret_ptr)
  {
    if (packet->command == GEARMAN_COMMAND_ECHO_RES)
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                         "GEAR length: %" PRIu64 " gearmand_command_t: %s echo: %.*",
                         uint64_t(packet->data_size),
                         gearman_strcommand(packet->command),
                         int(packet->data_size),
                         packet->data);
    }
    else
    {
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                         "GEAR length: %" PRIu64 " gearmand_command_t: %s",
                         uint64_t(packet->data_size),
                         gearman_strcommand(packet->command));
    }

    if (packet->args_size == 0)
    {
      ret_ptr= GEARMAN_SUCCESS;
      return 0;
    }

    if (packet->args_size > data_size)
    {
      ret_ptr= GEARMAN_FLUSH_DATA;
      return 0;
    }

    memcpy(data, packet->args, packet->args_size);
    ret_ptr= GEARMAN_SUCCESS;

    return packet->args_size;
  }

private:
};

static Geartext gear_context;

static gearmand_error_t _gear_con_add(gearman_server_con_st *connection)
{
  gearmand_info("Gear connection made");

  connection->set_protocol(&gear_context);

  return GEARMAN_SUCCESS;
}

namespace gearmand {
namespace protocol {

Gear::Gear() :
  Plugin("Gear")
{
  command_line_options().add_options()
    ("port,p", boost::program_options::value(&_port)->default_value(GEARMAN_DEFAULT_TCP_PORT_STRING),
     "Port the server should listen on.");
}

Gear::~Gear()
{
}

gearmand_error_t Gear::start(gearmand_st *gearmand)
{
  gearmand_info("Initializing Gear");

  gearmand_error_t rc;
  if (_port.empty())
  {
    struct servent *gearman_servent= getservbyname(GEARMAN_DEFAULT_TCP_SERVICE, NULL);

    if (gearman_servent and gearman_servent->s_name)
    {
      rc= gearmand_port_add(gearmand, gearman_servent->s_name, _gear_con_add);
    }
    else
    {
      rc= gearmand_port_add(gearmand, GEARMAN_DEFAULT_TCP_PORT_STRING, _gear_con_add);
    }
  }
  else
  {
    rc= gearmand_port_add(gearmand, _port.c_str(), _gear_con_add);
  }

  return rc;
}

} // namespace protocol
} // namespace gearmand
