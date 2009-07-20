/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef __GEARMAN_TEST_H__
#define __GEARMAN_TEST_H__

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct world_st world_st;
typedef struct collection_st collection_st;
typedef struct test_st test_st;

typedef enum {
  TEST_SUCCESS,
  TEST_FAILURE,
  TEST_MEMORY_ALLOCATION_FAILURE,
  TEST_MAXIMUM_RETURN /* Always add new error code before */
} test_return;

struct test_st {
  const char *name;
  uint8_t requires_flush;
  test_return (*function)(void *object);
};

struct collection_st {
  const char *name;
  test_return (*flush)(void);
  void *(*create)(void *collection_object);
  void (*destroy)(void *object);
  test_return (*pre)(void *object);
  test_return (*post)(void *object);
  test_st *tests;
};

struct world_st {
  collection_st *collections;
  void *(*create)(void);
  void (*destroy)(void *collection_object);
};

/* How we make all of this work :) */
void get_world(world_st *world);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_TEST_H__ */
