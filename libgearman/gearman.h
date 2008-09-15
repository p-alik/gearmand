/*
 * Summary: interface for gearman server
 * Description: main include file for libgearman
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <libgearman/libgearman_config.h>
#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/watchpoint.h>
#include <libgearman/byte_array.h>
#include <libgearman/result.h>
#include <libgearman/job.h>
#include <libgearman/worker.h>
#include <libgearman/server.h>
#include <libgearman/quit.h>
#include <libgearman/behavior.h>
#include <libgearman/dispatch.h>
#include <libgearman/response.h>
#include <libgearman/straction.h>
#include <libgearman/echo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These are Private and should not be used by applications */
#define GEARMAN_H_VERSION_STRING_LENGTH 12

struct gearman_st {
  gearman_allocated is_allocated;
  gearman_server_st *hosts;
  unsigned int number_of_hosts;
  int cached_errno;
  uint32_t flags;
  int send_size;
  int recv_size;
  int32_t poll_timeout;
  int32_t connect_timeout;
  int32_t retry_timeout;
  gearman_result_st result;
  gearman_hash hash;
  gearman_server_distribution distribution;
  void *user_data;
};

/* Public API */
gearman_st *gearman_create(gearman_st *ptr);
void gearman_free(gearman_st *ptr);
gearman_st *gearman_clone(gearman_st *clone, gearman_st *ptr);

char *gearman_strerror(gearman_st *ptr, gearman_return rc);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
