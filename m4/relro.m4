# Set -z,relro.
# Note this is only for shared objects.
ac_ld_relro=no
if test x"$with_gnu_ld" = x"yes"; then
  AC_MSG_CHECKING([for ld that supports -Wl,-z,relro])
  cxx_z_relo=`$LD -v --help 2>/dev/null | grep "z relro"`
  if test -n "$cxx_z_relo"; then
    LDFLAGS="$LDFLAGS -z relro -z now"
    ac_ld_relro=yes
  fi
  AC_MSG_RESULT($ac_ld_relro)
fi
