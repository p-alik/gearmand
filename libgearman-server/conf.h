/* Gearman server and library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Configuration Declarations
 */

#ifndef __GEARMAN_SERVER_CONF_H__
#define __GEARMAN_SERVER_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_conf Configuration Declarations
 * @ingroup gearman_server
 * @{
 */

struct gearman_conf_option_st
{
  size_t value_count;
  gearman_conf_module_st *module;
  const char *name;
  const char *value_name;
  const char *help;
  char **value_list;
};

struct gearman_conf_st
{
  struct {
    bool allocated;
  } options;
  gearman_return_t last_return;
  int last_errno;
  size_t module_count;
  size_t option_count;
  size_t short_count;
  gearman_conf_module_st **module_list;
  gearman_conf_option_st *option_list;
  struct option *option_getopt;
  char option_short[GEARMAN_CONF_MAX_OPTION_SHORT];
  char last_error[GEARMAN_MAX_ERROR_SIZE];
};

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

#endif /* __GEARMAN_SERVER_CONF_H__ */
