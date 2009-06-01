/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Struct definitions
 */

#ifndef __MODCONF_STRUCTS_H__
#define __MODCONF_STRUCTS_H__

#ifdef __cplusplus
extern "C" {
#endif
 
/**
 * @ingroup modconf
 */
struct modconf_st
{
  modconf_module_st **module_list;
  modconf_option_st *option_list;
  struct option *option_getopt;
  size_t module_count;
  size_t option_count;
  size_t short_count;
  int last_errno;
  modconf_options_t options;
  modconf_return_t last_return;
  char option_short[MODCONF_MAX_OPTION_SHORT];
  char last_error[MODCONF_MAX_ERROR_SIZE];
};

/**
 * @ingroup modconf
 */
struct modconf_option_st
{
  modconf_module_st *module;
  const char *name;
  const char *value_name;
  const char *help;
  char **value_list;
  size_t value_count;
};

/**
 * @ingroup modconf_module
 */
struct modconf_module_st
{
  modconf_st *modconf;
  const char *name;
  modconf_module_options_t options;
  size_t current_option;
  size_t current_value;
};

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_STRUCTS_H__ */
