========
Postgres
========


The current schema, as of .21 is::

   CREATE TABLE <table> ( unique_key VARCHAR(GEARMAN_UNIQUE_SIZE),
                          function_name VARCHAR(255), 
                          priority INTEGER, 
                          data BYTEA, 
                          when_to_run INTEGER, 
                          UNIQUE (unique_key, function_name));

