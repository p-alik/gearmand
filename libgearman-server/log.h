/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/*
  All logging facilities within the server.
*/

#include <stdio.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

#ifdef __cplusplus
#define gearman_literal_param(X) (X), (size_t(sizeof(X) - 1))
#else
#define gearman_literal_param(X) (X), ((size_t)((sizeof(X) - 1)))
#endif

#define GEARMAN_DEFAULT_LOG_PARAM AT, __func__

GEARMAN_INTERNAL_API
void gearmand_initialize_thread_logging(const char *identity);

/**
 * Log a fatal message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_fatal(const char *position, const char *func, const char *format, ...);
#define gearmand_fatal(_mesg) gearmand_log_fatal(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))

GEARMAN_INTERNAL_API
void gearmand_log_fatal_perror(const char *position, const char *function, const char *message);
#define gearmand_fatal_perror(_mesg) gearmand_log_fatal_perror(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))


/**
 * Log an error message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_error(const char *position, const char *function, const char *format, ...);
#define gearmand_error(_mesg) gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))

GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_perror(const char *position, const char *function, const char *message);
#define gearmand_perror(_mesg) gearmand_log_perror(GEARMAN_DEFAULT_LOG_PARAM,  (_mesg))

GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_gerror(const char *position, const char *function, const gearmand_error_t rc, const char *format, ...);
#define gearmand_gerror(_mesg, _gearmand_errot_t) gearmand_log_gerror(GEARMAN_DEFAULT_LOG_PARAM, (_gearmand_errot_t), (_mesg))

GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_gerror_warn(const char *position, const char *function, const gearmand_error_t rc, const char *format, ...);
#define gearmand_gerror_warn(_mesg, _gearmand_errot_t) gearmand_log_gerror_warn(GEARMAN_DEFAULT_LOG_PARAM, (_gearmand_errot_t), (_mesg))

GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_gai_error(const char *position, const char *function, const int rc, const char *message);
#define gearmand_gai_error(_mesg, _gai_int) gearmand_log_gai_error(GEARMAN_DEFAULT_LOG_PARAM, (_gai_int), (_mesg))

GEARMAN_INTERNAL_API
gearmand_error_t gearmand_log_memory_error(const char *position, const char *function, const char *allocator, const char *type, size_t count, size_t size);
#define gearmand_merror(__allocator, __object_type, __count) gearmand_log_memory_error(GEARMAN_DEFAULT_LOG_PARAM, (__allocator), (#__object_type), (__count), (sizeof(__object_type)))


GEARMAN_INTERNAL_API
void gearmand_log_notice(const char *position, const char *function, const char *format, ...);

/**
 * Log an info message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_info(const char *position, const char *function, const char *format, ...);
#define gearmand_info(_mesg) gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))

/**
 * Log an info message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_warning(const char *position, const char *function, const char *format, ...);
#define gearmand_warning(_mesg) gearmand_log_warning(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))

/**
 * Log a debug message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_debug(const char *position, const char *function, const char *format, ...);
#define gearmand_debug(_mesg) gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, (_mesg))

#ifdef __cplusplus
}
#endif
