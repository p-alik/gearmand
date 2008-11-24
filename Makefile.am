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

SUBDIRS = libgearman server tests examples

docs: all
	doxygen docs/Doxyfile.api
	doxygen docs/Doxyfile.dev

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
