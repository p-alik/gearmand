/**
 * @file
 * @brief libtokyocabinet Queue Storage Declarations
 */

#ifndef __GEARMAN_QUEUE_LIBTOKYOCABINET_H__
#define __GEARMAN_QUEUE_LIBTOKYOCABINET_H__

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
gearman_return_t gearman_queue_libtokyocabinet_conf(gearman_conf_st *conf);

/**
 * Initialize the queue.
 */
gearman_return_t gearman_queue_libtokyocabinet_init(gearman_st *gearman,
                                                    gearman_conf_st *conf);

/**
 * De-initialize the queue.
 */
gearman_return_t gearman_queue_libtokyocabinet_deinit(gearman_st *gearman);

/**
 * Initialize the queue for a gearmand object.
 */
gearman_return_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                     gearman_conf_st *conf);

/**
 * De-initialize the queue for a gearmand object.
 */
gearman_return_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *gearmand);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_QUEUE_LIBTOKYOCABINET_H__ */
