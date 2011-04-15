======================
Using gearman_status_t
======================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_status_t

.. c:function:: bool gearman_status_is_successful(const gearman_status_t);

.. c:function:: gearman_task_st *gearman_status_task(const gearman_status_t);

-----------
DESCRIPTION
-----------

gearman_status_t requires no allocation or deallocation.

gearman_status_is_successful() determines whether or not a call to :manpage:`gearman_client_execute(3)` was successful or not.

gearman_status_task() will provide a gearman_task_st if one is available.

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

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
