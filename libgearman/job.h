/*
 * Summary: Job functions used for libgearman.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_JOB_H__
#define __GEARMAN_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_job_st {
  gearman_action action;
  gearman_allocated is_allocated;
  gearman_st *root;
  gearman_byte_array_st function;
  gearman_byte_array_st unique;
  gearman_byte_array_st value;
  gearman_byte_array_st handle;
  uint32_t flags;
  uint32_t cursor;
};

/* Result Struct */
void gearman_job_free(gearman_job_st *job);
void gearman_job_reset(gearman_job_st *ptr);

gearman_job_st *gearman_job_create(gearman_st *ptr, gearman_job_st *job);

gearman_return gearman_job_submit(gearman_job_st *ptr);
gearman_return gearman_job_result(gearman_job_st *ptr, 
                                  gearman_result_st *result);

gearman_return gearman_job_set_function(gearman_job_st *ptr, char *function);
gearman_return gearman_job_set_value(gearman_job_st *ptr, char *value,
                                     size_t length);

uint64_t gearman_job_get_behavior(gearman_st *ptr, gearman_job_behavior flag);
gearman_return gearman_job_set_behavior(gearman_job_st *ptr, 
                                        gearman_job_behavior flag, 
                                        uint64_t data);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_JOB_H__ */
