/*
 * Summary: Response function for libgearman
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_RESPONSE_H__
#define __GEARMAN_RESPONSE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines */
gearman_return gearman_response(gearman_server_st *ptr, 
                                gearman_byte_array_st *handle,
                                gearman_result_st *result);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_RESPONSE_H__ */
