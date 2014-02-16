.. _glossary:

Glossary 
========


.. glossary::
   
   client
      A client submits work in the form of a task, :c:type:`gearman_task_st` to a Gearman server. Client are represented by :c:type:`gearman_client_st`.

   worker
      Responds to tasks that originate from a client. A Gearman server transports the requests.

   task
      Client requests, they are represented by :c:type:`gearman_task_st`. Each task is assigned a job handle, :c:type:`gearman_job_handle_t`, by the server.

   job
      A job represents a task once it is sent to worker. It is represented as :c:type:`gearman_job_st`.

   function 
      A "subprogram" that takes a set of parameters and returns a result (or just just a status). Functions are defined for workers.
  
   reducer 
      A function that takes a piece of data from mapper and returns a value that will be sent to an aggregator function. Any function can be a reducer.

   mapper 
      Mapper functions take incoming data and "map" it out to hosts. The "map" typically is a function that splits up the incoming work. The function that receives the mapped work is the reducer. Work is collected from the reducer and given to an aggregator function. 

   aggregator 
      A function which takes data and compiles it into a single return value. Aggregator functions are defined by :c:type:`gearman_aggregator_fn`.

   context 
      Context are user supplied variables/structures that can be attached to :c:type:`gearman_worker_st`, :c:type:`gearman_client_st`, :c:type:`gearman_task_st`, :c:type:`gearman_job_st`, and similar objects that are passed as opaque objects that the library will ignore (i.e. they are there for developers to use to store state).
