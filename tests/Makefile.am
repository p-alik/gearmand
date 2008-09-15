INCLUDES = -I$(top_builddir)
LDADDS = ../libgearman/libgearman.la

LIBS = 

noinst_PROGRAMS = client_test testworker
noinst_HEADERS = test.h

client_test_SOURCES = test.c function.c
client_test_LDADD = $(LDADDS)

testworker_SOURCES = test.c worker_drone.c
testworker_LDADD = $(LDADDS)

record:
	./client_test output.txt > output.res
test: client_test
	./client_test output.txt > output.cmp
	diff output.res output.cmp

valgrind:
	libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes  client_test output.txt
