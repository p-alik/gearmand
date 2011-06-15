=====================================
Welcome to the Gearman documentation!
=====================================

Gearman provides a generic application framework to farm out work to other
machines or processes that are better suited to do the work. It allows you
to do work in parallel, to load balance processing, and to call functions
between languages. It can be used in a variety of applications, from
high-availability web sites, to the transport of database replication events.

-------------
Introduction:
-------------
.. toctree::
   :maxdepth: 1
   
   license
   how_to_report_a_bug
   glossary

------
Server
------
.. toctree::
   :maxdepth: 1

   gearmand

------------------
Command Line Tools
------------------

.. toctree::
   :maxdepth: 1

   bin/gearman
   bin/gearadmin

---------------------
C/C++ Client Library:
---------------------

******
Basics
******

.. toctree::
   :titlesonly:

   libgearman
   changes

****************
Client Functions
****************

.. toctree::
   :titlesonly:

   libgearman/gearman_client
   libgearman/gearman_client_add_server
   libgearman/gearman_client_options
   libgearman/gearman_client_do
   libgearman/gearman_client_do_background
   libgearman/gearman_execute
   libgearman/gearman_execute_map_reduce
   libgearman/gearman_client_error
   libgearman/gearman_client_set_log_fn
   libgearman/gearman_client_job_status

**************
Creating Tasks
**************

.. toctree::
   :titlesonly:

   libgearman/gearman_client_add_task
   libgearman/gearman_client_run_tasks
   libgearman/gearman_client_add_task_background
   libgearman/gearman_client_add_task_status

****************
Worker Functions
****************

.. toctree::
   :titlesonly:

   libgearman/gearman_worker_create
   libgearman/gearman_worker_add_server
   libgearman/gearman_worker_options
   libgearman/gearman_worker_add_function
   libgearman/gearman_worker_define_function
   libgearman/gearman_worker_error
   libgearman/gearman_worker_set_log_fn

****
Misc
****

.. toctree::
   :titlesonly:

   libgearman/gearman_misc_functions
   libgearman/namespace
   libgearman/gearman_client_wait
   libgearman/gearman_client_echo

**********
Structures
**********

.. toctree::
   :titlesonly:

   libgearman/gearman_client_st 
   libgearman/gearman_job_st
   libgearman/gearman_task_st
   libgearman/gearman_worker_st 
   libgearman/gearman_job_handle_t
   libgearman/gearman_argument_t 
   libgearman/gearman_string_t 
   libgearman/gearman_result_t 
   libgearman/gearman_work_t


*****
Types
*****

.. toctree::
   :titlesonly:

   libgearman/gearman_return_t 
   libgearman/gearman_actions_t
   libgearman/gearman_log_fn
   libgearman/gearman_verbose_t

*****
Extra
*****

.. toctree::
   :titlesonly:

   libgearman/error_descriptions


------------------
Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

