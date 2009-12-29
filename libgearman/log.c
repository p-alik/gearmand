/* Gearman server and library
 * Copyright (C) 2008-2009 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman State Definitions
 */

#include "common.h"

void gearman_log(gearman_universal_st *state, gearman_verbose_t verbose,
                 const char *format, va_list args)
{
  char log_buffer[GEARMAN_MAX_ERROR_SIZE];

  if (state->log_fn == NULL)
  {
    printf("%5s: ", gearman_verbose_name(verbose));
    vprintf(format, args);
    printf("\n");
  }
  else
  {
    vsnprintf(log_buffer, GEARMAN_MAX_ERROR_SIZE, format, args);
    state->log_fn(log_buffer, verbose, (void *)state->log_context);
  }
}
