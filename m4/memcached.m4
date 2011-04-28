AC_DEFUN([WITH_MEMCACHED],
  [AC_ARG_WITH([memcached],
    [AS_HELP_STRING([--with-memcached],
      [Memcached binary to use for make test])],
    [ac_cv_with_memcached="$withval"],
    [ac_cv_with_memcached=memcached])

  # just ignore the user if --without-memcached is passed.. it is
  # only used by make test
  AS_IF([test "x$withval" = "xno"],
    [
      ac_cv_with_memcached=memcached
      MEMCACHED_BINARY=memcached
    ],
    [
       AS_IF([test -f "$withval"],
         [
           ac_cv_with_memcached=$withval
           MEMCACHED_BINARY=$withval
         ],
         [
           AC_PATH_PROG([MEMCACHED_BINARY], [$ac_cv_with_memcached], "no")
         ])
    ])
  AM_CONDITIONAL(HAVE_MEMCACHED, test "x${ac_cv_with_memcached}" != "xno")
  AC_DEFINE_UNQUOTED([MEMCACHED_BINARY], "$MEMCACHED_BINARY", [Name of the memcached binary used in make test])
  AC_SUBST(MEMCACHED_BINARY)
])
