/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman Declarations
 */

#ifndef __GEARMAN_CORE_H__
#define __GEARMAN_CORE_H__

#include <libgearman/state.h>
#include <libgearman/command.h>
#include <libgearman/packet.h>
#include <libgearman/connection.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman_state_state
 */
/**
 * Utility function used for parsing server lists.
 *
 * @param[in] servers String containing a list of servers to parse.
 * @param[in] callback Function to call for each server that is found.
 * @param[in] context Argument to pass along with callback function.
 * @return Standard Gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *callback,
                                       const void *context);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CORE_H__ */
