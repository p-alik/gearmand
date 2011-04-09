======================
gearman_client_execute
======================

.. index:: object: gearman_client_execute

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>


.. c:function:: gearman_status_t gearman_client_execute(gearman_client_st *client,
                                        const gearman_function_st *function,
                                        gearman_unique_t *unique,
                                        const gearman_workload_t *workload);

-----------
DESCRIPTION
-----------

gearman_client_execute() can be used to execute tasks against a gearman network.


------
RETURN
------


gearman_client_execute() returns a gearman_status_t.  


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(1)` :manpage:`libgearman(3)` :manpage:`gearman_status_t(3)`
