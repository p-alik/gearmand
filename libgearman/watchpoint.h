/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GEARMAN_WATCHPOINT_H__
#define __GEARMAN_WATCHPOINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libgearman/str_action.h>
#include <libgearman/str_error.h>

/* Some personal debugging functions */
#ifdef HAVE_DEBUG
#define WATCHPOINT { fprintf(stderr, "\nWATCHPOINT %s:%d (%s)\n", \
                           __FILE__, __LINE__, __func__); \
                     fflush(stdout); }
#define WATCHPOINT_ERROR(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                      __FILE__, __LINE__, __func__, \
                                      gearman_strerror(A)); \
                              fflush(stdout); }
#define WATCHPOINT_ACTION(A) { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", \
                                       __FILE__, __LINE__, __func__, \
                                       gearman_straction(A)); \
                               fflush(stdout); }
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

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_WATCHPOINT_H__ */
