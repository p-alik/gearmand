/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <libgearman-1.0/visibility.h>

typedef enum
{
  GEARMAND_WAKEUP_PAUSE,
  GEARMAND_WAKEUP_SHUTDOWN,
  GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL,
  GEARMAND_WAKEUP_CON,
  GEARMAND_WAKEUP_RUN
} gearmand_wakeup_t;

#ifdef __cplusplus
extern "C" {
#endif

GEARMAN_INTERNAL_API
const char *gearmand_strwakeup(gearmand_wakeup_t arg);

#ifdef __cplusplus
}
#endif
