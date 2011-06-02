================
Worker structure
================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_worker_st

.. c:function:: int gearman_worker_timeout(gearman_worker_st *worker);

.. c:function:: void gearman_worker_set_timeout(gearman_worker_st *worker, int timeout);

.. c:function:: void *gearman_worker_context(const gearman_worker_st *worker);

.. c:function:: void gearman_worker_set_context(gearman_worker_st *worker, void *context);

.. c:function:: void gearman_worker_set_log_fn(gearman_worker_st *worker, gearman_log_fn *function, void *context, gearman_verbose_t verbose);

.. c:function:: void gearman_worker_set_workload_malloc_fn(gearman_worker_st *worker, gearman_malloc_fn *function, void *context);

.. c:function:: void gearman_worker_set_workload_free_fn(gearman_worker_st *worker, gearman_free_fn *function, void *context);

.. c:function:: gearman_return_t gearman_worker_wait(gearman_worker_st *worker);

.. c:function:: gearman_return_t gearman_worker_register(gearman_worker_st *worker, const char *function_name, uint32_t timeout);

.. c:function:: gearman_return_t gearman_worker_unregister(gearman_worker_st *worker, const char *function_name);

.. c:function:: gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker);

.. c:function:: gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker, gearman_job_st *job, gearman_return_t *ret_ptr);

.. c:function:: void gearman_job_free_all(gearman_worker_st *worker);

.. c:function:: bool gearman_worker_function_exist(gearman_worker_st *worker, const char *function_name, size_t function_length);

.. c:function:: gearman_return_t gearman_worker_add_function(gearman_worker_st *worker, const char *function_name, uint32_t timeout, gearman_worker_fn *function, void *context);

.. c:function:: gearman_return_t gearman_worker_work(gearman_worker_st *worker);

-----------
DESCRIPTION
-----------

This a complete list of all functions that work with a gearman_worker_st,
see their individual pages to learn more about them.

------
RETURN
------

Various

----
HOME
----

To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_

--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
