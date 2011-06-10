==================================
Gearman Client (gearman_client_st)
==================================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_client_st

.. c:function:: int gearman_client_timeout(gearman_client_st *client)

.. c:function:: void gearman_client_set_timeout(gearman_client_st *client, int timeout)

.. c:function:: void *gearman_client_context(const gearman_client_st *client)

.. c:function:: void gearman_client_set_context(gearman_client_st *client, void *context)

.. c:function:: void gearman_client_set_workload_malloc_fn(gearman_client_st *client, gearman_malloc_fn *function, void *context)

.. c:function:: void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context)

.. c:function:: void gearman_client_task_free_all(gearman_client_st *client)

.. c:function:: void gearman_client_set_task_context_free_fn(gearman_client_st *client, gearman_task_context_free_fn *function)

Link with -lgearman

-----------
DESCRIPTION
-----------

:c:type:`gearman_client_st` is used for client communication with the server.

:c:func:`gearman_client_context()` and :c:func:`gearman_client_set_context()` can be used to store an arbitrary object for the user.

:c:func:`gearman_client_set_task_context_free_fn()` sets a trigger that will be called when a :c:type:`gearman_task_st` is released.

:c:func:`gearman_client_timeout()` and :c:func:`gearman_client_set_timeout()` are to get and set the current timeout value for the client.

Normally :manpage:`malloc(3)` and :manpage:`free(3)` are used for allocation and releasing workloads. :c:func:`gearman_client_set_workload_malloc_fn()` and :c:func:`gearman_client_set_workload_free_fn` can be used to replace these with custom functions.

:c:func:`gearman_client_task_free_all()` is used to free all current :c:type:`gearman_task_st` that have been created with the :c:type:`gearman_client_st`. 

.. warning:: 
  By calling :c:func:`gearman_client_task_free_all()` you can end up with a SEGFAULT if you try to use any :c:type:`gearman_task_st` that you have kept pointers too.


------------
RETURN VALUE
------------

:c:func:`gearman_client_timeout()` returns an integer representing the amount of time in milliseconds. A value of -1 means an infinite timeout value. See :manpage:`poll(3)` for more details.


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_

.. seealso::
  :manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_create(3)`
