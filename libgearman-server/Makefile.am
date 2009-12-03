# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

if HAVE_LIBDRIZZLE
QUEUE_LIBDRIZZLE_H= queue_libdrizzle.h
QUEUE_LIBDRIZZLE_C= queue_libdrizzle.c
endif

if HAVE_LIBMEMCACHED
QUEUE_LIBMEMCACHED_H= queue_libmemcached.h
QUEUE_LIBMEMCACHED_C= queue_libmemcached.c
endif

if HAVE_LIBSQLITE3
QUEUE_LIBSQLITE3_H= queue_libsqlite3.h
QUEUE_LIBSQLITE3_C= queue_libsqlite3.c
endif

if HAVE_LIBPQ
QUEUE_LIBPQ_H= queue_libpq.h
QUEUE_LIBPQ_C= queue_libpq.c
endif

if HAVE_LIBTOKYOCABINET
QUEUE_LIBTOKYOCABINET_H= queue_libtokyocabinet.h
QUEUE_LIBTOKYOCABINET_C= queue_libtokyocabinet.c
endif

lib_LTLIBRARIES= libgearman-server.la

libgearman_serverincludedir= ${includedir}/libgearman-server

dist_libgearman_serverinclude_HEADERS= \
	client.h \
	conf.h \
	conf_module.h \
	conn.h \
	constants.h \
	function.h \
	job.h \
	packet.h \
	server.h \
	structs.h \
	thread.h \
	worker.h \
	$(QUEUE_LIBDRIZZLE_H) \
	$(QUEUE_LIBMEMCACHED_H) \
	$(QUEUE_LIBSQLITE3_H) \
	$(QUEUE_LIBPQ_H) \
	$(QUEUE_LIBTOKYOCABINET_H) \
	protocol_http.h \
	gearmand.h \
	gearmand_thread.h \
	gearmand_con.h

noinst_HEADERS= \
	common.h

libgearman_server_la_SOURCES= \
	client.c \
	conf.c \
	conf_module.c \
	conn.c \
	function.c \
	job.c \
	packet.c \
	server.c \
	thread.c \
	worker.c \
	$(QUEUE_LIBDRIZZLE_C) \
	$(QUEUE_LIBMEMCACHED_C) \
	$(QUEUE_LIBSQLITE3_C) \
	$(QUEUE_LIBPQ_C) \
	$(QUEUE_LIBTOKYOCABINET_C) \
	protocol_http.c \
	gearmand.c \
	gearmand_thread.c \
	gearmand_con.c

libgearman_server_la_CFLAGS= \
	${AM_CFLAGS} \
	-DBUILDING_LIBGEARMAN

libgearman_server_la_LDFLAGS= \
	-version-info \
	$(GEARMAN_SERVER_LIBRARY_VERSION) \
	$(LD_VERSION_SCRIPT_SERVER)

libgearman_server_la_LIBADD= \
	$(top_builddir)/libgearman/libgearman.la \
	$(LTLIBEVENT) \
	$(LTLIBDRIZZLE) \
	$(LTLIBMEMCACHED) \
	$(LTLIBSQLITE3) \
	$(LTLIBPQ) \
	$(LTLIBTOKYOCABINET)	

EXTRA_DIST= libgearman-server.ver
