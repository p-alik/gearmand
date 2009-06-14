/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <assert.h>
#include <fnmatch.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "test.h"

/* Prototypes */
long int timedif(struct timeval a, struct timeval b);
void print_break(void);

int main(int argc, char *argv[])
{
  unsigned int x;
  char *collection_to_run;
  char *wildcard;
  world_st world;
  collection_st *collection;
  void *collection_object;
  collection_st *next;
  long int load_time;
  test_return rc;

  memset(&world, 0, sizeof(world_st));
  get_world(&world);
  collection= world.collections;

  if (world.create)
    collection_object= world.create();
  else
    collection_object= NULL;

  if (argc > 1)
    collection_to_run= argv[1];
  else
    collection_to_run= NULL;

  if (argc == 3)
    wildcard= argv[2];
  else
    wildcard= NULL;

  srandom((unsigned int)time(NULL));

  for (next= collection; next->name; next++)
  {
    test_st *run;

    run= next->tests;
    if (collection_to_run && fnmatch(collection_to_run, next->name, 0))
      continue;

    print_break();

    printf("%s\n\n", next->name);
    fprintf(stderr, "%s\n\n", next->name);

    for (x= 0; run->name; run++)
    {
      struct timeval start_time, end_time;
      void *object= NULL;

      if (wildcard && fnmatch(wildcard, run->name, 0))
        continue;

      printf("Testing %-50s", run->name);
      fprintf(stderr, "Testing %-50s", run->name);

      if (run->requires_flush && next->flush)
        next->flush();

      if (next->create)
        object= next->create(collection_object);

      if (next->pre)
      {
        rc= next->pre(object);

        if (rc != TEST_SUCCESS)
        {
          printf("[ skipping ]\n");
          fprintf(stderr, "[ skipping ]\n");

          if (next->destroy)
            next->destroy(object);

          continue;
        }
      }

      gettimeofday(&start_time, NULL);
      rc= run->function(object);
      gettimeofday(&end_time, NULL);

      load_time= timedif(end_time, start_time);

      if (rc == TEST_SUCCESS)
      {
        printf("[ ok     ]\n");
        fprintf(stderr, "[ ok     %ld.%03ld ]\n", load_time / 1000,
                load_time % 1000);
      }
      else
      {
        printf("[ failed ]\n");
        fprintf(stderr, "[ failed %ld.%03ld ]\n", load_time / 1000,
                load_time % 1000);
      }

      if (next->post)
        (void)(next->post(object));

      if (next->destroy)
        next->destroy(object);
    }
  }

  if (world.destroy)
    world.destroy(collection_object);

  print_break();

  printf("Test run complete\n");
  fprintf(stderr, "Test run complete\n");

  print_break();

  return 0;
}

long int timedif(struct timeval a, struct timeval b)
{
  register int us, s;

  us = (int)(a.tv_usec - b.tv_usec);
  us /= 1000;
  s = (int)(a.tv_sec - b.tv_sec);
  s *= 1000;
  return s + us;
}

void print_break(void)
{
  printf("\n==========================================================================\n\n");
  fprintf(stderr, "\n==========================================================================\n\n");
}
