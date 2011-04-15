=================
gearman_version()
=================

.. index:: object: gearman_version

-------
LIBRARY
-------

 C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS 
--------

#include <libgearman/gearman.h>

.. c:function:: const char *gearman_version(gearman_verbose_t verbose)

-----------
DESCRIPTION
-----------


Return the version of the library.


------
RETURN
------

A constant C style string. No deallocation is required.

----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
