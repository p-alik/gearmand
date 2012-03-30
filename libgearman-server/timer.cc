/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/ All
 *  rights reserved.
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

#include <libgearman-server/common.h>
#include <libgearman-server/gearmand.h>

#include <ctime>
#include <event.h>

static struct timeval current_epoch;
static struct event clock_event;

struct timeval get_current_epoch()
{
  return current_epoch;
}

static bool _shutdown_clock= false;

void shutdown_current_epoch_handler()
{
  _shutdown_clock= true;
}

void current_epoch_handler(const int, const short, void*)
{
  static bool initialized= false;

  if (_shutdown_clock)
  {
    if (initialized) 
    {
      evtimer_del(&clock_event);
    }

    return;
  }

  if (initialized) 
  {
    evtimer_del(&clock_event);
  }
  else
  {
    initialized= true;
  }

  evtimer_set(&clock_event, current_epoch_handler, 0);
  event_base_set(Gearmand()->base, &clock_event);

  struct timeval wait_for;
  wait_for.tv_sec= 1;
  wait_for.tv_usec= 0;

  evtimer_add(&clock_event, &wait_for);

  struct timeval tv;
  gettimeofday(&current_epoch, NULL);
}
