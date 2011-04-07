

.. highlightlang:: c

Query Object
------------

.. index:: object: gearman_task_st

This a complete list of all functions that work with a gearman_task_st,
see their individual pages to learn more about them.


.. c:function:: void gearman_task_free(gearman_task_st *task);

.. c:function:: const void *gearman_task_context(const gearman_task_st *task);

.. c:function:: void gearman_task_set_context(gearman_task_st *task, void *context);

.. c:function:: const char *gearman_task_function_name(const gearman_task_st *task);

.. c:function:: const char *gearman_task_unique(const gearman_task_st *task);

.. c:function:: const char *gearman_task_job_handle(const gearman_task_st *task);

.. c:function:: bool gearman_task_is_known(const gearman_task_st *task);

.. c:function:: bool gearman_task_is_running(const gearman_task_st *task);

.. c:function:: uint32_t gearman_task_numerator(const gearman_task_st *task);

.. c:function:: uint32_t gearman_task_denominator(const gearman_task_st *task);

.. c:function:: void gearman_task_give_workload(gearman_task_st *task, const void *workload,
                                size_t workload_size);

.. c:function:: size_t gearman_task_send_workload(gearman_task_st *task, const void *workload,
                                  size_t workload_size,
                                  gearman_return_t *ret_ptr);

.. c:function:: const void *gearman_task_data(const gearman_task_st *task);

.. c:function:: size_t gearman_task_data_size(const gearman_task_st *task);

.. c:function:: void *gearman_task_take_data(gearman_task_st *task, size_t *data_size);

.. c:function:: size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                              size_t data_size, gearman_return_t *ret_ptr);
