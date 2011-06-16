====================================
Describing Workload (gearman_work_t)
====================================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_work_t

.. c:function:: gearman_work_t gearman_work(gearman_job_priority_t priority);

.. c:function:: gearman_work_t gearman_work_epoch(time_t epoch, gearman_job_priority_t priority);

.. c:function:: gearman_work_t gearman_work_background(gearman_job_priority_t priority);

.. c:function:: gearman_work_t gearman_work_reducer(const char *name, size_t name_length, gearman_job_priority_t priority);

.. c:function:: gearman_work_t gearman_work_epoch_with_reducer(time_t epoch, gearman_job_priority_t priority, const char *name, size_t name_length);

.. c:function:: gearman_work_t gearman_work_background_with_reducer(gearman_job_priority_t priority, const char *name, size_t name_length);

.. c:function:: void gearman_work_set_context(gearman_work_t *, void *);

Compile and link with -lgearman

-----------
DESCRIPTION
-----------

:c:type:`gearman_work_t` describe work for :c:func:`gearman_execute()`.

:c:func:`gearman_work()` creates a :c:type:`gearman_work_t` with a priority.

:c:func:`gearman_work_epoch()` creates a :c:type:`gearman_work_t` which tells :c:func:`gearman_execute()` to execute the workload at the time specified by epoch.

:c:func:`gearman_work_background()` creates a :c:type:`gearman_work_t` which tells :c:func:`gearman_execute()` to execute the workload as a background job.

:c:func:`gearman_work_reducer()`, :c:func:`gearman_work_reducer()`, and :c:func:`gearman_work_reducer()`, do the same as there non-reducer counterparts but specify a reducer function that will be executed on the result of the function specified for :c:func:`gearman_execute()`.

--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_execute()`
