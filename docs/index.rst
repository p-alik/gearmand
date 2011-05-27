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

   bin/gearman
   bin/gearadmin

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

   libgearman/gearman_client
   libgearman/do
   libgearman/gearman_execute 
   libgearman/gearman_execute_map_reduce

************
Worker Calls
************

.. toctree::
   :maxdepth: 1

   libgearman/gearman_worker
   libgearman/gearman_worker_add_function
   libgearman/gearman_worker_add_map_function

****
Misc
****

.. toctree::
   :maxdepth: 1

   libgearman/gearman_misc_functions
   libgearman/namespace

**********
Structures
**********

.. toctree::
   :maxdepth: 1

   libgearman/gearman_client_st 
   libgearman/gearman_job_st
   libgearman/gearman_task_st
   libgearman/gearman_worker_st 
   libgearman/gearman_return_t 
   libgearman/gearman_actions_t

*****
Extra
*****

.. toctree::
   :maxdepth: 1

   libgearman/error_descriptions



------------------
Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

