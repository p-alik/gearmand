=================== 
libgearman examples
=================== 


------------------------
Simple gearman_execute()
------------------------

The following examples shows how to use :c:func:`gearman_execute` to request data be sent to a function named "reverse" and print out the values.

.. literalinclude:: examples/gearman_execute_example.c  
   :language: c

---------------------------------------
gearman_execute() with reducer function
---------------------------------------

In this example we call the function count and tell it to map values using
word_split.

.. literalinclude:: examples/gearman_execute_partition.c
  :language: c


--------------------------
Simple gearman_client_do()
--------------------------

.. literalinclude:: examples/gearman_client_do_example.c
  :language: c
