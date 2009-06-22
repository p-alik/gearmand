/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef __GEARMAN_WATCHPOINT_H__
#define __GEARMAN_WATCHPOINT_H__

#include <libgearman/constants.h>
#ifdef HAVE_GEARMAN_ACTION
#include <libgearman/str_action.h>
#endif

/* Some personal debugging functions */
#ifdef HAVE_DEBUG
#define WATCHPOINT { fprintf(stderr, "\nWATCHPOINT %s:%d (%s)\n", \
                           __FILE__, __LINE__, __func__); \
                     fflush(stdout); }
#define WATCHPOINT_ERROR(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                      __FILE__, __LINE__, __func__, \
                                      gearman_strerror(A)); \
                              fflush(stdout); }
#ifdef HAVE_GEARMAN_ACTION
#define WATCHPOINT_ACTION(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                       __FILE__, __LINE__, __func__, \
                                       gearman_straction(A)); \
                               fflush(stdout); }
#endif
#define WATCHPOINT_IFERROR(A) { if (A != GEARMAN_SUCCESS) { \
                                  fprintf(stderr, \
                                          "\nWATCHPOINT %s:%d (%s) %s\n", \
                                          __FILE__, __LINE__, __func__, \
                                          gearman_strerror(A)); \
                                  fflush(stdout); } }
#define WATCHPOINT_STRING(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                       __FILE__, __LINE__, __func__, A); \
                                       fflush(stdout); }
#define WATCHPOINT_STRING_LENGTH(A,B) { fprintf(stderr, \
                                            "\nWATCHPOINT %s:%d (%s) %.*s\n", \
                                            __FILE__, __LINE__, __func__, \
                                            (int)B, A); \
                                        fflush(stdout); }
#define WATCHPOINT_NUMBER(A) { fprintf(stderr, \
                                       "\nWATCHPOINT %s:%d (%s) %u\n", \
                                       __FILE__, __LINE__, __func__, \
                                       (size_t)(A)); \
                               fflush(stdout); }
#define WATCHPOINT_ERRNO(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                      __FILE__, __LINE__, __func__, \
                                      strerror(A)); \
                              fflush(stdout); }
#define WATCHPOINT_ASSERT(A) assert((A));
#else /* !HAVE_DEBUG */
#define WATCHPOINT {}
#define WATCHPOINT_ERROR(A) {}
#define WATCHPOINT_ACTION(A) {}
#define WATCHPOINT_IFERROR(A) {}
#define WATCHPOINT_STRING(A) {}
#define WATCHPOINT_STRING_LENGTH(A,B) {}
#define WATCHPOINT_NUMBER(A) {}
#define WATCHPOINT_ERRNO(A) {}
#define WATCHPOINT_ASSERT(A) {}
#endif /* HAVE_DEBUG */


#endif /* __GEARMAN_WATCHPOINT_H__ */
