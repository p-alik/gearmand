if test x$PERL = x; then
  if test \! "x`which perl 2> /dev/null | grep -v '^no'`" = x; then
    PERL=perl
  else
    echo "perl wasn't found, exiting"; exit 1
  fi
fi
echo "Generating docs..."
cat libgearman/*.h libgearman-server/*.h | $PERL docs/man_gen.perl > docs/man_list

