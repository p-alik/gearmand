dnl  Copyright (C) 2011 Brian Aker (brian@tangent.org)

AC_DEFUN([_SEARCH_LIBPQ],[
  AC_REQUIRE([AX_LIB_POSTGRESQL])

  AS_IF([test "x$found_postgresql" = "xyes"],[ ],
    [
    AC_DEFINE([HAVE_POSTGRESQL], [0], [If libpq is available])
    ac_cv_libpq="no"
    ])

  AM_CONDITIONAL(HAVE_LIBPQ, [test "x$ac_cv_lib_pq_main" = "xyes"])
  ])

AC_DEFUN([AX_HAVE_LIBPQ],[

  AC_ARG_ENABLE([libpq],
    [AS_HELP_STRING([--disable-libpq],
      [Build with libpq, ie Postgres, support @<:@default=on@:>@])],
    [ac_cv_libpq="$enableval"],
    [ac_cv_libpq="yes"])

  _SEARCH_LIBPQ
])
