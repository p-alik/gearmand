# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4

SUBDIRS = libgearman gearmand tests examples support

docs: all
	${DOXYGEN} docs/Doxyfile.api
	${DOXYGEN} docs/Doxyfile.dev

test: all
	(cd tests; ${MAKE} test)

test-extended: all
	(cd tests; ${MAKE} test-extended)

valgrind:
	(cd tests; ${MAKE} valgrind)

# This needs to be fixed to not use Brian's home directory. :)
rpm: all dist
	cp gearmand-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	rpmbuild -ba support/gearmand.spec
	cp ~/rpmbuild/RPMS/x86_64/gearmand-$(VERSION)*.rpm .
	cp ~/rpmbuild/SRPMS/gearmand-$(VERSION)*.rpm .
