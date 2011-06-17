==============
Durable Queues
==============

Durable queues store jobs so that in the case of a failure of a Gearman server, the background jobs will run once the server is restarted.

Currently Drizzle, MySQL, Postgres, TokyoCabinet, Memcached, and SQLite can all be used as backends.

-------
Details
-------

.. toctree::
   :titlesonly:

   queues/drizzle
   queues/mysql
   queues/postgres
   queues/sqlite
