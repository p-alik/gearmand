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


#pragma once

#define GEARMAN_BASE_TEST_PORT 32143
#define BURNIN_TEST_PORT GEARMAN_BASE_TEST_PORT +1
#define CLIENT_TEST_PORT GEARMAN_BASE_TEST_PORT +2
#define DRIZZLE_TEST_PORT GEARMAN_BASE_TEST_PORT +3
#define INTERNAL_TEST_PORT GEARMAN_BASE_TEST_PORT +4
#define MEMCACHED_TEST_PORT GEARMAN_BASE_TEST_PORT +5
#define ROUND_ROBIN_WORKER_TEST_PORT GEARMAN_BASE_TEST_PORT +6
#define SQLITE_TEST_PORT GEARMAN_BASE_TEST_PORT +7
#define TOKYOCABINET_TEST_PORT GEARMAN_BASE_TEST_PORT +8
#define WORKER_TEST_PORT GEARMAN_BASE_TEST_PORT +9
#define CYCLE_TEST_PORT GEARMAN_BASE_TEST_PORT +10
#define GEARADMIN_TEST_PORT GEARMAN_BASE_TEST_PORT +11
#define BLOBSLAP_CLIENT_TEST_PORT GEARMAN_BASE_TEST_PORT +12
#define STRESS_WORKER_PORT GEARMAN_BASE_TEST_PORT +13
#define EPHEMERAL_PORT GEARMAN_BASE_TEST_PORT +14
#define REDIS_PORT GEARMAN_BASE_TEST_PORT +15
#define GEARMAN_MAX_TEST_PORT REDIS_PORT
