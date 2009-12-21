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

#ifndef __GEARMAN_COMMAND_H__
#define __GEARMAN_COMMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup gearman_packet
 */
struct gearman_command_info_st
{
  const char *name;
  const uint8_t argc;
  const bool data;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_COMMAND_H__ */
