/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman core definitions
 */

#include "modconf_common.h"

/*
 * Public definitions
 */

const char *gmodconf_version(void)
{
    return PACKAGE_VERSION;
}

const char *gmodconf_bugreport(void)
{
    return PACKAGE_BUGREPORT;
}

modconf_st *gmodconf_create(modconf_st *modconf)
{
  if (modconf == NULL)
  {
    modconf= malloc(sizeof(modconf_st));
    if (modconf == NULL)
      return NULL;

    modconf->options= MODCONF_ALLOCATED;
  }
  else
    modconf->options= 0;

  modconf->module_list= NULL;
  modconf->module_count= 0;
  modconf->option_list= NULL;
  modconf->option_count= 0;
  modconf->option_short[0]= 0;
  modconf->short_count= 0;
  modconf->last_return= MODCONF_SUCCESS;
  modconf->last_errno= 0;
  modconf->last_error[0]= 0;

  /* We always need a NULL terminated getopt list. */
  modconf->option_getopt= malloc(sizeof(struct option));
  if (modconf->option_getopt == NULL)
  {
    gmodconf_free(modconf);
    return NULL;
  }

  memset(modconf->option_getopt, 0, sizeof(sizeof(struct option)));

  return modconf;
}

void gmodconf_free(modconf_st *modconf)
{
  uint32_t x;

  for (x= 0; x < modconf->module_count; x++)
    gmodconf_module_free(modconf->module_list[x]);

  for (x= 0; x < modconf->option_count; x++)
  {
    free((char *)modconf->option_getopt[x].name);

    if (modconf->option_list[x].value_list != NULL)
      free(modconf->option_list[x].value_list);
  }

  if (modconf->module_list != NULL)
    free(modconf->module_list);

  if (modconf->option_list != NULL)
    free(modconf->option_list);

  if (modconf->option_getopt != NULL)
    free(modconf->option_getopt);

  if (modconf->options & MODCONF_ALLOCATED)
    free(modconf);
}

modconf_return_t gmodconf_return(modconf_st *modconf)
{
  return modconf->last_return;
}

const char *gmodconf_error(modconf_st *modconf)
{
  return (const char *)(modconf->last_error);
}

int gmodconf_errno(modconf_st *modconf)
{
  return modconf->last_errno;
}

void gmodconf_set_options(modconf_st *modconf, modconf_options_t options,
                          uint32_t data)
{
  if (data)
    modconf->options |= options;
  else
    modconf->options &= ~options;
}

modconf_return_t gmodconf_parse_args(modconf_st *modconf, int argc,
                                     char *argv[])
{
  int c;
  int index;
  modconf_option_st *option;
  char **value_list;

  /* This will supress errors being printed to stderr. */
  opterr= 0;

  while (1)
  {
    c= getopt_long(argc, argv, modconf->option_short, modconf->option_getopt,
                   &index);
    if (c == -1)
      break;

    switch (c)
    {
    case 0:
      /* We have a long option, use index. */
      break;

    default:
      /* Find the long option index that matches the short character. */
      for (index= 0; index < (int)modconf->option_count; index++)
      {
        if (modconf->option_getopt[index].val == c)
          break;
      }

      if (index == (int)modconf->option_count)
      {
        MODCONF_ERROR_SET(modconf, "ERROR",
                          " Unknown option: %s", argv[optind - 1]);
        return MODCONF_UNKNOWN_OPTION;
      }
    }

    option= &modconf->option_list[index];
    value_list= realloc(option->value_list,
                        sizeof(char *) * (option->value_count + 1));
    if (value_list == NULL)
    {
      MODCONF_ERROR_SET(modconf, "modconf_parswe_args", "realloc");
      return MODCONF_MEMORY_ALLOCATION_FAILURE;
    }

    option->value_list= value_list;
    option->value_list[option->value_count]= optarg;
    option->value_count++;
  }

  if (optind < argc)
  {
    MODCONF_ERROR_SET(modconf, "modconf_parse_args", "Unknown option: %s",
                      argv[optind]);
    return MODCONF_UNKNOWN_OPTION;
  }

  return MODCONF_SUCCESS;
}

modconf_return_t gmodconf_parse_file(modconf_st *modconf, const char *file)
{
  (void)modconf;
  (void)file;
  return MODCONF_SUCCESS;
}

void gmodconf_usage(modconf_st *modconf)
{
  uint32_t x;
  uint32_t y;
  modconf_module_st *module;
  modconf_option_st *option;
  bool print_header;
  char display[MODCONF_DISPLAY_WIDTH];
  size_t max_length;
  size_t new_length;
  const char *help_start;
  const char *help_next;

  for (x= 0; x < modconf->module_count; x++)
  {
    module= modconf->module_list[x];
    print_header= true;

    /* Find the maximum option length for this module. */
    max_length= 0;

    for (y= 0; y < modconf->option_count; y++)
    {
      if (module != modconf->option_list[y].module)
        continue;

      new_length= strlen(modconf->option_getopt[y].name);

      if (modconf->option_list[y].value_name != NULL)
        new_length+= strlen(modconf->option_list[y].value_name) + 1;

      if (new_length > max_length)
        max_length= new_length;
    }

    /* Truncate option length if it's too long. */
    max_length+= 8;
    if (max_length > MODCONF_DISPLAY_WIDTH - 2)
      max_length= MODCONF_DISPLAY_WIDTH - 2;

    /* Print out all options for this module. */
    for (y= 0; y < modconf->option_count; y++)
    {
      option= &modconf->option_list[y];

      if (option->module != module)
        continue;

      if (print_header)
      {
        printf("\n%s Options:\n\n",
               module->name == NULL ? "Main" : module->name);
        print_header= false;
      }

      /* Build string with possible short option. */
      snprintf(display, MODCONF_DISPLAY_WIDTH, "     --%s%s%s%80s",
               modconf->option_getopt[y].name,
               option->value_name == NULL ? "" : "=",
               option->value_name == NULL ? "" : option->value_name, "");
      display[max_length - 1]= ' ';
      display[max_length]= 0;

      if (modconf->option_getopt[y].val != 0)
      { 
        display[1]= '-';
        display[2]= modconf->option_getopt[y].val;
        display[3]= ',';
      }

      /* If there is no help, just print the option. */
      if (option->help == NULL)
      {
        printf("%s\n", display);
        continue;
      }

      /* Make sure the help string is properly wrapped. */
      help_start= option->help;

      while (strlen(help_start) > (MODCONF_DISPLAY_WIDTH - max_length))
      {
        help_next= help_start + (MODCONF_DISPLAY_WIDTH - max_length);
        while (help_next != help_start && *help_next != ' ')
          help_next--;

        if (help_next == help_start)
          help_next= strchr(help_start, ' ');

        if (help_next == NULL)
          break;

        printf("%s%.*s\n", display, (int)(help_next - help_start), help_start);
        memset(display, ' ', max_length - 1);

        help_start= help_next + 1;
      }

      printf("%s%s\n", display, help_start);
    }
  }

  printf("\n\n");
}
