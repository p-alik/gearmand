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

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cstring>
#include <libgearman/gearman.h>
#include <tests/protocol.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

test_return_t check_gearman_command_t(void *)
{
  test_compare(0, int(GEARMAN_COMMAND_TEXT));
  test_compare(1, int(GEARMAN_COMMAND_CAN_DO));
  test_compare(2, int(GEARMAN_COMMAND_CANT_DO));
  test_compare(3, int(GEARMAN_COMMAND_RESET_ABILITIES));
  test_compare(4, int(GEARMAN_COMMAND_PRE_SLEEP));
  test_compare(5, int(GEARMAN_COMMAND_UNUSED));
  test_compare(6, int(GEARMAN_COMMAND_NOOP));
  test_compare(7, int(GEARMAN_COMMAND_SUBMIT_JOB));
  test_compare(8, int(GEARMAN_COMMAND_JOB_CREATED));
  test_compare(9, int(GEARMAN_COMMAND_GRAB_JOB));
  test_compare(10, int(GEARMAN_COMMAND_NO_JOB));
  test_compare(11, int(GEARMAN_COMMAND_JOB_ASSIGN));
  test_compare(12, int(GEARMAN_COMMAND_WORK_STATUS));
  test_compare(13, int(GEARMAN_COMMAND_WORK_COMPLETE));
  test_compare(14, int(GEARMAN_COMMAND_WORK_FAIL));
  test_compare(15, int(GEARMAN_COMMAND_GET_STATUS));
  test_compare(16, int(GEARMAN_COMMAND_ECHO_REQ));
  test_compare(17, int(GEARMAN_COMMAND_ECHO_RES));
  test_compare(18, int(GEARMAN_COMMAND_SUBMIT_JOB_BG));
  test_compare(19, int(GEARMAN_COMMAND_ERROR));
  test_compare(20, int(GEARMAN_COMMAND_STATUS_RES));
  test_compare(21, int(GEARMAN_COMMAND_SUBMIT_JOB_HIGH));
  test_compare(22, int(GEARMAN_COMMAND_SET_CLIENT_ID));
  test_compare(23, int(GEARMAN_COMMAND_CAN_DO_TIMEOUT));
  test_compare(24, int(GEARMAN_COMMAND_ALL_YOURS));
  test_compare(25, int(GEARMAN_COMMAND_WORK_EXCEPTION));
  test_compare(26, int(GEARMAN_COMMAND_OPTION_REQ));
  test_compare(27, int(GEARMAN_COMMAND_OPTION_RES));
  test_compare(28, int(GEARMAN_COMMAND_WORK_DATA));
  test_compare(29, int(GEARMAN_COMMAND_WORK_WARNING));
  test_compare(30, int(GEARMAN_COMMAND_GRAB_JOB_UNIQ));
  test_compare(31, int(GEARMAN_COMMAND_JOB_ASSIGN_UNIQ));
  test_compare(32, int(GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG));
  test_compare(33, int(GEARMAN_COMMAND_SUBMIT_JOB_LOW));
  test_compare(34, int(GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG));
  test_compare(35, int(GEARMAN_COMMAND_SUBMIT_JOB_SCHED));
  test_compare(36, int(GEARMAN_COMMAND_SUBMIT_JOB_EPOCH));
  test_compare(37, int(GEARMAN_COMMAND_SUBMIT_REDUCE_JOB));
  test_compare(38, int(GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND));
  test_compare(39, int(GEARMAN_COMMAND_GRAB_JOB_ALL));
  test_compare(40, int(GEARMAN_COMMAND_JOB_ASSIGN_ALL));

  return TEST_SUCCESS;
}
