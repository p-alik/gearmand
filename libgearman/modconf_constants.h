/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Defines, typedefs, and enums
 */

#ifndef __MODCONF_CONSTANTS_H__
#define __MODCONF_CONSTANTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup modconf_constants Gearman Constants
 * @{
 */

/* Defines. */
#define MODCONF_MAX_ERROR_SIZE 1024
#define MODCONF_MAX_OPTION_SHORT 128
#define MODCONF_DISPLAY_WIDTH 80

/* Types. */
typedef struct modconf_st modconf_st;
typedef struct modconf_option_st modconf_option_st;
typedef struct modconf_module_st modconf_module_st;

/**
 * Return codes.
 */
typedef enum
{
  MODCONF_SUCCESS,
  MODCONF_MEMORY_ALLOCATION_FAILURE,
  MODCONF_UNKNOWN_OPTION,
  MODCONF_MAX_RETURN /* Always add new error code before */
} modconf_return_t;

/** @} */

/**
 * @ingroup modconf
 * Options for modconf_st.
 */
typedef enum
{
  MODCONF_ALLOCATED= (1 << 0)
} modconf_options_t;

/**
 * @ingroup modconf_module
 * Options for modconf_module_st.
 */
typedef enum
{
  MODCONF_MODULE_ALLOCATED= (1 << 0)
} modconf_module_options_t;

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_CONSTANTS_H__ */
