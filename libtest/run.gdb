set logging on
set logging overwrite on
set environment LIBTEST_IN_GDB=1
break malloc_error_break
run
thread apply all bt
