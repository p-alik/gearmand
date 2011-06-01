===========================
Working with gearman_job_st
===========================

.. index:: object: gearman_job_st

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_job_st

.. c:function:: void gearman_job_free(gearman_job_st *job);

.. c:function:: gearman_return_t gearman_job_send_data(gearman_job_st *job, const void *data, size_t data_size);

.. c:function:: gearman_return_t gearman_job_send_warning(gearman_job_st *job, const void *warning, size_t warning_size);

.. c:function:: gearman_return_t gearman_job_send_status(gearman_job_st *job, uint32_t numerator, uint32_t denominator);

.. c:function:: gearman_return_t gearman_job_send_complete(gearman_job_st *job, const void *result, size_t result_size);

.. c:function:: gearman_return_t gearman_job_send_exception(gearman_job_st *job, const void *exception, size_t exception_size);

.. c:function:: gearman_return_t gearman_job_send_fail(gearman_job_st *job);

.. c:function:: const char *gearman_job_handle(const gearman_job_st *job);

.. c:function:: const char *gearman_job_function_name(const gearman_job_st *job);

.. c:function:: const char *gearman_job_unique(const gearman_job_st *job);

.. c:function:: const void *gearman_job_workload(const gearman_job_st *job);

.. c:function:: size_t gearman_job_workload_size(const gearman_job_st *job);

.. c:function:: void *gearman_job_take_workload(gearman_job_st *job, size_t *data_size);

-----------
DESCRIPTION
-----------

This a complete list of all functions that work with
a :c:type:`gearman_job_st`, see their individual pages to learn more about
them.

:c:func:`gearman_job_free()` is used to free a job. This only needs to be
done if a task was created with a preallocated structure.

:c:func:`gearman_job_handle()` returns the job handle(see
:c:type:`gearman_job_t` for more information).

:c:func:`gearman_job_function_name()` return the name of the function that
the job was set to execute against.

:c:func:`gearman_job_unique()` return the unique value that was used for
:c:type:`gearman_job_st`. 

returns the :c:type:`gearman_job_st` workload. The size of it can be
determined with :c:func:`gearman_job_workload_size()`.
:c:func:`gearman_job_take_workload()` is the same as
:c:func:`gearman_job_workload()` with the exception that the result must be
:manpage:`free(3)` by the caller.

------
RETURN
------

A value of :c:type:`gearman_return_t`  is returned.  On success that value
will be :c:type::`GEARMAN_SUCCESS`.  Use :c:func:`gearman_strerror()` to
translate this value to a printable string.

----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
