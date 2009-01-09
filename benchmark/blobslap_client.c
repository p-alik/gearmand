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
#include <stdint.h>
#include <limits.h>
#include <time.h>

#include <libgearman/gearman.h>

static gearman_return_t _created(gearman_task_st *task);
static gearman_return_t _status(gearman_task_st *task);
static gearman_return_t _complete(gearman_task_st *task);
static gearman_return_t _fail(gearman_task_st *task);

static void _usage(char *name);

static uint8_t verbose;

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  char *blob= (char *)malloc(UINT32_MAX);
  char *function_name= NULL;
  unsigned short port= 0;
  gearman_return_t ret;
  gearman_client_st client;
  gearman_task_st **task;
  uint8_t random_data = 0;
  uint8_t blob_done = 0;
  uint32_t num_tasks = 65535;
  uint32_t x;
  uint32_t blob_size;
  uint32_t min = 1024;
  uint32_t max = 102400;

  while ((c = getopt(argc, argv, "f:h:rm:M:n:p:v")) != EOF)
  {
    switch(c)
    {
    case 'f':
      function_name= optarg;
      break;

    case 'h':
      host= optarg;
      break;

    case 'r':
      srand(time(NULL));
      random_data= 1;
      break;

    case 'm':
      min= atoi(optarg);
      break;

    case 'M':
      max= atoi(optarg);
      break;

    case 'n':
      num_tasks= atoi(optarg);
      break;

    case 'p':
      port= atoi(optarg);
      break;

    case 'v':
      verbose= 1;
      break;

    default:
      _usage(argv[0]);
      exit(1);
    }
  }

  if(argc != (optind + 1))
  {
    _usage(argv[0]);
    exit(1);
  }

  task = (gearman_task_st **)malloc(num_tasks * sizeof(gearman_task_st));

  /* This creates the client data and starts our connection to
     the job server. */
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

  /* This is where we need to figure out what function we should be
     benchmarking, and where we perform any tasks specific to that function. */
  if (strcmp("reverse", function_name))
  {
    /* We are slapping with the reverse function here. */
    for (x= 0; x < num_tasks; x++)
    {
      /* Is blob data random or not? */
      if (random_data)
      {
        /* Choose a random size between min and max. */
        blob_size = (min + rand()) % (max + 1);

        /* The blob will be the same char over and over for speed. */
        memset(blob, 'x', blob_size); 
      }
      else if (!(blob_done))
      {
        blob_done = 1;
        free(blob);
        blob = argv[optind];
        blob_size = strlen(argv[optind]);
      }

      if (gearman_client_add_task(&client, task[x], NULL, "reverse", NULL,
                                  (void *)blob, (size_t)blob_size,
                                  &ret) == NULL || ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_client_error(&client));
        exit(1);
      }
    }
  }
  else if (strcmp("gmdb_slap", function_name))
  {
    /* We are slapping with gearman mail database here. */
    for (x= 0; x < num_tasks; x++)
    {
      /* Is blob data random or not? */
      if (random_data)
      {
        /* Choose a random size between min and max. */
        blob_size = (min + rand()) % (max + 1);

        /* The blob will be the same char over and over for speed. */
        memset(blob, 'x', blob_size); 
      }
      else if (!(blob_done))
      {
        blob_done = 1;
        free(blob);
        blob = argv[optind];
        blob_size = strlen(argv[optind]);
      }

      if (gearman_client_add_task(&client, task[x], NULL, "gmdb_slap", NULL,
                                  (void *)blob, (size_t)blob_size,
                                  &ret) == NULL || ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_client_error(&client));
        exit(1);
      }
    }
  }
  else
  {
    fprintf(stderr, "error: could not find function name: %s\n",
            function_name);
    exit(1);
  }

  ret= gearman_client_run_tasks(&client, NULL, _created, NULL, _status,
                                _complete, _fail);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  gearman_client_free(&client);
  free(task);

  return 0;
}

static gearman_return_t _created(gearman_task_st *task)
{
  if (verbose)
    printf("Created: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _status(gearman_task_st *task)
{
  if (verbose)
  {
    printf("Status: %s (%u/%u)\n", gearman_task_job_handle(task),
           gearman_task_numerator(task), gearman_task_denominator(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _complete(gearman_task_st *task)
{
  if (verbose)
  {
    printf("Completed: %s %.*s\n", gearman_task_job_handle(task),
           (int)gearman_task_data_size(task), (char *)gearman_task_data(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _fail(gearman_task_st *task)
{
  if (verbose)
    printf("Failed: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static void _usage(char *name)
{
  printf("\nusage: %s [-rv] [-f <function_name>] [-h <host>] [-m <min_size>]\n"
         "\t[-M <max_size>] [-n <num_runs>] [-p <port>] <string>\n", name);
  printf("\t-f <function_name> - worker function to test\n");
  printf("\t-h <host> - job server host\n");
  printf("\t-m <min_size> - minimum blob size (default 1024)\n");
  printf("\t-M <max_size> - maximum blob size (default 102400)\n");
  printf("\t-n <num_run> - number of times to run (default 65535)\n");
  printf("\t-p <port> - job server port\n");
  printf("\t-r - generate random blob data (will ignore string)\n");
  printf("\t-v - print verbose messages (this will slow things down)\n");
}
