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

#ifndef __GEARMAND_LOG_H__
#define __GEARMAND_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

#ifdef __cplusplus
#define STRING_WITH_LEN(X) (X), (static_cast<size_t>((sizeof(X) - 1)))
#else
#define STRING_WITH_LEN(X) (X), ((size_t)((sizeof(X) - 1)))
#endif

GEARMAN_INTERNAL_API
void gearmand_initialize_thread_logging(const char *identity);

/**
 * Log a fatal message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_fatal(const char *format, ...);

GEARMAN_INTERNAL_API
void gearmand_log_fatal_perror(const char *message, const char *file);

#define gearmand_fatal(A) do { gearmand_log_fatal("%s -> %s", A, AT); } while (0)
#define gearmand_fatal_perror(A) do { gearmand_log_fatal_perror(AT, A); } while (0)


/**
 * Log an error message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_error(const char *format, ...);

GEARMAN_INTERNAL_API
void gearmand_log_perror(const char *position, const char *message);

GEARMAN_INTERNAL_API
void gearmand_log_gerror(const char *position, const char *message, const gearmand_error_t rc);

GEARMAN_INTERNAL_API
void gearmand_log_gai_error(const char *position, const char *message, const int rc);

#define gearmand_error(A) do { gearmand_log_error("%s -> %s", A, AT); } while (0)
#define gearmand_perror(A) do { gearmand_log_perror(AT, A); } while (0)
#define gearmand_gerror(A,B) do { gearmand_log_gerror(AT, A, B); } while (0)
#define gearmand_gai_error(A,B) do { gearmand_log_gai_error(AT, A, B); } while (0)


/**
 * Log an info message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_info(const char *format, ...);
#define gearmand_info(A) do { gearmand_log_info("%s -> %s", A, AT); } while (0)

/**
 * Log a debug message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_debug(const char *format, ...);
#define gearmand_debug(A) do { gearmand_log_debug("%s -> %s", A, AT); } while (0)


/**
 * Log a crazy message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_crazy(const char *format, ...);

#ifdef DEBUG
#define gearmand_crazy(A) do { gearmand_log_crazy("%s -> %s", A, AT); } while (0)
#else
#define gearmand_crazy(A)
#endif

GEARMAN_INTERNAL_API
void gearman_conf_error_set(gearman_conf_st *conf, const char *msg, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_LOG_H__ */
