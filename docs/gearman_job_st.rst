

.. highlightlang:: c

Job Functions
-------------

.. index:: object: gearman_job_st

This a complete list of all functions that work with a gearman_job_st,
see their individual pages to learn more about them.

.. c:function:: void gearman_job_free(gearman_job_st *job);

.. c:function:: gearman_return_t gearman_job_send_data(gearman_job_st *job, const void *data,
                                       size_t data_size);

.. c:function:: gearman_return_t gearman_job_send_warning(gearman_job_st *job,
                                          const void *warning,
                                          size_t warning_size);

.. c:function:: gearman_return_t gearman_job_send_status(gearman_job_st *job,
                                         uint32_t numerator,
                                         uint32_t denominator);

.. c:function:: gearman_return_t gearman_job_send_complete(gearman_job_st *job,
                                           const void *result,
                                           size_t result_size);

.. c:function:: gearman_return_t gearman_job_send_exception(gearman_job_st *job,
                                            const void *exception,
                                            size_t exception_size);

.. c:function:: gearman_return_t gearman_job_send_fail(gearman_job_st *job);

.. c:function:: const char *gearman_job_handle(const gearman_job_st *job);

.. c:function:: const char *gearman_job_function_name(const gearman_job_st *job);

.. c:function:: const char *gearman_job_unique(const gearman_job_st *job);

.. c:function:: const void *gearman_job_workload(const gearman_job_st *job);

.. c:function:: size_t gearman_job_workload_size(const gearman_job_st *job);

.. c:function:: void *gearman_job_take_workload(gearman_job_st *job, size_t *data_size);
