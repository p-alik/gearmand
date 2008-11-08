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

void reverse(gearman_st *gearman, gearman_job_st *job);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return ret;
  gearman_st gearman;
  gearman_con_st con;
  gearman_worker_st worker;
  gearman_job_st job;

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
      printf("\nusage: %s [-p <port>] [-h]\n", argv[0]);
      printf("\t-h <host> - job server host\n");
      printf("\t-p <port> - job server port\n");
      return EINVAL;
    }
  }

  (void)gearman_create(&gearman);
  (void)gearman_con_add(&gearman, &con, host, port);
  (void)gearman_worker_create(&gearman, &worker);

  ret= gearman_worker_register_function(&worker, "reverse");
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_error(&gearman));
    exit(1);
  }

  while (1)
  {
    gearman_worker_grab_job(&worker, &job, &ret);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_error(&gearman));
      exit(1);
    }

    reverse(&gearman, &job);
  }

  return 0;
}

void reverse(gearman_st *gearman, gearman_job_st *job)
{
  gearman_return ret;
  char *arg;
  char *buffer;
  size_t arg_len;
  size_t x;
  size_t y;

  arg= gearman_job_arg(job);
  arg_len= gearman_job_arg_len(job);

  buffer= malloc(arg_len);
  if (buffer == NULL)
  {
    fprintf(stderr, "malloc:%d\n", errno);
    exit(1);
  }

  for (y= 0, x= arg_len; x; x--, y++)
    buffer[y]= arg[x - 1];

  ret= gearman_job_send_result(job, buffer, arg_len);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_error(gearman));
    exit(1);
  }

  free(buffer);
}
