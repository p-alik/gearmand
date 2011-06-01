
================
Client callbacks
================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_actions_t

.. c:function:: void gearman_client_set_workload_fn(gearman_client_st *client, gearman_workload_fn *function)

.. c:function:: void gearman_client_set_created_fn(gearman_client_st *client, gearman_created_fn *function)

.. c:function:: void gearman_client_set_data_fn(gearman_client_st *client, gearman_data_fn *function)

.. c:function:: void gearman_client_set_warning_fn(gearman_client_st *client, gearman_warning_fn *function)

.. c:function:: void gearman_client_set_status_fn(gearman_client_st *client, gearman_universal_status_fn *function)

.. c:function:: void gearman_client_set_complete_fn(gearman_client_st *client, gearman_complete_fn *function)

.. c:function:: void gearman_client_set_exception_fn(gearman_client_st *client, gearman_exception_fn *function)

.. c:function:: void gearman_client_set_fail_fn(gearman_client_st *client, gearman_fail_fn *function)

Link to -lgearman

-----------
DESCRIPTION
-----------

Callbacks for client execution task states.

:c:func:`gearman_client_do_job_handle` sets the callback function that will
be called if server is to make a request to the client to provide more data.

:c:func:`gearman_client_set_created_fn`,
:c:func:`gearman_client_set_data_fn`,
:c:func:`gearman_client_set_warning_fn`,
:c:func:`gearman_client_set_status_fn`,
:c:func:`gearman_client_set_complete_fn`,
:c:func:`gearman_client_set_exception_fn`, and
:c:func:`gearman_client_set_fail_fn`, set callback functions for the
different states of execution for a client request. Each request, ie
a creation of :c:type:`gearman_task_st`, keeps a copy of callbacks when it
is created.  

--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_error()` or :manpage:`gearman_worker_error()`
