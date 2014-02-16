=======
Drizzle
=======


The current schema, as of .21 is::

   CREATE TABLE <table> ( unique_key VARCHAR(GEARMAN_UNIQUE_SIZE),
                          function_name VARCHAR(255), 
                          priority INT, 
                          data LONGBLOB, 
                          when_to_run BIGINT, 
                          unique key (unique_key, function_name));
