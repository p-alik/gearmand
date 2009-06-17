# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

LDADD= $(LTLIBUUID) $(LTLIBEVENT) $(top_builddir)/libgearman/libgearman.la

noinst_PROGRAMS= \
	blobslap_client \
	blobslap_worker

noinst_HEADERS= \
	benchmark.h

blobslap_client_SOURCES= blobslap_client.c benchmark.c

blobslap_worker_SOURCES= blobslap_worker.c benchmark.c
