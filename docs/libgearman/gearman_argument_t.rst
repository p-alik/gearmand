==============================
Arguments (gearman_argument_t)
==============================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_argument_t

.. c:function:: gearman_argument_t gearman_argument_make(const char *name, const size_t name_length, const char *value, const size_t value_size)

Compile and link with -lgearman

-----------
DESCRIPTION
-----------

The :c:func:`gearman_argument_make` function initializes :c:type:`gearman_argument_t`. 

:c:type:`gearman_argument_t` is an abstraction used passing arguments too :c:func:`gearman_execute`. The arguments are not copied, so any object that is used to initialize :c:type:`gearman_argument_t` must continue to exist throughout the lifetime/usage of the structure.
   
--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_error()` or :manpage:`gearman_worker_error()`

