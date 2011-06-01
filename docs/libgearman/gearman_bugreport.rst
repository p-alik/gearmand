===================
gearman_bugreport()
===================

.. index:: object: gearman_bugreport

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: const char *gearman_bugreport(void)

Link with -lgearman

-----------
DESCRIPTION
-----------

:c:func:`gearman_bugreport()` returns the url to the bug tracking site for Gearmand

------
RETURN
------

URL

----
HOME
----

To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
