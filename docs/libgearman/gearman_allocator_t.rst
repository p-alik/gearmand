================================================
Defining custom allocators (gearman_allocator_t)
================================================


--------
SYNOPSIS
--------

#include <libgearman/gearman.h>

.. c:type:: gearman_allocator_t

.. c:type:: gearman_malloc_fn

.. c:type:: gearman_free_fn

.. c:type:: gearman_realloc_fn

.. c:type:: gearman_calloc_fn

.. c:function:: gearman_return_t gearman_client_set_memory_allocators(gearman_client_st *, gearman_malloc_fn *malloc_fn, gearman_free_fn *free_fn, gearman_realloc_fn *realloc_fn, gearman_calloc_fn *calloc_fn, void *context)

.. c:function:: gearman_return_t gearman_worker_set_memory_allocators(gearman_worker_st *, gearman_malloc_fn *malloc_fn, gearman_free_fn *free_fn, gearman_realloc_fn *realloc_fn, gearman_calloc_fn *calloc_fn, void *context)

Link to -lgearman

-----------
DESCRIPTION
-----------

Install callbacks for custom allocation.

Normally :manpage:`malloc(3)` and :manpage:`free(3)` are used for allocation and releasing workloads. You can use :c:func:`gearman_client_set_memory_allocators` and :c:func:`gearman_worker_set_memory_allocators` to set your own custom allocators.

------------
RETURN VALUE
------------

None

.. seealso::

   :manpage:`gearmand(8)` :manpage:`libgearman(3)` :manpage:`gearman_client_st(3)`
