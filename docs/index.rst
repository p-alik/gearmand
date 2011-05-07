=====================================
Welcome to the Gearman documentation!
=====================================

Gearman provides a generic application framework to farm out work to other
machines or processes that are better suited to do the work. It allows you
to do work in parallel, to load balance processing, and to call functions
between languages. It can be used in a variety of applications, from
high-availability web sites to the transport of database replication events.
In other words, it is the nervous system for how distributed processing
communi

-------------
Introduction:
-------------
.. toctree::
   :maxdepth: 2
   
   license
   how_to_report_a_bug

------
Server
------
.. toctree::
   :maxdepth: 2

   gearmand

------------------
Command Line Tools
------------------
.. toctree::
   :maxdepth: 2

   gearman
   gearadmin

---------------------
C/C++ Client Library:
---------------------

**************
Function Calls
**************

.. toctree::
   :maxdepth: 2

   libgearman
   do
   gearman_client
   gearman_worker
   gearman_client_execute
   gearman_misc_functions


**********
Structures
**********

.. toctree::
   :maxdepth: 2

   gearman_client_st 
   gearman_function_st
   gearman_job_st
   gearman_status_t
   gearman_task_st
   gearman_unique_t
   gearman_worker_st

------------------
Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

