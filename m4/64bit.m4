dnl ---------------------------------------------------------------------------
dnl Macro: 64BIT
dnl ---------------------------------------------------------------------------
AC_CHECK_PROGS(ISAINFO, [isainfo], [no])
if test "x$ISAINFO" != "xno"
then
   isainfo_b=`isainfo -b` 
   spro_common_flags="-mt"
   if test "x$isainfo_b" = "x64"
   then
      AC_ARG_ENABLE([64bit],
         [AS_HELP_STRING([--disable-64bit],
            [Build 64 bit binary @<:@default=on@:>@])],
         [ac_enable_64bit="$enableval"],
         [ac_enable_64bit="yes"])

      if test "x$ac_enable_64bit" = "xyes"
      then
         CFLAGS="-m64 $CFLAGS"
         CXXFLAGS="-m64 $CXXFLAGS"
         if test "$target_cpu" = "sparc" -a "x$SUNCC" = "xyes"
         then
            CFLAGS="-xmemalign=8s $CFLAGS"
            CXXFLAGS="-xmemalign=8s $CXXFLAGS"
         fi
      fi
   fi
fi
dnl ---------------------------------------------------------------------------
dnl End Macro: 64BIT
dnl ---------------------------------------------------------------------------
