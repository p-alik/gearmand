==========================
Executing a map/reduce job
==========================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_task_st *gearman_execute_map_reduce(gearman_client_st *client, const char *mapper_name, const size_t mapper_length, const char *reducer_name, const size_t reducer_length, const char *unique_str, const size_t unique_length, gearman_work_t *workload, gearman_argument_t *arguments);

.. c::type typedef gearman_worker_error_t (gearman_mapper_fn)(gearman_job_st *job, void *context);

Link with -lgearman

-----------
DESCRIPTION
-----------

gearman_client_execute_reduce() takes a given :c:type::`gearman_argument_t` and executs it against a :c:type::`gearman_mapper_fn` function. This function is specified via the 
mapper_name argument. The mapper function will then break the work up into units, and send each of them to the function named reducer function. Once all work is completed, the mapper function will aggregate the work and return a result.

If any of the units of work error, the job will be aborted. The resulting value will be stored in the :c:type::`gearman_task_st`.


------------
RETURN VALUE
------------

gearman_client_execute_reduce() returns a pointer to a gearman_task_st. On error a NULL will be returned. The error can be examined with c:function::`gearman_client_error()`.

----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_strerror(3)` :manpage:`gearman_client_error`

