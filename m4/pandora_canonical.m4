dnl  Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems, Inc.
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Which version of the canonical setup we're using
AC_DEFUN([PANDORA_CANONICAL_VERSION],[0.177])

AC_DEFUN([PANDORA_MSG_ERROR],[
  AS_IF([test "x${pandora_cv_skip_requires}" != "xno"],[
    AC_MSG_ERROR($1)
  ],[
    AC_MSG_WARN($1)
  ])
])

AC_DEFUN([PANDORA_FORCE_DEPEND_TRACKING],[
  AC_ARG_ENABLE([fat-binaries],
    [AS_HELP_STRING([--enable-fat-binaries],
      [Enable fat binary support on OSX @<:@default=off@:>@])],
    [ac_enable_fat_binaries="$enableval"],
    [ac_enable_fat_binaries="no"])

  dnl Force dependency tracking on for Sun Studio builds
  AS_IF([test "x${enable_dependency_tracking}" = "x"],[
    enable_dependency_tracking=yes
  ])
  dnl If we're building OSX Fat Binaries, we have to turn off -M options
  AS_IF([test "x${ac_enable_fat_binaries}" = "xyes"],[
    enable_dependency_tracking=no
  ])
])

AC_DEFUN([PANDORA_BLOCK_BAD_OPTIONS],[
  AS_IF([test "x${prefix}" = "x"],[
    PANDORA_MSG_ERROR([--prefix requires an argument])
  ])
])

dnl The standard setup for how we build Pandora projects
AC_DEFUN([PANDORA_CANONICAL_TARGET],[
  AC_REQUIRE([PANDORA_FORCE_DEPEND_TRACKING])
  ifdef([m4_define],,[define([m4_define],   defn([define]))])
  ifdef([m4_undefine],,[define([m4_undefine],   defn([undefine]))])
  m4_define([PCT_ALL_ARGS],[$*])
  m4_define([PCT_DONT_SUPPRESS_INCLUDE],[no])
  m4_define([PCT_VERSION_FROM_VC],[no])
  m4_foreach([pct_arg],[$*],[
    m4_case(pct_arg,
      [dont-suppress-include], [
        m4_undefine([PCT_DONT_SUPPRESS_INCLUDE])
        m4_define([PCT_DONT_SUPPRESS_INCLUDE],[yes])
      ],
      [version-from-vc], [
        m4_undefine([PCT_VERSION_FROM_VC])
        m4_define([PCT_VERSION_FROM_VC],[yes])
    ])
  ])

  PANDORA_BLOCK_BAD_OPTIONS

  # We need to prevent canonical target
  # from injecting -O2 into CFLAGS - but we won't modify anything if we have
  # set CFLAGS on the command line, since that should take ultimate precedence
  AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],
        [CFLAGS=""])
  AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],
        [CXXFLAGS=""])
  
  AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability nostdinc subdir-objects foreign tar-ustar])

  m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_EARLY
  ],[
    PANDORA_EXTENSIONS 
  ])
  
  AC_REQUIRE([AC_PROG_CC])

  m4_if(PCT_VERSION_FROM_VC,yes,[
    PANDORA_VC_INFO_HEADER
  ],[
    PANDORA_TEST_VC_DIR

    changequote(<<, >>)dnl
    PANDORA_RELEASE_ID=`echo $VERSION | sed 's/[^0-9]//g'`
    changequote([, ])dnl

    PANDORA_RELEASE_COMMENT=""
    AC_DEFINE_UNQUOTED([PANDORA_RELEASE_VERSION],["$VERSION"],
                       [Version of the software])

    AC_SUBST(PANDORA_RELEASE_COMMENT)
    AC_SUBST(PANDORA_RELEASE_VERSION)
    AC_SUBST(PANDORA_RELEASE_ID)
  ])
  PANDORA_VERSION

  dnl Once we can use a modern autoconf, we can use this
  dnl AC_PROG_CC_C99
  AC_REQUIRE([AC_PROG_CXX])
  PANDORA_EXTENSIONS
  AM_PROG_CC_C_O



  PANDORA_PLATFORM

  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_INIT
    AC_CONFIG_LIBOBJ_DIR([gnulib])
  ])

  PANDORA_CHECK_C_VERSION
  PANDORA_CHECK_CXX_VERSION

  AC_C_BIGENDIAN
  AC_C_CONST
  AC_C_INLINE
  AC_C_VOLATILE
  AC_C_RESTRICT

  AC_HEADER_TIME
  AC_STRUCT_TM
  AC_TYPE_SIZE_T
  AC_SYS_LARGEFILE
  PANDORA_CLOCK_GETTIME

  AC_CHECK_HEADERS(sys/socket.h)

  # off_t is not a builtin type
  AC_CHECK_SIZEOF(off_t, 4)
  AS_IF([test "$ac_cv_sizeof_off_t" -eq 0],[
    PANDORA_MSG_ERROR("${PACKAGE} needs an off_t type.")
  ])

  AC_CHECK_SIZEOF(size_t)
  AS_IF([test "$ac_cv_sizeof_size_t" -eq 0],[
    PANDORA_MSG_ERROR("${PACKAGE} needs an size_t type.")
  ])

  AC_DEFINE_UNQUOTED([SIZEOF_SIZE_T],[$ac_cv_sizeof_size_t],[Size of size_t as computed by sizeof()])
  AC_CHECK_SIZEOF(long long)
  AC_DEFINE_UNQUOTED([SIZEOF_LONG_LONG],[$ac_cv_sizeof_long_long],[Size of long long as computed by sizeof()])
  AC_CACHE_CHECK([if time_t is unsigned], [ac_cv_time_t_unsigned],[

  AC_LANG_PUSH([C])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[
#include <time.h>
      ]],
      [[
      int array[(((time_t)-1) > 0) ? 1 : -1];
      ]])
    ],[
      ac_cv_time_t_unsigned=yes
    ],[
      ac_cv_time_t_unsigned=no
    ])
  ])
  AS_IF([test "$ac_cv_time_t_unsigned" = "yes"],[
    AC_DEFINE([TIME_T_UNSIGNED], 1, [Define to 1 if time_t is unsigned])
  ])
  AC_LANG_POP


  PANDORA_OPTIMIZE

  PANDORA_HAVE_GCC_ATOMICS

  PANDORA_HEADER_ASSERT

  PANDORA_WARNINGS(PCT_ALL_ARGS)

  PANDORA_ENABLE_DTRACE

  AC_LIB_PREFIX
  PANDORA_HAVE_BETTER_MALLOC

  AC_CHECK_PROGS([PERL], [perl])
  AC_CHECK_PROGS([DPKG_GENSYMBOLS], [dpkg-gensymbols], [:])
  AC_CHECK_PROGS([LCOV], [lcov], [echo lcov not found])
  AC_CHECK_PROGS([LCOV_GENHTML], [genhtml], [echo genhtml not found])

  AC_CHECK_PROGS([SPHINXBUILD], [sphinx-build], [:])
  AS_IF([test "x${SPHINXBUILD}" != "x:"],[
    AC_CACHE_CHECK([if sphinx is new enough],[ac_cv_recent_sphinx],[
    
    ${SPHINXBUILD} -Q -C -b man -d conftest.d . . >/dev/null 2>&1
    AS_IF([test $? -eq 0],[ac_cv_recent_sphinx=yes],
          [ac_cv_recent_sphinx=no])
    rm -rf conftest.d
    ])
  ])

  AM_CONDITIONAL(HAVE_DPKG_GENSYMBOLS,[test "x${DPKG_GENSYMBOLS}" != "x:"])
  AM_CONDITIONAL(HAVE_SPHINX,[test "x${SPHINXBUILD}" != "x:"])
  AM_CONDITIONAL(HAVE_RECENT_SPHINX,[test "x${ac_cv_recent_sphinx}" = "xyes"])

  m4_if(m4_substr(m4_esyscmd(test -d src && echo 0),0,1),0,[
    AM_CPPFLAGS="-I\$(top_srcdir)/src -I\$(top_builddir)/src ${AM_CPPFLAGS}"
  ],[
    AM_CPPFLAGS="-I\$(top_srcdir) -I\$(top_builddir) ${AM_CPPFLAGS}"
  ])

  PANDORA_USE_PIPE

  AM_CFLAGS="-std=c99 ${AM_CFLAGS} ${CC_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"
  AM_CXXFLAGS="${AM_CXXFLAGS} ${CXX_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"

  AC_SUBST([AM_CFLAGS])
  AC_SUBST([AM_CXXFLAGS])
  AC_SUBST([AM_CPPFLAGS])
  AC_SUBST([AM_LDFLAGS])

])
