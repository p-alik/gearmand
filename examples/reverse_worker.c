/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static uint8_t *reverse(gearman_job_st *job, uint8_t *workload,
                        size_t workload_size, size_t *result_size,
                        gearman_return *ret_ptr);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return ret;
  gearman_worker_st worker;

  while((c = getopt(argc, argv, "h:p:")) != EOF)
  {
    switch(c)
    {
    case 'h':
      host= optarg;
      break;

    case 'p':
      port= atoi(optarg);
      break;

    default:
      usage(argv[0]);
      exit(1);
    }
  }

  (void)gearman_worker_create(&worker);

  ret= gearman_worker_server_add(&worker, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  ret= gearman_worker_register_function(&worker, "reverse", reverse);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  /* This while loop has no body. */
  while (gearman_worker_work(&worker) == GEARMAN_SUCCESS);
  fprintf(stderr, "%s\n", gearman_worker_error(&worker));

  gearman_worker_free(&worker);

  return 0;
}

static uint8_t *reverse(gearman_job_st *job, uint8_t *workload,
                        size_t workload_size, size_t *result_size,
                        gearman_return *ret_ptr)
{
  uint8_t *result;
  size_t x;
  size_t y;

  result= malloc(workload_size);
  if (result == NULL)
  {
    fprintf(stderr, "malloc:%d\n", errno);
    exit(1);
  }

  for (y= 0, x= workload_size; x; x--, y++)
    result[y]= workload[x - 1];

  printf("Job=%s Workload=%.*s Result=%.*s\n", gearman_job_handle(job),
         (int)workload_size, workload, (int)workload_size, result);

  ret_ptr= GEARMAN_SUCCESS;
  *result_size= workload_size;
  return result;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>]\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
