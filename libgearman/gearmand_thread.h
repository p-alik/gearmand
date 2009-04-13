/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand Thread Declarations
 */

#ifndef __GEARMAND_THREAD_H__
#define __GEARMAND_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand_thread Gearmand Threads
 * Thread handling for gearmand.
 * @{
 */

/**
 * Create a new gearmand thread.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @return Standard gearman return value.
 */
gearman_return_t gearmand_thread_create(gearmand_st *gearmand);

/**
 * Free resources used by a thread.
 * @param thread Thread previously initialized with gearmand_thread_create.
 */
void gearmand_thread_free(gearmand_thread_st *thread);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_THREAD_H__ */
