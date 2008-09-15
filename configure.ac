AC_INIT(libgearman/io.c)
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(libgearman/libgearman_config.h)
AC_CANONICAL_SYSTEM

GEARMAN_LIBRARY_NAME=libgearman

#release versioning
GEARMAN_MAJOR_VERSION=0
GEARMAN_MINOR_VERSION=1
GEARMAN_MICRO_VERSION=0

#API version
GEARMAN_API_VERSION=1.0
AC_SUBST(GEARMAN_API_VERSION)

#shared library versioning
GEARMAN_LIBRARY_VERSION=1:0:0
#                         | | |
#                  +------+ | +---+
#                  |        |     |
#                 current:revision:age
#                  |        |     |
#                  |        |     +- increment if interfaces have been added
#                  |        |        set to zero if interfaces have been removed or changed
#                  |        +- increment if source code has changed
#                  |           set to zero if current is incremented
#                  +- increment if interfaces have been added, removed or changed

AC_SUBST(GEARMAN_LIBRARY_VERSION)

PACKAGE=$GEARMAN_LIBRARY_NAME
AC_SUBST(GEARMAN_LIBRARY_NAME)

GEARMAN_VERSION=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION.$GEARMAN_MICRO_VERSION
GEARMAN_RELEASE=$GEARMAN_MAJOR_VERSION.$GEARMAN_MINOR_VERSION
AC_SUBST(GEARMAN_RELEASE)
AC_SUBST(GEARMAN_VERSION)

VERSION=$GEARMAN_RELEASE

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

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

# We only support GCC and Sun's forte at the moment
if test "$GCC" = "yes"
then
  if test "$ENABLE_DEBUG" = "yes"
  then
    CFLAGS="-Wall -ggdb -DHAVE_DEBUG"
  else
    CFLAGS="-Wall -O3"
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
