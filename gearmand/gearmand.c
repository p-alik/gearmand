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
  gearmand_st *gearmand;

  while ((c = getopt(argc, argv, "b:hp:")) != EOF)
  {
    switch(c)
    {
    case 'b':
      backlog= atoi(optarg);
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 'h':
    default:
      printf("\nusage: %s [-h] [-p <port>]\n", argv[0]);
      printf("\t-b <backlog> - number of backlog connections for listen\n");
      printf("\t-h           - print this help menu\n");
      printf("\t-p <port>    - port for server to listen on\n");
      return 1;
    }
  }

  gearmand= gearmand_init(port, backlog);
  if (gearmand == NULL)
    return 1;

  gearmand_run(gearmand);

  gearmand_destroy(gearmand);

  return 0;
}
