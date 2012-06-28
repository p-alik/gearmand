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

#include <libgearman-server/timer.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include <signal.h>

static pthread_once_t start_key_once= PTHREAD_ONCE_INIT;
static pthread_t thread_id;

static struct timeval current_epoch;

static void* current_epoch_handler(void*)
{
  gearmand_debug("staring up Epoch thread");

  pollfd fds[1];
  while (true)
  {
    memset(fds, 0, sizeof(pollfd));
    fds[0].fd= -1; //STDIN_FILENO;
    fds[0].events= POLLIN;
    fds[0].revents= 0;

    int active_fd;
    if ((active_fd= poll(fds, 1, 1000)) == -1)
    {
      switch (errno)
      {
      case EINTR: // Shutdown
        pthread_exit(NULL);

      default:
        gearmand_perror("poll");
      }
    }

    if (active_fd == 1)
    {
      gettimeofday(&current_epoch, NULL);
    }
  }

  pthread_exit(NULL);
}

static void startup(void)
{
  gettimeofday(&current_epoch, NULL);

  int error;
  if ((error= pthread_create(&thread_id, NULL, current_epoch_handler, NULL)))
  {
    fprintf(stderr, "pthread_create failed\n");
  }
}

namespace libgearman {
namespace server {

Epoch::Epoch()
{
  (void) pthread_once(&start_key_once, startup);
}

Epoch::~Epoch()
{
  gearmand_debug("shutting down Epoch thread");
  int error;
  if ((error= pthread_kill(thread_id, SIGTERM)) != 0)
  {
    errno= error;
    gearmand_perror("pthread_kill(thread_id, SIGTERM)");
  }
  pthread_join(thread_id, 0);

  gearmand_debug("shutdown of Epoch completed");
}

struct timeval Epoch::current()
{
  return current_epoch;
}

} // server
} // libgearman
