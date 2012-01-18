dnl  Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems, Inc.
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
 
dnl Provides support for finding libtokyocabinet.
dnl LIBTOKYOCABINET_CFLAGS will be set, in addition to LIBTOKYOCABINET and LTLIBTOKYOCABINET

AC_DEFUN([_PANDORA_SEARCH_LIBTOKYOCABINET],[
  AC_REQUIRE([AX_CHECK_LIBRARY])

  AS_IF([test "x$ac_enable_libtokyocabinet" = "xyes"],[
    AX_CHECK_LIBRARY([LIBTOKYOCABINET], [tcadb.h], [tokyocabinet],
      [
      LIBTOKYOCABINET_LDFLAGS="-ltokyocabinet"
      AC_DEFINE([HAVE_LIBTOKYOCABINET], [1], [If TokyoCabinet is available])
      ],
      [
      AC_DEFINE([HAVE_LIBTOKYOCABINET], [0], [If TokyoCabinet is available])
      ac_enable_libtokyocabinet="no"
      ])

    ],
    [
    AC_DEFINE([HAVE_LIBTOKYOCABINET], [0], [If TokyoCabinet is available])
    ])

  AM_CONDITIONAL(HAVE_LIBTOKYOCABINET, [test "x$ac_cv_lib_tokyocabinet_main" = "xyes"])
  ])

AC_DEFUN([PANDORA_HAVE_LIBTOKYOCABINET],[

  AC_ARG_ENABLE([libtokyocabinet],
    [AS_HELP_STRING([--disable-libtokyocabinet],
      [Build with libtokyocabinet support @<:@default=on@:>@])],
    [ac_enable_libtokyocabinet="$enableval"],
    [ac_enable_libtokyocabinet="yes"])

  _PANDORA_SEARCH_LIBTOKYOCABINET
])
