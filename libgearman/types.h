/*
 * Summary: Types for libgearman
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_TYPES_H__
#define __GEARMAN_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gearman_st gearman_st;
typedef struct gearman_result_st gearman_result_st;
typedef struct gearman_job_st gearman_job_st;
typedef struct gearman_byte_array_st gearman_byte_array_st;
typedef struct gearman_server_st gearman_server_st;
typedef struct gearman_worker_st gearman_worker_st;

#ifdef GEARMAN_INTERNAL 
typedef struct giov_st giov_st;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_TYPES_H__ */
