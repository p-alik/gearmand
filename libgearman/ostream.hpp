/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
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

#include <iostream>
#include <ostream>

static inline std::ostream& operator<<(std::ostream& output, const gearman_task_st& arg)
{
  output << " handle:" << arg.job_handle[0] ? arg.job_handle : "unset";
  output << " state:";
  switch (arg.state)
  {
  case GEARMAN_TASK_STATE_NEW: output << "GEARMAN_TASK_STATE_NEW"; break; 
  case GEARMAN_TASK_STATE_SUBMIT: output << "GEARMAN_TASK_STATE_SUBMIT"; break; 
  case GEARMAN_TASK_STATE_WORKLOAD: output << "GEARMAN_TASK_STATE_WORKLOAD"; break; 
  case GEARMAN_TASK_STATE_WORK: output << "GEARMAN_TASK_STATE_WORK"; break; 
  case GEARMAN_TASK_STATE_CREATED: output << "GEARMAN_TASK_STATE_CREATED"; break; 
  case GEARMAN_TASK_STATE_DATA: output << "GEARMAN_TASK_STATE_DATA"; break; 
  case GEARMAN_TASK_STATE_WARNING: output << "GEARMAN_TASK_STATE_WARNING"; break; 
  case GEARMAN_TASK_STATE_STATUS: output << "GEARMAN_TASK_STATE_STATUS"; break; 
  case GEARMAN_TASK_STATE_COMPLETE: output << "GEARMAN_TASK_STATE_COMPLETE"; break; 
  case GEARMAN_TASK_STATE_EXCEPTION: output << "GEARMAN_TASK_STATE_EXCEPTION"; break; 
  case GEARMAN_TASK_STATE_FAIL: output << "GEARMAN_TASK_STATE_FAIL"; break; 
  case GEARMAN_TASK_STATE_FINISHED: output << "GEARMAN_TASK_STATE_FINISHED"; break; 
  default:
                                    output << "UNKNOWN";
                                    break;
  }

  return output;
}
