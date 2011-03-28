/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Definition for gearman_command_info_st
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GEARMAN_CORE
/**
 * @ingroup gearman_packet
 */
struct gearman_command_info_st
{
  const char *name;
  const uint8_t argc; // Number of arguments to commands.
  const bool data;
};
#endif /* GEARMAN_CORE */

#ifdef __cplusplus
}
#endif
