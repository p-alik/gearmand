/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Cycle the Gearmand server
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


/*
  Test that we are cycling the servers we are creating during testing.
*/

#include <config.h>

#include <libtest/test.hpp>

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include "tests/ports.h"

static std::string executable;

static test_return_t check_args_test(void *)
{
  const char *args[]= { "--check-args", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_backlog_test(void *)
{
  const char *args[]= { "--check-args", "--backlog=10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_backlog_test(void *)
{
  const char *args[]= { "--check-args", "-b 10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_daemon_test(void *)
{
  const char *args[]= { "--check-args", "--daemon", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_daemon_test(void *)
{
  const char *args[]= { "--check-args", "-d", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_file_descriptors_test(void *)
{
  const char *args[]= { "--check-args", "--file-descriptors=10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_file_descriptors_test(void *)
{
  const char *args[]= { "--check-args", "-f 10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_help_test(void *)
{
  const char *args[]= { "--check-args", "--help", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_help_test(void *)
{
  const char *args[]= { "--check-args", "-h", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_log_file_test(void *)
{
  const char *args[]= { "--check-args", "--log-file=\"tmp/foo\"", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_log_file_test(void *)
{
  const char *args[]= { "--check-args", "-l \"tmp/foo\"", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_listen_test(void *)
{
  const char *args[]= { "--check-args", "--listen=10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_listen_test(void *)
{
  const char *args[]= { "--check-args", "-L 10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_port_test(void *)
{
  const char *args[]= { "--check-args", "--port=10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_port_test(void *)
{
  const char *args[]= { "--check-args", "-p 10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_pid_file_test(void *)
{
  const char *args[]= { "--check-args", "--pid-file=\"tmp/gearmand.pid\"", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_pid_file_test(void *)
{
  const char *args[]= { "--check-args", "-P \"tmp/gearmand.pid\"", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_round_robin_test(void *)
{
  const char *args[]= { "--check-args", "--round-robin", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_round_robin_test(void *)
{
  const char *args[]= { "--check-args", "-R", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_threads_test(void *)
{
  const char *args[]= { "--check-args", "--threads=10", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_threads_test(void *)
{
  const char *args[]= { "--check-args", "-t 8", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_user_test(void *)
{
  const char *args[]= { "--check-args", "--user=nobody", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_user_test(void *)
{
  const char *args[]= { "--check-args", "-u nobody", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_version_test(void *)
{
  const char *args[]= { "--check-args", "--version", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_version_test(void *)
{
  const char *args[]= { "--check-args", "-V", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t verbose_test(void *)
{
  const char *args[]= { "--check-args", "-vvv", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_worker_wakeup_test(void *)
{
  const char *args[]= { "--check-args", "--worker-wakeup=4", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_worker_wakeup_test(void *)
{
  const char *args[]= { "--check-args", "-V", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t protocol_test(void *)
{
  const char *args[]= { "--check-args", "--protocol=http", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t queue_test(void *)
{
  const char *args[]= { "--check-args", "--queue=builtin", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t long_job_retries_test(void *)
{
  const char *args[]= { "--check-args", "--job-retries=4", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t short_job_retries_test(void *)
{
  const char *args[]= { "--check-args", "-j 6", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t http_port_test(void *)
{
  const char *args[]= { "--check-args", "--protocol=http", "--http-port=8090",  0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}


test_st gearmand_option_tests[] ={
  {"--check-args", 0, check_args_test},
  {"--backlog=", 0, long_backlog_test},
  {"-b", 0, short_backlog_test},
  {"--daemon", 0, long_daemon_test},
  {"-d", 0, short_daemon_test},
  {"--file-descriptors=", 0, long_file_descriptors_test},
  {"-f", 0, short_file_descriptors_test},
  {"--help", 0, long_help_test},
  {"-h", 0, short_help_test},
  {"--log-file=", 0, long_log_file_test},
  {"-l", 0, short_log_file_test},
  {"--listen=", 0, long_listen_test},
  {"-L", 0, short_listen_test},
  {"--port=", 0, long_port_test},
  {"-p", 0, short_port_test},
  {"--pid-file=", 0, long_pid_file_test},
  {"-P", 0, short_pid_file_test},
  {"--round-robin", 0, long_round_robin_test},
  {"-R", 0, short_round_robin_test},
  {"--threads=", 0, long_threads_test},
  {"-T", 0, short_threads_test},
  {"--user=", 0, long_user_test},
  {"-u", 0, short_user_test},
  {"--user=", 0, long_user_test},
  {"-u", 0, short_user_test},
  {"-vvv", 0, verbose_test},
  {"--version", 0, long_version_test},
  {"-V", 0, short_version_test},
  {"--worker_wakeup=", 0, long_worker_wakeup_test},
  {"-w", 0, short_worker_wakeup_test},
  {"--protocol=", 0, protocol_test},
  {"--queue=", 0, queue_test},
  {"--job-retries=", 0, long_job_retries_test},
  {"-j", 0, short_job_retries_test},
  {0, 0, 0}
};

test_st gearmand_httpd_option_tests[] ={
  {"--http-port=", 0, http_port_test},
  {0, 0, 0}
};

collection_st collection[] ={
  {"basic options", 0, 0, gearmand_option_tests},
  {"httpd options", 0, 0, gearmand_httpd_option_tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
}

