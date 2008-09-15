/* Server IO, Not public! */
#include "common.h"

ssize_t gearman_io_write(gearman_server_st *ptr,
                        char *buffer, size_t length, char with_flush);
void gearman_io_reset(gearman_server_st *ptr);
ssize_t gearman_io_read(gearman_server_st *ptr,
                          char *buffer, size_t length);
gearman_return gearman_io_close(gearman_server_st *ptr);
