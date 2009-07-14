dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_HAVE_LIBDRIZZLE],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libdrizzle
  dnl --------------------------------------------------------------------
  
  AC_LIB_HAVE_LINKFLAGS(drizzle,,[
    #include <libdrizzle/drizzle_client.h>
  ],[
    drizzle_st drizzle;
    drizzle_version();
  ])
  
  AM_CONDITIONAL(HAVE_LIBDRIZZLE, [test "x${ac_cv_libdrizzle}" = "xyes"])

])

