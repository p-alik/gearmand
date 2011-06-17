======
SQLite
======


The current schema, as of .21 is::

   CREATE TABLE <table> ( unique_key TEXT,
                          function_name TEXT,
                          priority INTEGER, 
                          data BLOB, 
                          when_to_run INTEGER, 
                          PRIMARY KEY (unique_key, function_name));
