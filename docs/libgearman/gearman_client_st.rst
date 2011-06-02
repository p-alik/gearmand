========================
Gearman Client Functions
========================

.. index:: object: gearman_client_st

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_client_st

.. c:function:: int gearman_client_timeout(gearman_client_st *client);

.. c:function:: void gearman_client_set_timeout(gearman_client_st *client, int timeout);

.. c:function:: void *gearman_client_context(const gearman_client_st *client);

.. c:function:: void gearman_client_set_context(gearman_client_st *client, void *context);

.. c:function:: void gearman_client_set_log_fn(gearman_client_st *client, gearman_log_fn *function, void *context, gearman_verbose_t verbose);

.. c:function:: void gearman_client_set_workload_malloc_fn(gearman_client_st *client, gearman_malloc_fn *function, void *context);

.. c:function:: void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context);

.. c:function:: gearman_return_t gearman_client_job_status(gearman_client_st *client, const char *job_handle, bool *is_known, bool *is_running, uint32_t *numerator, uint32_t *denominator);

.. c:function:: void gearman_client_task_free_all(gearman_client_st *client);

.. c:function:: void gearman_client_set_task_context_free_fn(gearman_client_st *client, gearman_task_context_free_fn *function);

.. c:function:: void gearman_client_clear_fn(gearman_client_st *client);

-----------
DESCRIPTION
-----------

gearman_client_st is used to create a client that can communicate with a
Gearman server.

This a complete list of all functions that work with a gearman_client_st.



------------
RETURN VALUE
------------


Various


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_

.. seealso::
  :manpage:`gearmand(8)` :manpage:`libgearman(3)`
