/* Gearman server and library
 * Copyright (C) 2011 Data Differential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <libgearman-1.0/visibility.h>
#include <libgearman-server/error/type.h>

#ifdef __cplusplus
extern "C" {
#endif

GEARMAN_API
const char *gearmand_strerror(gearmand_error_t rc);

#ifdef __cplusplus
}
#endif
