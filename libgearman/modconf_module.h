/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief modconf module declarations
 */

#ifndef __MODCONF_MODULE_H__
#define __MODCONF_MODULE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup modconf_module modconf module interface
 * @{
 */

/**
 * Initialize a modconf module structure.
 */
modconf_module_st *gmodconf_module_create(modconf_st *modconf,
                                          modconf_module_st *module,
                                          const char *name);

/**
 * Free a modconf module structure.
 */
void gmodconf_module_free(modconf_module_st *module);

/**
 * Find a modconf module structure by name.
 */
modconf_module_st *gmodconf_module_find(modconf_st *modconf, const char *name);

/**
 * Add option for a module.
 */
void gmodconf_module_add_option(modconf_module_st *module, const char *name,
                                int short_name, const char *value_name,
                                const char *help);

/**
 * Loop through all values that were given for a set of module options.
 */
bool gmodconf_module_value(modconf_module_st *module, const char **name,
                           const char **value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_MODULE_H__ */
