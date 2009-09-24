# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

sbin_PROGRAMS= gearmand

gearmand_LDADD= \
	$(top_builddir)/libgearman-server/libgearman-server.la \
	${BETTER_MALLOC_LIBS}

gearmand_SOURCES= gearmand.c

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
