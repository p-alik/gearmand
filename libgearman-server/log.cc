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

#include <libgearman-server/common.h>

#include <algorithm>
#include <errno.h>
#include <cstring>

static pthread_key_t logging_key;
static pthread_once_t intitialize_log_once = PTHREAD_ONCE_INIT;

static void delete_log(void *ptr)
{
  if (ptr)
  {
    free(ptr);
  }
}

static void create_log(void)
{
  (void) pthread_key_create(&logging_key, delete_log);
}

void gearmand_initialize_thread_logging(const char *identity)
{
  (void) pthread_once(&intitialize_log_once, create_log);

  void *ptr;
  if ((ptr = pthread_getspecific(logging_key)) == NULL)
  {
    const char *key_to_use= strdup(identity);
    (void) pthread_setspecific(logging_key, key_to_use);
  }
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/**
 * Log a message.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] verbose Logging level of the message.
 * @param[in] format Format and variable argument list of message.
 * @param[in] args Variable argument list that has been initialized.
 */

static void gearmand_log(gearmand_verbose_t verbose, const char *format, va_list args)
{
  char log_buffer[GEARMAN_MAX_ERROR_SIZE*2];

  (void) pthread_once(&intitialize_log_once, create_log);

  const char *identity;
  identity= (const char *)pthread_getspecific(logging_key);

  if (identity == NULL)
    identity= "[  main ]";

  if (Gearmand() && Gearmand()->log_fn)
  {
    int length= snprintf(log_buffer, sizeof(log_buffer), "%s ", identity);

    // We just return whatever we have if this occurs
    if (length < -1 || (size_t)length >= sizeof(log_buffer))
    {
      Gearmand()->log_fn(log_buffer, verbose, (void *)Gearmand()->log_context);
      return;
    }
    size_t remaining_size= sizeof(log_buffer) - (size_t)length;
    char *ptr= log_buffer;
    ptr+= length;

    vsnprintf(ptr, remaining_size, format, args);
    Gearmand()->log_fn(log_buffer, verbose, (void *)Gearmand()->log_context);
  }
  else
  {
    fprintf(stderr, "%s %7s: ", identity,  gearmand_verbose_name(verbose));
    vprintf(format, args);
    fprintf(stderr, "\n");
  }
}


void gearmand_log_fatal(const char *format, ...)
{
  va_list args;

  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_FATAL)
  {
    va_start(args, format);
    gearmand_log(GEARMAND_VERBOSE_FATAL, format, args);
    va_end(args);
  }
}

void gearmand_log_fatal_perror(const char *position, const char *message)
{
  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_FATAL)
  {
    const char *errmsg_ptr;
    char errmsg[GEARMAN_MAX_ERROR_SIZE]; 
    errmsg[0]= 0; 

#ifdef STRERROR_R_CHAR_P
    errmsg_ptr= strerror_r(errno, errmsg, sizeof(errmsg));
#else
    strerror_r(errno, errmsg, sizeof(errmsg));
    errmsg_ptr= errmsg;
#endif

    char final[BUFSIZ];
    snprintf(final, sizeof(final), "%s(%s) -> %s ", message, errmsg_ptr, position);
    gearmand_log_fatal(final);
  }
}

void gearmand_log_error(const char *format, ...)
{
  va_list args;

  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_ERROR)
  {
    va_start(args, format);
    gearmand_log(GEARMAND_VERBOSE_ERROR, format, args);
    va_end(args);
  }
}

void gearmand_log_info(const char *format, ...)
{
  va_list args;

  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_INFO)
  {
    va_start(args, format);
    gearmand_log(GEARMAND_VERBOSE_INFO, format, args);
    va_end(args);
  }
}

void gearmand_log_debug(const char *format, ...)
{
  va_list args;

  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_DEBUG)
  {
    va_start(args, format);
    gearmand_log(GEARMAND_VERBOSE_DEBUG, format, args);
    va_end(args);
  }
}

void gearmand_log_crazy(const char *format, ...)
{
#ifdef DEBUG
  va_list args;

  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_CRAZY)
  {
    va_start(args, format);
    gearmand_log(GEARMAND_VERBOSE_CRAZY, format, args);
    va_end(args);
  }
#else
  (void)format;
#endif
}

void gearmand_log_perror(const char *position, const char *message)
{
  if (!Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_ERROR)
  {
    const char *errmsg_ptr;
    char errmsg[GEARMAN_MAX_ERROR_SIZE]; 
    errmsg[0]= 0; 

#ifdef STRERROR_R_CHAR_P
    errmsg_ptr= strerror_r(errno, errmsg, sizeof(errmsg));
#else
    strerror_r(errno, errmsg, sizeof(errmsg));
    errmsg_ptr= errmsg;
#endif
    gearmand_log_error("%s(%s) -> %s ", message, errmsg_ptr, position);
  }
}

gearmand_error_t gearmand_log_gerror(const char *position, const char *message, const gearmand_error_t rc)
{
  if (rc != GEARMAN_SUCCESS)
  {
    gearmand_log_error("%s(%s) -> %s ", message, gearmand_strerror(rc), position);
  }

  return rc;
}

void gearmand_log_gai_error(const char *position, const char *message, const int rc)
{
  if (rc == EAI_SYSTEM)
  {
    gearmand_log_perror(position, message);
    return;
  }

  gearmand_log_error("%s getaddrinfo(%s) -> %s", message, gai_strerror(rc), position);
}
