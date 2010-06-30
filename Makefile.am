# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4

bin_PROGRAMS =
sbin_PROGRAMS = 
check_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =
EXTRA_DIST = 
CLEANFILES =
TESTS =


include benchmark/include.am
include bin/include.am
include examples/include.am
include gearmand/include.am
include libgearman/include.am
include libgearman-server/include.am
include tests/include.am
include unittests/include.am
include support/include.am

include docs/man_list

TESTS += ${check_PROGRAMS}

EXTRA_DIST+= \
	docs/Doxyfile.api \
	docs/Doxyfile.dev \
	docs/api_header.html \
	docs/dev_header.html \
	docs/doxygen.h \
	scripts/README.solaris

docs: all
	${DOXYGEN} docs/Doxyfile.api
	${DOXYGEN} docs/Doxyfile.dev

test: check

rpm: all dist
	cp gearmand-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	rpmbuild -ba support/gearmand.spec
	cp ~/rpmbuild/RPMS/x86_64/gearmand-$(VERSION)*.rpm .
	cp ~/rpmbuild/SRPMS/gearmand-$(VERSION)*.rpm .

merge-clean:
	find ./ | $(GREP) \.orig | xargs rm -f
	find ./ | $(GREP) \.rej | xargs rm -f
	find ./ | $(GREP) \~$$ | xargs rm -f
	bzr unknowns
