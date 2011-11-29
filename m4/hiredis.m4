dnl  Copyright (C) 2011 Brian Aker (brian@tangent.org)
dnl This file is free software; Sun Microsystems, Inc.
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl

AC_DEFUN([_SEARCH_LIBHIREDIS],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl Do this by hand. Need to check for hiredis/hiredis.h, but hires may or may
  dnl not be a lib is weird.
  AC_CHECK_HEADERS(hiredis/hiredis.h)
  AC_LIB_HAVE_LINKFLAGS(hiredis,,
  [
    #include <hiredis/hiredis.h>
  ],
  [
    redisConnect("127.0.0.1", 5555);
  ])

  AM_CONDITIONAL(HAVE_HIREDIS, [test "x${ac_cv_libhiredis}" = "xyes"])
  AS_IF([test "x${ac_cv_libhiredis}" = "xyes"],
        [
          AC_DEFINE([HAVE_HIREDIS], [1], [If Hiredis available])
         ],
         [
          AC_DEFINE([HAVE_HIREDIS], [0], [If Hiredis is available])
        ])
])

AC_DEFUN([_AX_HAVE_LIBHIREDIS],[

  AC_ARG_ENABLE([hires],
    [AS_HELP_STRING([--disable-hires],
      [Build with hires support @<:@default=on@:>@])],
    [ac_enable_hires="$enableval"],
    [ac_enable_hires="yes"])

  _SEARCH_LIBHIREDIS
])

AC_DEFUN([AX_HAVE_LIBHIREDIS],[
  AC_REQUIRE([_AX_HAVE_LIBHIREDIS])
])
