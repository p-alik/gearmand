#  Copyright (C) 2011 Brian Aker (brian@tangent.org)
#
# serial 2

AC_DEFUN([_SEARCH_HIREDIS],
    [AC_REQUIRE([AX_CHECK_LIBRARY])

    AS_IF([test "x$ac_enable_hiredis" = "xyes"],
      [AX_CHECK_LIBRARY([HIREDIS],[hiredis/hiredis.h],[hiredis])],
      [AC_DEFINE([HAVE_HIREDIS],[0],[If Hiredis is available])])

    AS_IF([test "x$ax_cv_have_HIREDIS" = xno],[ac_enable_hiredis="no"])
    ])

AC_DEFUN([AX_ENABLE_LIBHIREDIS],
    [AC_ARG_ENABLE([hiredis],
      [AS_HELP_STRING([--disable-hiredis],
        [Build with hiredis support @<:@default=on@:>@])],
      [ac_enable_hiredis="$enableval"],
      [ac_enable_hiredis="yes"])

    AS_IF([test "x$ac_enable_hiredis" = xyes],
      [_SEARCH_HIREDIS])
    ])
