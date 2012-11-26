#  Copyright (C) 2011 Brian Aker (brian@tangent.org)

AC_DEFUN([_SEARCH_LIBHIREDIS],
    [AC_REQUIRE([AX_CHECK_LIBRARY])
    AS_IF([test "x$ac_enable_hires" = xyes],
      [AX_CHECK_LIBRARY([HIREDIS],[hiredis/hiredis.h],[hiredis])],
      [AC_DEFINE([HAVE_HIREDIS],[0],[If Hiredis is available])])])

  AC_DEFUN([AX_HAVE_LIBHIREDIS],
      [AC_ARG_ENABLE([hires],
        [AS_HELP_STRING([--disable-hires],
          [Build with hires support @<:@default=on@:>@])],
        [ac_enable_hires="$enableval"],
        [ac_enable_hires="yes"])

      AS_IF([test "x$ac_enable_hires" = xyes],[_SEARCH_LIBHIREDIS])])
