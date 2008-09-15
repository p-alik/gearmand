/*
 * Summary: Call quit on a server
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_QUIT_H__
#define __GEARMAN_QUIT_H__

#ifdef __cplusplus
extern "C" {
#endif

void gearman_quit(gearman_st *ptr);
void gearman_quit_server(gearman_server_st *ptr, uint8_t io_death);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_QUIT_H__ */
