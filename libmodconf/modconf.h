/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief modconf core declarations
 */

#ifndef __MODCONF_H__
#define __MODCONF_H__

#include <inttypes.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif
#include <sys/types.h>

#include <libmodconf/constants.h>
#include <libmodconf/structs.h>
#include <libmodconf/module.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup modconf modconf Core Interface
 * @{
 */

/**
 * Return modconf version.
 */
const char *modconf_version(void);

/**
 * Return modconf bug report URL.
 */
const char *modconf_bugreport(void);

/**
 * Initialize a modconf structure.
 */
modconf_st *modconf_create(modconf_st *modconf);

/**
 * Free a modconf structure.
 */
void modconf_free(modconf_st *modconf);

/**
 * Return an return code for the last library error encountered.
 */
modconf_return_t modconf_return(modconf_st *modconf);

/**
 * Return an error string for last library error encountered.
 */
const char *modconf_error(modconf_st *modconf);

/**
 * Value of errno in the case of a MODCONF_ERRNO return value.
 */
int modconf_errno(modconf_st *modconf);

/**
 * Set options for a modconf structure.
 */
void modconf_set_options(modconf_st *modconf, modconf_options_t options,
                         uint32_t data);

/**
 * Parse command line arguments.
 */
modconf_return_t modconf_parse_args(modconf_st *modconf, int argc,
                                    char *argv[]);

/**
 * Parse configuration file.
 */
modconf_return_t modconf_parse_file(modconf_st *modconf, const char *file);

/**
 * Print usage information to stdout.
 */
void modconf_usage(modconf_st *modconf);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_H__ */
