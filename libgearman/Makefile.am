INCLUDES = -I$(top_builddir)
LIBS =

pkginclude_HEADERS= constants.h dispatch.h gearman.h io.h response.h result.h server.h byte_array.h types.h watchpoint.h straction.h echo.h job.h worker.h

lib_LTLIBRARIES = libgearman.la

libgearman_la_SOURCES = \
			echo.c \
			gearman.c \
			job.c \
			behavior.c \
			dispatch.c \
			find_server.c \
			connect.c \
			io.c \
			response.c \
			result.c \
			server.c \
			byte_array.c \
			str_action.c \
			str_error.c \
			quit.c \
			worker.c

libgearman_la_LIBADD =
libgearman_la_LDFLAGS = -version-info $(GEARMAN_LIBRARY_VERSION)

