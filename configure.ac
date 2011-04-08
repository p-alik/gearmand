# Gearman server and library
# Copyright (C) 2011 Data Differential, http://datadifferential.com/
# Copyright (C) 2008 Brian Aker, Eric Day, Monty Taylor
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT([gearmand],[0.20],[https://launchpad.net/gearmand])
AC_CONFIG_SRCDIR(libgearman/gearman.cc)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)

PANDORA_CANONICAL_TARGET(require-cxx)

GEARMAN_LIBRARY_VERSION=5:1:1
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

BOOST_REQUIRE(1.37.0)
BOOST_PROGRAM_OPTIONS(mt-)


PANDORA_REQUIRE_PTHREAD
PANDORA_REQUIRE_LIBEVENT
PANDORA_REQUIRE_LIBUUID

PANDORA_HAVE_BETTER_MALLOC
PANDORA_HAVE_LIBSQLITE3
PANDORA_HAVE_LIBDRIZZLE
PANDORA_HAVE_LIBMEMCACHED(0.42)
PANDORA_HAVE_MEMCACHED
PANDORA_HAVE_LIBPQ
PANDORA_HAVE_LIBTOKYOCABINET
AC_FUNC_STRERROR_R
SOCKET_SEND_FLAGS

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

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
AC_TYPE_UINT8_T
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 gettimeofday memchr memmove memset socket strcasecmp strdup strerror strncasecmp uname])

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
