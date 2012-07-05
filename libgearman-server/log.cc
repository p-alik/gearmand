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

#include <config.h>
#include <libgearman-server/common.h>
#include <libgearman-server/timer.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>

static pthread_key_t logging_key;
static pthread_once_t intitialize_log_once= PTHREAD_ONCE_INIT;

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

  if (pthread_getspecific(logging_key) == NULL)
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

static void gearmand_log(const char *position, const char *func /* func */, 
                         gearmand_verbose_t verbose,
                         const gearmand_error_t error_arg,
                         const char *format, va_list args)
{
  struct timeval current_epoch;

  if (Gearmand() and Gearmand()->verbose < GEARMAND_VERBOSE_DEBUG)
  {
    current_epoch= libgearman::server::Epoch::current();
    current_epoch.tv_usec= 0;
  }
  else
  {
    (void)gettimeofday(&current_epoch, NULL);
  }

  struct tm current_tm;
  if (current_epoch.tv_sec == 0)
  {
    (void)gettimeofday(&current_epoch, NULL);
  }

  if ((gmtime_r(&current_epoch.tv_sec, &current_tm) == NULL))
  {
    memset(&current_epoch, 0, sizeof(current_epoch));
  }

  (void) pthread_once(&intitialize_log_once, create_log);

  const char *identity= (const char *)pthread_getspecific(logging_key);

  if (identity == NULL)
  {
    identity= "[  main ]";
  }

  char log_buffer[GEARMAN_MAX_ERROR_SIZE*2] = { 0 };
  if (Gearmand() && Gearmand()->log_fn)
  {
    char *log_buffer_ptr= log_buffer;
    size_t remaining_size= sizeof(log_buffer);

    {
      int length= snprintf(log_buffer, sizeof(log_buffer), "%04d-%02d-%02d %02d:%02d:%02d.%06d %s ",
                           int(1900 + current_tm.tm_year), current_tm.tm_mon, current_tm.tm_mday, current_tm.tm_hour,
                           current_tm.tm_min, current_tm.tm_sec, int(current_epoch.tv_usec),
                           identity);
      // We just return whatever we have if this occurs
      if (length <= 0 or (size_t)length >= sizeof(log_buffer))
      {
        remaining_size= 0;
      }
      else
      {
        remaining_size-= size_t(length);
        log_buffer_ptr+= length;
      }
    }

    if (remaining_size)
    {
      int length= vsnprintf(log_buffer_ptr, remaining_size, format, args);
      if (length <= 0 or size_t(length) >= remaining_size)
      { 
        remaining_size= 0;
      }
      else
      {
        remaining_size-= size_t(length);
        log_buffer_ptr+= length;
      }
    }

    if (remaining_size and error_arg != GEARMAN_SUCCESS)
    {
      int length= snprintf(log_buffer_ptr, remaining_size, " %s(%s)", func, gearmand_strerror(error_arg));
      if (length <= 0 or size_t(length) >= remaining_size)
      {
        remaining_size= 0;
      }
      else
      {
        remaining_size-= size_t(length);
        log_buffer_ptr+= length;
      }
    }

    if (remaining_size and position and verbose != GEARMAND_VERBOSE_INFO)
    {
      int length= snprintf(log_buffer_ptr, remaining_size, " -> %s", position);
      if (length <= 0 or size_t(length) >= remaining_size)
      {
        remaining_size= 0;
      }
    }

    // Make sure this is null terminated
    log_buffer[sizeof(log_buffer) -1]= 0;
  }

  if (Gearmand() and Gearmand()->log_fn)
  {
    Gearmand()->log_fn(log_buffer, verbose, (void *)Gearmand()->log_context);
  }
  else
  {
    fprintf(stderr, "%s -> %s",
            log_buffer,  gearmand_verbose_name(verbose));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
  }
}


void gearmand_log_fatal(const char *position, const char *func, const char *format, ...)
{
  va_list args;

  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_FATAL)
  {
    va_start(args, format);
    gearmand_log(position, func, GEARMAND_VERBOSE_FATAL, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }
}

void gearmand_log_fatal_perror(const char *position, const char *function, const char *message)
{
  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_FATAL)
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

    gearmand_log_fatal(position, function, "%s(%s)", message, errmsg_ptr);
  }
}

gearmand_error_t gearmand_log_error(const char *position, const char *function, const char *format, ...)
{
  va_list args;

  if (not Gearmand() or Gearmand()->verbose >= GEARMAND_VERBOSE_ERROR)
  {
    va_start(args, format);
    gearmand_log(position, function, GEARMAND_VERBOSE_ERROR, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }

  return GEARMAN_UNKNOWN_OPTION;
}

void gearmand_log_warning(const char *position, const char *function, const char *format, ...)
{
  va_list args;

  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_WARN)
  {
    va_start(args, format);
    gearmand_log(position, function, GEARMAND_VERBOSE_WARN, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }
}

// LOG_NOTICE is only used for reporting job status.
void gearmand_log_notice(const char *position, const char *function, const char *format, ...)
{
  va_list args;

  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_NOTICE)
  {
    va_start(args, format);
    gearmand_log(position, function, GEARMAND_VERBOSE_NOTICE, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }
}

void gearmand_log_info(const char *position, const char *function, const char *format, ...)
{
  va_list args;

  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_INFO)
  {
    va_start(args, format);
    gearmand_log(position, function, GEARMAND_VERBOSE_INFO, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }
}

void gearmand_log_debug(const char *position, const char *function, const char *format, ...)
{
  va_list args;

  if (not Gearmand() || Gearmand()->verbose >= GEARMAND_VERBOSE_DEBUG)
  {
    va_start(args, format);
    gearmand_log(position, function, GEARMAND_VERBOSE_DEBUG, GEARMAN_SUCCESS, format, args);
    va_end(args);
  }
}

gearmand_error_t gearmand_log_perror(const char *position, const char *function, const char *message)
{
  if (not Gearmand() or (Gearmand()->verbose >= GEARMAND_VERBOSE_ERROR))
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
    gearmand_log_error(position, function, "%s(%s)", message, errmsg_ptr);
  }

  return GEARMAN_ERRNO;
}

gearmand_error_t gearmand_log_gerror(const char *position, const char *function, const gearmand_error_t rc, const char *format, ...)
{
  if (gearmand_failed(rc) and rc != GEARMAN_IO_WAIT)
  {
    va_list args;

    if (Gearmand() == NULL or Gearmand()->verbose >= GEARMAND_VERBOSE_ERROR)
    {
      va_start(args, format);
      gearmand_log(position, function, GEARMAND_VERBOSE_ERROR, rc, format, args);
      va_end(args);
    }
  }
  else if (rc == GEARMAN_IO_WAIT)
  { }

  return rc;
}

gearmand_error_t gearmand_log_gerror_warn(const char *position, const char *function, const gearmand_error_t rc, const char *format, ...)
{
  if (gearmand_failed(rc) and rc != GEARMAN_IO_WAIT)
  {
    va_list args;

    if (Gearmand() == NULL or Gearmand()->verbose >= GEARMAND_VERBOSE_WARN)
    {
      va_start(args, format);
      gearmand_log(position, function, GEARMAND_VERBOSE_WARN, rc, format, args);
      va_end(args);
    }
  }
  else if (rc == GEARMAN_IO_WAIT)
  { }

  return rc;
}

gearmand_error_t gearmand_log_gai_error(const char *position, const char *function, const int rc, const char *message)
{
  if (rc == EAI_SYSTEM)
  {
    return gearmand_log_perror(position, function, message);
  }

  gearmand_log_error(position, function, "%s getaddrinfo(%s)", message, gai_strerror(rc));

  return GEARMAN_GETADDRINFO;
}

gearmand_error_t gearmand_log_memory_error(const char *position, const char *function, const char *allocator, const char *object_type, size_t count, size_t size)
{
  if (count > 1)
  {
    gearmand_log_error(position, function, "%s(%s, count: %lu size: %lu)", allocator, object_type, static_cast<unsigned long>(count), static_cast<unsigned long>(size));
  }
  else
  {
    gearmand_log_error(position, function, "%s(%s, size: %lu)", allocator, object_type, static_cast<unsigned long>(count), static_cast<unsigned long>(size));
  }

  return GEARMAN_MEMORY_ALLOCATION_FAILURE;
}
