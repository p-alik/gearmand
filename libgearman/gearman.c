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
