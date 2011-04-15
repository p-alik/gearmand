/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief System Include Files
 */

#pragma once

#include <config.h>
#include <stdint.h>

#define BUILDING_LIBGEARMAN
#define GEARMAN_CORE

#include <libgearman/gearman.h>

/* These are private not to be installed headers */
#include <libgearman/byteorder.h>
#include <libgearman/function.h>
