# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

noinst_PROGRAMS= client_test worker_test
noinst_HEADERS= test.h test_gearmand.h test_worker.h

client_test_SOURCES= test.c test_gearmand.c test_worker.c client_test.c
client_test_LDFLAGS= $(top_builddir)/libgearman/libgearman.la

worker_test_SOURCES= test.c test_gearmand.c worker_test.c
worker_test_LDFLAGS= $(top_builddir)/libgearman/libgearman.la

record: client_test worker_test
	./client_test > client_test.rec
	./worker_test > worker_test.rec

test: client_test worker_test
	./client_test > client_test.res
	./worker_test > worker_test.res
	diff client_test.rec client_test.res
	diff worker_test.rec worker_test.res

valgrind: client_test worker_test
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  client_test
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  worker_test
