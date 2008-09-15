/*
 * Summary: Dispatch function for libgearman
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_DISPATCH_H__
#define __GEARMAN_DISPATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GEARMAN_INTERNAL 
gearman_return gearman_dispatch(gearman_server_st *ptr, 
                                gearman_action action,
                                giov_st *giov,
                                uint8_t with_flush);

/* IO Vector */
struct giov_st {
  char *arg;
  size_t arg_length;
};

#endif

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_DISPATCH_H__ */
