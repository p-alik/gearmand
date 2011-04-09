=====================
gearman_parse_servers
=====================

.. index:: object: gearman_parse_servers

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *function,
                                       void *context)

-----------
DESCRIPTION
-----------

gearman_parse_servers(3) parses a list of servers and call the
gearman_parse_server_fn for each server.

------
RETURN
------

A value of type \ ``gearman_return_t``\  is returned.
On success that value will be \ ``GEARMAN_SUCCESS``\ .
Use gearman_strerror() to translate this value to a printable string.

----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(1)` :manpage:`libgearman(3)`
