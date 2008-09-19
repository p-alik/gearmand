/*
 * Summary: String structure used for libgearman.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_BYTE_ARRAY_H__
#define __GEARMAN_BYTE_ARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_byte_array_st {
  gearman_st *root;
  gearman_allocated is_allocated;
  size_t current_size;
  size_t block_size;
  char *end;
  char *byte_array;
};

#define gearman_byte_array_length(A) (size_t)((A)->end - (A)->byte_array)
#define gearman_byte_array_set_length(A, B) (A)->end= (A)->byte_array + B
#define gearman_byte_array_size(A) (A)->current_size
#define gearman_byte_array_value(A) (A)->byte_array

gearman_byte_array_st *gearman_byte_array_create(gearman_st *ptr,
                                             gearman_byte_array_st *byte_array,
                                             size_t initial_size);
gearman_return gearman_byte_array_check(gearman_byte_array_st *byte_array,
                                        size_t need);
char *gearman_byte_array_c_copy(gearman_byte_array_st *byte_array);
gearman_return gearman_byte_array_append_character(gearman_byte_array_st *byte_array,
                                                   char character);
gearman_return gearman_byte_array_append(gearman_byte_array_st *byte_array,
                                         const char *value, size_t length);
size_t gearman_byte_array_backspace(gearman_byte_array_st *byte_array,
                                    size_t remove);
gearman_return gearman_byte_array_reset(gearman_byte_array_st *byte_array);
gearman_return gearman_byte_array_store(gearman_byte_array_st *byte_array,
                                        const char *value, size_t length);
void gearman_byte_array_free(gearman_byte_array_st *byte_array);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_BYTE_ARRAY_H__ */
