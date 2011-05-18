/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman Declarations
 */

#pragma once

/**
  @todo this is only used by the server and should be made private.
 */
typedef struct gearman_connection_st gearman_connection_st;
typedef gearman_return_t (gearman_event_watch_fn)(gearman_connection_st *con,
                                                  short events, void *context);

/**
 * @ingroup gearman_universal
 */
struct gearman_universal_st
{
  struct {
    bool dont_track_packets;
    bool non_blocking;
    bool stored_non_blocking;
  } options;
  gearman_verbose_t verbose;
  uint32_t con_count;
  uint32_t packet_count;
  uint32_t pfds_size;
  uint32_t sending;
  int timeout; // Used by poll()
  gearman_connection_st *con_list;
  gearman_packet_st *packet_list;
  struct pollfd *pfds;
  gearman_log_fn *log_fn;
  void *log_context;
  gearman_malloc_fn *workload_malloc_fn;
  void *workload_malloc_context;
  gearman_free_fn *workload_free_fn;
  void *workload_free_context;
  struct {
    gearman_return_t rc;
    int last_errno;
    char last_error[GEARMAN_MAX_ERROR_SIZE];
  } error;
};

#ifdef GEARMAN_CORE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the error string.
 *
 * @param[in] gearman Structure previously initialized with gearman_universal_create() or
 *  gearman_clone().
 * @param[in] function Name of function the error happened in.
 * @param[in] format Format and variable argument list of message.
 */
GEARMAN_INTERNAL_API
void gearman_universal_set_error(gearman_universal_st *gearman,
				 gearman_return_t rc,
				 const char *function,
                                 const char *format, ...);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

#define gearman_perror(A, B) do { gearman_universal_set_perror(AT, (A), (B)); } while (0)
#define gearman_error(A, B, C) do { gearman_universal_set_error((A), (B), AT, (C)); } while (0)
#define gearman_gerror(A, B) do { gearman_universal_set_error((A), (B), AT, " "); } while (0)

/**
 * Set custom memory allocation function for workloads. Normally gearman uses
 * the standard system malloc to allocate memory used with workloads. The
 * provided function will be used instead.
 *
 * @param[in] gearman Structure previously initialized with gearman_universal_create() or
 *  gearman_clone().
 * @param[in] function Memory allocation function to use instead of malloc().
 * @param[in] context Argument to pass into the callback function.
 */
GEARMAN_INTERNAL_API
void gearman_set_workload_malloc_fn(gearman_universal_st *gearman,
                                    gearman_malloc_fn *function,
                                    void *context);

/**
 * Set custom memory free function for workloads. Normally gearman uses the
 * standard system free to free memory used with workloads. The provided
 * function will be used instead.
 *
 * @param[in] gearman Structure previously initialized with gearman_universal_create() or
 *  gearman_clone().
 * @param[in] function Memory free function to use instead of free().
 * @param[in] context Argument to pass into the callback function.
 */
GEARMAN_INTERNAL_API
void gearman_set_workload_free_fn(gearman_universal_st *gearman,
                                  gearman_free_fn *function,
                                  void *context);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* GEARMAN_CORE */
