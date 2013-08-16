# vim:ft=automake
#
# Gearman server and library
# Copyright (C) 2011-2012 DataDifferential LLC, http://datadifferential.com/
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

ACLOCAL_AMFLAGS= -I $(top_srcdir)/m4

notrans_dist_man_MANS=
noinst_PROGRAMS=

EXTRA_DIST=$(notrans_dist_man_MANS)

# The man pages depend on the --help strings and the version number.
common_mandeps = $(top_srcdir)/configure.ac

clean-local: clean-docs-check

include $(top_srcdir)/docs/include.am
include $(top_srcdir)/man/include.am
