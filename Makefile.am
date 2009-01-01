# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4

SUBDIRS = libgearman gearmand tests examples

docs: all
	${DOXYGEN} docs/Doxyfile.api
	${DOXYGEN} docs/Doxyfile.dev

test: all
	cd tests; make test; cd ..

test-extended: all
	cd tests; make test-extended; cd ..

valgrind:
	cd tests; make valgrind; cd ..

# This needs to be fixed to not use Brian's home directory. :)
rpm: all dist
	cp libgearman-$(VERSION).tar.gz /home/brian/rpmbuild/SOURCES/
	rpmbuild -ba support/libgearman.spec
	cp /home/brian/rpmbuild/RPMS/x86_64/libgearman-$(VERSION)*.rpm .
	cp /home/brian/rpmbuild/SRPMS/libgearman-$(VERSION)*.rpm .
