/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief modconf module definitions
 */

#include "modconf_common.h"

/*
 * Public definitions
 */

modconf_module_st *gmodconf_module_create(modconf_st *modconf,
                                          modconf_module_st *module,
                                          const char *name)
{
  modconf_module_st **module_list;

  if (module == NULL)
  {
    module= malloc(sizeof(modconf_module_st));
    if (module == NULL)
    {
      MODCONF_ERROR_SET(modconf, "modconf_module_create", "malloc");
      return NULL;
    }

    module->options= MODCONF_MODULE_ALLOCATED;
  }
  else
    module->options= 0;

  module->modconf= modconf;
  module->name= name;
  module->current_option= 0;
  module->current_value= 0;

  module_list= realloc(modconf->module_list, sizeof(modconf_module_st *) *
                       (modconf->module_count + 1));
  if (module_list == NULL)
  {
    gmodconf_module_free(module);
    MODCONF_ERROR_SET(modconf, "modconf_module_create", "realloc");
    return NULL;
  }

  modconf->module_list= module_list;
  modconf->module_list[modconf->module_count]= module;
  modconf->module_count++;

  return module;
}

void gmodconf_module_free(modconf_module_st *module)
{
  if (module->options & MODCONF_MODULE_ALLOCATED)
    free(module);
}

modconf_module_st *gmodconf_module_find(modconf_st *modconf, const char *name)
{
  uint32_t x;

  for (x= 0; x < modconf->module_count; x++)
  {
    if (name == NULL || modconf->module_list[x]->name == NULL)
    {
      if (name == modconf->module_list[x]->name)
        return modconf->module_list[x];
    }
    else if (!strcmp(name, modconf->module_list[x]->name))
      return modconf->module_list[x];
  }

  return NULL;
}

void gmodconf_module_add_option(modconf_module_st *module, const char *name,
                                int short_name, const char *value_name,
                                const char *help)
{
  modconf_st *modconf= module->modconf;
  modconf_option_st *option_list;
  struct option *option_getopt;
  uint32_t x;

  /* Unset short_name if it's already in use. */
  for (x= 0; x < modconf->option_count && short_name != 0; x++)
  {
    if (modconf->option_getopt[x].val == short_name)
      short_name= 0;
  }

  /* Make room in option lists. */
  option_list= realloc(modconf->option_list,
                       sizeof(modconf_option_st) * (modconf->option_count + 1));
  if (option_list == NULL)
  {
    MODCONF_ERROR_SET(modconf, "modconf_module_add_option", "realloc");
    modconf->last_return= MODCONF_MEMORY_ALLOCATION_FAILURE;
    return;
  }

  modconf->option_list= option_list;
  option_list= &modconf->option_list[modconf->option_count];

  option_getopt= realloc(modconf->option_getopt,
                         sizeof(struct option) * (modconf->option_count + 2));
  if (option_getopt == NULL)
  {
    MODCONF_ERROR_SET(modconf, "modconf_module_add_option", "realloc");
    modconf->last_return= MODCONF_MEMORY_ALLOCATION_FAILURE;
    return;
  }

  modconf->option_getopt= option_getopt;
  option_getopt= &modconf->option_getopt[modconf->option_count];

  modconf->option_count++;
  memset(&modconf->option_getopt[modconf->option_count], 0,
         sizeof(sizeof(struct option)));

  option_list->module= module;
  option_list->name= name;
  option_list->value_name= value_name;
  option_list->help= help;
  option_list->value_list= NULL;
  option_list->value_count= 0;

  if (module->name == NULL)
  {
    /* Allocate this one as well so we can always free option_getopt->name. */
    option_getopt->name= strdup(name);
    if (option_getopt->name == NULL)
    {
      MODCONF_ERROR_SET(modconf, "modconf_module_add_option", "strdup");
      modconf->last_return= MODCONF_MEMORY_ALLOCATION_FAILURE;
      return;
    }
  }
  else
  {
    option_getopt->name= malloc(strlen(module->name) + strlen(name) + 2);
    if (option_getopt->name == NULL)
    {
      MODCONF_ERROR_SET(modconf, "modconf_module_add_option", "malloc");
      modconf->last_return= MODCONF_MEMORY_ALLOCATION_FAILURE;
      return;
    }

    sprintf((char *)option_getopt->name, "%s-%s", module->name, name);
  }

  option_getopt->has_arg= value_name == NULL ? 0 : 1;
  option_getopt->flag= NULL;
  option_getopt->val= short_name;

  /* Add short_name to the short option list. */
  if (short_name != 0 && modconf->short_count < (MODCONF_MAX_OPTION_SHORT - 2))
  {
    modconf->option_short[modconf->short_count++]= (char)short_name;
    if (value_name != NULL)
      modconf->option_short[modconf->short_count++]= ':';
    modconf->option_short[modconf->short_count]= '0';
  }
}

bool gmodconf_module_value(modconf_module_st *module, const char **name,
                           const char **value)
{
  modconf_option_st *option;

  for (; module->current_option < module->modconf->option_count;
       module->current_option++)
  {
    option= &module->modconf->option_list[module->current_option];
    if (option->module != module)
      continue;

    if (module->current_value < option->value_count)
    {
      *name= option->name;
      *value= option->value_list[module->current_value++];
      return true;
    }

    module->current_value= 0;
  }

  module->current_option= 0;

  return false;
}
