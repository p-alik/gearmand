#include <assert.h>
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

  if (argc < 2)
    return 0;

  /* Create Connection */
  {
    gearman_return rc;

    gear_con= gearman_create(NULL);

    assert(gear_con);

    rc= gearman_server_add(gear_con, "localhost", 0);
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
    gearman_job_set_value(job, argv[1], strlen(argv[1]));

    rc= gearman_job_submit(job);

    assert(rc == GEARMAN_SUCCESS);

    rc= gearman_job_result(job, result);

    if (rc == GEARMAN_SUCCESS)
      printf("Returned: %.*s\n", gearman_result_length(result), gearman_result_value(result));
    else
      printf("Failure: %s\n", gearman_strerror(gear_con, rc));
  }

  return 0;
}
