/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief libmemcached Queue Storage Definitions
 */

#include <config.h>
#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/base.h>
#include <libgearman-server/plugins/queue/libmemcached/queue.h>
#include <libmemcached/memcached.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

using namespace gearmand;

/**
 * @addtogroup gearmand::plugins::queue::Libmemcachedatic Static libmemcached Queue Storage Functions
 * @ingroup gearman_queue_libmemcached
 * @{
 */

/**
 * Default values.
 */
#define GEARMAN_QUEUE_LIBMEMCACHED_DEFAULT_PREFIX "gear_"

namespace gearmand { namespace plugins { namespace queue { class Libmemcached;  }}}

namespace gearmand {
namespace queue {

class LibmemcachedQueue : public gearmand::queue::Context 
{
public:
  LibmemcachedQueue(plugins::queue::Libmemcached* arg, memcached_server_st* servers) :
    memc_(NULL),
    clone_(NULL),
    __queue(arg)
  { 
    memc_= memcached_create(NULL);
    assert(memc_);

    memcached_server_push(memc_, servers);
  }

  memcached_st* clone()
  {
    if (clone_)
    {
      clone_= memcached_clone(NULL, memc_);
    }

    return clone_;
  }

  ~LibmemcachedQueue()
  {
    memcached_free(memc_);
    memcached_free(clone_);
  }

  gearmand_error_t add(gearman_server_st *server,
                       const char *unique, size_t unique_size,
                       const char *function_name, size_t function_name_size,
                       const void *data, size_t data_size,
                       gearman_job_priority_t priority,
                       int64_t when)
  {
  }

  gearmand_error_t flush(gearman_server_st *server);

  gearmand_error_t done(gearman_server_st *server,
                        const char *unique, size_t unique_size,
                        const char *function_name, size_t function_name_size);

  gearmand_error_t replay(gearman_server_st *server);

private:
  memcached_st* memc_;
  memcached_st* clone_;
  gearmand::plugins::queue::Libmemcached *__queue;
};

} // namespace queue
} // namespace gearmand

namespace gearmand {
namespace plugins {
namespace queue {

class Libmemcached : public gearmand::plugins::Queue {
public:
  Libmemcached ();
  ~Libmemcached ();

  gearmand_error_t initialize();

  std::string server_list;
private:

};

Libmemcached::Libmemcached() :
  Queue("libmemcached")
{
  command_line_options().add_options()
    ("libmemcached-servers", boost::program_options::value(&server_list), "List of Memcached servers to use.");
}

Libmemcached::~Libmemcached()
{
}

gearmand_error_t Libmemcached::initialize()
{
  gearmand_info("Initializing libmemcached module");

  memcached_server_st *servers= memcached_servers_parse(server_list.c_str());
  if (servers == NULL)
  {
    gearmand_log_error(GEARMAN_DEFAULT_LOG_PARAM, "memcached_servers_parse");

    return GEARMAN_QUEUE_ERROR;
  }

  gearmand::queue::LibmemcachedQueue* exec_queue= new gearmand::queue::LibmemcachedQueue(this, servers);
  gearman_server_set_queue(&Gearmand()->server, exec_queue);

  memcached_server_list_free(servers);

  return GEARMAN_SUCCESS;
}

void initialize_libmemcached()
{
  static Libmemcached local_instance;
}

} // namespace queue
} // namespace plugins
} // namespace gearmand

/* Queue callback functions. */

namespace gearmand {
namespace queue {

gearmand_error_t LibmemcachedQueue::flush(gearman_server_st *)
{
  gearmand_debug("libmemcached flush");

  return GEARMAN_SUCCESS;
}

gearmand_error_t LibmemcachedQueue::done(gearman_server_st*,
                                         const char *unique, size_t unique_size,
                                         const char *function_name, size_t function_name_size)
{
  char key[MEMCACHED_MAX_KEY];

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "libmemcached done: %.*s", (uint32_t)unique_size, (char *)unique);

  size_t key_length= (size_t)snprintf(key, MEMCACHED_MAX_KEY, "%s%.*s-%.*s",
                                      GEARMAN_QUEUE_LIBMEMCACHED_DEFAULT_PREFIX,
                                      (int)function_name_size,
                                      (const char *)function_name, (int)unique_size,
                                      (const char *)unique);

  /* For the moment we will assume it happened */
  memcached_return rc= memcached_delete(memc_, (const char *)key, key_length, 0);
  if (rc != MEMCACHED_SUCCESS)
  {
    return GEARMAN_QUEUE_ERROR;
  }

  return GEARMAN_SUCCESS;
}

class Replay
{
public:
  Replay(LibmemcachedQueue* queue_arg, gearman_server_st* server_arg) :
    queue_(queue_arg),
    server_(server_arg)
  { }

  LibmemcachedQueue* queue()
  {
    return queue_;
  }

  gearman_server_st* server()
  {
    return server_;
  }

private:
  LibmemcachedQueue* queue_;
  gearman_server_st* server_;
};

static memcached_return callback_loader(const memcached_st*,
                                        memcached_result_st* result,
                                        void *context)
{
  Replay* replay= (Replay*)context;

  const char *key= memcached_result_key_value(result);
  if (strncmp(key, GEARMAN_QUEUE_LIBMEMCACHED_DEFAULT_PREFIX, strlen(GEARMAN_QUEUE_LIBMEMCACHED_DEFAULT_PREFIX)))
  {
    return MEMCACHED_SUCCESS;
  }

  const char* function= key +strlen(GEARMAN_QUEUE_LIBMEMCACHED_DEFAULT_PREFIX);

  const char* unique= index(function, '-');
  if (unique == NULL)
  {
    return MEMCACHED_SUCCESS;
  }

  size_t function_len= size_t(unique -function);
  unique++;

  assert(unique);
  assert(strlen(unique));
  assert(function);
  assert(function_len);

  std::vector<char> data;
  /* need to make a copy here ... gearman_server_job_free will free it later */
  try {
    data.resize(memcached_result_length(result));
  }
  catch(...)
  {
    gearmand_perror("malloc");
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  } 
  memcpy(&data[0], memcached_result_value(result), data.size());

  /* Currently not looking at failure cases */
  replay->queue()->add(replay->server(),
                       unique, strlen(unique),
                       function, function_len,
                       &data[0], data.size(),
                       static_cast<gearman_job_priority_t>(memcached_result_flags(result)), int64_t(0));


  return MEMCACHED_SUCCESS;
}

/* Grab the object and load it into the loader */
static memcached_return callback_for_key(const memcached_st*,
                                         const char *key, size_t key_length,
                                         void *context)
{
  LibmemcachedQueue *queue= (LibmemcachedQueue*)context;
  memcached_execute_function callbacks[1];
  char *passable[1];

  callbacks[0]= (memcached_execute_fn)&callback_loader;

  passable[0]= (char *)key;
  memcached_return_t rc= memcached_mget(queue->clone(), passable, &key_length, 1);
  (void)rc;

  /* Just void errors for the moment, since other treads might have picked up the object. */
  (void)memcached_fetch_execute(queue->clone(), callbacks, context, 1);

  return MEMCACHED_SUCCESS;
}

/*
  If we have any failures for loading values back into replay we just ignore them.
*/
gearmand_error_t LibmemcachedQueue::replay(gearman_server_st *server)
{
  memcached_dump_func callbacks[1];

  callbacks[0]= (memcached_dump_fn)&callback_for_key;

  gearmand_info("libmemcached replay start");

  memcached_st* clone= memcached_clone(NULL, memc_);

  if (clone)
  {
    Replay context(this, server);
    (void)memcached_dump(clone, callbacks, (void *)&context, 1);
    memcached_free(clone);
  }

  return GEARMAN_SUCCESS;
}

} // queue
} // gearmand

