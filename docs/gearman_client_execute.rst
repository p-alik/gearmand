

.. highlightlang:: c

Executing a Client request
--------------------------

.. index:: object: gearman_client_execute

gearman_client_execute() can be used to execute tasks against a gearman network.


.. c:function:: gearman_status_t gearman_client_execute(gearman_client_st *client,
                                        const gearman_function_st *function,
                                        gearman_unique_t *unique,
                                        const gearman_workload_t *workload);
