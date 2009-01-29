# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

AM_CFLAGS= -I$(top_builddir)
AM_LDFLAGS= $(LTLIBUUID) $(LTLIBEVENT) $(top_builddir)/libgearman/libgearman.la

bin_PROGRAMS= gearmand

gearmand_SOURCES= gearmand.c

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
