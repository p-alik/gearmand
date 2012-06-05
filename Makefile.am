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
BUILT_SOURCES =
TESTS =
.PHONY =

CLEANFILES+= ${BUILT_SOURCES}

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
include libgearman-1.0/include.am
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

BUILT_SOURCES+= configmake.h
noinst_HEADERS+= configmake.h
configmake.h: ${top_srcdir}/Makefile.in
	@echo "Making $@"
	@rm -f $@-t $@
	@{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  echo '#define PREFIX "$(prefix)"'; \
	  echo '#define SYSCONFDIR "$(sysconfdir)"'; \
	  echo '#define GEARMAND_CONFIG "$(sysconfdir)/gearmand.conf"'; \
	  echo '#define LOCALSTATEDIR "$(localstatedir)"'; \
	  echo '#define GEARMAND_PID "$(localstatedir)/gearmand.pid"'; \
	  } | sed '/""/d' > $@-t
	@if diff $@-t $@ >/dev/null 2>&1 ; then \
	  rm @-t ; \
	  else \
	  mv $@-t $@ ; \
	  fi
clean-local:
	-rm -rf docs/_build docs/doctrees

include rpm.am
