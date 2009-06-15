/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Example Client Using Callbacks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#define REVERSE_TASKS 10

static gearman_return_t created(gearman_task_st *task);
static gearman_return_t data(gearman_task_st *task);
static gearman_return_t status(gearman_task_st *task);
static gearman_return_t complete(gearman_task_st *task);
static gearman_return_t fail(gearman_task_st *task);

static void usage(char *name);

int main(int argc, char *argv[])
{
  int c;
  char *host= NULL;
  in_port_t port= 0;
  gearman_return_t ret;
  gearman_client_st client;
  gearman_task_st task[REVERSE_TASKS];
  uint32_t x;

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

  for (x= 0; x < REVERSE_TASKS; x++)
  {
    if (gearman_client_add_task(&client, &(task[x]), NULL, "reverse", NULL,
                                (void *)argv[optind],
                                (size_t)strlen(argv[optind]), &ret) == NULL ||
        ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }
  }

  gearman_client_set_created_fn(&client, created);
  gearman_client_set_data_fn(&client, data);
  gearman_client_set_status_fn(&client, status);
  gearman_client_set_complete_fn(&client, complete);
  gearman_client_set_fail_fn(&client, fail);
  ret= gearman_client_run_tasks(&client);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  gearman_client_free(&client);

  return 0;
}

static gearman_return_t created(gearman_task_st *task)
{
  printf("Created: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t data(gearman_task_st *task)
{
  printf("Data: %s %.*s\n", gearman_task_job_handle(task),
         (int)gearman_task_data_size(task), (char *)gearman_task_data(task));
  return GEARMAN_SUCCESS;
}

static gearman_return_t status(gearman_task_st *task)
{
  printf("Status: %s (%u/%u)\n", gearman_task_job_handle(task),
         gearman_task_numerator(task), gearman_task_denominator(task));
  return GEARMAN_SUCCESS;
}

static gearman_return_t complete(gearman_task_st *task)
{
  printf("Completed: %s %.*s\n", gearman_task_job_handle(task),
         (int)gearman_task_data_size(task), (char *)gearman_task_data(task));
  return GEARMAN_SUCCESS;
}

static gearman_return_t fail(gearman_task_st *task)
{
  printf("Failed: %s\n", gearman_task_job_handle(task));
  return GEARMAN_SUCCESS;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>] <string>\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
