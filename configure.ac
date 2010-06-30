# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day, Monty Taylor
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT([gearmand],[0.14],[https://launchpad.net/gearmand])
AC_CONFIG_SRCDIR(libgearman/gearman.c)
AC_CONFIG_AUX_DIR(config)

PANDORA_CANONICAL_TARGET(require-cxx)

GEARMAN_LIBRARY_VERSION=4:0:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                |        |        or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed
AC_SUBST(GEARMAN_LIBRARY_VERSION)


PANDORA_REQUIRE_PTHREAD
PANDORA_REQUIRE_LIBEVENT
PANDORA_REQUIRE_LIBUUID

PANDORA_HAVE_LIBGTEST
PANDORA_HAVE_BETTER_MALLOC
PANDORA_HAVE_LIBSQLITE3
PANDORA_HAVE_LIBDRIZZLE
PANDORA_HAVE_LIBMEMCACHED(0.39)
PANDORA_HAVE_MEMCACHED
PANDORA_HAVE_LIBPQ
PANDORA_HAVE_LIBTOKYOCABINET


AC_CHECK_HEADERS(errno.h fcntl.h getopt.h netinet/tcp.h pwd.h signal.h)
AC_CHECK_HEADERS(stdarg.h stddef.h stdio.h stdlib.h string.h)
AC_CHECK_HEADERS(sys/resource.h sys/stat.h)
AC_CHECK_HEADERS(sys/socket.h sys/types.h sys/utsname.h unistd.h strings.h)


AC_CONFIG_FILES(Makefile
                libgearman/configure.h
                scripts/gearmand
                scripts/gearmand-init
                scripts/gearmand.xml
                scripts/smf_install.sh
                support/gearmand.pc
                support/gearmand.spec)

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Warnings as failure:       $ac_cv_warnings_as_errors"
echo "   * Building with libsqlite3   $ac_cv_libsqlite3"
echo "   * Building with libdrizzle   $ac_cv_libdrizzle"
echo "   * Building with libmemcached $ac_cv_libmemcached"
echo "   * Building with libpq        $ac_cv_libpq"
echo "   * Building with tokyocabinet $ac_cv_libtokyocabinet"
echo ""
echo "---"
