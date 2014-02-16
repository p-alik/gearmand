========================== 
Strings (gearman_string_t)
==========================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_string_t

.. c:macro:: size_t gearman_size(gearman_string_t)

.. c:macro:: const char *gearman_c_str(gearman_string_t)

Compile and link with -lgearman

-----------
DESCRIPTION
-----------

The :c:type:`gearman_string_t` is a simple type representing a "string".
Once created :c:type:`gearman_string_t` are not mutable once created. They
only point to the strings that define them and require no deallocation.
   
--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)`



