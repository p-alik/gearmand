# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4

bin_PROGRAMS =
sbin_PROGRAMS = 
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
nobase_pkginclude_HEADERS =
noinst_HEADERS =
EXTRA_DIST = 


SUBDIRS = \
	. \
	tests

include benchmark/include.am
include bin/include.am
include examples/include.am
include gearmand/include.am
include libgearman/include.am
include libgearman-server/include.am
include docs/man_list

EXTRA_DIST+= \
	docs/Doxyfile.api \
	docs/Doxyfile.dev \
	docs/api_header.html \
	docs/dev_header.html \
	docs/doxygen.h \
  scripts/gearmand-init \
	scripts/gearmand.xml \
	scripts/gearmand \
	scripts/README.solaris \
	support/gearmand.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = support/gearmand.pc

docs: all
	${DOXYGEN} docs/Doxyfile.api
	${DOXYGEN} docs/Doxyfile.dev

test: check

valgrind:
	(cd tests; ${MAKE} valgrind)

rpm: all dist
	cp gearmand-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	rpmbuild -ba support/gearmand.spec
	cp ~/rpmbuild/RPMS/x86_64/gearmand-$(VERSION)*.rpm .
	cp ~/rpmbuild/SRPMS/gearmand-$(VERSION)*.rpm .
