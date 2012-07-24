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

#include <config.h>

#include "libgearman/common.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

static inline bool set_cloexec(int pipedes_[2], const size_t x)
{
  int returned_flags;
  do 
  {
    returned_flags= fcntl(pipedes_[x], F_GETFD, 0);
  } while (returned_flags == -1 and (errno == EINTR or errno == EAGAIN));

  if (returned_flags == -1)
  {
#if 0
    perror("fcntl(pipedes_[x], F_GETFD, 0)");
#endif
    return false;
  }

  int retval;
  do
  { 
    retval= fcntl (pipedes_[x], F_SETFD, FD_CLOEXEC);
  } while (retval == -1 && (errno == EINTR or errno == EAGAIN));

  if (retval == -1)
  {
#if 0
    perror("fcntl (pipedes_[x], F_SETFD, FD_CLOEXEC)");
#endif
    return false;
  }

  return true;
}

static inline bool set_nonblock(int pipedes_[2], const size_t x)
{
  int returned_flags;
  do 
  {
    returned_flags= fcntl(pipedes_[x], F_GETFL, 0);
  } while (returned_flags == -1 and (errno == EINTR or errno == EAGAIN));

  if (returned_flags == -1)
  {
#if 0
    perror("fcntl(pipedes_[x], F_GETFL, 0)");
#endif
    return false;
  }

  int retval;
  do
  {
    retval= fcntl(pipedes_[x], F_SETFL, returned_flags | O_NONBLOCK);
  } while (retval == -1 and (errno == EINTR or errno == EAGAIN));

  if (retval == -1)
  {
#if 0
    perror("fcntl(pipedes_[x], F_SETFL, returned_flags | O_NONBLOCK)");
#endif
    return false;
  }

  return true;
}

bool setup_shutdown_pipe(int pipedes_[2])
{
  if (HAVE_PIPE2)
  {
#if defined(HAVE_PIPE2) && HAVE_PIPE2
    if (pipe2(pipedes_, O_NONBLOCK|O_CLOEXEC) == -1)
    {
      return false;
    }
#else
    assert(0); // This should never be reached
#endif
  }
  else if (pipe(pipedes_) == -1)
  {
#if 0
    perror("pipe");
#endif
    return false;
  }

  for (size_t x= 0; x < 2; ++x)
  {
    bool success= true;

#ifdef F_SETNOSIGPIPE
    if (F_SETNOSIGPIPE)
    {
      int fcntl_sig_error;
      do 
      {
        fcntl_sig_error= fcntl(pipedes_[x], F_SETNOSIGPIPE, 0);
      } while (fcntl_sig_error == -1 and (errno == EINTR or errno == EAGAIN));

      if (fcntl_sig_error == -1)
      {
#if 0
        perror ("fcntl_sig_error= fcntl(pipedes_[x], F_SETNOSIGPIPE, 0)");
#endif
        success= false;
      }
    }
#endif // F_SETNOSIGPIPE

    if (HAVE_PIPE2)
    { }
    else
    {
      if ((success= set_cloexec(pipedes_, x)))
      {
        success= set_nonblock(pipedes_, x);
      }
    }
    
    if (success == false)
    {
      if (errno != EBADF)
      {
        close(pipedes_[0]);
        close(pipedes_[1]);
      }

      return false;
    }
  }

  return true;
}
