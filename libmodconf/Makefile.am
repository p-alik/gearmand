# Module configuration library
# Copyright (C) 2009 Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

lib_LTLIBRARIES= libmodconf.la

libmodconfincludedir= ${includedir}/libmodconf

dist_libmodconfinclude_HEADERS= \
	modconf.h \
	constants.h \
	structs.h

noinst_HEADERS= \
	common.h

libmodconf_la_SOURCES= \
	modconf.c \
	module.c

libmodconf_la_LDFLAGS= -version-info $(MODCONF_LIBRARY_VERSION)
