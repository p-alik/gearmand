/* Gearman server and library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman conf module definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_conf_module_st *gearman_conf_module_create(gearman_conf_st *conf,
                                                   gearman_conf_module_st *module,
                                                   const char *name)
{
  gearman_conf_module_st **module_list;

  if (module == NULL)
  {
    module= (gearman_conf_module_st *)malloc(sizeof(gearman_conf_module_st));
    if (module == NULL)
    {
      gearman_conf_error_set(conf, "gearman_conf_module_create", "malloc");
      return NULL;
    }

    module->options.allocated= true;
  }
  else
  {
    module->options.allocated= false;
  }

  module->current_option= 0;
  module->current_value= 0;
  module->conf= conf;
  module->name= name;

  module_list= (gearman_conf_module_st **)realloc(conf->module_list, sizeof(gearman_conf_module_st *) *
                                                  (conf->module_count + 1));
  if (module_list == NULL)
  {
    gearman_conf_module_free(module);
    gearman_conf_error_set(conf, "gearman_conf_module_create", "realloc");
    return NULL;
  }

  conf->module_list= module_list;
  conf->module_list[conf->module_count]= module;
  conf->module_count++;

  return module;
}

void gearman_conf_module_free(gearman_conf_module_st *module)
{
  if (module->options.allocated)
    free(module);
}

gearman_conf_module_st *gearman_conf_module_find(gearman_conf_st *conf,
                                                 const char *name)
{
  for (uint32_t x= 0; x < conf->module_count; x++)
  {
    if (name == NULL || conf->module_list[x]->name == NULL)
    {
      if (name == conf->module_list[x]->name)
        return conf->module_list[x];
    }
    else if (!strcmp(name, conf->module_list[x]->name))
      return conf->module_list[x];
  }

  return NULL;
}

void gearman_conf_module_add_option(gearman_conf_module_st *module,
                                    const char *name, int short_name,
                                    const char *value_name, const char *help)
{
  gearman_conf_st *conf= module->conf;
  gearman_conf_option_st *option_list;
  struct option *option_getopt;

  /* Unset short_name if it's already in use. */
  for (uint32_t x= 0; x < conf->option_count && short_name != 0; x++)
  {
    if (conf->option_getopt[x].val == short_name)
      short_name= 0;
  }

  /* Make room in option lists. */
  option_list= (gearman_conf_option_st *)realloc(conf->option_list, sizeof(gearman_conf_option_st) *
                                                 (conf->option_count + 1));
  if (option_list == NULL)
  {
    gearman_conf_error_set(conf, "gearman_conf_module_add_option", "realloc");
    conf->last_return= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return;
  }

  conf->option_list= option_list;
  option_list= &conf->option_list[conf->option_count];

  option_getopt= (struct option *)realloc(conf->option_getopt,
                                          sizeof(struct option) * (conf->option_count + 2));
  if (option_getopt == NULL)
  {
    gearman_conf_error_set(conf, "gearman_conf_module_add_option", "realloc");
    conf->last_return= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return;
  }

  conf->option_getopt= option_getopt;
  option_getopt= &conf->option_getopt[conf->option_count];

  conf->option_count++;
  memset(&conf->option_getopt[conf->option_count], 0,
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
      gearman_conf_error_set(conf, "gearman_conf_module_add_option", "strdup");
      conf->last_return= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return;
    }
  }
  else
  {
    // 2 is the - plus null for the const char string
    size_t option_string_length= strlen(module->name) + strlen(name) + 2;
    char *option_string= (char *) malloc(option_string_length * sizeof(char));

    if (option_string == NULL)
    {
      gearman_conf_error_set(conf, "gearman_conf_module_add_option", "malloc");
      conf->last_return= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return;
    }

    snprintf(option_string, option_string_length, "%s-%s", module->name, name);

    option_getopt->name= option_string;
  }

  option_getopt->has_arg= value_name == NULL ? 0 : 1;
  option_getopt->flag= NULL;
  option_getopt->val= short_name;

  /* Add short_name to the short option list. */
  if (short_name != 0 &&
      conf->short_count < (GEARMAN_CONF_MAX_OPTION_SHORT - 2))
  {
    conf->option_short[conf->short_count++]= (char)short_name;
    if (value_name != NULL)
      conf->option_short[conf->short_count++]= ':';
    conf->option_short[conf->short_count]= '0';
  }
}

bool gearman_conf_module_value(gearman_conf_module_st *module,
                               const char **name, const char **value)
{
  gearman_conf_option_st *option;

  for (; module->current_option < module->conf->option_count;
       module->current_option++)
  {
    option= &module->conf->option_list[module->current_option];
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
