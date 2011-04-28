AC_DEFUN([_WITH_LIBDRIZZLE],
  [ AC_ARG_ENABLE([libdrizzle],
    [AS_HELP_STRING([--disable-libdrizzle],
      [Build with libdrizzle support @<:@default=on@:>@])],
    [ac_enable_libdrizzle="$enableval"],
    [ac_enable_libdrizzle="yes"])

    AS_IF([test "x$ac_enable_libdrizzle" = "xyes"],
         [ PKG_CHECK_MODULES([libdrizzle], 
			     [ libdrizzle=1.0 >= 0.1 ], 
			     [AC_DEFINE([HAVE_LIBDRIZZLE], [ 1 ], [Enable libdrizzle support])], 
			     [ac_enable_libdrizzle="no"] )])
    AM_CONDITIONAL(HAVE_LIBDRIZZLE, test "x${ac_enable_libdrizzle}" = "xyes")
    ])
])

AC_DEFUN([WITH_LIBDRIZZLE], [ AC_REQUIRE([_WITH_LIBDRIZZLE]) ])
