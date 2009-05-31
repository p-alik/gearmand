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
modconf_module_st *modconf_module_create(modconf_st *modconf,
                                         modconf_module_st *module,
                                         const char *name);

/**
 * Free a modconf module structure.
 */
void modconf_module_free(modconf_module_st *module);

/**
 * Add option for a module.
 */
void modconf_module_add_option(modconf_module_st *module, const char *name,
                               int short_name, const char *value_name,
                               const char *help);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_MODULE_H__ */
