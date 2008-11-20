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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return ret;
  gearman_client_st client;
  char *result;
  size_t result_size;

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

  if(argc != (optind + 1))
  {
    usage(argv[0]);
    exit(1);
  }

  (void)gearman_client_create(&client);

  ret= gearman_client_server_add(&client, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  result= (char *)gearman_client_do(&client, "reverse", (void *)argv[optind],
                                    (size_t)strlen(argv[optind]), &result_size,
                                    &ret);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  printf("Result=%.*s\n", (int)result_size, result);

  free(result);
  gearman_client_free(&client);

  return 0;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>] <string>\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
