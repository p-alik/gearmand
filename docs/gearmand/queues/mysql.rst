=====
MySQL
=====


The current schema, as of .21 is::

   CREATE TABLE <table> ( unique_key VARCHAR(GEARMAN_UNIQUE_SIZE),
                          function_name VARCHAR(255), 
                          priority INT, 
                          data LONGBLOB, 
                          when_to_run BIGINT, 
                          unique key (unique_key, function_name));

The MySQL server communicates via the libdrizzle library. As of version 0.35 there is now a native MySQL queue that makes use of libmysql.

To launch Gearman with MySQL support, you need to specify the following options::

    gearmand  --queue-type=MySQL \
              --mysql-host=localhost \
              --mysql-port=3306 \
              --mysql-user=gearman \
              --mysql-password=your_pw \
              --mysql-db=gearman \
              --mysql-table=gearman_queue

You will need to make sure that the appropriate permissions are setup for the user that you use. Gearman will handle the creation of the table when it first starts up.
