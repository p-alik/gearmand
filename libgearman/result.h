/*
 * Summary: Result structure used for libgearman.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_RESULT_H__
#define __GEARMAN_RESULT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_result_st {
  gearman_action action;
  gearman_allocated is_allocated;
  gearman_st *root;
  gearman_byte_array_st handle;
  gearman_byte_array_st value;
  uint32_t numerator;
  uint32_t denominator;
};

/* Result Struct */
void gearman_result_free(gearman_result_st *result);
void gearman_result_reset(gearman_result_st *ptr);
gearman_result_st *gearman_result_create(gearman_st *ptr, 
                                         gearman_result_st *result);
char *gearman_result_value(gearman_result_st *ptr);
size_t gearman_result_length(gearman_result_st *ptr);

char *gearman_result_handle(gearman_result_st *ptr);
size_t gearman_result_handle_length(gearman_result_st *ptr);

gearman_return gearman_result_set_value(gearman_result_st *ptr, char *value, size_t length);
gearman_return gearman_result_set_handle(gearman_result_st *ptr, char *handle, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_RESULT_H__ */
