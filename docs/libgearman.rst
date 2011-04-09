=============
libgearman(3)
=============

.. index:: object: libgearman

-------
LIBRARY
-------

 C Client Library for Gearmand (libgearman, -lgearman)


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

-----------
DESCRIPTION
-----------

\ **libgearman**\  is a small, thread-safe client library for the
gearman protocol. The code has all been written with an eye to allow
for both web and embedded usage. It handles the work behind routing
particular keys to specific servers that you specify (and values are
matched based on server order as supplied by you). It implements both
the client and worker interfaces.

All operations are performed against either a \ ``gearman_client_st``\  structure
or a \ ``gearman_worker_st``\.
These structures can either be dynamically allocated or statically
allocated and then initialized by gearman_create(). Functions have been
written in order to encapsulate all structures in the library. It is
recommended that you operate directly against the structure.

Nearly all functions return a \ ``gearman_return_t``\  value.
This value can be translated to a printable string with gearman_strerror(3).

\ ``gearman_client_st``\  and \ ``gearman_worker_st``\ structures are thread-safe, but each thread must
contain its own structure (that is, if you want to share these among
threads you must provide your own locking). No global variables are
used in this library.

If you are working with GNU autotools you will want to add the following to
your configure.ac to properly include libgearman in your application.

PKG_CHECK_MODULES(DEPS, libgearman >= 0.8.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

Hope you enjoy it!

CONSTANTS
---------


A number of constants have been provided for in the library.


.. c:var:: GEARMAN_DEFAULT_TCP_PORT
 
The default port used by gearmand(3).

.. c:var:: GEARMAN_DEFAULT_TCP_PORT
 
The default service used by gearmand(3).

.. c:var:: LIBGEARMAN_VERSION_STRING
 
String value of the libgearman version such as "0.20.4"


.. c:var:: LIBGEARMAN_VERSION_HEX
 
Hex value of the version number. "0x00048000" This can be used for comparing versions based on number.


---------------------
THREADS AND PROCESSES
---------------------


When using threads or forked processes it is important to keep an instance
of \ ``gearman_client_st``\ or \ ``gearman_worker_st``\  per process or thread. Without creating your own locking
structures you can not share a single \ ``gearman_client_st``\  or \ ``gearman_worker_st``\.


----
HOME
----


To find out more information please check:
`https://launchpad.net/libgearman <https://launchpad.net/gearmand>`_


------
AUTHOR
------


Brian Aker, <brian@tangent.org> of Data Differential, http://datadifferential.com/


--------
SEE ALSO
--------

:manpage:`gearmand(1)` :manpage:`libgearman_examples(3)`

