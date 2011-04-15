======================
gearman_verbose_name()
======================

.. index:: object: gearman_verbose_name


-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS 
--------

#include <libgearman/gearman.h>

.. c:function:: const char *gearman_verbose_name(gearman_verbose_t verbose)

-----------
DESCRIPTION 
-----------

Take a gearman_verbose_t and return a character representation of it.

------
RETURN
------

A character string representing the verbose leval.

----
HOME
----

To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`
