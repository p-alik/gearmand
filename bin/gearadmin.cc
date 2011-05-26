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

#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libgearman/protocol.h>
#include <boost/program_options.hpp>

#include <util/instance.h>

using namespace gearman_util;

/*
  This is a very quickly build application, I am just tired of telneting to the port.
*/

#define STRING_WITH_LEN(X) (X), (static_cast<size_t>((sizeof(X) - 1)))


int main(int args, char *argv[])
{
  boost::program_options::options_description desc("Options");
  std::string host;
  std::string port;

  desc.add_options()
    ("help", "Options related to the program.")
    ("host,h", boost::program_options::value<std::string>(&host)->default_value("localhost"),"Connect to the host")
    ("port,p", boost::program_options::value<std::string>(&port)->default_value(GEARMAN_DEFAULT_TCP_PORT_STRING), "Port number or service to use for connection")
    ("server-version", "Fetch the version number for the server.")
    ("server-verbose", "Fetch the verbose setting for the server.")
    ("create-function",  boost::program_options::value<std::string>(), "Create the function from the server.") 
    ("drop-function",  boost::program_options::value<std::string>(), "Drop the function from the server.")
    ("status", "Status for the server.")
    ("workers", "Workers for the server.")
    ("shutdown", "Shutdown server.")
            ;

  boost::program_options::variables_map vm;
  try
  {
    boost::program_options::store(boost::program_options::parse_command_line(args, argv, desc), vm);
    boost::program_options::notify(vm);
  }
  catch(std::exception &e)
  { 
    std::cerr <<  argv[0] << " : " << e.what() << std::endl;
    std::cerr <<  std::endl << desc << std::endl;
    return EXIT_FAILURE;
  }

  Instance instance;

  instance.set_host(host);
  instance.set_port(port);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }

  if (vm.count("shutdown"))
  {
    instance.push(new Operation(STRING_WITH_LEN("shutdown\r\n"), false));
  }

  if (vm.count("status"))
  {
    instance.push(new Operation(STRING_WITH_LEN("status\r\n")));
  }

  if (vm.count("server-version"))
  {
    instance.push(new Operation(STRING_WITH_LEN("version\r\n")));
  }

  if (vm.count("server-verbose"))
  {
    instance.push(new Operation(STRING_WITH_LEN("verbose\r\n")));
  }

  if (vm.count("drop-function"))
  {
    std::string execute(STRING_WITH_LEN("drop function "));
    execute.append(vm["drop-function"].as<std::string>());
    execute.append("\r\n");
    instance.push(new Operation(execute.c_str(), execute.size()));
  }

  if (vm.count("create-function"))
  {
    std::string execute(STRING_WITH_LEN("create function "));
    execute.append(vm["create-function"].as<std::string>());
    execute.append("\r\n");
    instance.push(new Operation(execute.c_str(), execute.size()));
  }

  instance.run();

  return EXIT_SUCCESS;
}
