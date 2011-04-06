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
#include <stdlib.h>

#include <libgearman/visibility.h>
#include <libgearman/configure.h>
#include <libgearman/constants.h>
#include <libgearman/strerror.h>

// Everything above this line must be in the order specified.
#include <libgearman/workload.h>
#include <libgearman/unique.h>
#include <libgearman/core.h>
#include <libgearman/task.h>
#include <libgearman/job.h>

#include <libgearman/worker.h>
#include <libgearman/client.h>
#include <libgearman/function.h>

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
 * There is no locking within a single gearman_universal_st structure, so for threaded
 * applications you must either ensure isolation in the application or use
 * multiple gearman_universal_st structures (for example, one for each thread).
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
                                       void *context);

/**
 * Get current socket I/O activity timeout value.
 *
 * @param[in] gearman_client_st or gearman_worker_st Structure previously initialized.
 * @return Timeout in milliseconds to wait for I/O activity. A negative value
 *  means an infinite timeout.
 * @note This is a utility macro.
 */
#define gearman_timeout(__object) ((__object)->gearman.timeout)

/**
 * Set socket I/O activity timeout for connections in a Gearman structure.
 *
 * @param[in] gearman_client_st or gearman_worker_st Structure previously initialized.
 * @param[in] timeout Milliseconds to wait for I/O activity. A negative value
 *  means an infinite timeout.
 * @note This is a utility macro.
 */
#define gearman_set_timeout(__object, __value) ((__object)->gearman.timeout)=(__value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
