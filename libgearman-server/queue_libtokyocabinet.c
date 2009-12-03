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
  if (strlen(function_name) != function_name_size ||
      strchr(function_name, '\n') != NULL)
     return GEARMAN_QUEUE_ERROR;
  
  switch (priority)
  {
   case GEARMAN_JOB_PRIORITY_HIGH:
   case GEARMAN_JOB_PRIORITY_MAX:     
     key= tcxstrnew2("0");
     break;
   case GEARMAN_JOB_PRIORITY_LOW:
     key= tcxstrnew2("2");
     break;
   case GEARMAN_JOB_PRIORITY_NORMAL:
   default:
     key= tcxstrnew2("1");
  }
  tcxstrcat(key, "\n", 1);
  tcxstrcat(key, unique, (int)unique_size);
  job_data= tcxstrnew2(function_name);
  tcxstrcat(job_data, "\n", 1);
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
  
  key= tcxstrnew2("1\n");
  tcxstrcat(key, unique, (int)unique_size);
  if (!(rc= tcbdbout3(queue->db, tcxstrptr(key), tcxstrsize(key))))
  {
     tcxstrclear(key);
     tcxstrcat(key, "0\n", 2);
     tcxstrcat(key, unique, (int)unique_size);
     if (!(rc= tcbdbout3(queue->db, tcxstrptr(key), tcxstrsize(key))))
     {  
        tcxstrclear(key);
        tcxstrcat(key, "2\n", 2);
        tcxstrcat(key, unique, (int)unique_size);
        rc= tcbdbout3(queue->db, tcxstrptr(key), tcxstrsize(key));
     }
  }
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
  const char *key_cstr;
  size_t key_cstr_size;
  const char *data_cstr;
  size_t data_cstr_size;
  const char *delim;
  const char *function_name;
  size_t function_name_size;
  const char *function_data;
  size_t function_data_size;
  char *function_data_copy;
  const char *unique;
  size_t unique_size;
  gearman_job_priority_t priority;
  gearman_return_t gret;
  
  GEARMAN_SERVER_DEBUG(server, "replaying: %s", (char *) tcxstrptr(key));
   
  key_cstr= tcxstrptr(key);
  key_cstr_size= (size_t)tcxstrsize(key);

  if (key_cstr_size <= (size_t) 2U) 
    return GEARMAN_QUEUE_ERROR;
  if (*key_cstr == '2')
    priority = GEARMAN_JOB_PRIORITY_LOW;
  else if (*key_cstr == '0')
    priority = GEARMAN_JOB_PRIORITY_HIGH;
  else
    priority = GEARMAN_JOB_PRIORITY_NORMAL;
  delim= strchr(key_cstr, '\n');
  if (delim == NULL || delim == key_cstr)
    return GEARMAN_QUEUE_ERROR;
  unique= delim + 1;
  if (*unique == 0)
    return GEARMAN_QUEUE_ERROR;
  unique_size= key_cstr_size - (size_t) 2U;

  data_cstr= tcxstrptr(data);
  data_cstr_size= (size_t)tcxstrsize(data);

  function_name= data_cstr;
  delim= strchr(data_cstr, '\n');
  if (delim == NULL || delim == function_name)
    return GEARMAN_QUEUE_ERROR;
  function_name_size= (size_t) (delim - function_name);
  function_data= delim + 1;
  function_data_size= (size_t) (data_cstr_size - function_name_size) - 1U;
  if ((function_data_copy = malloc(function_data_size + 1U)) == NULL)
  {
    GEARMAN_SERVER_ERROR_SET(server, "_callback_for_record", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }
  if (function_data_size > (size_t) 0U) 
     memcpy(function_data_copy, function_data, function_data_size);
  *(function_data_copy + function_data_size) = 0;
  gret = (*add_fn)(server, add_context, unique, unique_size,
                   function_name, function_name_size,
                   function_data_copy, function_data_size,
                   priority);
  if (gret != GEARMAN_SUCCESS)
  {
     free(function_data_copy);
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
      if (!tcbdbcurnext(cur))
         break;
    }
    else
    {     
      if (!tcbdbcurout(cur))
         break;
    }     
  }
  tcxstrdel(key);
  tcxstrdel(data);
  tcbdbcurdel(cur);
  tcbdbsync(queue->db);
  tcbdboptimize(queue->db, 0, 0, 0, -1, -1, UINT8_MAX);
   
  return gret;
}
