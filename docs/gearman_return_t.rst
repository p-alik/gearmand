=====================
Client Error messages
=====================

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_return_t

.. c:function:: const char *gearman_strerror(gearman_return_t rc)

.. c:function:: bool gearman_success(gearman_return_t rc)

.. c:function:: bool gearman_failed(gearman_return_t rc)

.. c:function:: bool gearman_continue(gearman_return_t rc)

-----------
DESCRIPTION
-----------

:c:type:`gearman_return_t` is used as a return/error type for all calls using :c:type:`gearman_client_st` and :c:type:`gearman_worker_st`. 
:c:type:`GEARMAN_SUCCESS` is returned upon success, otherwise an error is returned. :c:func:`gearman_failed()` can be used to see if the return value is a failing value.
You can print a text version of the error message with :c:func:`gearman_strerror()`.

:c:func:`gearman_success()` return true if :c:type:`GEARMAN_SUCCESS` tested true.

:c:func:`gearman_failed()` return true if any value other then :c:type:`GEARMAN_SUCCESS` was provided.

:c:func:`gearman_continue()` returns true if any error related to non-blocking IO
occurred. This should be used for testing loops.

Possible values of :c:type:`gearman_return_t`:

.. c:type:: GEARMAN_SUCCESS

Success

.. c:type:: GEARMAN_IO_WAIT 

Blocking IO was found. gearman_continue() can be used to
test for this.

.. c:type:: GEARMAN_ERRNO 

System error occurred. Use either :c:func:`gearman_client_errno()` or :c:func:`gearman_worker_errno()` 

.. c:type:: GEARMAN_NO_ACTIVE_FDS 

No active connections were available.  gearman_continue() can be used to test for this.

.. c:type:: GEARMAN_GETADDRINFO 

Name resolution failed for a host.

.. c:type:: GEARMAN_NO_SERVERS 

No servers have been provided for the client/worker.

.. c:type:: GEARMAN_LOST_CONNECTION 

Connection was lost to the given server.

.. c:type:: GEARMAN_MEMORY_ALLOCATION_FAILURE 

Memory allocation failed.

.. c:type:: GEARMAN_SERVER_ERROR 

An error occurred on the server.

.. c:type:: GEARMAN_NOT_CONNECTED 

Client/Worker is not currently connected to the
server.

.. c:type:: GEARMAN_COULD_NOT_CONNECT 

Server name was valid, but a connection could not
be made.

.. c:type:: GEARMAN_ECHO_DATA_CORRUPTION 

Either :c:func:`gearman_client_echo()` or
:c:func:`gearman_work_echo()` echo was unsuccessful.

.. c:type:: GEARMAN_UNKNOWN_STATE 

The gearman_return_t was never set.

.. c:type:: GEARMAN_FLUSH_DATA 

Internal state, should never be seen by either client or worker.

.. c:type:: GEARMAN_SEND_BUFFER_TOO_SMALL 

Send buffer was too small.

.. c:type:: GEARMAN_TIMEOUT 

A timeout occurred when making a request to the server.

.. c:type:: GEARMAN_ARGUMENT_TOO_LARGE 

Argument was too large for the current buffer.

.. c:type:: GEARMAN_INVALID_ARGUMENT 

One of the arguments to the given API call was invalid. EINVAL will be set if :c:func:`gearman_client_error()` or :c:func:`gearman_worker_error()` were not settable.


***********
CLIENT ONLY
***********

.. c:type:: GEARMAN_NEED_WORKLOAD_FN 

A client was asked for work, but no :c:type:`gearman_workload_fn` callback was
specified. See :c:func:`gearman_client_set_workload_fn()`

.. c:type:: GEARMAN_WORK_FAIL  

A task has failed, and the worker has exited with an error or it called :c:func:`ggearman_job_send_fail()`

***********
WORKER ONLY
***********

.. c:type:: GEARMAN_INVALID_FUNCTION_NAME 

A worker was sent a request for a job that it did not have a valid function for.

.. c:type:: GEARMAN_INVALID_WORKER_FUNCTION 

No callback was provided by the worker for a given function.

.. c:type:: GEARMAN_NO_REGISTERED_FUNCTION 

A request for removing a given function from a worker was invalid since that function did not exist.

.. c:type:: GEARMAN_NO_REGISTERED_FUNCTIONS 

The worker has not registered any functions.

.. c:type:: GEARMAN_NO_JOBS 

No jobs were found for the worker.

****************
WORKER TO CLIENT
****************

Client which have registed a custom :c:type:`gearman_actions_t` may use these
value as return values to the calling client.

.. c:type:: GEARMAN_WORK_DATA 

Worker has sent a chunked piece of data to the client via :c:func:`gearman_job_send_data()`

.. c:type:: GEARMAN_WORK_WARNING 

Worker has issued a warning to the client via :c:func:`gearman_job_send_warning()`

.. c:type:: GEARMAN_WORK_STATUS 

Status has been updated by the worker via :c:func:`ggearman_job_send_status()`

.. c:type:: GEARMAN_WORK_EXCEPTION 

Worker has sent an exception the client via :c:func:`ggearman_job_send_exception()`

.. c:type:: GEARMAN_WORK_FAIL  

A task has failed, and the worker has exited with an error or it called :c:func:`ggearman_job_send_fail()`

.. c:type:: GEARMAN_PAUSE 

Used only in custom application for client return based on :c:type:`GEARMAN_WORK_DATA`, :c:type:`GEARMAN_WORK_WARNING`, :c:type:`GEARMAN_WORK_EXCEPTION`, :c:type:`GEARMAN_WORK_FAIL`, or :c:type:`GEARMAN_WORK_STATUS`. :c:func:`gearman_continue()` can be used to check for this value.

*********
TASK ONLY
*********

.. c:type:: GEARMAN_NOT_FLUSHING

:c:func:`gearman_task_send_workload()` failed, it was not in the correct state. 

.. c:type:: GEARMAN_DATA_TOO_LARGE 

:c:func:`gearman_task_send_workload()` failed, the data was too large to be sent.

********
PROTOCOL
********

If any of these errors occurred the connection will be dropped/reset.

.. c:type:: GEARMAN_INVALID_MAGIC

.. c:type:: GEARMAN_INVALID_COMMAND

.. c:type:: GEARMAN_INVALID_PACKET

.. c:type:: GEARMAN_UNEXPECTED_PACKET

.. c:type:: GEARMAN_TOO_MANY_ARGS

   
--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_error()` or :manpage:`gearman_worker_error()`
