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
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

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

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_STDC
AM_SANITY_CHECK

AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --preserve-dup-deps"
AC_SUBST(LIBTOOL)

AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_HEADER_TIME
AC_CHECK_HEADERS(assert.h errno.h fcntl.h netinet/tcp.h stdarg.h stdio.h)
AC_CHECK_HEADERS(stdlib.h string.h unistd.h)

AC_CHECK_LIB(event, event_init)
AC_CHECK_HEADERS(event.h)

AC_CHECK_LIB(uuid, uuid_generate)
AC_CHECK_HEADERS(uuid/uuid.h)

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [Build with debug enabled @<:@default=no@:>@])],
  [ac_enable_debug="$enableval"],
  [ac_enable_debug="no"])

AC_ARG_ENABLE([fail],
  [AS_HELP_STRING([--disable-fail],
    [Turn warnings into failures @<:@default=yes@:>@])],
  [ac_warn_fail="$enableval"],
  [ac_warn_fail="yes"])

AC_ARG_ENABLE([strict-aliasing],
  [AS_HELP_STRING([--enable-strict-aliasing],
    [Enable warnings about stict-aliasing @<:@default=no@:>@])],
  [ac_warn_strict_aliasing="$enableval"],
  [ac_warn_strict_aliasing="no"])

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
  if test "$ac_enable_debug" = "yes"
  then
    AC_DEFINE([HAVE_DEBUG], [1], [Enables debug support])
    AC_SUBST(HAVE_DEBUG)
    DEBUG_CFLAGS="-ggdb -DHAVE_DEBUG"
  else
    DEBUG_CFLAGS="-O3"
  fi

  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-Werror"
  fi

  if test "$ac_warn_strict_aliasing" = "yes"
  then
    W_STRICT_ALIASING="-Wstrict-aliasing"
  else
    W_STRICT_ALIASING="-Wno-strict-aliasing"
  fi

  if test "$ac_warn_pedantic" = "yes"
  then
    W_PEDANTIC="-pedantic -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls ${W_STRICT_ALIASING}"
  fi

  if test "$ac_warn_unreachable" = "yes"
  then
    W_UNREACHABLE="-Wunreachable-code"
  fi

  CFLAGS="-std=gnu99 -W -Wall -Wextra ${DEBUG_CFLAGS} ${W_FAIL} ${W_PEDANTIC} ${W_UNREACHABLE} ${CFLAGS}"
fi

AC_CONFIG_FILES(Makefile libgearman/Makefile server/Makefile
                tests/Makefile examples/Makefile)

AC_OUTPUT
