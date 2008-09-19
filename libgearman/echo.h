/*
 * Summary: Call echo on a server
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_ECHO_H__
#define __GEARMAN_ECHO_H__

#ifdef __cplusplus
extern "C" {
#endif

gearman_return gearman_echo(gearman_st *ptr, char *message,
                            size_t message_length, char *function);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_ECHO_H__ */
