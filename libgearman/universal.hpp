/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

GEARMAN_LOCAL
void gearman_universal_set_perror(const char *position, gearman_universal_st &universal, const char *message);

GEARMAN_LOCAL
void gearman_universal_initialize(gearman_universal_st &self, const gearman_universal_options_t *options= NULL);

GEARMAN_LOCAL
void gearman_universal_clone(gearman_universal_st &destination, const gearman_universal_st &source);

GEARMAN_LOCAL
void gearman_universal_free(gearman_universal_st &gearman);

GEARMAN_LOCAL
void gearman_free_all_packets(gearman_universal_st &gearman);

GEARMAN_LOCAL
bool gearman_request_option(gearman_universal_st &universal, gearman_string_t &option);

GEARMAN_LOCAL
gearman_return_t gearman_universal_set_option(gearman_universal_st &self, gearman_universal_options_t option, bool value);

GEARMAN_LOCAL
void gearman_set_log_fn(gearman_universal_st &self, gearman_log_fn *function, void *context, gearman_verbose_t verbose);

GEARMAN_LOCAL
void gearman_universal_set_timeout(gearman_universal_st &self, int timeout);

GEARMAN_LOCAL
int gearman_universal_timeout(gearman_universal_st &self);

/**
 * Wait for I/O on connections.
 *
 */
GEARMAN_LOCAL
gearman_return_t gearman_wait(gearman_universal_st&);

static inline void gearman_universal_add_options(gearman_universal_st &self, gearman_universal_options_t options)
{
  (void)gearman_universal_set_option(self, options, true);
}

static inline void gearman_universal_remove_options(gearman_universal_st &self, gearman_universal_options_t options)
{
  (void)gearman_universal_set_option(self, options, false);
}

static inline bool gearman_universal_is_non_blocking(gearman_universal_st &self)
{
  return self.options.non_blocking;
}

static inline const char *gearman_universal_error(const gearman_universal_st &self)
{
  if (self.error.last_error[0] == 0)
      return NULL;

  return static_cast<const char *>(self.error.last_error);
}

static inline gearman_return_t gearman_universal_error_code(const gearman_universal_st &self)
{
  return self.error.rc;
}

static inline int gearman_universal_errno(const gearman_universal_st &self)
{
  return self.error.last_errno;
}
