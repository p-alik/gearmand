# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
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

lib_LTLIBRARIES= libgearman.la

libgearmanincludedir= ${includedir}/libgearman

dist_libgearmaninclude_HEADERS= \
	client.h \
	con.h \
	constants.h \
	gearman.h \
	job.h \
	packet.h \
	structs.h \
	task.h \
	worker.h

noinst_HEADERS= \
	common.h

libgearman_la_SOURCES= \
	client.c \
	con.c \
	gearman.c \
	job.c \
	packet.c \
	task.c \
	worker.c

libgearman_la_LDFLAGS= -version-info $(GEARMAN_LIBRARY_VERSION)
