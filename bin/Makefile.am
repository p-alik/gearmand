# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

AM_LDFLAGS= $(LIBUUID) $(LIBEVENT) $(top_builddir)/libgearman/libgearman.la

noinst_PROGRAMS= \
	gearman

gearman_CFLAGS= $(AM_CFLAGS) -I$(top_builddir)
gearman_SOURCES= gearman.c
