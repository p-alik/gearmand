/*
 * Summary: Call quit on a server
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_BEHAVIOR_H__
#define __GEARMAN_BEHAVIOR_H__

#ifdef __cplusplus
extern "C" {
#endif

gearman_return gearman_behavior_set(gearman_st *ptr, 
                                    gearman_behavior flag, 
                                    uint64_t data);

uint64_t gearman_behavior_get(gearman_st *ptr, 
                              gearman_behavior flag);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_BEHAVIOR_H__ */
