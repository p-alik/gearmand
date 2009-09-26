/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Error function declarations
 */

#ifndef __GEARMAN_ERROR_H__
#define __GEARMAN_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_error Logging Handling
 * This is used by the client to handle error messages;
 *
 * @{
 */

/**
 * Function to set error string.
 * @ingroup gearman_constants
 */
GEARMAN_API
void gearman_error_set(gearman_st *gear, const char *function, const char *format, ...);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_ERROR_H__ */
