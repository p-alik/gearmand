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

#include <libgearman/modconf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_protocol_http HTTP Protocol Functions
 * @ingroup gearman_protocol_http
 * @{
 */

/**
 * Get module configuration options.
 */
modconf_return_t gearman_protocol_http_modconf(modconf_st *modconf);

/**
 * Initialize the HTTP protocol module.
 */
gearman_return_t gearmand_protocol_http_init(gearmand_st *gearmand,
                                             modconf_st *modconf);

/**
 * De-initialize the HTTP protocol module.
 */
gearman_return_t gearmand_protocol_http_deinit(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_PROTOCOL_HTTP_H__ */
