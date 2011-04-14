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

.. c:function:: gearman_client_st *gearman_client_create(gearman_client_st *client);

.. c:function:: gearman_client_st *gearman_client_clone(gearman_client_st *client, const gearman_client_st *from);

.. c:function:: void gearman_client_free(gearman_client_st *client);

.. c:function:: const char *gearman_client_error(const gearman_client_st *client);

.. c:function:: int gearman_client_errno(const gearman_client_st *client);

.. c:function:: gearman_client_options_t gearman_client_options(const gearman_client_st *client);

.. c:function:: void gearman_client_add_options(gearman_client_st *client, gearman_client_options_t options);

.. c:function:: void gearman_client_remove_options(gearman_client_st *client, gearman_client_options_t options);

.. c:function:: int gearman_client_timeout(gearman_client_st *client);

.. c:function:: void gearman_client_set_timeout(gearman_client_st *client, int timeout);

.. c:function:: void *gearman_client_context(const gearman_client_st *client);

.. c:function:: void gearman_client_set_context(gearman_client_st *client, void *context);

.. c:function:: void gearman_client_set_log_fn(gearman_client_st *client, gearman_log_fn *function, void *context, gearman_verbose_t verbose);

.. c:function:: void gearman_client_set_workload_malloc_fn(gearman_client_st *client, gearman_malloc_fn *function, void *context);

.. c:function:: void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context);

.. c:function:: gearman_return_t gearman_client_add_server(gearman_client_st *client, const char *host, in_port_t port); 

.. c:function:: gearman_return_t gearman_client_add_servers(gearman_client_st *client, const char *servers);

.. c:function:: void gearman_client_remove_servers(gearman_client_st *client); 

.. c:function:: gearman_return_t gearman_client_wait(gearman_client_st *client);

.. c:function:: void *gearman_client_do(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, size_t *result_size, gearman_return_t *ret_ptr);

.. c:function:: void *gearman_client_do_high(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, size_t *result_size, gearman_return_t *ret_ptr);

.. c:function:: void *gearman_client_do_low(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, size_t *result_size, gearman_return_t *ret_ptr);

.. c:function:: const char *gearman_client_do_job_handle(const gearman_client_st *client);

.. c:function:: void gearman_client_do_status(gearman_client_st *client, uint32_t *numerator, uint32_t *denominator);

.. c:function:: gearman_return_t gearman_client_do_background(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, char *job_handle);

.. c:function:: gearman_return_t gearman_client_do_high_background(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, char *job_handle);

.. c:function:: gearman_return_t gearman_client_do_low_background(gearman_client_st *client, const char *function_name, const char *unique, const void *workload, size_t workload_size, char *job_handle);

.. c:function:: gearman_return_t gearman_client_job_status(gearman_client_st *client, const char *job_handle, bool *is_known, bool *is_running, uint32_t *numerator, uint32_t *denominator);

.. c:function:: gearman_return_t gearman_client_echo(gearman_client_st *client, const void *workload, size_t workload_size);

.. c:function:: void gearman_client_task_free_all(gearman_client_st *client);

.. c:function:: void gearman_client_set_task_context_free_fn(gearman_client_st *client, gearman_task_context_free_fn *function);

.. c:function:: gearman_status_t gearman_client_execute(gearman_client_st *client, const gearman_function_st *function, gearman_unique_t *unique, const gearman_workload_t *workload);


.. c:function:: gearman_task_st *gearman_client_add_task(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st *gearman_client_add_task_high(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st *gearman_client_add_task_low(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st *gearman_client_add_task_background(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st * gearman_client_add_task_high_background(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st * gearman_client_add_task_low_background(gearman_client_st *client, gearman_task_st *task, void *context, const char *function_name, const char *unique, const void *workload, size_t workload_size, gearman_return_t *ret_ptr);

.. c:function:: gearman_task_st *gearman_client_add_task_status(gearman_client_st *client, gearman_task_st *task, void *context, const char *job_handle, gearman_return_t *ret_ptr);

.. c:function:: void gearman_client_set_workload_fn(gearman_client_st *client, gearman_workload_fn *function);

.. c:function:: void gearman_client_set_created_fn(gearman_client_st *client, gearman_created_fn *function);

.. c:function:: void gearman_client_set_data_fn(gearman_client_st *client, gearman_data_fn *function);

.. c:function:: void gearman_client_set_warning_fn(gearman_client_st *client, gearman_warning_fn *function);

.. c:function:: void gearman_client_set_status_fn(gearman_client_st *client, gearman_universal_status_fn *function);

.. c:function:: void gearman_client_set_complete_fn(gearman_client_st *client, gearman_complete_fn *function);

.. c:function:: void gearman_client_set_exception_fn(gearman_client_st *client, gearman_exception_fn *function);

.. c:function:: void gearman_client_set_fail_fn(gearman_client_st *client, gearman_fail_fn *function);

.. c:function:: void gearman_client_clear_fn(gearman_client_st *client);

.. c:function:: gearman_return_t gearman_client_run_tasks(gearman_client_st *client);

.. c:function:: bool gearman_client_compare(const gearman_client_st *first, const gearman_client_st *second);

-----------
DESCRIPTION
-----------

gearman_client_st is used to create a client that can communicate with a
Gearman server.

This a complete list of all functions that work with a gearman_client_st.



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
