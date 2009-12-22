/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"
#include "test_worker.h"

#define CLIENT_TEST_PORT 32123


collection_st collection[] ={
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
}
