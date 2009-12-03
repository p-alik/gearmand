dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBTOKYOCABINET],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libtokyocabinet
  dnl --------------------------------------------------------------------

  AC_LIB_HAVE_LINKFLAGS(tokyocabinet,,[
    #include <tcutil.h>
    #include <tcbdb.h>    
  ], [
    TCMAP *map;

    map = tcmapnew();
    tcmapdel(map);
  ])
  
  AM_CONDITIONAL(HAVE_LIBTOKYOCABINET, [test "x${ac_cv_libtokyocabinet}" = "xyes"])
  
])

AC_DEFUN([PANDORA_HAVE_LIBTOKYOCABINET],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBTOKYOCABINET])
])

AC_DEFUN([PANDORA_REQUIRE_LIBTOKYOCABINET],[
  AC_REQUIRE([PANDORA_HAVE_LIBTOKYOCABINET])
  AS_IF([test x$ac_cv_libtokyocabinet = xno],
      AC_MSG_ERROR([libtokyocabbinet is required for ${PACKAGE}]))
])
