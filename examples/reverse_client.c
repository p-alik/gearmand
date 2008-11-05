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
  gearman_client_st *client;
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

    client= gearman_client_create(NULL);

    assert(client);

    rc= gearman_client_server_add(client, "localhost", 0);

    if (rc != GEARMAN_SUCCESS)
    {
      printf("Failure: %s\n", gearman_strerror(rc));
      exit(0);
    }
  }

  /* Send the data */
  {
    gearman_return rc;
    ssize_t job_length;
    uint8_t *job_result;

    job_result= gearman_client_do(client, "reverse",
                                  argv[optind], strlen(argv[optind]), &job_length, &rc);

    if (rc == GEARMAN_SUCCESS)
    {
      printf("Returned: %.*s\n", job_length, job_result);
      free(job_result);
    }
    else
      printf("Failure: %s\n", gearman_strerror(rc));
  }

  return 0;
}
