================================
Issuing a single background task
================================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_return_t gearman_client_do_background(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, char *job_handle);


-----------
DESCRIPTION
-----------


:c:func:`gearman_client_do_background()` executes a single request to the
gearmand server and returns the status via :c:type:`gearman_return_t`. 

:c:func:`gearman_client_do_background_high()` and
:c:funcrman_client_do_background_low():`()` are identical to
:c:func:`gearman_client_do_background()`, only they set the priority to either
high or low. 


If job_handle is not NULL, it will be populated with the name of the job_handle
for the task created. The job handle needs to be the size of
:c:macro:`GEARMAN_JOB_HANDLE_SIZE`.

------------
RETURN VALUE
------------

:c:type:`gearman_return_t`

-------
Example
-------

.. literalinclude:: examples/gearman_client_do_background_example.c
  :language: c


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


.. seealso::
  :manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_strerror(3)`

