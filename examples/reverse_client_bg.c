/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Example Background Client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static void usage(char *name);

int main(int argc, char *argv[])
{
  int c;
  char *host= NULL;
  in_port_t port= 0;
  gearman_return_t ret;
  gearman_client_st client;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;

  while ((c = getopt(argc, argv, "h:p:")) != -1)
  {
    switch(c)
    {
    case 'h':
      host= optarg;
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
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

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  ret= gearman_client_add_server(&client, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  ret= gearman_client_do_background(&client, "reverse", NULL,
                                    (void *)argv[optind],
                                    (size_t)strlen(argv[optind]), job_handle);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  printf("Background Job Handle=%s\n", job_handle);

  while (1)
  {
    ret= gearman_client_job_status(&client, job_handle, &is_known, &is_running,
                                   &numerator, &denominator);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }

    printf("Known=%s, Running=%s, Percent Complete=%u/%u\n",
            is_known ? "true" : "false", is_running ? "true" : "false",
            numerator, denominator);

    if (!is_known)
      break;

    sleep(1);
  }

  gearman_client_free(&client);

  return 0;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>] <string>\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
