# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

AM_CFLAGS= -I$(top_builddir)

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
	server_con.h \
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
	server_con.c \
	task.c \
	worker.c

libgearman_la_LDFLAGS= -version-info $(GEARMAN_LIBRARY_VERSION)
