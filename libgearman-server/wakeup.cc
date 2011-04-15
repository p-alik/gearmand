/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <stdlib.h>

#include <libgearman-server/wakeup.h>

const char *gearmand_strwakeup(gearmand_wakeup_t arg)
{
  switch (arg)
  {
  case GEARMAND_WAKEUP_PAUSE:
    return "GEARMAND_WAKEUP_PAUSE";

  case GEARMAND_WAKEUP_SHUTDOWN:
      return "GEARMAND_WAKEUP_SHUTDOWN";

  case GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL:
      return "GEARMAND_WAKEUP_SHUTDOWN_GRACEFUL";

  case GEARMAND_WAKEUP_CON:
      return "GEARMAND_WAKEUP_CON";

  case GEARMAND_WAKEUP_RUN:
      return "GEARMAND_WAKEUP_RUN";

  default:
    abort();
  }
}

