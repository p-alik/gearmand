/**
 * @file
 * @brief Tokyo Cabinet Queue Storage Definitions
 */

#include "common.h"

#include <libgearman-server/queue_libtokyocabinet.h>
#include <tcutil.h>
#include <tcbdb.h>

/**
 * @addtogroup gearman_queue_libtokyocabinet libtokyocabinet Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/*
 * Private declarations
 */

/**
 * Structure for libtokyocabinet specific data.
 */
typedef struct
{
  TCBDB *db;
} gearman_queue_libtokyocabinet_st;

/* Queue callback functions. */
static gearman_return_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const void *unique,
                                             size_t unique_size,
                                             const void *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearman_job_priority_t priority);
static gearman_return_t _libtokyocabinet_flush(gearman_server_st *server, void *context);
static gearman_return_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const void *unique,
                                              size_t unique_size, 
                                              const void *function_name, 
                                              size_t function_name_size);
static gearman_return_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context);

/*
 * Public definitions
 */

gearman_return_t gearman_server_queue_libtokyocabinet_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "libtokyocabinet");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  gearman_conf_module_add_option(module, "file", 0, "FILE_NAME",
                                 "File name of the database.");
  gearman_conf_module_add_option(module, "optimize", 0, "yes/no",
                                 "Optimize database on open. [default=yes]");
  return gearman_conf_return(conf);
}

gearman_return_t gearman_queue_libtokyocabinet_init(gearman_server_st *server,
                                                    gearman_conf_st *conf)
{
  gearman_queue_libtokyocabinet_st *queue;
  gearman_conf_module_st *module;
  const char *name;
  const char *value;
  const char *opt_file= NULL;
  const char *opt_optimize= NULL;

  GEARMAN_SERVER_INFO(server, "Initializing libtokyocabinet module")

  queue= calloc(1, sizeof(gearman_queue_libtokyocabinet_st));
  if (queue == NULL)
  {
    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  if ((queue->db= tcbdbnew()) == NULL)
  {
    free(queue);
    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "tcbdbnew")
    return GEARMAN_QUEUE_ERROR;
  }
   
  tcbdbsetdfunit(queue->db, 8);

  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libtokyocabinet");
  if (module == NULL)
  {
    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                              "modconf_module_find:NULL")
    return GEARMAN_QUEUE_ERROR;
  }

  while (gearman_conf_module_value(module, &name, &value))
  { 
    if (!strcmp(name, "file"))
      opt_file= value;
    else if (!strcmp(name, "optimize"))
      opt_optimize= value;
    else
    {
      tcbdbdel(queue->db);
      free(queue);
      GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                               "Unknown argument: %s", name)
      return GEARMAN_QUEUE_ERROR;
    }
  }
     
  if (opt_file == NULL)
  {
    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "No --file given")
    return GEARMAN_QUEUE_ERROR;
  }

#ifdef HAVE_EVENT_BASE_NEW
  if (!tcbdbsetmutex(queue->db))
  {
    tcbdbdel(queue->db);
    free(queue);

    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "tcbdbsetmutex")
    return GEARMAN_QUEUE_ERROR;
  }   
#endif
   
  if (!tcbdbopen(queue->db, opt_file, BDBOWRITER | BDBOCREAT))
  {
    tcbdbdel(queue->db);
    free(queue);

    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "tcbdbopen")

    return GEARMAN_QUEUE_ERROR;
  }

  if (opt_optimize == NULL || !strcasecmp(opt_optimize, "yes"))
  {
    GEARMAN_SERVER_INFO(server, "libtokyocabinet optimizing database file");
    if (!tcbdboptimize(queue->db, 0, 0, 0, -1, -1, UINT8_MAX))
    {
      tcbdbdel(queue->db);
      free(queue);
      GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                               "tcbdboptimize")

      return GEARMAN_QUEUE_ERROR;
    }
  }

  gearman_server_set_queue_context(server, queue);

  gearman_server_set_queue_add_fn(server, _libtokyocabinet_add);
  gearman_server_set_queue_flush_fn(server, _libtokyocabinet_flush);
  gearman_server_set_queue_done_fn(server, _libtokyocabinet_done);
  gearman_server_set_queue_replay_fn(server, _libtokyocabinet_replay);   
   
  return GEARMAN_SUCCESS;
}
   
gearman_return_t gearman_queue_libtokyocabinet_deinit(gearman_server_st *server)
{
  gearman_queue_libtokyocabinet_st *queue;

  GEARMAN_SERVER_INFO(server, "Shutting down libtokyocabinet queue module");

  queue= (gearman_queue_libtokyocabinet_st *)gearman_server_queue_context(server);
  gearman_server_set_queue_context(server, NULL);
  tcbdbdel(queue->db);

  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                     gearman_conf_st *conf)
{
  return gearman_queue_libtokyocabinet_init(&(gearmand->server), conf);
}

gearman_return_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *gearmand)
{
  return gearman_queue_libtokyocabinet_deinit(&(gearmand->server));
}

/*
 * Private definitions
 */

static gearman_return_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const void *unique,
                                             size_t unique_size,
                                             const void *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearman_job_priority_t priority)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
  bool rc;
  TCXSTR *key;
  TCXSTR *job_data;

  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet add: %.*s",
                       (uint32_t)unique_size, (char *)unique);
  
  char key_str[GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN];
  size_t key_length= (size_t)snprintf(key_str, GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN, "%.*s-%.*s",
                               (int)function_name_size,
                               (const char *)function_name, (int)unique_size,
                               (const char *)unique);

  key= tcxstrnew();
  tcxstrcat(key, key_str, (int)key_length);

  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet key: %.*s", (int)key_length, key_str);

  job_data= tcxstrnew();

  tcxstrcat(job_data, (const char *)function_name, (int)function_name_size);
  tcxstrcat(job_data, "\0", 1);
  tcxstrcat(job_data, (const char *)unique, (int)unique_size);
  tcxstrcat(job_data, "\0", 1);

  switch (priority)
  {
   case GEARMAN_JOB_PRIORITY_HIGH:
   case GEARMAN_JOB_PRIORITY_MAX:     
     tcxstrcat2(job_data,"0");
     break;
   case GEARMAN_JOB_PRIORITY_LOW:
     tcxstrcat2(job_data,"2");
     break;
   case GEARMAN_JOB_PRIORITY_NORMAL:
   default:
     tcxstrcat2(job_data,"1");
  }

  tcxstrcat(job_data, (const char *)data, (int)data_size);

  rc= tcbdbput(queue->db, tcxstrptr(key), tcxstrsize(key),
               tcxstrptr(job_data), tcxstrsize(job_data));

  tcxstrdel(key);
  tcxstrdel(job_data);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libtokyocabinet_flush(gearman_server_st *server,
                                               void *context __attribute__((unused)))
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
   
  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet flush");
  if (!tcbdbsync(queue->db))
     return GEARMAN_QUEUE_ERROR;
   
  return GEARMAN_SUCCESS;
}

static gearman_return_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const void *unique,
                                              size_t unique_size, 
                                              const void *function_name,
                                              size_t function_name_size)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
  bool rc;
  TCXSTR *key;

  (void) function_name;
  (void) function_name_size;   
  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet add: %.*s",
                       (uint32_t)unique_size, (char *)unique);
  
  char key_str[GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN];
  size_t key_length= (size_t)snprintf(key_str, GEARMAN_QUEUE_TOKYOCABINET_MAX_KEY_LEN, "%.*s-%.*s",
                               (int)function_name_size,
                               (const char *)function_name, (int)unique_size,
                               (const char *)unique);

  key= tcxstrnew();
  tcxstrcat(key, key_str, (int)key_length);
  rc= tcbdbout3(queue->db, tcxstrptr(key), tcxstrsize(key));
  tcxstrdel(key);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _callback_for_record(gearman_server_st *server,
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
  gearman_job_priority_t priority;
  gearman_return_t gret;
  
  GEARMAN_SERVER_DEBUG(server, "replaying: %s", (char *) tcxstrptr(key));

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
    priority = GEARMAN_JOB_PRIORITY_LOW;
  else if (*data_cstr == '0')
    priority = GEARMAN_JOB_PRIORITY_HIGH;
  else
    priority = GEARMAN_JOB_PRIORITY_NORMAL;

  ++data_cstr;
  --data_cstr_size;

  // data is freed later so we must make a copy
  void *data_ptr= malloc(data_cstr_size);
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


static gearman_return_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
  TCXSTR *key;
  TCXSTR *data;
  BDBCUR *cur;
  gearman_return_t gret;
  gearman_return_t tmp_gret;   
   
  GEARMAN_SERVER_INFO(server, "libtokyocabinet replay start")
  
  cur= tcbdbcurnew(queue->db);
  if (!cur)
    return GEARMAN_QUEUE_ERROR;

  if (!tcbdbcurfirst(cur))
  {
    tcbdbcurdel(cur);
    return GEARMAN_SUCCESS;
  }
  key= tcxstrnew();
  data= tcxstrnew();
  gret= GEARMAN_SUCCESS;
  while (tcbdbcurrec(cur, key, data))
  {     
    tmp_gret= _callback_for_record(server, key, data, add_fn, add_context);
    if (tmp_gret != GEARMAN_SUCCESS)
    {
      gret= GEARMAN_QUEUE_ERROR;
      break;
    }
    if (!tcbdbcurnext(cur))
      break;
  }
  tcxstrdel(key);
  tcxstrdel(data);
  tcbdbcurdel(cur);

  return gret;
}
