==========================
gearman_status_t functions
==========================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: bool gearman_status_is_successful(const gearman_status_t);

.. c:function:: gearman_task_st *gearman_status_task(const gearman_status_t);

-----------
DESCRIPTION
-----------

gearman_status_t requires no allocation or deallocation.

This a complete list of all functions that work with a gearman_status_t,
see their individual pages to learn more about them.

------
RETURN
------

gearman_status_is_successful() return true on success. gearman_status_task()
will return a gearman_task_st if one is available.

----
HOME
----

To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_

--------
SEE ALSO
--------

:manpage:`gearmand(1)` :manpage:`libgearman(3)`
