/* Gearman server and library
 * Copyright (C) 2009 Cory Bennett
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libsqlite3 Queue Storage Definitions
 */

#include <config.h>
#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/sqlite/queue.h>
#include <libgearman-server/plugins/queue/base.h>

#include "libgearman-server/plugins/queue/sqlite/instance.hpp"

/** Default values.
 */
#define GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE "gearman_queue"

namespace gearmand {
namespace plugins {
namespace queue {

class Sqlite : public gearmand::plugins::Queue
{
public:
  Sqlite();
  ~Sqlite();

  gearmand_error_t initialize();

  std::string schema;
  std::string table;

private:
};

Sqlite::Sqlite() :
  Queue("libsqlite3")
{
  command_line_options().add_options()
    ("libsqlite3-db", boost::program_options::value(&schema), "Database file to use.")
    ("libsqlite3-table", boost::program_options::value(&table)->default_value(GEARMAN_QUEUE_SQLITE_DEFAULT_TABLE), "Table to use.")
    ;
}

Sqlite::~Sqlite()
{
}

gearmand_error_t Sqlite::initialize()
{
  gearmand::queue::Instance* exec_queue= new gearmand::queue::Instance(schema, table);

  if (exec_queue == NULL)
  {
  }

  gearmand_error_t rc;
  if ((rc= exec_queue->init()) != GEARMAN_SUCCESS)
  {
    delete exec_queue;
    return rc;
  }
  gearman_server_set_queue(&Gearmand()->server, exec_queue);

  return rc;
}

void initialize_sqlite()
{
  static Sqlite local_instance;
}

} // namespace queue
} // namespace plugins
} // namespace gearmand
