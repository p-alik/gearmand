/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman State Definitions
 */

#include "common.h"

/**
 * @addtogroup gearman_universal_static Static Gearman Declarations
 * @ingroup gearman_universal
 * @{
 */

/**
 * Names of the verbose levels provided.
 */
static const char *_verbose_name[GEARMAN_VERBOSE_MAX]=
{
  "NEVER",
  "FATAL",
  "ERROR",
  "INFO",
  "DEBUG",
  "CRAZY"
};

/** @} */

/*
 * Public Definitions
 */

const char *gearman_version(void)
{
    return PACKAGE_VERSION;
}

const char *gearman_bugreport(void)
{
    return PACKAGE_BUGREPORT;
}

const char *gearman_verbose_name(gearman_verbose_t verbose)
{
  if (verbose >= GEARMAN_VERBOSE_MAX)
    return "UNKNOWN";

  return _verbose_name[verbose];
}

gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *function,
                                       void *context)
{
  const char *ptr= servers;
  size_t x;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  gearman_return_t ret;

  if (ptr == NULL)
    return (*function)(NULL, 0, (void *)context);

  while (1)
  {
    x= 0;

    while (*ptr != 0 && *ptr != ',' && *ptr != ':')
    {
      if (x < (NI_MAXHOST - 1))
        host[x++]= *ptr;

      ptr++;
    }

    host[x]= 0;

    if (*ptr == ':')
    {
      ptr++;
      x= 0;

      while (*ptr != 0 && *ptr != ',')
      {
        if (x < (NI_MAXSERV - 1))
          port[x++]= *ptr;

        ptr++;
      }

      port[x]= 0;
    }
    else
      port[0]= 0;

    ret= (*function)(host, (in_port_t)atoi(port), (void *)context);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    if (*ptr == 0)
      break;

    ptr++;
  }

  return GEARMAN_SUCCESS;
}
