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

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <inttypes.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/uio.h>
#include <stdarg.h>

#include <libgearman/visibility.h>
#include <libgearman/constants.h>

// Everything above this line must be in the order specified.

#include <libgearman/state.h>
#include <libgearman/command.h>
#include <libgearman/packet.h>
#include <libgearman/conn.h>
#include <libgearman/task.h>
#include <libgearman/job.h>

#include <libgearman/worker.h>
#include <libgearman/client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman Gearman Declarations
 *
 * This is a low level interface for gearman library instances. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 *
 * There is no locking within a single gearman_state_st structure, so for threaded
 * applications you must either ensure isolation in the application or use
 * multiple gearman_state_st structures (for example, one for each thread).
 *
 * @{
 */

/**
 * Get Gearman library version.
 *
 * @return Version string of library.
 */
GEARMAN_API
const char *gearman_version(void);

/**
 * Get bug report URL.
 *
 * @return Bug report URL string.
 */
GEARMAN_API
const char *gearman_bugreport(void);

/**
 * Get string with the name of the given verbose level.
 *
 * @param[in] verbose Verbose logging level.
 * @return String form of verbose level.
 */
GEARMAN_API
const char *gearman_verbose_name(gearman_verbose_t verbose);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
