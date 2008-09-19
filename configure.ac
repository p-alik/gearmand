AC_INIT(gearmand, [1.0.0], [http://launchpad.net/gearman/])
AC_CONFIG_SRCDIR(libgearman/io.c)
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(libgearman/libgearman_config.h)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

GEARMAN_LIBRARY_NAME=libgearman
#shared library versioning
GEARMAN_LIBRARY_VERSION=1:0:0
#                         | | |
#                  +------+ | +---+
#                  |        |     |
#                 current:revision:age
#                  |        |     |
#                  |        |     +- increment if interfaces have been added
#                  |        |        set to zero if interfaces have been
#                  |        |        removed or changed
#                  |        +- increment if source code has changed
#                  |           set to zero if current is incremented
#                  +- increment if interfaces have been added, removed or
#                     changed

#release versioning
GEARMAN_MAJOR_VERSION=0
GEARMAN_MINOR_VERSION=1
GEARMAN_MICRO_VERSION=0
GEARMAN_VERSION=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION.$GEARMAN_MICRO_VERSION
GEARMAN_RELEASE=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION

#API version
GEARMAN_API_VERSION=1.0

PACKAGE=$GEARMAN_LIBRARY_NAME
VERSION=$GEARMAN_RELEASE

AC_SUBST(GEARMAN_LIBRARY_NAME)
AC_SUBST(GEARMAN_LIBRARY_VERSION)
AC_SUBST(GEARMAN_RELEASE)
AC_SUBST(GEARMAN_VERSION)
AC_SUBST(GEARMAN_API_VERSION)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AM_SANITY_CHECK
LIBTOOL="$LIBTOOL --preserve-dup-deps"
AC_SUBST(LIBTOOL)dnl
AC_LANG_CPLUSPLUS
AC_C_BIGENDIAN

sinclude(config/debug.m4)
sinclude(config/dtrace.m4)

AC_ARG_ENABLE([fail],
    [AS_HELP_STRING([--enable-fail],
       [Turn warnings into failures @<:@default=no@:>@])],
    [ac_warn_fail="$enableval"],
    [ac_warn_fail="no"])

# We only support GCC and Sun's forte at the moment
if test "$GCC" = "yes"
then
  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-Werror"
  fi

  if test "$ENABLE_DEBUG" = "yes"
  then
    CFLAGS="-W -Wall -Wextra -ggdb -DHAVE_DEBUG $W_FAIL"
  else
    CFLAGS="-W -Wall -Wextra -O3 $W_FAIL"
  fi
else
  CFLAGS="-Xa -xstrconst -mt -D_FORTEC_ -fast -m64"
  LDFLAGS="-lsocket -lnsl"
  DTRACEFLAGS="-64"
fi

AC_C_CONST
AC_HEADER_TIME
AC_TYPE_SIZE_T
AC_SEARCH_LIBS(getopt_long, gnugetopt)
AC_OUTPUT(Makefile libgearman/Makefile tests/Makefile server/Makefile examples/Makefile)
