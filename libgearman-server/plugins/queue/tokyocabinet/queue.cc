/**
 * @file
 * @brief Tokyo Cabinet Queue Storage Definitions
 */

#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/tokyocabinet/queue.h>
#include <libgearman-server/plugins/queue/base.h>

#include <tcutil.h>
#include <tcadb.h>

namespace gearmand { namespace plugins { namespace queue { class TokyoCabinet;  }}}

/**
 * It is unclear from tokyocabinet's public headers what, if any, limit there is. 4k seems sane.
 */

#define GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN 4096
gearmand_error_t _initialize(gearman_server_st *server,
                             gearmand::plugins::queue::TokyoCabinet *queue);

namespace gearmand {
namespace plugins {
namespace queue {

class TokyoCabinet : public Queue {
public:
  TokyoCabinet();
  ~TokyoCabinet();

  gearmand_error_t initialize();

  TCADB *db;
  std::string filename;
  bool optimize;
private:
};

TokyoCabinet::TokyoCabinet() :
  Queue("tokyocabinet")
{
  command_line_options().add_options()
    ("libtokyocabinet-file", boost::program_options::value(&filename), "File name of the database. [see: man tcadb, tcadbopen() for name guidelines]")
    ("libtokyocabinet-optimize", boost::program_options::value(&optimize)->default_value(true), "Optimize database on open. [default=true]");

  db= tcadbnew();
}

TokyoCabinet::~TokyoCabinet()
{
  tcadbdel(db);
}

gearmand_error_t TokyoCabinet::initialize()
{
  return _initialize(&Gearmand()->server, this);
}

void initialize_tokyocabinet()
{
  static TokyoCabinet local_instance;
}

} // namespace queue
} // namespace plugin
} // namespace gearmand


/**
 * @addtogroup gearman_queue_libtokyocabinet libtokyocabinet Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/*
 * Private declarations
 */

/* Queue callback functions. */
static gearmand_error_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const char *unique,
                                             size_t unique_size,
                                             const char *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearmand_job_priority_t priority);
static gearmand_error_t _libtokyocabinet_flush(gearman_server_st *server, void *context);
static gearmand_error_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const char *unique,
                                              size_t unique_size, 
                                              const char *function_name, 
                                              size_t function_name_size);
static gearmand_error_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context);

#pragma GCC diagnostic ignored "-Wold-style-cast"

/**
 * Missing function from tcadb.c ??
 */
static const char * _libtokyocabinet_tcaerrmsg(TCADB *db)
{
  switch (tcadbomode(db))
  {
  case ADBOHDB:
    return tcerrmsg(tchdbecode((TCHDB *)tcadbreveal(db)));
  case ADBOBDB:
    return tcerrmsg(tcbdbecode((TCBDB *)tcadbreveal(db)));
  default:
    return tcerrmsg(TCEMISC);
  }
}

gearmand_error_t _initialize(gearman_server_st *server,
                             gearmand::plugins::queue::TokyoCabinet *queue)
{
  gearmand_log_info("Initializing libtokyocabinet module");

  if ((queue->db= tcadbnew()) == NULL)
  {
    gearmand_log_error("_initialize",
                             "tcadbnew");
    return GEARMAN_QUEUE_ERROR;
  }
     
  if (queue->filename.empty())
  {
    gearmand_log_error("_initialize", "No --file given");
    return GEARMAN_QUEUE_ERROR;
  }

  if (not tcadbopen(queue->db, queue->filename.c_str()))
  {
    tcadbdel(queue->db);

    gearmand_log_error("_initialize",
                       "tcadbopen(%s): %s", queue->filename.c_str(), _libtokyocabinet_tcaerrmsg(queue->db));

    return GEARMAN_QUEUE_ERROR;
  }

  if (queue->optimize)
  {
    gearmand_log_info("libtokyocabinet optimizing database file");
    if (not tcadboptimize(queue->db, NULL))
    {
      tcadbdel(queue->db);
      gearmand_log_error("_initialize", "tcadboptimize");

      return GEARMAN_QUEUE_ERROR;
    }
  }

  gearman_server_set_queue(server, queue, _libtokyocabinet_add, _libtokyocabinet_flush, _libtokyocabinet_done, _libtokyocabinet_replay);   
   
  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearmand_error_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const char *unique,
                                             size_t unique_size,
                                             const char *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearmand_job_priority_t priority)
{
  (void)server;
  gearmand::plugins::queue::TokyoCabinet *queue= (gearmand::plugins::queue::TokyoCabinet *)context;
  bool rc;
  TCXSTR *key;
  TCXSTR *job_data;

  gearmand_log_debug("libtokyocabinet add: %.*s", (uint32_t)unique_size, (char *)unique);

  char key_str[GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN];
  size_t key_length= (size_t)snprintf(key_str, GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN, "%.*s-%.*s",
                               (int)function_name_size,
                               (const char *)function_name, (int)unique_size,
                               (const char *)unique);

  key= tcxstrnew();
  tcxstrcat(key, key_str, (int)key_length);

  gearmand_log_debug("libtokyocabinet key: %.*s", (int)key_length, key_str);

  job_data= tcxstrnew();

  tcxstrcat(job_data, (const char *)function_name, (int)function_name_size);
  tcxstrcat(job_data, "\0", 1);
  tcxstrcat(job_data, (const char *)unique, (int)unique_size);
  tcxstrcat(job_data, "\0", 1);

  switch (priority)
  {
   case GEARMAND_JOB_PRIORITY_HIGH:
   case GEARMAND_JOB_PRIORITY_MAX:     
     tcxstrcat2(job_data,"0");
     break;
   case GEARMAND_JOB_PRIORITY_LOW:
     tcxstrcat2(job_data,"2");
     break;
   case GEARMAND_JOB_PRIORITY_NORMAL:
   default:
     tcxstrcat2(job_data,"1");
  }

  tcxstrcat(job_data, (const char *)data, (int)data_size);

  rc= tcadbput(queue->db, tcxstrptr(key), tcxstrsize(key),
               tcxstrptr(job_data), tcxstrsize(job_data));

  tcxstrdel(key);
  tcxstrdel(job_data);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libtokyocabinet_flush(gearman_server_st *server,
                                               void *context __attribute__((unused)))
{
  (void)server;
  gearmand::plugins::queue::TokyoCabinet *queue= (gearmand::plugins::queue::TokyoCabinet *)context;
   
  gearmand_log_debug("libtokyocabinet flush");

  if (!tcadbsync(queue->db))
     return GEARMAN_QUEUE_ERROR;
   
  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const char *unique,
                                              size_t unique_size, 
                                              const char *function_name,
                                              size_t function_name_size)
{
  (void)server;
  gearmand::plugins::queue::TokyoCabinet *queue= (gearmand::plugins::queue::TokyoCabinet *)context;
  bool rc;
  TCXSTR *key;

  (void) function_name;
  (void) function_name_size;   
  gearmand_log_debug("libtokyocabinet add: %.*s", (uint32_t)unique_size, (char *)unique);
  
  char key_str[GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN];
  size_t key_length= (size_t)snprintf(key_str, GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN, "%.*s-%.*s",
                                      (int)function_name_size,
                                      (const char *)function_name, (int)unique_size,
                                      (const char *)unique);

  key= tcxstrnew();
  tcxstrcat(key, key_str, (int)key_length);
  rc= tcadbout(queue->db, tcxstrptr(key), tcxstrsize(key));
  tcxstrdel(key);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearmand_error_t _callback_for_record(gearman_server_st *server,
                                             TCXSTR *key, TCXSTR *data,
                                             gearman_queue_add_fn *add_fn,
                                             void *add_context)
{
  char *data_cstr;
  size_t data_cstr_size;
  const char *function;
  size_t function_len;
  char *unique;
  size_t unique_len;
  gearmand_job_priority_t priority;
  gearmand_error_t gret;
  
  gearmand_log_debug("replaying: %s", (char *) tcxstrptr(key));

  data_cstr= (char *)tcxstrptr(data);
  data_cstr_size= (size_t)tcxstrsize(data);

  function= data_cstr;
  function_len= strlen(function);

  unique= data_cstr+function_len+1;
  unique_len= strlen(unique); // strlen is only safe because tcxstrptr guarantees nul term

  // +2 for nulls
  data_cstr += unique_len+function_len+2;
  data_cstr_size -= unique_len+function_len+2;

  assert(unique);
  assert(unique_len);
  assert(function);
  assert(function_len);

  // single char for priority
  if (*data_cstr == '2')
    priority = GEARMAND_JOB_PRIORITY_LOW;
  else if (*data_cstr == '0')
    priority = GEARMAND_JOB_PRIORITY_HIGH;
  else
    priority = GEARMAND_JOB_PRIORITY_NORMAL;

  ++data_cstr;
  --data_cstr_size;

  // data is freed later so we must make a copy
  void *data_ptr= (void *)malloc(data_cstr_size);
  if (data_ptr == NULL)
  {
    return GEARMAN_QUEUE_ERROR;
  }
  memcpy(data_ptr, data_cstr, data_cstr_size); 

  gret = (*add_fn)(server, add_context, unique, unique_len,
                   function, function_len,
                   data_ptr, data_cstr_size,
                   priority);

  if (gret != GEARMAN_SUCCESS)
  {
     return gret;
  }   
  return GEARMAN_SUCCESS;
}


static gearmand_error_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context)
{
  gearmand::plugins::queue::TokyoCabinet *queue= (gearmand::plugins::queue::TokyoCabinet *)context;
  TCXSTR *key;
  TCXSTR *data;
  void *iter= NULL;
  int iter_size= 0;
  gearmand_error_t gret;
  gearmand_error_t tmp_gret;   
   
  gearmand_log_info("libtokyocabinet replay start");
  
  if (!tcadbiterinit(queue->db))
  {
    return GEARMAN_QUEUE_ERROR;
  }
  key= tcxstrnew();
  data= tcxstrnew();
  gret= GEARMAN_SUCCESS;
  uint64_t x= 0;
  while ((iter= tcadbiternext(queue->db, &iter_size)))
  {     
    tcxstrclear(key);
    tcxstrclear(data);
    tcxstrcat(key, iter, iter_size);
    free(iter);
    iter= tcadbget(queue->db, tcxstrptr(key), tcxstrsize(key), &iter_size);
    if (! iter) {
      gearmand_log_info("libtokyocabinet replay key disappeared: %s", (char *)tcxstrptr(key));
      continue;
    }
    tcxstrcat(data, iter, iter_size);
    free(iter);
    tmp_gret= _callback_for_record(server, key, data, add_fn, add_context);
    if (tmp_gret != GEARMAN_SUCCESS)
    {
      gret= GEARMAN_QUEUE_ERROR;
      break;
    }
    ++x;
  }
  tcxstrdel(key);
  tcxstrdel(data);

  gearmand_log_info("libtokyocabinet replayed %ld records", x);

  return gret;
}
