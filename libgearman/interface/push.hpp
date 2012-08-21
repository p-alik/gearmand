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

#include "libgearman/interface/client.hpp" 
#include "libgearman/interface/universal.hpp" 

/**
  Push the state of IO
*/
class PushBlocking {
public:
  PushBlocking(gearman_universal_st& arg) :
    _original(arg.options.non_blocking),
    _universal(arg)
  {
    _universal.non_blocking(false);
  }

  PushBlocking(gearman_client_st* client_shell) :
    _original(client_shell->impl()->universal.is_non_blocking()),
    _universal(client_shell->impl()->universal)
  {
    _universal.non_blocking(false);
  }

  ~PushBlocking()
  {
    _universal.non_blocking(_original);
  }

private:
  bool _original;
  gearman_universal_st& _universal;
};

#define PUSH_BLOCKING(__univeral) PushBlocking _push_block((__univeral));

class PushNonBlocking {
public:
  PushNonBlocking(gearman_universal_st& arg) :
    _original(arg.options.non_blocking),
    _universal(arg)
  {
    _universal.non_blocking(true);
  }

  PushNonBlocking(gearman_client_st* client_shell) :
    _original(client_shell->impl()->universal.options.non_blocking),
    _universal(client_shell->impl()->universal)
  {
    _universal.non_blocking(true);
  }

  ~PushNonBlocking()
  {
    _universal.non_blocking(_original);
  }

private:
  bool _original;
  gearman_universal_st& _universal;
};

#define PUSH_NON_BLOCKING(__univeral) PushNonBlocking _push_block((__univeral));

