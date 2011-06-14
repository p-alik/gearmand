/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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

#include <libgearman/common.h>

#include <libgearman/packet.hpp>
#include <libgearman/function/base.hpp>
#include <libgearman/function/mapper.hpp>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch"
#endif

/*
  Mapper function
*/
gearman_function_error_t Mapper::callback(gearman_job_st* job, void *context_arg)
{
  if (gearman_job_is_map(job))
    gearman_job_build_reducer(job, aggregator_fn);

  gearman_return_t error= _mapper_fn(job, context_arg);
  switch (error)
  {
  case GEARMAN_FATAL:
    job->error_code= GEARMAN_FATAL;
    return GEARMAN_FUNCTION_FATAL;

  case GEARMAN_ERROR:
    job->error_code= GEARMAN_ERROR;
    return GEARMAN_FUNCTION_ERROR;

  case GEARMAN_SUCCESS:
    job->error_code= GEARMAN_SUCCESS;
    return GEARMAN_FUNCTION_SUCCESS;
  }

  return GEARMAN_FUNCTION_INVALID_ARGUMENT;
}
