============== 
Executing Work
============== 


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_task_st *gearman_execute(gearman_client_st *client, const char *function_name, size_t function_name_length, const char *unique_str, size_t unique_length, gearman_work_t *workload, gearman_argument_t *arguments)

.. c:type:: gearman_work_t

Link with -lgearman

-----------
DESCRIPTION
-----------

:c:func:`gearman_execute()` is used to create a new :c:type:`gearman_task_st` that is executed against the function that is found via the function_name argument. 

:c:type:`gearman_work_t` can be used to describe the work that will be executed. The argument unique_str is optional, but if supplied it is used for coalescence by :program:`gearmand`.

:c:type:`gearman_argument_t` is the worked that the client will send the to the server

------------
RETURN VALUE
------------


:c:func:`gearman_execute()` returns a c:type:`gearman_task_st`.  


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
