AC_DEFUN([WITH_DRIZZLED],
  [AC_ARG_WITH([drizzled],
    [AS_HELP_STRING([--with-drizzled],
      [Drizzled binary to use for make test])],
    [ac_cv_with_drizzled="$withval"],
    [ac_cv_with_drizzled=drizzled])

  # Disable if --without-drizzled
  # only used by make test
  AS_IF([test "x$withval" = "xno"],
    [
      ac_cv_with_drizzled=
      DRIZZLED_BINARY=
    ],
    [
       AS_IF([test -f "$withval"],
         [
           ac_cv_with_drizzled=$withval
           DRIZZLED_BINARY=$withval
           AC_DEFINE_UNQUOTED([DRIZZLED_BINARY], "$DRIZZLED_BINARY", [Name of the drizzled binary used in make test])
         ],
         [
           ac_cv_with_drizzled=
           DRIZZLED_BINARY=
         ])
    ])
  AM_CONDITIONAL(HAVE_DRIZZLED, test -n ${ac_cv_with_drizzled})
  AC_SUBST(DRIZZLED_BINARY)
])
