/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Local Gearman Declarations
 */

#ifndef __GEARMAND_LOG_H__
#define __GEARMAND_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GEARMAN_CORE

/**
 * @addtogroup gearman_local Local Gearman Declarations
 * @ingroup gearman_universal
 * @{
 */

/**
 * Log a message.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] verbose Logging level of the message.
 * @param[in] format Format and variable argument list of message.
 * @param[in] args Variable argument list that has been initialized.
 */
GEARMAN_INTERNAL_API
void gearmand_log(gearmand_st *gearman, gearman_verbose_t verbose,
                 const char *format, va_list args);

/**
 * Log a fatal message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_fatal(gearmand_st *gearman, const char *format, ...);

/**
 * Log an error message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_error(gearmand_st *gearman, const char *format, ...);

/**
 * Log an info message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_info(gearmand_st *gearman, const char *format, ...);

/**
 * Log a debug message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_debug(gearmand_st *gearman, const char *format, ...);

/**
 * Log a crazy message, see gearmand_log() for argument details.
 */
GEARMAN_INTERNAL_API
void gearmand_log_crazy(gearmand_st *gearman, const char *format, ...);

GEARMAN_INTERNAL_API
void gearman_conf_error_set(gearman_conf_st *conf, const char *msg, const char *format, ...);

#endif /* GEARMAN_CORE */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_LOG_H__ */
