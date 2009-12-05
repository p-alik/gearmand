# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

if HAVE_LIBMEMCACHED
LIBMEMCACHED_PIDFILE = ${abs_top_builddir}/tests/Xumemc.pid
LIBMEMCACHED_TEST= memcached_test
LIBMEMCACHED_SETUP= $(MEMCACHED_BINARY) -d -P ${LIBMEMCACHED_PIDFILE} -p 12555
LIBMEMCACHED_TEARDOWN= cat ${LIBMEMCACHED_PIDFILE} | xargs kill
LIBMEMCACHED_RM= rm ${LIBMEMCACHED_PIDFILE}
LIBMEMCACHED_RES= memcached_test.res
LIBMEMCACHED_REC= memcached_test.rec
LIBMEMCACHED_RECORD= ./memcached_test > memcached_test.rec
LIBMEMCACHED_RUN= ./memcached_test > memcached_test.res
LIBMEMCACHED_CHECK= diff ${top_srcdir}/tests/memcached_test.rec memcached_test.res
LIBMEMCACHED_VALGRIND= libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  memcached_test
memcached_test_SOURCES= test.c test_gearmand.c memcached_test.c
endif

if HAVE_LIBSQLITE3
SQLITE_TEST= sqlite_test
SQLITE_RES= sqlite_test.res
SQLITE_REC= sqlite_test.rec
SQLITE_RECORD= ./sqlite_test > sqlite_test.rec
SQLITE_RUN= ./sqlite_test > sqlite_test.res
SQLITE_CHECK= diff ${top_srcdir}/tests/sqlite_test.rec sqlite_test.res
SQLITE_RM= rm -f gearman.sql gearman.sql-journal
SQLITE_VALGRIND= libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  sqlite_test
sqlite_test_SOURCES= test.c test_gearmand.c sqlite_test.c
endif

LDADD= \
	$(LTLIBUUID) \
	$(LTLIBEVENT) \
	$(top_builddir)/libgearman/libgearman.la \
	$(top_builddir)/libgearman-server/libgearman-server.la

noinst_PROGRAMS= client_test worker_test cpp_test $(LIBMEMCACHED_TEST) $(SQLITE_TEST)
noinst_HEADERS= test.h test_gearmand.h test_worker.h

client_test_SOURCES= test.c test_gearmand.c test_worker.c client_test.c

worker_test_SOURCES= test.c test_gearmand.c worker_test.c

# Test linking with C++ application
cpp_test_SOURCES= cpp_test.cc

CLEANFILES= client_test.res worker_test.res $(LIBMEMCACHED_RES) $(SQLITE_RES)

EXTRA_DIST= client_test.rec worker_test.rec $(LIBMEMCACHED_REC) $(SQLITE_REC)

record: client_test worker_test $(LIBMEMCACHED_TEST) $(SQLITE_TEST)
	./client_test > client_test.rec
	./worker_test > worker_test.rec
	$(LIBMEMCACHED_RECORD)
	$(SQLITE_RECORD)

test: check

check: client_test worker_test $(LIBMEMCACHED_TEST) $(SQLITE_TEST)
	./client_test > client_test.res
	diff ${top_srcdir}/tests/client_test.rec client_test.res
	./worker_test > worker_test.res
	diff ${top_srcdir}/tests/worker_test.rec worker_test.res
	$(LIBMEMCACHED_SETUP)
	$(LIBMEMCACHED_RUN)
	$(LIBMEMCACHED_CHECK)
	$(LIBMEMCACHED_TEARDOWN)
	$(LIBMEMCACHED_RM)
	$(SQLITE_RUN)
	$(SQLITE_CHECK)
	$(SQLITE_RM)

valgrind: client_test worker_test $(LIBMEMCACHED_TEST) $(SQLITE_VALGRIND)
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  client_test
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  worker_test
	$(LIBMEMCACHED_VALGRIND)
	$(SQLITE_VALGRIND)
