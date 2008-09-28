# Gearman server and library
# Copyright (C) 2008 Brian Aker
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

AM_CFLAGS= -I$(top_builddir)
AM_LDFLAGS= $(top_builddir)/libgearman/libgearman.la

noinst_PROGRAMS= client_test testworker
noinst_HEADERS= test.h

client_test_SOURCES= test.c function.c

testworker_SOURCES= test.c worker_drone.c

record:
	./client_test output.txt > output.res
test: client_test
	./client_test output.txt > output.cmp
	diff output.res output.cmp

valgrind:
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  client_test output.txt
