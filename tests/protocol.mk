# vim:ft=automake
#
# Gearman server and library
# Copyright (C) 2013 Data Differential, http://datadifferential.com/
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in the parent directory for full text.

tests_protocol_SOURCES=
tests_protocol_LDADD=

tests_protocol_SOURCES+= tests/protocol.cc
tests_protocol_LDADD+= ${LIBGEARMAN_1_0_CLIENT_LDADD}
tests_protocol_LDADD+= libgearman/libgearmancore.la
check_PROGRAMS+= tests/protocol
noinst_PROGRAMS+= tests/protocol

test-protocol: tests/protocol gearmand/gearmand
	@tests/protocol

gdb-protocol: tests/protocol gearmand/gearmand
	@$(GDB_COMMAND) tests/protocol

valgrind-protocol: tests/protocol gearmand/gearmand
	@$(VALGRIND_COMMAND) tests/protocol

