# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

lib_LTLIBRARIES= libgearman.la

libgearmanincludedir= ${includedir}/libgearman

dist_libgearmaninclude_HEADERS= \
	client.h \
	con.h \
	constants.h \
	gearman.h \
        gearmand.h \
	job.h \
	packet.h \
	server.h \
	server_client.h \
	server_con.h \
	server_job.h \
	server_function.h \
	server_worker.h \
	structs.h \
	task.h \
	worker.h

noinst_HEADERS= \
	common.h

libgearman_la_SOURCES= \
	client.c \
	con.c \
	gearman.c \
	gearmand.c \
	job.c \
	packet.c \
	server.c \
	server_client.c \
	server_con.c \
	server_job.c \
	server_function.c \
	server_worker.c \
	task.c \
	worker.c

libgearman_la_LDFLAGS= -version-info $(GEARMAN_LIBRARY_VERSION) 
libgearman_la_LIBADD= $(EVENT_LIBS) $(UUID_LIBS)
libgearman_la_CFLAGS= $(AM_CFLAGS) $(UUID_CFLAGS) $(EVENT_CFLAGS)
