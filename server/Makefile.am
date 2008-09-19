AM_CFLAGS = -I$(top_builddir)

bin_PROGRAMS = gearmand

noinst_HEADERS = server_common.h \
		 connection.h

gearmand_SOURCES = main.c connection.c
gearmand_LDFLAGS = -levent  $(top_builddir)/libgearman/libgearman.la

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
