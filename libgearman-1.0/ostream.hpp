/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Gearman Server and Client
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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

#include <libgearman-1.0/gearman.h>

#include <ostream>

static inline std::ostream& operator<<(std::ostream& output, const enum gearman_return_t &arg)
{
  output << gearman_strerror(arg);
  return output;
}

static inline std::ostream& operator<<(std::ostream& output, const gearman_packet_st &arg)
{
  const char* command_str;
  switch(arg.command)
  {
  case GEARMAN_COMMAND_TEXT: command_str=  "GEARMAN_COMMAND_TEXT";
  case GEARMAN_COMMAND_CAN_DO: command_str=  "GEARMAN_COMMAND_CAN_DO";
  case GEARMAN_COMMAND_CANT_DO: command_str=  "GEARMAN_COMMAND_CANT_DO";
  case GEARMAN_COMMAND_RESET_ABILITIES: command_str=  "GEARMAN_COMMAND_RESET_ABILITIES";
  case GEARMAN_COMMAND_PRE_SLEEP: command_str=  "GEARMAN_COMMAND_PRE_SLEEP";
  case GEARMAN_COMMAND_UNUSED: command_str=  "GEARMAN_COMMAND_UNUSED";
  case GEARMAN_COMMAND_NOOP: command_str=  "GEARMAN_COMMAND_NOOP";
  case GEARMAN_COMMAND_SUBMIT_JOB: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB";
  case GEARMAN_COMMAND_JOB_CREATED: command_str=  "GEARMAN_COMMAND_JOB_CREATED";
  case GEARMAN_COMMAND_GRAB_JOB: command_str=  "GEARMAN_COMMAND_GRAB_JOB";
  case GEARMAN_COMMAND_NO_JOB: command_str=  "GEARMAN_COMMAND_NO_JOB";
  case GEARMAN_COMMAND_JOB_ASSIGN: command_str=  "GEARMAN_COMMAND_JOB_ASSIGN";
  case GEARMAN_COMMAND_WORK_STATUS: command_str=  "GEARMAN_COMMAND_WORK_STATUS";
  case GEARMAN_COMMAND_WORK_COMPLETE: command_str=  "GEARMAN_COMMAND_WORK_COMPLETE";
  case GEARMAN_COMMAND_WORK_FAIL: command_str=  "GEARMAN_COMMAND_WORK_FAIL";
  case GEARMAN_COMMAND_GET_STATUS: command_str=  "GEARMAN_COMMAND_GET_STATUS";
  case GEARMAN_COMMAND_ECHO_REQ: command_str=  "GEARMAN_COMMAND_ECHO_REQ";
  case GEARMAN_COMMAND_ECHO_RES: command_str=  "GEARMAN_COMMAND_ECHO_RES";
  case GEARMAN_COMMAND_SUBMIT_JOB_BG: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_BG";
  case GEARMAN_COMMAND_ERROR: command_str=  "GEARMAN_COMMAND_ERROR";
  case GEARMAN_COMMAND_STATUS_RES: command_str=  "GEARMAN_COMMAND_STATUS_RES";
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_HIGH";
  case GEARMAN_COMMAND_SET_CLIENT_ID: command_str=  "GEARMAN_COMMAND_SET_CLIENT_ID";
  case GEARMAN_COMMAND_CAN_DO_TIMEOUT: command_str=  "GEARMAN_COMMAND_CAN_DO_TIMEOUT";
  case GEARMAN_COMMAND_ALL_YOURS: command_str=  "GEARMAN_COMMAND_ALL_YOURS";
  case GEARMAN_COMMAND_WORK_EXCEPTION: command_str=  "GEARMAN_COMMAND_WORK_EXCEPTION";
  case GEARMAN_COMMAND_OPTION_REQ: command_str=  "GEARMAN_COMMAND_OPTION_REQ";
  case GEARMAN_COMMAND_OPTION_RES: command_str=  "GEARMAN_COMMAND_OPTION_RES";
  case GEARMAN_COMMAND_WORK_DATA: command_str=  "GEARMAN_COMMAND_WORK_DATA";
  case GEARMAN_COMMAND_WORK_WARNING: command_str=  "GEARMAN_COMMAND_WORK_WARNING";
  case GEARMAN_COMMAND_GRAB_JOB_UNIQ: command_str=  "GEARMAN_COMMAND_GRAB_JOB_UNIQ";
  case GEARMAN_COMMAND_JOB_ASSIGN_UNIQ: command_str=  "GEARMAN_COMMAND_JOB_ASSIGN_UNIQ";
  case GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG";
  case GEARMAN_COMMAND_SUBMIT_JOB_LOW: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_LOW";
  case GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG";
  case GEARMAN_COMMAND_SUBMIT_JOB_SCHED: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_SCHED";
  case GEARMAN_COMMAND_SUBMIT_JOB_EPOCH: command_str=  "GEARMAN_COMMAND_SUBMIT_JOB_EPOCH";
  case GEARMAN_COMMAND_SUBMIT_REDUCE_JOB: command_str=  "GEARMAN_COMMAND_SUBMIT_REDUCE_JOB";
  case GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND: command_str=  "GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND";
  case GEARMAN_COMMAND_GRAB_JOB_ALL: command_str=  "GEARMAN_COMMAND_GRAB_JOB_ALL";
  case GEARMAN_COMMAND_JOB_ASSIGN_ALL: command_str=  "GEARMAN_COMMAND_JOB_ASSIGN_ALL";
  case GEARMAN_COMMAND_MAX: command_str=  "GEARMAN_COMMAND_MAX";
  case GEARMAN_COMMAND_GET_STATUS_UNIQUE: command_str=  "GEARMAN_COMMAND_GET_STATUS_UNIQUE";
  case GEARMAN_COMMAND_STATUS_RES_UNIQUE: command_str=  "GEARMAN_COMMAND_STATUS_RES_UNIQUE";
  }

  output << std::boolalpha 
    << "command: " << command_str
    << " argc: " << int(arg.argc);

  for (uint8_t x= 0; x < arg.argc; x++)
  {
    output << "arg[" << int(x) << "]: ";
    output.write(arg.arg[x], arg.arg_size[x]);
    output << " ";
  }

  return output;
}

static inline std::ostream& operator<<(std::ostream& output, const gearman_task_st &arg)
{
  output << std::boolalpha 
    << "job: " << gearman_task_job_handle(&arg)
    << " unique: " << gearman_task_unique(&arg)
    << " has_result:" << bool(arg.result_ptr)
    << " complete: " << gearman_task_numerator(&arg) << "/" << gearman_task_denominator(&arg)
    << " state: " << gearman_task_strstate(&arg)
    << " is_known: " << gearman_task_is_known(&arg)
    << " is_running: " << gearman_task_is_running(&arg)
    << " ret: " << gearman_task_return(&arg);

  if (arg.recv)
  {
    output << " " << *arg.recv;
  }

  return output;
}

static inline std::ostream& operator<<(std::ostream& output, const gearman_status_t &arg)
{
  output << std::boolalpha 
    << " is_known: " << gearman_status_is_known(arg)
    << " is_running: " << gearman_status_is_running(arg)
    << " complete: " << gearman_status_numerator(arg) << "/" << gearman_status_denominator(arg)
    << " ret: " << gearman_strerror(gearman_status_return(arg));

  return output;
}
