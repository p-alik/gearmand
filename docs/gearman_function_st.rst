

.. highlightlang:: c

Using Functions
---------------

.. index:: object: gearman_function_st

This a complete list of all functions that work with a gearman_function_st,
see their individual pages to learn more about them. gearman_function_st must be decallocated when you finish with them.


.. c:function:: gearman_function_st *gearman_function_create(const char *, size_t size);

.. c:function:: void gearman_function_free(gearman_function_st *);
