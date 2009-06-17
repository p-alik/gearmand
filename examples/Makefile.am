# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

LDADD= $(LTLIBUUID) $(LTLIBEVENT) $(top_builddir)/libgearman/libgearman.la

noinst_PROGRAMS= \
	echo_client \
	echo_worker \
	reverse_client \
	reverse_client_bg \
	reverse_client_cb \
	reverse_worker \
	wc_worker

echo_client_SOURCES= echo_client.c

echo_worker_SOURCES= echo_worker.c

reverse_client_SOURCES= reverse_client.c

reverse_client_bg_SOURCES= reverse_client_bg.c

reverse_client_cb_SOURCES= reverse_client_cb.c

reverse_worker_SOURCES= reverse_worker.c

wc_worker_SOURCES= wc_worker.c
