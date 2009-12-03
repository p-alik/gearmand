/**
 * @File
 * @brief Tokyo Cabinet Queue Storage Definitions
 */

#include "common.h"

#include <libgearman-server/queue_libtokyocabinet.h>
#include <tcutil.h>
#include <tchdb.h>

/**
 * @addtogroup gearman_queue_libtokyocabinet libtokyocabinet Queue Storage Functions
 * @ingroup gearman_queue
 * @{
 */

/**
 * Prefix for keys, optional for Tokyo Cabinet queue storage.
 */
/* #define GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX "gear_" */

/*
 * Private declarations
 */

/**
 * Structure for libtokyocabinet specific data.
 */
typedef struct
{
  TCHDB *db;
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

gearman_return_t gearman_queue_libtokyocabinet_conf(gearman_conf_st *conf)
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

  if ((queue->db= tchdbnew()) == NULL)
  {
    free(queue);
    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "tchdbnew")
    return GEARMAN_QUEUE_ERROR;
  }

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
      tchdbdel(queue->db);
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
  if (!tchdbsetmutex(queue->db))
  {
    tchdbdel(queue->db);
    free(queue);

    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                      "tchdbsetmutex")
    return GEARMAN_QUEUE_ERROR;
  }   
#endif
   
  if (!tchdbopen(queue->db, opt_file, HDBOWRITER | HDBOCREAT))
  {
    tchdbdel(queue->db);
    free(queue);

    GEARMAN_SERVER_ERROR_SET(server, "gearman_queue_libtokyocabinet_init",
                             "tchdbopen")

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
  tchdbdel(queue->db);

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

  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet add: %.*s",
                       (uint32_t)unique_size, (char *)unique);

#ifdef GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX
  key= tcxstrnew2(GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX);
#else
  key= tcxstrnew();
#endif
  tcxstrcat(key, function_name, (int)function_name_size);
  tcxstrcat(key, "-", 1);
  tcxstrcat(key, unique, (int)unique_size);
  rc= tchdbputasync(queue->db, tcxstrptr(key), tcxstrsize(key),
                    (const char *)data, (int)data_size);
  (void) priority;               
  tcxstrdel(key);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libtokyocabinet_flush(gearman_server_st *server,
                                               void *context __attribute__((unused)))
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
   
  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet flush");
  if (!tchdbsync(queue->db))
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

  GEARMAN_SERVER_DEBUG(server, "libtokyocabinet add: %.*s",
                       (uint32_t)unique_size, (char *)unique);

#ifdef GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX
  key= tcxstrnew2(GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX);
#else
  key= tcxstrnew();
#endif
  tcxstrcat(key, function_name, (int)function_name_size);
  tcxstrcat(key, "-", 1);
  tcxstrcat(key, unique, (int)unique_size);
  rc= tchdbout(queue->db, tcxstrptr(key), tcxstrsize(key));
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
  char *data_cstr;
  size_t data_cstr_size;
  const char *key_delim;
  const char *function_name;
  size_t function_name_size;
  const char *unique;
  size_t unique_size;
      
  GEARMAN_SERVER_DEBUG(server, "replaying: %s", (char *) tcxstrptr(key));
   
  key_cstr= tcxstrptr(key);
  key_cstr_size= (size_t)tcxstrsize(key);
   
#ifdef GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX
  if (tcxstrsize(key) < sizeof GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX)
    return GEARMAN_QUEUE_ERROR;
  if (memcmp(key_cstr, GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX,
             sizeof GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX - 1))
    return GEARMAN_QUEUE_ERROR;
  key_cstr+= sizeof GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX - 1;
  key_cstr_size-= sizeof GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX - 1;
#endif
  
  key_delim= strchr(key_cstr, '-');
  if (key_delim == NULL || key_delim == key_cstr)
    return GEARMAN_QUEUE_ERROR;
  function_name= key_cstr;
  function_name_size= (size_t) (key_delim - key_cstr);
  unique= key_delim + 1;
  if (*unique == 0)
    return GEARMAN_QUEUE_ERROR;
  unique_size= key_cstr_size - function_name_size - 1;
   
  data_cstr_size= (size_t)tcxstrsize(data);
  if ((data_cstr = malloc(data_cstr_size + 1)) == NULL)
  {
    GEARMAN_SERVER_ERROR_SET(server, "_callback_for_record", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }   
  memcpy(data_cstr, tcxstrptr(data), data_cstr_size + 1);
  (void)(*add_fn)(server, add_context, unique, unique_size,
                  function_name, function_name_size,
                  data_cstr, data_cstr_size, 0);

  return GEARMAN_SUCCESS;
}


static gearman_return_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
  TCXSTR *key;
  TCXSTR *data;
  gearman_return_t gret;
   
  GEARMAN_SERVER_INFO(server, "libtokyocabinet replay start")
  
  if (!tchdbiterinit(queue->db))
    return GEARMAN_QUEUE_ERROR;

  key= tcxstrnew();
  data= tcxstrnew();   
  while (tchdbiternext3(queue->db, key, data))
  {     
    gret= _callback_for_record(server, key, data, add_fn, add_context);
    if (gret != GEARMAN_SUCCESS)
    {
      tcxstrdel(key);
      tcxstrdel(data);
      return gret;
    }
  }
  tcxstrdel(key);
  tcxstrdel(data);
   
  return GEARMAN_SUCCESS;
}
