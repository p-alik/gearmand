============= 
Running tasks 
============= 

-------- 
SYNOPSIS 
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_return_t gearman_client_run_tasks(gearman_client_st *client);

Link with -lgearman

-----------
DESCRIPTION
-----------

:c:func:`gearman_client_run_tasks()` executes all tasks that have been added via :c:func:`gearman_client_add_task()`, :c:func:`gearman_client_add_task_status()` or :c:func:`gearman_client_add_task_background()`.

------
RETURN
------

:c:type:`gearman_return_t`

----
HOME
----

To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
