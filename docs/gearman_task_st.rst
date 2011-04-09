===============
gearman_task_st
===============

.. index:: object: gearman_task_st

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)


--------
SYNOPSIS
--------


#include <libgearman/gearman.h>

.. c:type:: gearman_task_st

.. c:function:: void gearman_task_free(gearman_task_st *task);

.. c:function:: const void *gearman_task_context(const gearman_task_st *task);

.. c:function:: void gearman_task_set_context(gearman_task_st *task, void *context);

.. c:function:: const char *gearman_task_function_name(const gearman_task_st *task);

.. c:function:: const char *gearman_task_unique(const gearman_task_st *task);

.. c:function:: const char *gearman_task_job_handle(const gearman_task_st *task);

.. c:function:: bool gearman_task_is_known(const gearman_task_st *task);

.. c:function:: bool gearman_task_is_running(const gearman_task_st *task);

.. c:function:: uint32_t gearman_task_numerator(const gearman_task_st *task);

.. c:function:: uint32_t gearman_task_denominator(const gearman_task_st *task);

.. c:function:: void gearman_task_give_workload(gearman_task_st *task, const void *workload, size_t workload_size);

.. c:function:: size_t gearman_task_send_workload(gearman_task_st *task, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: const void *gearman_task_data(const gearman_task_st *task);

.. c:function:: size_t gearman_task_data_size(const gearman_task_st *task);

.. c:function:: void *gearman_task_take_data(gearman_task_st *task, size_t *data_size);

.. c:function:: size_t gearman_task_recv_data(gearman_task_st *task, void *data, size_t data_size, gearman_return_t *ret_ptr);

-----------
DESCRIPTION
-----------

This a complete list of all functions that work with a gearman_task_st,
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

:manpage:`gearmand(1)` :manpage:`libgearman(3)`
