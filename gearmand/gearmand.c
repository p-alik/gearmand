/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgearman/gearman.h>

int main(int argc, char *argv[])
{
  char c;
  in_port_t port= GEARMAN_DEFAULT_TCP_PORT;
  int backlog= 32;
  uint8_t verbose= 0;
  gearmand_st *gearmand;
  gearman_return_t ret;

  while ((c = getopt(argc, argv, "b:hp:v")) != EOF)
  {
    switch(c)
    {
    case 'b':
      backlog= atoi(optarg);
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 'v':
      verbose++;
      break;

    case 'h':
    default:
      printf("\nusage: %s [-h] [-p <port>]\n", argv[0]);
      printf("\t-b <backlog> - number of backlog connections for listen\n");
      printf("\t-h           - print this help menu\n");
      printf("\t-p <port>    - port for server to listen on\n");
      printf("\t-v           - increase verbosity level by one\n");
      return 1;
    }
  }

  gearmand= gearmand_create(port);
  if (gearmand == NULL)
  {
    printf("gearmand_create:NULL\n");
    return 1;
  }

  gearmand_set_backlog(gearmand, backlog);
  gearmand_set_verbose(gearmand, verbose);

  ret= gearmand_run(gearmand);
  if (ret != GEARMAN_SHUTDOWN && ret != GEARMAN_SUCCESS)
    printf("gearmand_run:%s\n", gearmand_error(gearmand));

  gearmand_free(gearmand);

  return ret == GEARMAN_SUCCESS ? 0 : 1;
}
