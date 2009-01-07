# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT(gearmand, [1.0.0], [https://launchpad.net/gearmand])
AC_CONFIG_SRCDIR(libgearman/gearman.c)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config.h])

GEARMAN_MAJOR_VERSION=1
GEARMAN_MINOR_VERSION=0
GEARMAN_MICRO_VERSION=0
GEARMAN_VERSION=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION.$GEARMAN_MICRO_VERSION
GEARMAN_RELEASE=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION
GEARMAN_LIBRARY_VERSION=$GEARMAN_MAJOR_VERSION:$GEARMAN_MINOR_VERSION:$GEARMAN_MICRO_VERSION

VERSION=$GEARMAN_RELEASE

AC_SUBST(GEARMAN_VERSION)
AC_SUBST(GEARMAN_RELEASE)
AC_SUBST(GEARMAN_LIBRARY_VERSION)

# Setting CFLAGS here prevents AC_CANONICAL_TARGET from injecting them
SAVE_CFLAGS=${CFLAGS}
SAVE_CXXFLAGS=${CXXFLAGS}
CFLAGS=
CXXFLAGS=

AC_CANONICAL_TARGET

CFLAGS=${SAVE_CFLAGS}
CXXFLAGS=${SAVE_CXXFLAGS}

AM_INIT_AUTOMAKE(-Wall -Werror nostdinc subdir-objects)

if test "x${enable_dependency_tracking}" = "x"
then
  enable_dependency_tracking=yes
fi

AC_PROG_CC
AM_PROG_CC_STDC

ACX_USE_SYSTEM_EXTENSIONS

AC_PROG_CPP
AM_PROG_CC_C_O

AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
ifdef([AC_PROG_CC_C99],[
    AC_PROG_CC_C99([$1],[$2],[$3])
],[
if "x$GCC" = "xyes"
then
  C99_SUPPORT_HACK="-std=gnu99"
fi
if "x$SUNCC" = "xyes"
then
  C99_SUPPORT_HACK="-xc99"
fi
])
AC_PROG_GCC_TRADITIONAL

# We use libtool
AC_PROG_LIBTOOL

AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_MSG_CHECKING("C Compiler version")
if test "$GCC" = "yes"
then
  CC_VERSION=`$CC --version | sed 1q`
elif test "$SUNCC" = "yes"
then
  CC_VERSION=`$CC -V 2>&1 | sed 1q`
else
  CC_VERSION=""
fi
AC_MSG_RESULT("$CC_VERSION")
AC_SUBST(CC_VERSION)

AC_DEFINE([_THREAD_SAFE],[1],[Thread safe code])
AC_DEFINE([_REENTRANT],[1],[Re-entrant code])


AC_HEADER_TIME
AC_CHECK_HEADERS(assert.h errno.h fcntl.h netinet/tcp.h signal.h stdarg.h)
AC_CHECK_HEADERS(stdio.h stdlib.h string.h sys/socket.h sys/types.h)
AC_CHECK_HEADERS(sys/utsname.h unistd.h)

#--------------------------------------------------------------------
# Check for libevent
#--------------------------------------------------------------------

SEARCH_FOR_LIB(event, event_init, event.h)

save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
CPPFLAGS="$EVENT_CFLAGS"
LIBS="$EVENT_LIBS"
AC_CACHE_CHECK([for bufferevent in libevent], ac_cv_libevent_works, [
  AC_TRY_LINK([
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <event.h>],[
        struct bufferevent bev;
        bufferevent_settimeout(&bev, 1, 1);
  ], ac_cv_libevent_works=yes, [
        AC_MSG_ERROR([you need to install a more recent version of libevent,
        check http://www.monkey.org/~provos/libevent/])
  ])

])
AC_CHECK_FUNCS(event_base_new)
CPPFLAGS="$save_CPPFLAGS"
LIBS="$save_LIBS"

#--------------------------------------------------------------------
# Check for libuuid
#--------------------------------------------------------------------

dnl Do this by hand instead of with SEARCH_FOR_LIB, because uuid is weird.
AC_CHECK_HEADERS(uuid/uuid.h)
if test "x$ac_cv_header_uuid_uuid_h" = "xno"
then
  AC_MSG_ERROR([Couldn't find uuid/uuid.h. Try installing libuuid development packages])
fi
AC_CHECK_LIB(uuid, uuid_generate)

#--------------------------------------------------------------------
# Check for tcmalloc
#--------------------------------------------------------------------

AC_ARG_ENABLE([tcmalloc],
    [AS_HELP_STRING([--enable-tcmalloc],
       [Enable linking with tcmalloc @<:@default=off@:>@])],
    [ac_enable_tcmalloc="$enableval"],
    [ac_enable_tcmalloc="no"])

if test "x$ac_enable_tcmalloc" != "xno"
then
  AC_CHECK_LIB(tcmalloc,malloc,[],[])
fi

if test "x$ac_cv_lib_tcmalloc_malloc" != "xyes"
then
  AC_CHECK_LIB(mtmalloc,malloc,[],[])
fi


# Build optimized or debug version ?
# First check for gcc and g++
SYMBOLS_CFLAGS="-g"
DEBUG_OPTIMIZE_CFLAGS=""
OPTIMIZE_CFLAGS="-O"
if test "$GCC" = "yes"
then
  SYMBOLS_CFLAGS="-ggdb3"
  DEBUG_OPTIMIZE_CFLAGS="-O0"
  OPTIMIZE_CFLAGS="-O3"
fi
if test "$SUNCC" = "yes"
then
  if test "$target_cpu" = "sparc"
  then
    MEMALIGN_FLAGS="-xmemalign=8s"
  fi
  SYMBOLS_CFLAGS="-g"
  DEBUG_OPTIMIZE_CFLAGS=""
  OPTIMIZE_CFLAGS="-xO4 -xlibmil -xdepend -Xa -mt -xstrconst -D_FORTEC_ ${MEMALIGN_FLAGS}"
fi

dnl TODO: Remove this define once we are using 2.61 across the board.
# AX_HEADER_ASSERT
# ----------------
# Check whether to enable assertions.
AC_DEFUN([AX_HEADER_ASSERT],
[
  AC_MSG_CHECKING([whether to enable assertions])
  AC_ARG_ENABLE([assert],
    [AS_HELP_STRING([--disable-assert],
       [Turn off assertions])],
    [ac_cv_assert="no"],
    [ac_cv_assert="yes"])
  AC_MSG_RESULT([$ac_cv_assert])
])
AX_HEADER_ASSERT

CFLAGS="${SYMBOLS_CFLAGS} ${CFLAGS}"

AC_ARG_WITH([debug],
    [AS_HELP_STRING([--with-debug],
       [Add debug code/turns off optimizations (yes|no) @<:@default=no@:>@])],
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "yes"
then
  # Medium debug.
  CFLAGS="${DEBUG_OPTIMIZE_CFLAGS} ${CFLAGS} ${SAVE_CFLAGS} -DDEBUG"
else
  # Optimized version. No debug
  CFLAGS="${OPTIMIZE_CFLAGS} ${CFLAGS} ${SAVE_CFLAGS}"
fi

AC_ARG_ENABLE([profiling],
    [AS_HELP_STRING([--enable-profiling],
       [Toggle profiling @<:@default=off@:>@])],
    [ac_profiling="$enableval"],
    [ac_profiling="no"])

AC_ARG_ENABLE([coverage],
    [AS_HELP_STRING([--enable-coverage],
       [Toggle coverage @<:@default=off@:>@])],
    [ac_coverage="$enableval"],
    [ac_coverage="no"])

AC_ARG_ENABLE([fail],
  [AS_HELP_STRING([--disable-fail],
    [Turn warnings into failures @<:@default=yes@:>@])],
  [ac_warn_fail="$enableval"],
  [ac_warn_fail="yes"])

AC_ARG_ENABLE([pedantic-warnings],
  [AS_HELP_STRING([--disable-pedantic-warnings],
    [Toggle pedanticness @<:@default=yes@:>@])],
  [ac_warn_pedantic="$enableval"],
  [ac_warn_pedantic="yes"])

AC_ARG_ENABLE([unreachable],
  [AS_HELP_STRING([--enable-unreachable],
    [Enable warnings about unreachable code @<:@default=no@:>@])],
  [ac_warn_unreachable="$enableval"],
  [ac_warn_unreachable="no"])

if test "$GCC" = "yes"
then

  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-Werror"
  fi
  BASE_WARNINGS="-W -Wall -Wextra -Winvalid-pch ${W_FAIL}"

  if test "$ac_profiling" = "yes"
  then
    GPROF_PROFILING="-pg"
  else
    GPROF_PROFILING=" "
  fi

  if test "$ac_coverage" = "yes"
  then
    GPROF_COVERAGE="-fprofile-arcs -ftest-coverage"
  else
    GPROF_COVERAGE=" "
  fi

  if test "$ac_warn_pedantic" = "yes"
  then
    W_PEDANTIC="-pedantic -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wswitch-enum -Wcast-align -Wlogical-op"
  fi

  if test "$ac_warn_unreachable" = "yes"
  then
    W_UNREACHABLE="-Wunreachable-code"
  fi

  CC_WARNINGS="${C99_SUPPORT_HACK} ${BASE_WARNINGS} ${W_PEDANTIC} ${W_UNREACHABLE} ${GPROF_PROFILING} ${GPROF_COVERAGE} "

  NO_REDUNDANT_DECLS="-Wno-redundant-decls"
  CPPFLAGS="${CPPFLAGS} -fpch-deps"
fi
if test "$SUNCC" = "yes"
then
  CC_WARNINGS="-v -xc99=all -errtags=yes -errwarn=%all -erroff=E_INTEGER_OVERFLOW_DETECTED"
fi


AM_CFLAGS="${CC_WARNINGS} ${AM_CFLAGS}"

AC_SUBST(NO_REDUNDANT_DECLS)

AC_SUBST([GLOBAL_CPPFLAGS],['-I$(top_srcdir) -I$(top_builddir)'])
AC_SUBST([AM_CPPFLAGS],['${GLOBAL_CPPFLAGS}'])
AC_SUBST([AM_CFLAGS])

AC_CHECK_PROGS([DOXYGEN], [doxygen])


AC_CONFIG_FILES(Makefile libgearman/Makefile gearmand/Makefile
                tests/Makefile examples/Makefile)

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $ac_enable_debug"
echo "   * Warnings as failure:       $ac_warn_fail"
echo ""
echo "---"

