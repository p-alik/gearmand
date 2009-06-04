/**
 * @file
 * @brief Tokyo Cabinet Queue Storage Definitions
 */

#include "common.h"

#include <libgearman/queue_libtokyocabinet.h>
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
static gearman_return_t _libtokyocabinet_add(gearman_st *gearman, void *fn_arg,
					     const void *unique,
					     size_t unique_size,
					     const void *function_name,
					     size_t function_name_size,
					     const void *data, size_t data_size,
					     gearman_job_priority_t priority);
static gearman_return_t _libtokyocabinet_flush(gearman_st *gearman, void *fn_arg);
static gearman_return_t _libtokyocabinet_done(gearman_st *gearman, void *fn_arg,
					      const void *unique,
					      size_t unique_size, 
					      const void *function_name, 
					      size_t function_name_size);
static gearman_return_t _libtokyocabinet_replay(gearman_st *gearman, void *fn_arg,
						gearman_queue_add_fn *add_fn,
						void *add_fn_arg);

/*
 * Public definitions
 */

modconf_return_t gearman_queue_libtokyocabinet_modconf(modconf_st *modconf)
{
  modconf_module_st *module;

  module= gmodconf_module_create(modconf, NULL, "libtokyocabinet");
  if (module == NULL)
    return MODCONF_MEMORY_ALLOCATION_FAILURE;

  gmodconf_module_add_option(module, "file", 0, "FILE_NAME",
                             "File name of the database.");
  return gmodconf_return(modconf);
}

gearman_return_t gearman_queue_libtokyocabinet_init(gearman_st *gearman,
						    modconf_st *modconf)
{
  gearman_queue_libtokyocabinet_st *queue;
  modconf_module_st *module;
  const char *name;
  const char *value;
  const char *opt_file= NULL;

  GEARMAN_INFO(gearman, "Initializing libtokyocabinet module")

  queue= calloc(1, sizeof(gearman_queue_libtokyocabinet_st));
  if (queue == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  if ((queue->db= tchdbnew()) == NULL)
  {
    free(queue);
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                      "tchdbnew")
    return GEARMAN_QUEUE_ERROR;
  }

  /* Get module and parse the option values that were given. */
  module= gmodconf_module_find(modconf, "libtokyocabinet");
  if (module == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                      "modconf_module_find:NULL")
    return GEARMAN_QUEUE_ERROR;
  }

  while (gmodconf_module_value(module, &name, &value))
  { 
    if (!strcmp(name, "file"))
      opt_file= value;
    else
    {
      tchdbdel(queue->db);
      free(queue);
      GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                        "Unknown argument: %s", name)
      return GEARMAN_QUEUE_ERROR;
    }
  }
     
  if (opt_file == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                      "No --file given")
    return GEARMAN_QUEUE_ERROR;
  }
  
#ifdef HAVE_EVENT_BASE_NEW
  if (!tchdbsetmutex(queue->db))
  {
    tchdbdel(queue->db);
    free(queue);

    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                      "tchdbsetmutex")
    return GEARMAN_QUEUE_ERROR;
  }   
#endif
   
  if (!tchdbopen(queue->db, opt_file, HDBOWRITER | HDBOCREAT))
  {
    tchdbdel(queue->db);
    free(queue);

    GEARMAN_ERROR_SET(gearman, "gearman_queue_libtokyocabinet_init",
                      "tchdbopen")

    return GEARMAN_QUEUE_ERROR;
  }

  gearman_set_queue_fn_arg(gearman, queue);

  gearman_set_queue_add(gearman, _libtokyocabinet_add);
  gearman_set_queue_flush(gearman, _libtokyocabinet_flush);
  gearman_set_queue_done(gearman, _libtokyocabinet_done);
  gearman_set_queue_replay(gearman, _libtokyocabinet_replay);   
   
  return GEARMAN_SUCCESS;
}
   
gearman_return_t gearman_queue_libtokyocabinet_deinit(gearman_st *gearman)
{
  gearman_queue_libtokyocabinet_st *queue;

  GEARMAN_INFO(gearman, "Shutting down libtokyocabinet queue module");

  queue= (gearman_queue_libtokyocabinet_st *)gearman_queue_fn_arg(gearman);
  gearman_set_queue_fn_arg(gearman, NULL);
  tchdbdel(queue->db);

  free(queue);

  return GEARMAN_SUCCESS;
}

gearman_return_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                  modconf_st *modconf)
{
  return gearman_queue_libtokyocabinet_init(gearmand->server.gearman, modconf);
}

gearman_return_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *gearmand)
{
  return gearman_queue_libtokyocabinet_deinit(gearmand->server.gearman);
}

/*
 * Private definitions
 */

static gearman_return_t _libtokyocabinet_add(gearman_st *gearman, void *fn_arg,
					     const void *unique,
					     size_t unique_size,
					     const void *function_name,
					     size_t function_name_size,
					     const void *data, size_t data_size,
					     gearman_job_priority_t priority)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)fn_arg;
  bool rc;
  TCXSTR *key;

  GEARMAN_DEBUG(gearman, "libtokyocabinet add: %.*s", (uint32_t)unique_size, (char *)unique);

#ifdef GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX
  key= tcxstrnew2(GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX);
#else
  key= tcxstrnew();
#endif
  tcxstrcat(key, function_name, function_name_size);
  tcxstrcat(key, "-", 1);
  tcxstrcat(key, unique, unique_size);
  rc= tchdbputasync(queue->db, tcxstrptr(key), tcxstrsize(key),
		   (const char *)data, data_size);
  (void) priority;		 
  tcxstrdel(key);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _libtokyocabinet_flush(gearman_st *gearman, void *fn_arg __attribute__((unused)))
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)fn_arg;
   
  GEARMAN_DEBUG(gearman, "libtokyocabinet flush");
  if (!tchdbsync(queue->db))
     return GEARMAN_QUEUE_ERROR;
   
  return GEARMAN_SUCCESS;
}

static gearman_return_t _libtokyocabinet_done(gearman_st *gearman, void *fn_arg,
                                           const void *unique,
                                           size_t unique_size, 
                                           const void *function_name, 
                                           size_t function_name_size)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)fn_arg;
  bool rc;
  TCXSTR *key;

  GEARMAN_DEBUG(gearman, "libtokyocabinet add: %.*s", (uint32_t)unique_size, (char *)unique);

#ifdef GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX
  key= tcxstrnew2(GEARMAN_QUEUE_LIBTOKYOCABINET_DEFAULT_PREFIX);
#else
  key= tcxstrnew();
#endif
  tcxstrcat(key, function_name, function_name_size);
  tcxstrcat(key, "-", 1);
  tcxstrcat(key, unique, unique_size);
  rc= tchdbout(queue->db, tcxstrptr(key), tcxstrsize(key));
  tcxstrdel(key);

  if (!rc)
    return GEARMAN_QUEUE_ERROR;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _callback_for_record(gearman_st *gearman,
					     TCXSTR *key, TCXSTR *data,
					     gearman_queue_add_fn *add_fn,
					     void *add_fn_arg)
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
      
  GEARMAN_DEBUG(gearman, "replaying: %s", (char *) tcxstrptr(key));
   
  key_cstr= tcxstrptr(key);
  key_cstr_size= tcxstrsize(key);
   
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
  function_name_size= key_delim - key_cstr;
  unique= key_delim + 1;
  if (*unique == 0)
    return GEARMAN_QUEUE_ERROR;
  unique_size= key_cstr_size - function_name_size - 1;
   
  data_cstr_size= tcxstrsize(data);
  if ((data_cstr = malloc(data_cstr_size + 1)) == NULL)
  {
    GEARMAN_ERROR_SET(gearman, "_callback_for_record", "malloc")
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }   
  memcpy(data_cstr, tcxstrptr(data), data_cstr_size + 1);
  (void)(*add_fn)(gearman, add_fn_arg, unique, unique_size,
		  function_name, function_name_size,
		  data_cstr, data_cstr_size, 0);

  return GEARMAN_SUCCESS;
}


static gearman_return_t _libtokyocabinet_replay(gearman_st *gearman, void *fn_arg,
						gearman_queue_add_fn *add_fn,
						void *add_fn_arg)
{
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)fn_arg;
  TCXSTR *key;
  TCXSTR *data;
  gearman_return_t gret;
   
  GEARMAN_INFO(gearman, "libtokyocabinet replay start")
  
  if (!tchdbiterinit(queue->db))
    return GEARMAN_QUEUE_ERROR;

  key= tcxstrnew();
  data= tcxstrnew();   
  while (tchdbiternext3(queue->db, key, data))
  {     
    gret= _callback_for_record(gearman, key, data, add_fn, add_fn_arg);
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
