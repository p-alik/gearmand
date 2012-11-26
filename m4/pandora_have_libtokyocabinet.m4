#  Copyright (C) 2009 Sun Microsystems, Inc.
#  This file is free software; Sun Microsystems, Inc.
#  gives unlimited permission to copy and/or distribute it,
#  with or without modifications, as long as this notice is preserved.
 
#  Provides support for finding libtokyocabinet.

AC_DEFUN([_PANDORA_SEARCH_LIBTOKYOCABINET],
    [AC_REQUIRE([AX_CHECK_LIBRARY])

    AS_IF([test "x$ac_enable_libtokyocabinet" = "xyes"],
      [AX_CHECK_LIBRARY([TOKYOCABINET], [tcadb.h], [tokyocabinet],,[ac_enable_libtokyocabinet="no"])],
      [AC_DEFINE([HAVE_TOKYOCABINET],[0],[If TokyoCabinet is available])])

    AS_IF([test "x$ax_cv_have_TOKYOCABINET" = xno],[ac_enable_libtokyocabinet="no"])
    ])

AC_DEFUN([PANDORA_HAVE_LIBTOKYOCABINET],
    [AC_ARG_ENABLE([libtokyocabinet],
      [AS_HELP_STRING([--disable-libtokyocabinet],
        [Build with libtokyocabinet support @<:@default=on@:>@])],
      [ac_enable_libtokyocabinet="$enableval"],
      [ac_enable_libtokyocabinet="yes"])

    _PANDORA_SEARCH_LIBTOKYOCABINET
    ])
