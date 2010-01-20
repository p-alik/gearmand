/**
 * @file
 * @brief libtokyocabinet Queue Storage Declarations
 */

#ifndef __GEARMAN_QUEUE_LIBTOKYOCABINET_H__
#define __GEARMAN_QUEUE_LIBTOKYOCABINET_H__

/**
 * It is unclear from tokyocabinet's public headers what, if any, limit there is. 4k seems sane.
 */
#define GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN 4096

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
GEARMAN_API
gearman_return_t gearman_server_queue_libtokyocabinet_conf(gearman_conf_st *conf);

/**
 * Initialize the queue.
 */
GEARMAN_API    
gearman_return_t gearman_queue_libtokyocabinet_init(gearman_server_st *server,
                                                    gearman_conf_st *conf);

/**
 * De-initialize the queue.
 */
GEARMAN_API
gearman_return_t gearman_queue_libtokyocabinet_deinit(gearman_server_st *server);

/**
 * Initialize the queue for a gearmand object.
 */
GEARMAN_API
gearman_return_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                     gearman_conf_st *conf);

/**
 * De-initialize the queue for a gearmand object.
 */
GEARMAN_API    
gearman_return_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *server);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_QUEUE_LIBTOKYOCABINET_H__ */
