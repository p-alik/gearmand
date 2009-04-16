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
        gearmand_thread.h \
        gearmand_con.h \
	job.h \
	packet.h \
	server.h \
	server_client.h \
	server_con.h \
	server_job.h \
	server_function.h \
	server_thread.h \
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
	gearmand_thread.c \
	gearmand_con.c \
	job.c \
	packet.c \
	server.c \
	server_client.c \
	server_con.c \
	server_job.c \
	server_function.c \
	server_thread.c \
	server_worker.c \
	task.c \
	worker.c

libgearman_la_LDFLAGS= -version-info $(GEARMAN_LIBRARY_VERSION) 
libgearman_la_LIBADD= $(LTLIBEVENT) $(LTLIBUUID)
