# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.


bin_PROGRAMS= gearmand

gearmand_CFLAGS= $(AM_CFLAGS) $(EVENT_CFLAGS)
gearmand_SOURCES= gearmand.c
gearmand_LDFLAGS= $(EVENT_LIBS) $(top_builddir)/libgearman/libgearman.la

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
