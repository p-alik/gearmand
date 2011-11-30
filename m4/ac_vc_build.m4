dnl Test to see if we are building from a repository, if so enable -Werror

AC_DEFUN([AC_TEST_FOR_REPOSITORY],[
  AS_IF([test -d ".bzr"],[
    CFLAGS="$CFLAGS -Werror"
    CXXFLAGS="$CXXFLAGS -Werror"
    ], [
    AS_IF([test -d ".hg"],[
      CFLAGS="$CFLAGS -Werror"
      CXXFLAGS="$CXXFLAGS -Werror"
    ])
  ])

])
