# Gearman server and library
# Copyright (C) 2011 Data Differential, http://datadifferential.com/
# Copyright (C) 2008 Brian Aker, Eric Day, Monty Taylor
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_INIT([gearmand],[0.26],[https://bugs.launchpad.net/gearmand], [gearmand], [http://gearman.info/])

AC_CONFIG_AUX_DIR(config)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(-Wall -Werror -Wno-portability subdir-objects foreign tar-ustar)

AC_CANONICAL_HOST
AC_CANONICAL_BUILD

AC_PREREQ(2.61)

AC_CONFIG_MACRO_DIR(m4)

AC_CONFIG_HEADERS([config.h:config.in])dnl Keep filename to 8.3 for MS-DOS.

AC_CONFIG_SRCDIR(libgearman/gearman.cc)

PANDORA_CANONICAL_TARGET(require-cxx, no-vc-changelog)

GEARMAN_LIBRARY_VERSION=6:0:0
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

LT_PREREQ([2.2])
LT_INIT()
LT_LANG([C++])
LT_LANG([C])
gl_VISIBILITY

AX_PTHREAD(, [AC_MSG_ERROR(could not find libpthread)])

dnl First look for Boost, maybe for a specific minimum version:
BOOST_REQUIRE([1.39])
BOOST_PROGRAM_OPTIONS
BOOST_THREADS

PANDORA_CXX_CSTDINT
PANDORA_CXX_CINTTYPES

WITH_LIBMEMCACHED
m4_include([m4/memcached.m4])
m4_include([m4/memcached_sasl.m4])
AM_CONDITIONAL(BUILDING_LIBMEMCACHED, false)

AM_CONDITIONAL(BUILDING_GEARMAN, true)
AM_CONDITIONAL(HAVE_LIBGEARMAN, true)
AC_SUBST(_WITH_LIBGEARMAN_SUPPORT, ["_WITH_LIBGEARMAN_SUPPORT 0"])
AC_DEFINE([GEARMAND_BINARY], "./gearmand/gearmand", [Name of the gearmand binary used in make test])
AC_DEFINE([HAVE_LIBGEARMAN], 1, [libgearman support])
AC_DEFINE([HAVE_GEARMAND_BINARY], 1, [libgearman support])

m4_include([m4/drizzled.m4])
WITH_LIBDRIZZLE

PANDORA_REQUIRE_LIBEVENT
PANDORA_REQUIRE_LIBUUID

AX_LIB_SQLITE3
AS_IF([test "x${ac_cv_libsqlite3}" = "xyes"],
      [
       AC_DEFINE([HAVE_LIBSQLITE3], [ 1 ], [Have the SQLITE3 library])
      ],
      [
       ac_cv_libsqlite3=no
       AC_DEFINE([HAVE_LIBSQLITE3], [ 0 ], [Have the SQLITE3 library])
      ])
AM_CONDITIONAL(HAVE_LIBSQLITE3, test "x${ac_cv_libsqlite3}" = "xyes")


PANDORA_HAVE_BETTER_MALLOC
PANDORA_HAVE_LIBPQ
PANDORA_HAVE_LIBTOKYOCABINET
AX_HAVE_LIBHIREDIS
AC_FUNC_STRERROR_R
SOCKET_SEND_FLAGS
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h netinet/tcp.h)
AC_CHECK_HEADERS(errno.h getopt.h pwd.h signal.h)
AC_CHECK_HEADERS(stdarg.h stddef.h stdio.h)
AC_CHECK_HEADERS(sys/resource.h sys/stat.h)
AC_CHECK_HEADERS(sys/socket.h sys/types.h sys/utsname.h strings.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
LT_LIB_M
AC_FUNC_ERROR_AT_LINE
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 gettimeofday memchr memmove memset socket strcasecmp strdup strerror strncasecmp uname])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])

AC_CHECK_FUNC(setsockopt, [], [AC_CHECK_LIB(socket, setsockopt)])
AC_CHECK_FUNC(bind, [], [AC_CHECK_LIB(bind, bind)])

AC_TEST_FOR_REPOSITORY

m4_include([m4/relro.m4])


AC_CONFIG_FILES([libtest/version.h])
AC_CONFIG_FILES(Makefile
                docs/conf.py
                libgearman-1.0/version.h
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
echo "   * Building with libdrizzle   $ac_enable_libdrizzle"
echo "   * Building with libmemcached $ac_enable_libmemcached"
echo "   * Building with libpq        $ac_cv_libpq"
echo "   * Building with tokyocabinet $ac_cv_libtokyocabinet"
echo ""
echo "---"
