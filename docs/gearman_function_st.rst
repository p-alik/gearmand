================================
Working with gearman_function_st
================================

.. index:: object: gearman_function_st

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_function_st

.. c:function:: gearman_function_st *gearman_function_create(const char *, size_t size);

.. c:function:: void gearman_function_free(gearman_function_st *);

-----------
DESCRIPTION
-----------

You gearman_function_st to describe functions to be executed against a server.

This a complete list of all functions that work with a gearman_function_st,
see their individual pages to learn more about them. gearman_function_st must be decallocated when you finish with them.

------
RETURN
------


Various


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(1)` :manpage:`libgearman(3)`
