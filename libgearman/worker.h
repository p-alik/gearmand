/*
 * Summary: Worker structure used for libgearman.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_WORKER_H__
#define __GEARMAN_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_worker_st {
  gearman_allocated is_allocated;
  int8_t server_key;
  gearman_st *root;
  time_t time_out;
  gearman_byte_array_st client_id;
  gearman_byte_array_st function;
  gearman_byte_array_st handle;
  uint32_t flags;
};

gearman_worker_st *gearman_worker_create(gearman_st *gear, 
                                         gearman_worker_st *ptr);
void gearman_worker_free(gearman_worker_st *worker);
void gearman_worker_reset(gearman_worker_st *ptr);

gearman_return gearman_worker_do(gearman_worker_st *ptr, char *function);
gearman_return gearman_worker_take(gearman_worker_st *ptr,
                                   gearman_result_st *result);

gearman_return gearman_worker_update(gearman_worker_st *ptr, uint32_t numerator,
                                     uint32_t denominator);

gearman_return gearman_worker_answer(gearman_worker_st *ptr,
                                     gearman_result_st *result);

gearman_return gearman_worker_set_id(gearman_worker_st *ptr, const char *id);
void gearman_worker_set_timeout(gearman_worker_st *ptr, time_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_WORKER_H__ */
