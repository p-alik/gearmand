# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT(gearmand, [0.4.0], [https://launchpad.net/gearmand])
AC_CONFIG_SRCDIR(libgearman/gearman.c)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config.h])

GEARMAN_MAJOR_VERSION=0
GEARMAN_MINOR_VERSION=4
GEARMAN_MICRO_VERSION=0
GEARMAN_VERSION=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION.$GEARMAN_MICRO_VERSION
GEARMAN_RELEASE=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION
GEARMAN_LIBRARY_VERSION=$GEARMAN_MAJOR_VERSION:$GEARMAN_MINOR_VERSION:$GEARMAN_MICRO_VERSION

PACKAGE=gearmand
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

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, "-Wall -Werror nostdinc subdir-objects")

if test "x${enable_dependency_tracking}" = "x"
then
  enable_dependency_tracking=yes
fi

if test -d ".bzr"
then
  building_from_bzr=yes
else
  building_from_bzr=no
fi

AC_PROG_CXX
AM_PROG_CC_STDC

ACX_USE_SYSTEM_EXTENSIONS

AC_PROG_CPP
AM_PROG_CC_C_O

AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
AC_PROG_GCC_TRADITIONAL

# We use libtool
AC_PROG_LIBTOOL
AM_SANITY_CHECK
LIBTOOL="$LIBTOOL --preserve-dup-deps"

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

# Build optimized or debug version ?
# First check for gcc and g++
if test "$GCC" = "yes"
then
  CFLAGS="-ggdb3 -std=gnu99 ${CFLAGS}"
  DEBUG_CFLAGS="-O0"
  OPTMIIZE_CFLAGS="-O3"
fi
if test "$SUNCC" = "yes"
then
  isainfo_k=`isainfo -k`
  if test "$target_cpu" = "sparc"
  then
    MEMALIGN_FLAGS="-xmemalign=8s"
    IS_64="-m64"
    LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
  else
    if test "$isainfo_k" = "amd64"
    then
      IS_64="-m64"
      LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
    fi
  fi
  CPPFLAGS="${CPPFLAGS} -I/usr/local/include"

  CFLAGS="-g -mt -xc99=all ${IS_64} ${MEMALIGN_FLAGS} ${CFLAGS}"
  CXXFLAGS="-g -xlang=c99 ${IS_64} ${CXXFLAGS}"
  DEBUG_CFLAGS="-xO0"
  OPTIMIZE_CFLAGS="-xO4 -xlibmil -xdepend -Xa -mt -xstrconst"
fi


AC_HEADER_TIME
AC_CHECK_HEADERS(assert.h errno.h fcntl.h netinet/tcp.h signal.h stdarg.h)
AC_CHECK_HEADERS(stdio.h stdlib.h string.h sys/socket.h sys/types.h)
AC_CHECK_HEADERS(sys/utsname.h unistd.h)

#--------------------------------------------------------------------
# Check for libevent
#--------------------------------------------------------------------

AC_LIB_HAVE_LINKFLAGS(event,,
[
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <event.h>
],[
  struct bufferevent bev;
  bufferevent_settimeout(&bev, 1, 1);
  event_init();
  event_loop(EVLOOP_ONCE);
])
AS_IF([test x$ac_cv_libevent = xno],
      AC_MSG_ERROR([A recent libevent is required for Gearman. Check http://www.monkey.org/~provos/libevent ]))

save_LIBS="${LIBS}"
LIBS="${LIBS} ${LTLIBEVENT}"
AC_CHECK_FUNCS(event_base_new)
AC_CHECK_FUNCS(event_base_get_method)
LIBS="$save_LIBS"

#--------------------------------------------------------------------
# Check for libuuid
#--------------------------------------------------------------------

AC_CHECK_HEADERS(uuid/uuid.h)
if test "x$ac_cv_header_uuid_uuid_h" = "xno"
then
  AC_MSG_ERROR([Couldn't find uuid/uuid.h. Try installing libuuid development packages])
fi
AC_LIB_HAVE_LINKFLAGS(uuid,,
[
#include <uuid/uuid.h>
],
[
  uuid_t uout;
  uuid_generate(uout);
])

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

dnl This is likely subverted by vpath builds. How do we find the original
dnl source dir in the configure step of a vpath build?
if test -d ".bzr"
then
  building_from_bzr=yes
else
  building_from_bzr=no
fi

AC_ARG_WITH([debug],
    [AS_HELP_STRING([--with-debug],
       [Add debug code/turns off optimizations (yes|no) @<:@default=no@:>@])],
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "yes"
then
  # Debugging. No optimization.
  CFLAGS="${DEBUG_CFLAGS} -DDEBUG ${CFLAGS}"
else
  # Optimized version. No debug
  CFLAGS="${OPTIMIZE_CFLAGS} ${CFLAGS}"
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
  [ac_warn_fail="$building_from_bzr"])

AC_ARG_ENABLE([pedantic-warnings],
  [AS_HELP_STRING([--disable-pedantic-warnings],
    [Toggle pedanticness @<:@default=yes@:>@])],
  [ac_warn_pedantic="$enableval"],
  [ac_warn_pedantic="$building_from_bzr"])

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
    W_PEDANTIC="-pedantic -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wswitch-default -Wswitch-enum -Wcast-align"
    save_CFLAGS="$CFLAGS"
    CFLAGS="-Wlogical-op"
    AC_TRY_COMPILE([#include <stdio>],,W_PEDANTIC="${W_PEDANTIC} -Wlogical-op")
    CFLAGS="$save_CFLAGS"
  fi

  if test "$ac_warn_unreachable" = "yes"
  then
    W_UNREACHABLE="-Wunreachable-code"
  fi

  CC_WARNINGS="${BASE_WARNINGS} ${W_PEDANTIC} ${W_UNREACHABLE} ${GPROF_PROFILING} ${GPROF_COVERAGE} "

  NO_REDUNDANT_DECLS="-Wno-redundant-decls"
  CPPFLAGS="-fpch-deps ${CPPFLAGS}"
fi
if test "$SUNCC" = "yes"
then

  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-errwarn=%all"
  fi

  AC_CACHE_CHECK([whether E_PASTE_RESULT_NOT_TOKEN is usable],
    [ac_cv_paste_result],
    [
      save_CFLAGS="${CFLAGS}"
      CFLAGS="-errwarn=%all -erroff=E_PASTE_RESULT_NOT_TOKEN ${CFLAGS}" 
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([
          AC_INCLUDES_DEFAULT
        ],[
          int x= 0;])],
        [ac_cv_paste_result=yes],
        [ac_cv_paste_result=no])
      CFLAGS="${save_CFLAGS}"
    ])
  AS_IF([test $ac_cv_paste_result = yes],
    [W_PASTE_RESULT=",E_PASTE_RESULT_NOT_TOKEN"])

      
  CC_WARNINGS="-v -errtags=yes ${W_FAIL} -erroff=E_INTEGER_OVERFLOW_DETECTED${W_PASTE_RESULT}"

fi


AM_CFLAGS="${CC_WARNINGS} ${AM_CFLAGS}"

AC_SUBST(NO_REDUNDANT_DECLS)

AM_CPPFLAGS="-I\$(top_srcdir) -I\$(top_builddir) ${CPPFLAGS}"
AM_CFLAGS="${CC_WARNINGS} ${CFLAGS}"

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])

dnl We've collected the flags in AM_*FLAGS now, so blank these.
CFLAGS=""
CPPFLAGS=""

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AC_CHECK_PROGS([PERL], [perl])


AC_CONFIG_FILES(Makefile libgearman/Makefile gearmand/Makefile bin/Makefile
                tests/Makefile examples/Makefile scripts/Makefile
                support/Makefile benchmark/Makefile scripts/gearmand-init
                support/gearmand.pc support/gearmand.spec)

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

