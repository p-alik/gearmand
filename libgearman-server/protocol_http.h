/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief HTTP Protocol Declarations
 */

#ifndef __GEARMAN_PROTOCOL_HTTP_H__
#define __GEARMAN_PROTOCOL_HTTP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_protocol_http HTTP Protocol Declarations
 * @ingroup gearman_server_protocol
 *
 * This module provides a simple HTTP interface into the Gearman job server. It
 * is also meant to serve as an example of how other protocols can plug into
 * the server. This module will ignore all headers except:
 *
 * Content-Length: SIZE
 * Connection: Keep-Alive
 * X-Gearman-Unique: UNIQUE_KEY
 * X-Gearman-Background: true
 * X-Gearman-Priority: HIGH | LOW
 *
 * All HTTP requests are translated into SUBMIT_JOB requests, and only
 * WORK_COMPLETE, WORK_FAIL, and JOB_CREATED responses are returned.
 * JOB_CREATED packet are only sent back if the "X-Gearman-Background: true"
 * header is given.
 *
 * @{
 */

/**
 * Get module configuration options.
 */
GEARMAN_API
gearman_return_t gearmand_protocol_http_conf(gearman_conf_st *conf);

/**
 * Initialize the HTTP protocol module.
 */
GEARMAN_API
gearman_return_t gearmand_protocol_http_init(gearmand_st *gearmand,
                                             gearman_conf_st *conf);

/**
 * De-initialize the HTTP protocol module.
 */
GEARMAN_API
gearman_return_t gearmand_protocol_http_deinit(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_PROTOCOL_HTTP_H__ */
