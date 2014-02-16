====================================
Client callbacks (gearman_actions_t)
====================================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_actions_t

.. c:type:: gearman_workload_fn

.. c:type:: gearman_created_fn

.. c:type:: gearman_data_fn

.. c:type:: gearman_warning_fn

.. c:type:: gearman_universal_status_fn

.. c:type:: gearman_exception_fn

.. c:type:: gearman_fail_fn

.. c:type:: gearman_complete_fn

.. c:function:: void gearman_client_set_workload_fn(gearman_client_st *client, gearman_workload_fn *function)

.. c:function:: void gearman_client_set_created_fn(gearman_client_st *client, gearman_created_fn *function)

.. c:function:: void gearman_client_set_data_fn(gearman_client_st *client, gearman_data_fn *function)

.. c:function:: void gearman_client_set_warning_fn(gearman_client_st *client, gearman_warning_fn *function)

.. c:function:: void gearman_client_set_status_fn(gearman_client_st *client, gearman_universal_status_fn *function)

.. c:function:: void gearman_client_set_complete_fn(gearman_client_st *client, gearman_complete_fn *function)

.. c:function:: void gearman_client_set_exception_fn(gearman_client_st *client, gearman_exception_fn *function)

.. c:function:: void gearman_client_set_fail_fn(gearman_client_st *client, gearman_fail_fn *function)

.. c:function:: void gearman_client_clear_fn(gearman_client_st *client)

.. c:function:: const char *gearman_client_do_job_handle(gearman_client_st *client)

Link to -lgearman

-----------
DESCRIPTION
-----------

Callbacks for client execution task states.

:c:func:`gearman_client_set_data_fn` sets the callback function that will
be called if server is to make a request to the client to provide more data.

:c:func:`gearman_client_do_job_handle` gest the job handle for the running task. This should be used between repeated
:c:func:`gearman_client_do` (and related) calls to get information.

:c:func:`gearman_client_clear_fn` can be called to remove all existing
:c:type:`gearman_actions_t` that have been set.

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

:c:func:`gearman_client_set_exception_fn` will only be called if exceptions are enabled on the server. You can do this by calling :c:func:`gearman_client_set_server_option`.

An example of this::

   const char *EXCEPTIONS="exceptions";
   gearman_client_set_server_option(client, EXCEPTIONS, strlen(EXCEPTIONS));

------------
RETURN VALUE
------------

None

.. seealso::

   :manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_error(3)` or :manpage:`gearman_worker_error(3)`
