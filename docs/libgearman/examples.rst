=================== 
libgearman examples
=================== 


------------------------
Simple gearman_execute()
------------------------

The following examples shows how to use :c:func:`gearman_execute)` to request data be sent to a function named "reverse" and print out the values.

.. literalinclude:: examples/gearman_execute_example.c  
   :language: c

---------------------------------------
gearman_execute() with reducer function
---------------------------------------

In this example we call the function word_split and tell it to reduce its results with the function count.

.. literalinclude:: examples/gearman_execute_map_reduce.c
  :language: c


--------------------------
Simple gearman_client_do()
--------------------------

.. literalinclude:: examples/gearman_client_do_example.c
  :language: c
