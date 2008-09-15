INCLUDES = -I$(top_builddir)
LDADDS = -levent $(top_builddir)/libgearman/libgearman.la

bin_PROGRAMS = gearmand

noinst_HEADERS = server_common.h \
		 connection.h

gearmand_SOURCES = main.c connection.c
gearmand_LDADD = $(LDADDS)
gearmand_LDFLAGS =

valgrind: gearmand
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  gearmand

debug: gearmand
	libtool --mode=execute gdb gearmand
