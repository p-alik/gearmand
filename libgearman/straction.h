/*
 * Summary: Create string responses from gearman_action enums
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_STRACTION_H__
#define __GEARMAN_STRACTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines */
char *gearman_straction(gearman_st *ptr, gearman_action rc);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_STRACTION_H__ */
