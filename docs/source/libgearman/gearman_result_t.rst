=============================
Arguments (gearman_result_st)
=============================

--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_result_st

.. c:function:: gearman_result_st *gearman_task_result(gearman_task_st *task)

.. c:function:: int64_t gearman_result_integer(const gearman_result_st *self)

.. c:function:: bool gearman_result_boolean(const gearman_result_st *self)

.. c:function:: gearman_string_t gearman_result_string(const gearman_result_st *self)

.. c:function:: gearman_return_t gearman_result_store_string(gearman_result_st *self, gearman_string_t arg)

.. c:function:: void gearman_result_store_integer(gearman_result_st *self, int64_t value)

.. c:function:: gearman_return_t gearman_result_store_value(gearman_result_st *self, const void *value, size_t size)

.. c:function:: size_t gearman_result_size(const gearman_result_st *self)

.. c:function:: const char *gearman_result_value(const gearman_result_st *self)

.. c:function:: bool gearman_result_is_null(const gearman_result_st *self)

Compile and link with -lgearman

-----------
DESCRIPTION
-----------

The :c:type:`gearman_result_st` type represents a result set. :c:type:`gearman_aggregator_fn` is passed on these types which it uses to create a final result that is returned to the client. 

:c:func:`gearman_task_result` returns :c:type:`gearman_result_st` from a :c:type:`gearman_task_st`.

A :c:type:`gearman_result_st` can return the resulting value as either a char pointer, boolean, :c:type:`gearman_string_t`, or int64_t.
   
--------
SEE ALSO
--------

:manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_error()` or :manpage:`gearman_worker_error()`


