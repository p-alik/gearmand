gearmand
========

SYNOPSIS
________


.. program:: gearmand

**General options**

.. option:: -b [ --backlog ] arg (=32)

Number of backlog connections for listen.

.. option:: --check-args

Check command line and configuration file argments and then exit.

.. option:: -d [ --daemon ]

Daemon, detach and run in the background.

.. option:: -f [ --file-descriptors ] arg

Number of file descriptors to allow for the process (total connections will be slightly less). Default is max allowed for user.

.. option:: -h [ --help ]

Print this help menu.

.. option:: -j [ --job-retries ] arg (=0)

Number of attempts to run the job before the job server removes it. This is helpful to ensure a bad job does not crash all available workers. Default is no limit.

.. option:: -l [ --log-file ] arg

Log file to write errors and information to.  Turning this option on also forces the first verbose level to be enabled.

.. option:: -L [ --listen ] arg

Address the server should listen on. Default is INADDR_ANY.

.. option:: -p [ --port ] arg (=4730)

Port the server should listen on.

.. option:: -P [ --pid-file ] arg

File to write process ID out to.

.. option:: -r [ --protocol ] arg

Load protocol module.

.. option:: -R [ --round-robin ]

Assign work in round-robin order per worker connection. The default is to assign work in the order of functions added by the worker.

.. option:: -q [ --queue-type ] arg

Persistent queue type to use.

.. option:: -t [ --threads ] arg (=4)

Number of I/O threads to use. Default=4.

.. option:: -u [ --user ] arg

Switch to given user after startup.

.. option:: -v [ --verbose ] arg (=v)

Increase verbosity level by one.

.. option:: -V [ --version ]

Display the version of gearmand and exit.

.. option:: -w [ --worker-wakeup ] arg (=0)

Number of workers to wakeup for each job received. The default is to wakeup all available workers.

**HTTP:**

.. option:: --http-port arg (=8080)

Port to listen on.

**sqlite**

.. option:: --libsqlite3-db arg

Database file to use.

.. option:: --libsqlite3-table arg (=gearman_queue)

Table to use.  

**Postgres**

.. option:: --libpq-conninfo arg

PostgreSQL connection information string.

.. option:: --libpq-table arg (=queue)

Table to use.

**tokyocabinet**

.. option:: --libtokyocabinet-file arg

File name of the database. [see: man tcadb, tcadbopen() for name guidelines]

.. option:: --libtokyocabinet-optimize

Optimize database on open. [default=true]



DESCRIPTION
___________


Gearman provides a generic application framework to farm out work to other machines or processes that are better suited to do the work. It allows you to do work in parallel, to load balance processing, and to call functions between languages. It can be used in a variety of applications, from high-availability web sites to the transport of database replication events. In other words, it is the nervous system for how distributed processing communicates. A few strong points about Gearman:

* Open Source - It's free! (in both meanings of the word) Gearman has an active open source community that is easy to get involved with if you need help or want to contribute.

* Multi-language - There are interfaces for a number of languages, and this list is growing. You also have the option to write heterogeneous applications with clients submitting work in one language and workers performing that work in another.

* Flexible - You are not tied to any specific design pattern. You can quickly put together distributed applications using any model you choose, one of those options being Map/Reduce.

* Fast - Gearman has a simple protocol and interface with a new optimized server in C to minimize your application overhead.

* Embeddable - Since Gearman is fast and lightweight, it is great for applications of all sizes. It is also easy to introduce into existing applications with minimal overhead.

* No single point of failure - Gearman can not only help scale systems, but can do it in a fault tolerant way.


HOME
____


To find out more information please check:
`https://launchpad.net/gearmand <https://launchpad.net/gearmand>`_


SEE ALSO
________


`gearmand` (1) `libgearman` (3)

