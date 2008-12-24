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
#define REVERSE_SIZE 1024

static gearman_return_t created(gearman_task_st *task);
static gearman_return_t status(gearman_task_st *task);
static gearman_return_t complete(gearman_task_st *task);
static gearman_return_t fail(gearman_task_st *task);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return_t ret;
  gearman_client_st client;
  gearman_task_st task[REVERSE_TASKS];
  uint32_t x;

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

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  gearman_client_set_options(&client, GEARMAN_CLIENT_BUFFER_RESULT, 1);

  ret= gearman_client_add_server(&client, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  for (x= 0; x < REVERSE_TASKS; x++)
  {
    if (gearman_client_add_task(&client, &(task[x]), NULL, "reverse",
                                (void *)argv[optind],
                                (size_t)strlen(argv[optind]), &ret) == NULL ||
        ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }
  }

  ret= gearman_client_run_tasks(&client, NULL, created, NULL, status, complete,
                                fail);
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
