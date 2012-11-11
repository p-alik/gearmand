/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2012 Sven Nierlein, sven@consol.de
 *  All rights reserved.
 *
 */


/**
 * @file
 * @brief Retention Queue Storage Definitions
 */

#include <gear_config.h>
#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/retention/queue.h>
#include <libgearman-server/plugins/queue/base.h>

#include <stdio.h>

namespace gearmand { namespace plugins { namespace queue { class Retention; }}}

#pragma GCC diagnostic ignored "-Wold-style-cast"

static gearmand_error_t _initialize(gearman_server_st& server, gearmand::plugins::queue::Retention *queue);

namespace gearmand {
namespace plugins {
namespace queue {

class Retention : public gearmand::plugins::Queue {
public:
  Retention();
  ~Retention();
  gearmand_error_t initialize();
  std::string retention_file;

private:
};

Retention::Retention() :
  Queue("retention")
{
    command_line_options().add_options()
      ("retention-file", boost::program_options::value(&retention_file)->default_value("/tmp/retention.dat"), "retention file to save jobs on shutdown");
}

Retention::~Retention()
{
}

gearmand_error_t Retention::initialize()
{
  return _initialize(Gearmand()->server, this);
}


void initialize_retention()
{
  static Retention local_instance;
}

} // namespace queue
} // namespace plugins
} // namespace gearmand



/**
 * @addtogroup gearman_queue_retention_static Static retention Queue Storage Definitions
 * @ingroup gearman_queue_retention
 * @{
 */

/* Queue callback functions. */
static gearmand_error_t __add(gearman_server_st *server __attribute__((unused)),
                              void *context __attribute__((unused)),
                              const char *unique __attribute__((unused)), size_t unique_size __attribute__((unused)),
                              const char *function_name __attribute__((unused)),
                              size_t function_name_size __attribute__((unused)),
                              const void *data __attribute__((unused)), size_t data_size __attribute__((unused)),
                              gearman_job_priority_t priority __attribute__((unused)),
                              int64_t when __attribute__((unused)))
{
  gearmand_debug(__func__);
  return GEARMAN_SUCCESS;
}


static gearmand_error_t __flush(gearman_server_st *server __attribute__((unused)),
                                void *context __attribute__((unused)))
{
  gearmand_debug(__func__);
  return GEARMAN_SUCCESS;
}

static gearmand_error_t __done(gearman_server_st *server __attribute__((unused)),
                               void *context __attribute__((unused)),
                               const char *unique __attribute__((unused)),
                               size_t unique_size __attribute__((unused)),
                               const char *function_name __attribute__((unused)),
                               size_t function_name_size __attribute__((unused)))
{
  gearmand_debug(__func__);
  return GEARMAN_SUCCESS;
}


static gearmand_error_t __replay(gearman_server_st *server,
                                 void *context,
                                 gearman_queue_add_fn *add_fn,
                                 void *add_context)
{
  int read;
  int when;
  int size;
  char *data_line;
  char *data;
  char *function;
  char *unique;
  char *prio_s;
  FILE * fp;
  gearman_job_priority_t priority= (gearman_job_priority_t)0;
  gearmand_error_t ret = GEARMAN_SUCCESS;
  gearmand::plugins::queue::Retention *queue= (gearmand::plugins::queue::Retention *)context;

  gearmand_debug(__func__);
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "reading retention file: %s", queue->retention_file.c_str());

  fp = fopen(queue->retention_file.c_str(), "r");
  if(!fp)
    return GEARMAN_SUCCESS;

  data_line = (char*)malloc(1024);
  if(data_line == NULL){
    gearmand_perror("malloc failed");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }
  while(!feof(fp)) {
    if(fgets(data_line, 1024, fp) == NULL)
      break; // empty files

    function = strsep( &data_line, ";" );
    unique   = strsep( &data_line, ";" );
    prio_s   = strsep( &data_line, ";" );
    priority = (gearman_job_priority_t) atoi(prio_s);
    when     = atoi(strsep( &data_line, ";" ));
    size     = atoi(strsep( &data_line, "\n" ));
    data     = (char*)malloc(size+1);
    if (data == NULL){
      gearmand_perror("malloc failed");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }
    read = fread(data, size, 1, fp);
    ret= (*add_fn)(server, add_context,
                   unique, (size_t) strlen(unique),
                   function, (size_t) strlen(function),
                   data, size,
                   priority,
                   when);
    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_JOB_EXISTS)
      return ret;

    // complete rest of line till newline
    if(fgets(data_line, 1024, fp) == NULL)
      break; // no more jobs
  }

  fclose(fp);

  // cleanup retention file
  if(unlink(queue->retention_file.c_str()) == -1)
    perror(queue->retention_file.c_str());

  return GEARMAN_SUCCESS;
}

static gearmand_error_t __shutdown(gearman_server_st *server,
                                 void *context)
{
  FILE * fp;
  gearmand::plugins::queue::Retention *queue= (gearmand::plugins::queue::Retention *)context;

  gearmand_debug(__func__);
  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "writing retention file: %s for %i jobs", queue->retention_file.c_str(), server->job_count);

  fp = fopen(queue->retention_file.c_str(), "w+");
  if(fp == NULL) {
    gearmand_perror(queue->retention_file.c_str());
    return GEARMAN_ERRNO;
  }

  for (uint32_t key= 0; key < GEARMAND_JOB_HASH_SIZE; key++) {
    if(server->job_hash[key] != NULL) {
      gearman_server_job_st *server_job = server->job_hash[key];
      fprintf(fp, "%s;%s;%i;%" PRId64 ";%i\n%.*s\n",
                  server_job->function->function_name,
                  server_job->unique,
                  (int)server_job->priority,
                  server_job->when,
                  server_job->data_size,
                  server_job->data_size, (const char*)server_job->data);
      while (server_job->unique_next != NULL) {
        server_job = server_job->unique_next;
        fprintf(fp, "%s;%s;%i;%" PRId64 ";%i\n%.*s\n",
                    server_job->function->function_name,
                    server_job->unique,
                    (int)server_job->priority,
                    server_job->when,
                    server_job->data_size,
                    server_job->data_size, (const char*)server_job->data);
      }
    }
  }

  fclose(fp);

  return GEARMAN_SUCCESS;
}

gearmand_error_t _initialize(gearman_server_st& server, gearmand::plugins::queue::Retention *queue)
{
  gearman_server_set_queue(server, queue, __add, __flush, __done, __replay, __shutdown);

  return GEARMAN_SUCCESS;
}
