=================================
Logging callback (gearman_log_fn)
=================================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_log_fn


----------- 
DESCRIPTION 
-----------

:c:type:gearman_log_fn is defined as::

  void (gearman_log_fn)(const char *line, gearman_verbose_t verbose, void *context)

------------
RETURN VALUE
------------

None

----
HOME
----

To find out more information please check:
`http://gearman.info/ <http://gearman.info/>`_

--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_set_log_fn(3)` :manpage:`gearman_worker_set_log_fn(3)`


