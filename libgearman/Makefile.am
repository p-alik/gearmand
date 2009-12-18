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
	conn.h \
	constants.h \
	gearman.h \
	job.h \
	packet.h \
	structs.h \
	task.h \
	visibility.h \
	worker.h

noinst_HEADERS= \
	common.h \
        gearman_local.h

libgearman_la_SOURCES= \
	client.c \
	conn.c \
	gearman.c \
	job.c \
	packet.c \
	task.c \
	worker.c

libgearman_la_CFLAGS= \
	${AM_CFLAGS} \
	-DBUILDING_LIBGEARMAN

libgearman_la_LDFLAGS= \
	-version-info \
	$(GEARMAN_LIBRARY_VERSION)

libgearman_la_LIBADD= \
	$(LTLIBUUID)

EXTRA_DIST= libgearman.ver
