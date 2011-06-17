
=============== 
Setting Options
=============== 


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:function:: gearman_client_options_t gearman_client_options(const gearman_client_st *client)

.. c:function:: void gearman_client_set_options(gearman_client_st *client, gearman_client_options_t options)

.. c:function:: void gearman_client_add_options(gearman_client_st *client, gearman_client_options_t options)

.. c:function:: void gearman_client_remove_options(gearman_client_st *client, gearman_client_options_t options)


-----------
DESCRIPTION
-----------

:c:func:`gearman_client_options()` returns the :c:type:`gearman_client_options_t` for :c:type:`gearman_client_st`. You enable options via :c:func:`gearman_client_add_options()` and disable options via :c:func:`gearman_client_remove_options()`.  

:c:func:`gearman_client_set_options()` has been DEPRECATED.


The currently supported options are:

.. c:type:: GEARMAN_CLIENT_NON_BLOCKING

Enable non-block IO for the client.

.. c:type:: GEARMAN_CLIENT_UNBUFFERED_RESULT

Only grab jobs that have been assigned unique values.

.. c:type:: GEARMAN_CLIENT_FREE_TASKS

Free clients after 


------------
RETURN VALUE
------------

Various

----
HOME
----

To find out more information please check:
`http://gearman.info/ <http://gearman.info/>`_

.. seealso::
  :manpage:`gearmand(8)` :manpage:`libgearman(3)`

