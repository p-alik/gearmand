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


gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *function,
                                       const void *context)
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
