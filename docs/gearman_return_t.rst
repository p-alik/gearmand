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

-----------
DESCRIPTION
-----------

gearman_return_t is used as a return/error type for all calls using :c:type:`gearman_client_st` and :c:type:`gearman_worker_st`. 
GEARMAN_SUCCESS is returned upon success, otherwise an error is returned. gearman_failed() can be used to see if the return value is a failing value.
You can print a text version of the error message with gearman_strerror().

GEARMAN_SUCCESS

GEARMAN_IO_WAIT

GEARMAN_SHUTDOWN

GEARMAN_SHUTDOWN_GRACEFUL

GEARMAN_ERRNO

GEARMAN_EVENT

GEARMAN_TOO_MANY_ARGS

GEARMAN_NO_ACTIVE_FDS

GEARMAN_INVALID_MAGIC

GEARMAN_INVALID_COMMAND

GEARMAN_INVALID_PACKET

GEARMAN_UNEXPECTED_PACKET

GEARMAN_GETADDRINFO

GEARMAN_NO_SERVERS

GEARMAN_LOST_CONNECTION

GEARMAN_MEMORY_ALLOCATION_FAILURE

GEARMAN_JOB_EXISTS

GEARMAN_JOB_QUEUE_FULL

GEARMAN_SERVER_ERROR

GEARMAN_WORK_ERROR

GEARMAN_WORK_DATA

GEARMAN_WORK_WARNING

GEARMAN_WORK_STATUS

GEARMAN_WORK_EXCEPTION

GEARMAN_WORK_FAIL

GEARMAN_NOT_CONNECTED

GEARMAN_COULD_NOT_CONNECT

GEARMAN_SEND_IN_PROGRESS

GEARMAN_RECV_IN_PROGRESS

GEARMAN_NOT_FLUSHING

GEARMAN_DATA_TOO_LARGE

GEARMAN_INVALID_FUNCTION_NAME

GEARMAN_INVALID_WORKER_FUNCTION

GEARMAN_NO_REGISTERED_FUNCTION

GEARMAN_NO_REGISTERED_FUNCTIONS

GEARMAN_NO_JOBS

GEARMAN_ECHO_DATA_CORRUPTION

GEARMAN_NEED_WORKLOAD_FN

GEARMAN_PAUSE

GEARMAN_UNKNOWN_STATE

GEARMAN_PTHREAD

GEARMAN_PIPE_EOF

GEARMAN_QUEUE_ERROR

GEARMAN_FLUSH_DATA

GEARMAN_SEND_BUFFER_TOO_SMALL

GEARMAN_IGNORE_PACKET

GEARMAN_UNKNOWN_OPTION

GEARMAN_TIMEOUT

GEARMAN_ARGUMENT_TOO_LARGE

GEARMAN_INVALID_ARGUMENT
   
--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
