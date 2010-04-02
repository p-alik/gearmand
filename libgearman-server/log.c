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


void gearmand_log(gearmand_st *state, gearman_verbose_t verbose,
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


void gearmand_log_fatal(gearmand_st *gearman, const char *format, ...)
{
  va_list args;

  if (gearman->verbose >= GEARMAN_VERBOSE_FATAL)
  {
    va_start(args, format);
    gearmand_log(gearman, GEARMAN_VERBOSE_FATAL, format, args);
    va_end(args);
  }
}

void gearmand_log_error(gearmand_st *gearman, const char *format, ...)
{
  va_list args;

  if (gearman->verbose >= GEARMAN_VERBOSE_ERROR)
  {
    va_start(args, format);
    gearmand_log(gearman, GEARMAN_VERBOSE_ERROR, format, args);
    va_end(args);
  }
}

void gearmand_log_info(gearmand_st *gearman, const char *format, ...)
{
  va_list args;

  if (gearman->verbose >= GEARMAN_VERBOSE_INFO)
  {
    va_start(args, format);
    gearmand_log(gearman, GEARMAN_VERBOSE_INFO, format, args);
    va_end(args);
  }
}

void gearmand_log_debug(gearmand_st *gearman, const char *format, ...)
{
  va_list args;

  if (gearman->verbose >= GEARMAN_VERBOSE_DEBUG)
  {
    va_start(args, format);
    gearmand_log(gearman, GEARMAN_VERBOSE_DEBUG, format, args);
    va_end(args);
  }
}

void gearmand_log_crazy(gearmand_st *gearman, const char *format, ...)
{
  va_list args;

  if (gearman->verbose >= GEARMAN_VERBOSE_CRAZY)
  {
    va_start(args, format);
    gearmand_log(gearman, GEARMAN_VERBOSE_CRAZY, format, args);
    va_end(args);
  }
}

void gearman_conf_error_set(gearman_conf_st *conf, const char *msg, const char *format, ...)
{
  va_list args;
  char *ptr;
  size_t length= strlen(msg);

  ptr= memcpy(conf->last_error, msg, length);
  ptr+= length;

  va_start(args, format);
  vsnprintf(ptr, GEARMAN_MAX_ERROR_SIZE- length, format, args);
  va_end(args);
}


