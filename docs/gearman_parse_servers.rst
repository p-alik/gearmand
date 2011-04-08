
.. index:: object: gearman_parse_servers

.. c:function:: gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *function,
                                       void *context)

RETURN
______

A value of type \ ``gearman_return_t``\  is returned.
On success that value will be \ ``GEARMAN_SUCCESS``\ .
Use gearman_strerror() to translate this value to a printable string.

HOME
____


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


SEE ALSO
________


gearmand(1) libgearman(3)

