# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I m4 --install

bin_PROGRAMS =
sbin_PROGRAMS = 
check_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =
man1_MANS =
man3_MANS =
man8_MANS =
DISTCLEANFILES =
EXTRA_DIST =
CLEANFILES =
TESTS =
.PHONY =

%.hpp.gch: %.hpp 
	$(CXXCOMPILE) -c $<

# Should always be first
include libtest/include.am
include libhostile/include.am

include benchmark/include.am
include bin/include.am
include examples/include.am
include gearmand/include.am
include libgearman-server/include.am
include libgearman/include.am
include util/include.am
include tests/include.am
include support/include.am
include docs/include.am

docs: html man

TESTS += $(check_PROGRAMS)

EXTRA_DIST+= HACKING
EXTRA_DIST+= ${man1_MANS}
EXTRA_DIST+= ${man3_MANS}
EXTRA_DIST+= ${man8_MANS}
EXTRA_DIST+= scripts/README.solaris

test: check

clean-local:
	-rm -rf docs/_build docs/doctrees

rpm-build: support/gearmand.spec
	@rm -f ~/rpmbuild/RPMS/x86_64/gearmand-$(VERSION)*.rpm
	@rm -f ~/rpmbuild/SRPMS/gearmand-$(VERSION)*.rpm
	@cp gearmand-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	@cp support/gearmand.init ~/rpmbuild/SOURCES/
	@rpmbuild -ba support/gearmand.spec
	@cp ~/rpmbuild/RPMS/x86_64/gearmand-$(VERSION)*.rpm .
	@cp ~/rpmbuild/RPMS/x86_64/gearmand-devel-$(VERSION)*.rpm .
	@cp ~/rpmbuild/RPMS/x86_64/gearmand-debuginfo-$(VERSION)*.rpm .
	@cp ~/rpmbuild/RPMS/x86_64/gearmand-server-$(VERSION)*.rpm .
	@cp ~/rpmbuild/SRPMS/gearmand-$(VERSION)*.rpm .

rpm-sign: rpm-build
	@rpm --addsign gearmand-$(VERSION)*.rpm gearmand-devel-$(VERSION)*.rpm gearmand-debuginfo-$(VERSION)*.rpm gearmand-server-$(VERSION)*.rpm
	@rpm --checksig gearmand-$(VERSION)*.rpm gearmand-devel-$(VERSION)*.rpm gearmand-debuginfo-$(VERSION)*.rpm gearmand-server-$(VERSION)*.rpm

rpm: man distcheck rpm-build

release: rpm rpm-sign
	@rpm --addsign gearmand-$(VERSION)*.rpm gearmand-devel-$(VERSION)*.rpm gearmand-debuginfo-$(VERSION)*.rpm gearmand-server-$(VERSION)*.rpm
	@rpm --checksig gearmand-$(VERSION)*.rpm gearmand-devel-$(VERSION)*.rpm gearmand-debuginfo-$(VERSION)*.rpm gearmand-server-$(VERSION)*.rpm

merge-clean:
	@find ./ | $(GREP) \.moved | xargs rm -r -f
	@find ./ | $(GREP) \.orig | xargs rm -f
	@find ./ | $(GREP) \.rej | xargs rm -f
	@find ./ | $(GREP) \~$$ | xargs rm -f
	@echo "The follow files need to be removed or checked in:"
	@bzr unknowns
