/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Error definitions
 */

#include "common.h"

/*
 * Private definitions
 */

void gearman_error_set(gearman_st *gearman, const char *function,
                       const char *format, ...)
{
  size_t length;
  char *ptr;
  char log_buffer[GEARMAN_MAX_ERROR_SIZE];
  va_list arg;

  va_start(arg, format);

  length= strlen(function);

  /* Copy the function name and : before the format */
  ptr= memcpy(log_buffer, function, length);
  ptr+= length;
  ptr[0]= ':';
  ptr++;

  length= (size_t)vsnprintf(ptr, GEARMAN_MAX_ERROR_SIZE - length - 1, format,
                            arg);

  if (gearman->log_fn == NULL)
    memcpy(gearman->last_error, log_buffer, length);
  else
  {
    (*(gearman->log_fn))(log_buffer, GEARMAN_VERBOSE_FATAL,
                         (void *)(gearman)->log_context);
  }

  va_end(arg);
}
