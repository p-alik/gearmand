# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

if HAVE_LIBMEMCACHED
LIBMEMCACHED_TEST= memcached_test
LIBMEMCACHED_RES= memcached_test.res
LIBMEMCACHED_REC= memcached_test.rec
LIBMEMCACHED_RECORD= ./memcached_test > memcached_test.rec
LIBMEMCACHED_RUN= ./memcached_test > memcached_test.res
LIBMEMCACHED_CHECK= diff ${top_srcdir}/tests/memcached_test.rec memcached_test.res
LIBMEMCACHED_VALGRIND= libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  memcached_test
memcached_test_SOURCES= test.c test_gearmand.c memcached_test.c
endif

LDADD= \
	$(LTLIBUUID) \
	$(LTLIBEVENT) \
	$(top_builddir)/libgearman/libgearman.la

noinst_PROGRAMS= client_test worker_test cpp_test $(LIBMEMCACHED_TEST)
noinst_HEADERS= test.h test_gearmand.h test_worker.h

client_test_SOURCES= test.c test_gearmand.c test_worker.c client_test.c

worker_test_SOURCES= test.c test_gearmand.c worker_test.c

# Test linking with C++ application
cpp_test_SOURCES= cpp_test.cc

CLEANFILES= client_test.res worker_test.res $(LIBMEMCACHED_RES)

EXTRA_DIST= client_test.rec worker_test.rec $(LIBMEMCACHED_REC)

record: client_test worker_test $(LIBMEMCACHED_TEST)
	./client_test > client_test.rec
	./worker_test > worker_test.rec
	$(LIBMEMCACHED_RECORD)

test: check

check: client_test worker_test $(LIBMEMCACHED_TEST)
	./client_test > client_test.res
	diff ${top_srcdir}/tests/client_test.rec client_test.res
	./worker_test > worker_test.res
	diff ${top_srcdir}/tests/worker_test.rec worker_test.res
	$(LIBMEMCACHED_RUN)
	$(LIBMEMCACHED_CHECK)

valgrind: client_test worker_test $(LIBMEMCACHED_TEST)
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  client_test
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  worker_test
	$(LIBMEMCACHED_VALGRIND)
