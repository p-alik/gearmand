=============== 
gearman_execute
=============== 

.. index:: object: gearman_execute

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_task_st *gearman_execute(gearman_client_st *client, const char *function_str, size_t function_length, const char *unique_str, size_t unique_length, gearman_work_t *workload, gearman_argument_t *arguments)


-----------
DESCRIPTION
-----------

gearman_execute() can be used to execute tasks against a gearman network.


------
RETURN
------


gearman_execute() returns a c:type:`gearman_task_st`.  


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
