/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <libgearman/gearman.h>

int main(int argc, char *argv[])
{
  gearman_st *gear_con;
  char c;
  unsigned short port= 0;

  while((c = getopt(argc, argv, "hp:")) != EOF)
  {
    switch(c)
    {
    case 'p':
      port= atoi(optarg);
      break;

    case 'h':
    default:
      printf("\nusage: %s [-p <port>] [-h]\n", argv[0]);
      printf("\t-h        - print this help menu\n");
      printf("\t-p <port> - port for server to listen on\n");
      return EINVAL;
    }
  }

  if (argc < 2)
    return 0;

  /* Create Connection */
  {
    gearman_return rc;

    gear_con= gearman_create(NULL);

    assert(gear_con);

    rc= gearman_server_add(gear_con, "localhost", 2323);
    assert(rc == GEARMAN_SUCCESS);
    rc= gearman_server_add(gear_con, "localhost", 2324);
    assert(rc == GEARMAN_SUCCESS);
    rc= gearman_server_add(gear_con, "localhost", port);
    assert(rc == GEARMAN_SUCCESS);

  }

  /* Send the data */
  {
    gearman_return rc;
    gearman_result_st *result;
    gearman_job_st *job;

    result= gearman_result_create(gear_con, NULL);
    job= gearman_job_create(gear_con, NULL);

    assert(result);
    assert(job);

    gearman_job_set_function(job, "echo");
    gearman_job_set_value(job, argv[optind], strlen(argv[optind]));

    rc= gearman_job_submit(job);

    assert(rc == GEARMAN_SUCCESS);

    rc= gearman_job_result(job, result);

    if (rc == GEARMAN_SUCCESS)
      printf("Returned: %.*s\n", (int32_t)gearman_result_length(result), gearman_result_value(result));
    else
      printf("Failure: %s\n", gearman_strerror(gear_con, rc));
  }

  return 0;
}
