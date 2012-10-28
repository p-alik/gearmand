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

#include "gear_config.h"
#include <libgearman/common.h>

#include <libgearman-1.0/visibility.h>
#include <libgearman/command.h>

#include "libgearman/assert.hpp"

/**
 * Command info. Update GEARMAN_MAX_COMMAND_ARGS to the largest number in the
 * args column.
 */
gearman_command_info_st gearmand_command_info_list[GEARMAN_COMMAND_MAX]=
{
  { "TEXT",               5, false },
  { "CAN_DO",             1, false },
  { "CANT_DO",            1, false },
  { "RESET_ABILITIES",    0, false },
  { "PRE_SLEEP",          0, false },
  { "UNUSED",             0, false },
  { "NOOP",               0, false },
  { "SUBMIT_JOB",         2, true  },
  { "JOB_CREATED",        1, false },
  { "GRAB_JOB",           0, false },
  { "NO_JOB",             0, false },
  { "JOB_ASSIGN",         2, true  },
  { "WORK_STATUS",        3, false },
  { "WORK_COMPLETE",      1, true  },
  { "WORK_FAIL",          1, false },
  { "GET_STATUS",         1, false },
  { "ECHO_REQ",           0, true  },
  { "ECHO_RES",           0, true  },
  { "SUBMIT_JOB_BG",      2, true  },
  { "ERROR",              2, false },
  { "STATUS_RES",         5, false },
  { "SUBMIT_JOB_HIGH",    2, true  },
  { "SET_CLIENT_ID",      1, false },
  { "CAN_DO_TIMEOUT",     2, false },
  { "ALL_YOURS",          0, false },
  { "WORK_EXCEPTION",     1, true  },
  { "OPTION_REQ",         1, false },
  { "OPTION_RES",         1, false },
  { "WORK_DATA",          1, true  },
  { "WORK_WARNING",       1, true  },
  { "GRAB_JOB_UNIQ",      0, false },
  { "JOB_ASSIGN_UNIQ",    3, true  },
  { "SUBMIT_JOB_HIGH_BG", 2, true  },
  { "SUBMIT_JOB_LOW",     2, true  },
  { "SUBMIT_JOB_LOW_BG",  2, true  },
  { "SUBMIT_JOB_SCHED",   7, true  },
  { "SUBMIT_JOB_EPOCH",   3, true  },
  { "GEARMAN_COMMAND_SUBMIT_REDUCE_JOB", 4, true },
  { "GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND", 4, true },
  { "GEARMAN_COMMAND_GRAB_JOB_ALL",    0, false  },
  { "GEARMAN_COMMAND_JOB_ASSIGN_ALL",    4, true  },
  { "GEARMAN_COMMAND_GET_STATUS_UNIQUE", 1, false },
  { "GEARMAN_COMMAND_STATUS_RES_UNIQUE", 6, false }
};

gearman_command_info_st *gearman_command_info(gearman_command_t command)
{
  assert(command >= GEARMAN_COMMAND_TEXT);
  assert(command < GEARMAN_COMMAND_MAX);
  return &gearmand_command_info_list[command];
}
