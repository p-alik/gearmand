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
 * Utility function used for error logging
 * @ingroup gearman_private
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Name of function the error happened in. 
 * @param[in] format Format and variable argument list of message.
 */
GEARMAN_LOCAL
void gearman_error_set(gearman_st *gearman, const char *function,
                       const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_ERROR_H__ */
