/* Gearman server and library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman conf declarations
 */

#ifndef __GEARMAN_CONF_H__
#define __GEARMAN_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman conf Gearman Conf Interface
 * @{
 */

/**
 * Initialize a gearman conf structure.
 */
GEARMAN_API
gearman_conf_st *gearman_conf_create(gearman_conf_st *conf);

/**
 * Free a gearman conf structure.
 */
GEARMAN_API
void gearman_conf_free(gearman_conf_st *conf);

/**
 * Return an return code for the last library error encountered.
 */
GEARMAN_API
gearman_return_t gearman_conf_return(gearman_conf_st *conf);

/**
 * Return an error string for last library error encountered.
 */
GEARMAN_API
const char *gearman_conf_error(gearman_conf_st *conf);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 */
GEARMAN_API
int gearman_conf_errno(gearman_conf_st *conf);

/**
 * Set options for a gearman conf structure.
 */
GEARMAN_API
void gearman_conf_set_options(gearman_conf_st *conf,
                              gearman_conf_options_t options, uint32_t data);

/**
 * Parse command line arguments.
 */
GEARMAN_API
gearman_return_t gearman_conf_parse_args(gearman_conf_st *conf, int argc,
                                         char *argv[]);

/**
 * Print usage information to stdout.
 */
GEARMAN_API
void gearman_conf_usage(gearman_conf_st *conf);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONF_H__ */
