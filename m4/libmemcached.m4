AC_DEFUN([_WITH_LIBMEMCACHED],
  [ AC_ARG_ENABLE([libmemcached],
   [AS_HELP_STRING([--disable-libmemcached],
     [Build with libmemcached support @<:@default=on@:>@])],
     [ac_enable_libmemcached="$enableval"],
     [ac_enable_libmemcached="yes"])

     AS_IF([test "x$ac_enable_libmemcached" = "xyes"],
       [ PKG_CHECK_MODULES([libmemcached], [ libmemcached >= 1.0 ], 
			   [
                            AC_DEFINE([HAVE_LIBMEMCACHED], [ 1 ], [Enable libmemcached support])
                            AC_SUBST(_WITH_LIBMEMCACHED_SUPPORT, ["_WITH_LIBMEMCACHED_SUPPORT 1"])
                            ],
                           [
                             [ac_enable_libmemcached="no"]
                             AC_DEFINE([HAVE_LIBMEMCACHED], [ 0 ], [Enable libmemcached support])
                             AC_SUBST(_WITH_LIBMEMCACHED_SUPPORT, ["_WITH_LIBMEMCACHED_SUPPORT 1"])
                             ])],
                             [
                             AC_DEFINE([HAVE_LIBMEMCACHED], [ 0 ], [Enable libmemcached support])
                             AC_SUBST(_WITH_LIBMEMCACHED_SUPPORT, ["_WITH_LIBMEMCACHED_SUPPORT 1"])
                             ]
                             )

     AM_CONDITIONAL(HAVE_LIBMEMCACHED, test "x${ac_enable_libmemcached}" = "xyes")
     ])

AC_DEFUN([WITH_LIBMEMCACHED], [ AC_REQUIRE([_WITH_LIBMEMCACHED]) ])
