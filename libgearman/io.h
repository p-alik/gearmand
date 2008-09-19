/*
 * Summary: Server IO, Not a public header!
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_IO_H__
#define __GEARMAN_IO_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t gearman_io_write(gearman_server_st *ptr, char *buffer, size_t length,
                         char with_flush);
void gearman_io_reset(gearman_server_st *ptr);
ssize_t gearman_io_read(gearman_server_st *ptr, char *buffer, size_t length);
gearman_return gearman_io_close(gearman_server_st *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_IO_H__ */
