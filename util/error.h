/* Gearman server and library
 * Copyright (C) 2011 DataDifferential
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <string>

struct gearman_client_st;
struct gearman_worker_st;

namespace gearman_util
{

namespace error {

void perror(const char *);
void message(const char *);
void message(const std::string &arg, gearman_return_t rc);
void message(const char *arg, const char *arg2);
void message(const char *arg, const gearman_client_st &);
void message(const char *arg, const gearman_worker_st &);

} // namespace error



} // namespace gearman_util
