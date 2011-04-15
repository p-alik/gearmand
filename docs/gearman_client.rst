===================================
Creating Gearman Client Connections
===================================

-------
LIBRARY
-------

C Client Library for Gearmand (libgearman, -lgearman)

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_client_st *gearman_client_create(gearman_client_st *client);

.. c:function:: gearman_client_st *gearman_client_clone(gearman_client_st *client, const gearman_client_st *from);

.. c:function:: void gearman_client_free(gearman_client_st *client);

-----------
DESCRIPTION
-----------


gearman_client_create() is used to create a \ ``gearman_client_st``\  structure that will then
be used by other libgearman(3) client functions to communicate with the server. You
should either pass a statically declared \ ``gearman_client_st``\  to gearman_client_create() or
a NULL. If a NULL passed in then a structure is allocated for you.

gearman_client_clone() is similar to gearman_client_create(3) but it copies the
defaults and list of servers from the source \ ``gearman_client_st``\ . If you pass a null as
the argument for the source to clone, it is the same as a call to gearman_client_create().
If the destination argument is NULL a \ ``gearman_client_st``\  will be allocated for you.

To clean up memory associated with a \ ``gearman_client_st``\  structure you should pass
it to gearman_client_free() when you are finished using it. gearman_client_free() is
the only way to make sure all memory is deallocated when you finish using
the structure.

You may wish to avoid using gearman_client_create(3) or gearman_client_clone(3) with a
stack based allocation, ie the first parameter. The most common issues related to ABI safety involve
heap allocated structures.


------
RETURN
------


gearman_client_create() returns a pointer to the gearman_client_st that was created
(or initialized). On an allocation failure, it returns NULL.

gearman_client_clone() returns a pointer to the gearman_client_st that was created
(or initialized). On an allocation failure, it returns NULL.


----
HOME
----


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_strerror(3)`
