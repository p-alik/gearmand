/**
 * @file
 * @brief libtokyocabinet Queue Storage Declarations
 */

#ifndef __GEARMAN_QUEUE_LIBTOKYOCABINET_H__
#define __GEARMAN_QUEUE_LIBTOKYOCABINET_H__

#include <libgearman/modconf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_queue_libtokyocabinet libtokyocabinet Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/**
 * Get module configuration options.
 */
modconf_return_t gearman_queue_libtokyocabinet_modconf(modconf_st *modconf);

/**
 * Initialize the queue.
 */
gearman_return_t gearman_queue_libtokyocabinet_init(gearman_st *gearman,
                                                    modconf_st *modconf);

/**
 * De-initialize the queue.
 */
gearman_return_t gearman_queue_libtokyocabinet_deinit(gearman_st *gearman);

/**
 * Initialize the queue for a gearmand object.
 */
gearman_return_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                     modconf_st *modconf);

/**
 * De-initialize the queue for a gearmand object.
 */
gearman_return_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_QUEUE_LIBTOKYOCABINET_H__ */
