#define GEARMAN_INTERNAL
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <event.h>

#include <libgearman/gearman.h>
#include "connection.h"

