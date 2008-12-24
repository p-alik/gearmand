# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

AM_CFLAGS= -I$(top_builddir)
AM_LDFLAGS=

bin_PROGRAMS=

noinst_HEADERS= \
	connection.h \
	server_common.h

gearmand_SOURCES= main.c connection.c
gearmand_LDFLAGS= -levent ${AM_LDFLAGS}

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
