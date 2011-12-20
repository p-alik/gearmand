/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 * Gearman server and library 
 * Copyright (C) 2011 DataDifferential All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <libgearman/gearman.h>
#include "arguments.h"

namespace gearman_client
{

Args::Args(int p_argc, char *p_argv[]) :
  _host(NULL),
  _port(0),
  _count(0),
  _unique(NULL),
  _job_per_newline(false),
  _strip_newline(false),
  _worker(false),
  _suppress_input(false),
  _prefix(false),
  _background(false),
  _daemon(false),
  _usage(false),
  _is_error(false),
  _priority(GEARMAN_JOB_PRIORITY_NORMAL),
  _timeout(-1),
  argv(p_argv),
  _error(0)
{
  init(p_argc);
}

Args::~Args()
{
}

void Args::init(int argc)
{
  int c;

  opterr= 0;

  while ((c = getopt(argc, argv, "bc:f:h:HILnNp:Pst:u:wi:d")) != -1)
  {
    switch(c)
    {
    case 'b':
      _background= true;
      break;

    case 'c':
      _count= static_cast<uint32_t>(atoi(optarg));
      break;

    case 'd':
      _daemon= true;
      break;

    case 'f':
      add(optarg);
      break;

    case 'h':
      _host= optarg;
      break;

    case 'i':
      _pid_file= optarg;
      break;

    case 'I':
      _priority= GEARMAN_JOB_PRIORITY_HIGH;
      break;

    case 'L':
      _priority= GEARMAN_JOB_PRIORITY_LOW;
      break;

    case 'n':
      _job_per_newline= true;
      break;

    case 'N':
      _job_per_newline= true;
      _strip_newline= true;
      break;

    case 'p':
      _port= static_cast<in_port_t>(atoi(optarg));
      break;

    case 'P':
      _prefix= true;
      break;

    case 's':
      _suppress_input= true;
      break;

    case 't':
      _timeout= atoi(optarg);
      break;

    case 'u':
      _unique= optarg;
      break;

    case 'w':
      _worker= true;
      break;

    case 'H':
      _usage= true;
      return;

    default:
      _is_error= true;
      return;
    }
  }

  argv+= optind;
}

} // namespace gearman_client
