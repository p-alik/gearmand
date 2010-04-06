/* Gearman server and library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman conf definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_conf_st *gearman_conf_create(gearman_conf_st *conf)
{
  if (conf == NULL)
  {
    conf= (gearman_conf_st *)malloc(sizeof(gearman_conf_st));
    if (conf == NULL)
      return NULL;

    conf->options.allocated= true;
  }
  else
  {
    conf->options.allocated= false;
  }

  conf->last_return= GEARMAN_SUCCESS;
  conf->last_errno= 0;
  conf->module_count= 0;
  conf->option_count= 0;
  conf->short_count= 0;
  conf->module_list= NULL;
  conf->option_list= NULL;
  conf->option_short[0]= 0;
  conf->last_error[0]= 0;

  /* We always need a NULL terminated getopt list. */
  conf->option_getopt= (struct option *)malloc(sizeof(struct option));
  if (conf->option_getopt == NULL)
  {
    gearman_conf_free(conf);
    return NULL;
  }

  memset(conf->option_getopt, 0, sizeof(sizeof(struct option)));

  return conf;
}

void gearman_conf_free(gearman_conf_st *conf)
{
  uint32_t x;

  for (x= 0; x < conf->module_count; x++)
    gearman_conf_module_free(conf->module_list[x]);

  for (x= 0; x < conf->option_count; x++)
  {
    free((char *)conf->option_getopt[x].name);

    if (conf->option_list[x].value_list != NULL)
      free(conf->option_list[x].value_list);
  }

  if (conf->module_list != NULL)
    free(conf->module_list);

  if (conf->option_list != NULL)
    free(conf->option_list);

  if (conf->option_getopt != NULL)
    free(conf->option_getopt);

  if (conf->options.allocated)
    free(conf);
}

gearman_return_t gearman_conf_return(gearman_conf_st *conf)
{
  return conf->last_return;
}

const char *gearman_conf_error(gearman_conf_st *conf)
{
  return (const char *)(conf->last_error);
}

int gearman_conf_errno(gearman_conf_st *conf)
{
  return conf->last_errno;
}

gearman_return_t gearman_conf_parse_args(gearman_conf_st *conf, int argc,
                                         char *argv[])
{
  int c;
  int opt_index;
  gearman_conf_option_st *option;
  char **value_list;

  /* This will supress errors being printed to stderr. */
  opterr= 0;

  while (1)
  {
    c= getopt_long(argc, argv, conf->option_short, conf->option_getopt,
                   &opt_index);
    if (c == -1)
      break;

    switch (c)
    {
    case 0:
      /* We have a long option, use index. */
      break;

    default:
      /* Find the long option index that matches the short character. */
      for (opt_index= 0; opt_index < (int)conf->option_count; opt_index++)
      {
        if (conf->option_getopt[opt_index].val == c)
          break;
      }

      if (opt_index == (int)conf->option_count)
      {
        gearman_conf_error_set(conf, "ERROR", " Unknown option: %s", argv[optind - 1]);
        return GEARMAN_UNKNOWN_OPTION;
      }
    }

    option= &conf->option_list[opt_index];
    value_list= (char **)realloc(option->value_list,
                                 sizeof(char *) * (option->value_count + 1));
    if (value_list == NULL)
    {
      gearman_conf_error_set(conf, "gearman_conf_parse_args", " realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    option->value_list= value_list;
    option->value_list[option->value_count]= optarg;
    option->value_count++;
  }

  if (optind < argc)
  {
    gearman_conf_error_set(conf, "gearman_conf_parse_args", "Unknown option: %s", argv[optind]);
    return GEARMAN_UNKNOWN_OPTION;
  }

  return GEARMAN_SUCCESS;
}

void gearman_conf_usage(gearman_conf_st *conf)
{
  uint32_t x;
  uint32_t y;
  gearman_conf_module_st *module;
  gearman_conf_option_st *option;
  bool print_header;
  char display[GEARMAN_CONF_DISPLAY_WIDTH];
  size_t max_length;
  size_t new_length;
  const char *help_start;
  const char *help_next;

  for (x= 0; x < conf->module_count; x++)
  {
    module= conf->module_list[x];
    print_header= true;

    /* Find the maximum option length for this module. */
    max_length= 0;

    for (y= 0; y < conf->option_count; y++)
    {
      if (module != conf->option_list[y].module)
        continue;

      new_length= strlen(conf->option_getopt[y].name);

      if (conf->option_list[y].value_name != NULL)
        new_length+= strlen(conf->option_list[y].value_name) + 1;

      if (new_length > max_length)
        max_length= new_length;
    }

    /* Truncate option length if it's too long. */
    max_length+= 8;
    if (max_length > GEARMAN_CONF_DISPLAY_WIDTH - 2)
      max_length= GEARMAN_CONF_DISPLAY_WIDTH - 2;

    /* Print out all options.allocated for this module. */
    for (y= 0; y < conf->option_count; y++)
    {
      option= &conf->option_list[y];

      if (option->module != module)
        continue;

      if (print_header)
      {
        printf("\n%s Options:\n\n",
               module->name == NULL ? "Main" : module->name);
        print_header= false;
      }

      /* Build string with possible short option. */
      snprintf(display, GEARMAN_CONF_DISPLAY_WIDTH, "     --%s%s%s%80s",
               conf->option_getopt[y].name,
               option->value_name == NULL ? "" : "=",
               option->value_name == NULL ? "" : option->value_name, "");
      display[max_length - 1]= ' ';
      display[max_length]= 0;

      if (conf->option_getopt[y].val != 0)
      {
        display[1]= '-';
        display[2]= (signed char)conf->option_getopt[y].val;
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

      while (strlen(help_start) > (GEARMAN_CONF_DISPLAY_WIDTH - max_length))
      {
        help_next= help_start + (GEARMAN_CONF_DISPLAY_WIDTH - max_length);
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
