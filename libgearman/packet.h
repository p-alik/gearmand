/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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
 * @brief Packet Declarations
 */

#pragma once

/**
 * @addtogroup gearman_packet Packet Declarations
 * @ingroup gearman_universal
 *
 * This is a low level interface for gearman packet. This is used internally
 * internally by both client and worker interfaces (or more specifically, tasks
 * and jobs), so you probably want to look there first. This is usually used to
 * write lower level clients, workers, proxies, or your own server.
 *
 * @{
 */

enum gearman_magic_t
{
  GEARMAN_MAGIC_TEXT,
  GEARMAN_MAGIC_REQUEST,
  GEARMAN_MAGIC_RESPONSE
};

/**
 * @ingroup gearman_packet
 */
struct gearman_packet_st
{
  struct {
    bool allocated;
    bool complete;
    bool free_data;
  } options;
  enum gearman_magic_t magic;
  gearman_command_t command;
  uint8_t argc;
  size_t args_size;
  size_t data_size;
  struct gearman_universal_st *universal;
  gearman_packet_st *next;
  gearman_packet_st *prev;
  char *args;
  const void *data;
  char *arg[GEARMAN_MAX_COMMAND_ARGS];
  size_t arg_size[GEARMAN_MAX_COMMAND_ARGS];
  char args_buffer[GEARMAN_ARGS_BUFFER_SIZE];
};

#ifdef GEARMAN_CORE

#ifdef __cplusplus
extern "C" {
#endif


typedef size_t (gearman_packet_pack_fn)(const gearman_packet_st *packet,
                                        void *data, size_t data_size,
                                        gearman_return_t *ret_ptr);
typedef size_t (gearman_packet_unpack_fn)(gearman_packet_st *packet,
                                          const void *data, size_t data_size,
                                          gearman_return_t *ret_ptr);

/**
 * Pack header.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_packet_pack_header(gearman_packet_st *packet);

/**
 * Unpack header.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_packet_unpack_header(gearman_packet_st *packet);


#ifdef __cplusplus
}
#endif

#endif /* GEARMAN_CORE */

