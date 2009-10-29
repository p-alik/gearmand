# Gearman server and library
# Copyright (C) 2008 Brian Aker, Eric Day, Monty Taylor
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT([gearmand],[0.11],[https://launchpad.net/gearmand])
AC_CONFIG_SRCDIR(libgearman/gearman.c)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

PANDORA_CANONICAL_TARGET

GEARMAN_LIBRARY_VERSION=2:1:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                |        |        or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed
AC_SUBST(GEARMAN_LIBRARY_VERSION)

GEARMAN_SERVER_LIBRARY_VERSION=0:1:0
#                              | | |
#                       +------+ | +---+
#                       |        |     |
#                    current:revision:age
#                       |        |     |
#                       |        |     +- increment if interfaces have been
#                       |        |        added, set to zero if interfaces
#                       |        |        have been removed or changed
#                       |        +- increment if source code has changed
#                       |           set to zero if current is incremented
#                       +- increment if interfaces have been added, removed
#                          or changed
AC_SUBST(GEARMAN_SERVER_LIBRARY_VERSION)

# lib versioning when linked with GNU ld.
AS_IF([test "$lt_cv_prog_gnu_ld" = "yes"],[
    LD_VERSION_SCRIPT="-Wl,--version-script=\$(top_srcdir)/libgearman/libgearman.ver"
    LD_VERSION_SCRIPT_SERVER="-Wl,--version-script=\$(top_srcdir)/libgearman-server/libgearman-server.ver"
])
AC_SUBST(LD_VERSION_SCRIPT)
AC_SUBST(LD_VERSION_SCRIPT_SERVER)

PANDORA_REQUIRE_PTHREAD

#--------------------------------------------------------------------
# Check for libevent
#--------------------------------------------------------------------

AC_LIB_HAVE_LINKFLAGS(event,,
[
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <event.h>
],[
  struct bufferevent bev;
  bufferevent_settimeout(&bev, 1, 1);
  event_init();
  event_loop(EVLOOP_ONCE);
])
AS_IF([test x$ac_cv_libevent = xno],
      AC_MSG_ERROR([A recent version of libevent is required to build gearmand. On Debian this can be found in libevent-dev. On RedHat its in libevent-devel. You can also install the latest from source at http://www.monkey.org/~provos/libevent.]))


save_LIBS="${LIBS}"
LIBS="${LIBS} ${LTLIBEVENT}"
AC_CHECK_FUNCS(event_base_new)
AC_CHECK_FUNCS(event_base_free)
AC_CHECK_FUNCS(event_base_get_method)
LIBS="$save_LIBS"

#--------------------------------------------------------------------
# Check for libuuid
#--------------------------------------------------------------------

AC_CHECK_HEADERS(uuid/uuid.h)
if test "x$ac_cv_header_uuid_uuid_h" = "xno"
then
  AC_MSG_ERROR([Couldn't find uuid/uuid.h. On Debian this can be found in uuid-dev. On Redhat this can be found in e2fsprogs-devel.])
fi
AC_LIB_HAVE_LINKFLAGS(uuid,,
[
#include <uuid/uuid.h>
],
[
  uuid_t uout;
  uuid_generate(uout);
])

PANDORA_HAVE_BETTER_MALLOC
PANDORA_HAVE_LIBSQLITE3
PANDORA_HAVE_LIBDRIZZLE
PANDORA_HAVE_LIBMEMCACHED
PANDORA_HAVE_LIBPQ


AC_CHECK_HEADERS(errno.h fcntl.h getopt.h netinet/tcp.h pwd.h signal.h)
AC_CHECK_HEADERS(stdarg.h stddef.h stdio.h stdlib.h string.h)
AC_CHECK_HEADERS(sys/resource.h sys/stat.h)
AC_CHECK_HEADERS(sys/socket.h sys/types.h sys/utsname.h unistd.h strings.h)


AC_CONFIG_FILES(Makefile
                libgearman/Makefile
                libgearman-server/Makefile
                gearmand/Makefile
                bin/Makefile
                tests/Makefile
                examples/Makefile
                scripts/Makefile
                support/Makefile
                benchmark/Makefile
                scripts/gearmand-init
                scripts/gearmand.xml
                scripts/gearmand
                scripts/smf_install.sh
                support/gearmand.pc
                support/gearmand.spec)

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Warnings as failure:       $ac_cv_warnings_as_errors"
echo "   * Building with libsqlite3   $ac_cv_libsqlite3"
echo "   * Building with libdrizzle   $ac_cv_libdrizzle"
echo "   * Building with libmemcached $ac_cv_libmemcached"
echo "   * Building with libpq        $ac_cv_libpq"
echo ""
echo "---"
