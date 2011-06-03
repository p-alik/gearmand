============================================
Creating a worker to handle a map/reduce job 
============================================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_return_t gearman_worker_add_map_function(gearman_worker_st *worker, const char *function_name, size_t functiona_name_length, uint32_t timeout, gearman_mapper_fn *mapper_function, gearman_aggregator_fn *aggregator_function, void *context)

.. c::type:: gearman_worker_error_t (gearman_mapper_fn)(gearman_job_st *job, void *context)

.. c::type:: gearman_return_t (gearman_aggregator_fn)(gearman_aggregator_st *, gearman_task_st *, gearman_result_st *)

Link with -lgearman

-----------
DESCRIPTION
-----------

:c:func:`gearman_worker_add_map_function()` creates a worker that will be
used to execute jobs created by :c:type:`gearman_client_execute_reduce()`.
:c:type:`gearman_mapper_fn` are passed a job and will need to return
a :c:type:`gearman_worker_error_t` upon exit.  For each call of
:c:func:`gearman_job_send_data()` a :c:type:`gearman_job_st` is created. 

Each job will be executed against the aggregate function that
:c:type:`gearman_client_execute_reduce()` specified. If any errors are
detected then the entire job is cancelled.  The gearman_aggregator_fn will
be called when all mapped jobs have completed. The result of this function
will be what is returned to the client.

The callback function needs to populute the ret value with one of the following errors:

:c:type:`GEARMAN_WORKER_SUCCESS`

:c:type:`GEARMAN_WORKER_FAIL`

:c:func:`gearman_job_send_complete()` and :c:func:`gearman_job_send_fail()` cannot be used with mapper functions.

------------
RETURN VALUE
------------

:c:type:`gearman_return_t`

----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_strerror(3)` :manpage:`gearman_client_error` :manpage:`gearman_client_execute_reduce`


