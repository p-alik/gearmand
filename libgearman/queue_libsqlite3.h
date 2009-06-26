/* Gearman server and library 
* Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libsqlite3 Queue Storage Declarations
 */

#ifndef __GEARMAN_QUEUE_LIBSQLITE3_H__
#define __GEARMAN_QUEUE_LIBSQLITE3_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_queue_libsqlite3 libsqlite3 Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/**
 * Get module configuration options.
 */
GEARMAN_API
gearman_return_t gearman_queue_libsqlite3_conf(gearman_conf_st *conf);

/**
 * Initialize the queue.
 */
GEARMAN_API
gearman_return_t gearman_queue_libsqlite3_init(gearman_st *gearman,
                                               gearman_conf_st *conf);

/**
 * De-initialize the queue.
 */
GEARMAN_API
gearman_return_t gearman_queue_libsqlite3_deinit(gearman_st *gearman);

/**
 * Initialize the queue for a gearmand object.
 */
GEARMAN_API
gearman_return_t gearmand_queue_libsqlite3_init(gearmand_st *gearmand,
                                                gearman_conf_st *conf);

/**
 * De-initialize the queue for a gearmand object.
 */
GEARMAN_API
gearman_return_t gearmand_queue_libsqlite3_deinit(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_QUEUE_LIBSQLITE3_H__ */
