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


To start gearmand edit /etc/default/gearman-job-server to include::

  export PGHOST=TheHostname
  export PGPORT=5432
  export PGUSER=gearman
  export PGPASSWORD=ThePassword
  export PGDATABASE=gearman
  PARAMS="--verbose -q libpq --libpq-table=gearmanqueue1 --verbose"

This is Debian specific so you will need to adapt it to your distribution.
