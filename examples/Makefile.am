INCLUDES = -I$(top_builddir)
LDADDS = ../libgearman/libgearman.la

LIBS = 

noinst_PROGRAMS = reverse_worker reverse_client
noinst_HEADERS = 

reverse_worker_SOURCES = reverse_worker.c
reverse_worker_LDADD = $(LDADDS)

reverse_client_SOURCES = reverse_client.c
reverse_client_LDADD = $(LDADDS)
