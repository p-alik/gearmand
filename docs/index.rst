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

   gearman
   gearadmin

---------------------
C/C++ Client Library:
---------------------

******
Basics
******

.. toctree::
   :maxdepth: 1

   libgearman

************
Client Calls
************

.. toctree::
   :maxdepth: 1

   gearman_client
   do
   gearman_execute 
   gearman_execute_map_reduce

************
Worker Calls
************

.. toctree::
   :maxdepth: 1

   gearman_worker
   gearman_worker_add_function
   gearman_worker_add_map_function

****
Misc
****

.. toctree::
   :maxdepth: 1

   gearman_misc_functions
   namespace

**********
Structures
**********

.. toctree::
   :maxdepth: 1

   gearman_client_st 
   gearman_job_st
   gearman_task_st
   gearman_worker_st 
   gearman_return_t 
   gearman_actions_t

*****
Extra
*****

.. toctree::
   :maxdepth: 1

   error_descriptions



------------------
Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

