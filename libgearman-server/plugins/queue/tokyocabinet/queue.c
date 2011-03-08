/**
 * @file
 * @brief Tokyo Cabinet Queue Storage Definitions
 */

#include <libgearman-server/common.h>

#include <libgearman-server/plugins/queue/tokyocabinet/queue.h>

#include <tcutil.h>
#include <tcadb.h>

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
  TCADB *db;
} gearman_queue_libtokyocabinet_st;

/* Queue callback functions. */
static gearmand_error_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const void *unique,
                                             size_t unique_size,
                                             const void *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearmand_job_priority_t priority);
static gearmand_error_t _libtokyocabinet_flush(gearman_server_st *server, void *context);
static gearmand_error_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const void *unique,
                                              size_t unique_size, 
                                              const void *function_name, 
                                              size_t function_name_size);
static gearmand_error_t _libtokyocabinet_replay(gearman_server_st *server, void *context,
                                                gearman_queue_add_fn *add_fn,
                                                void *add_context);

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

/*
 * Public definitions
 */

gearmand_error_t gearman_server_queue_libtokyocabinet_conf(gearman_conf_st *conf)
{
  gearman_conf_module_st *module;

  module= gearman_conf_module_create(conf, NULL, "libtokyocabinet");
  if (module == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  gearman_conf_module_add_option(module, "file", 0, "FILE_NAME",
                                 "File name of the database. [see: man tcadb, tcadbopen() for name guidelines]");
  gearman_conf_module_add_option(module, "optimize", 0, "yes/no",
                                 "Optimize database on open. [default=yes]");
  return gearman_conf_return(conf);
}

gearmand_error_t gearman_queue_libtokyocabinet_init(gearman_server_st *server,
                                                    gearman_conf_st *conf)
{
  gearman_queue_libtokyocabinet_st *queue;
  gearman_conf_module_st *module;
  const char *name;
  const char *value;
  const char *opt_file= NULL;
  const char *opt_optimize= NULL;

  gearmand_log_info("Initializing libtokyocabinet module");

  queue= calloc(1, sizeof(gearman_queue_libtokyocabinet_st));
  if (queue == NULL)
  {
    gearmand_log_error("gearman_queue_libtokyocabinet_init", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  if ((queue->db= tcadbnew()) == NULL)
  {
    free(queue);
    gearmand_log_error("gearman_queue_libtokyocabinet_init",
                             "tcadbnew");
    return GEARMAN_QUEUE_ERROR;
  }
   
  /* Get module and parse the option values that were given. */
  module= gearman_conf_module_find(conf, "libtokyocabinet");
  if (module == NULL)
  {
    gearmand_log_error("gearman_queue_libtokyocabinet_init",
                              "modconf_module_find:NULL");
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
      tcadbdel(queue->db);
      free(queue);
      gearmand_log_error("gearman_queue_libtokyocabinet_init",
                               "Unknown argument: %s", name);
      return GEARMAN_QUEUE_ERROR;
    }
  }
     
  if (opt_file == NULL)
  {
    gearmand_log_error("gearman_queue_libtokyocabinet_init",
                             "No --file given");
    return GEARMAN_QUEUE_ERROR;
  }

  if (!tcadbopen(queue->db, opt_file))
  {
    tcadbdel(queue->db);
    free(queue);

    gearmand_log_error("gearman_queue_libtokyocabinet_init",
                             "tcadbopen(%s): %s", opt_file, _libtokyocabinet_tcaerrmsg(queue->db));

    return GEARMAN_QUEUE_ERROR;
  }

  if (opt_optimize == NULL || !strcasecmp(opt_optimize, "yes"))
  {
    gearmand_log_info("libtokyocabinet optimizing database file");
    if (!tcadboptimize(queue->db, NULL))
    {
      tcadbdel(queue->db);
      free(queue);
      gearmand_log_error("gearman_queue_libtokyocabinet_init",
                               "tcadboptimize");

      return GEARMAN_QUEUE_ERROR;
    }
  }

  gearman_server_set_queue(server, queue, _libtokyocabinet_add, _libtokyocabinet_flush, _libtokyocabinet_done, _libtokyocabinet_replay);   
   
  return GEARMAN_SUCCESS;
}
   
gearmand_error_t gearman_queue_libtokyocabinet_deinit(gearman_server_st *server)
{
  gearman_queue_libtokyocabinet_st *queue;

  gearmand_log_info("Shutting down libtokyocabinet queue module");

  queue= (gearman_queue_libtokyocabinet_st *)gearman_server_queue_context(server);
  gearman_server_set_queue(server, NULL, NULL, NULL, NULL, NULL);
  tcadbdel(queue->db);

  free(queue);

  return GEARMAN_SUCCESS;
}

gearmand_error_t gearmand_queue_libtokyocabinet_init(gearmand_st *gearmand,
                                                     gearman_conf_st *conf)
{
  return gearman_queue_libtokyocabinet_init(gearmand_server(gearmand), conf);
}

gearmand_error_t gearmand_queue_libtokyocabinet_deinit(gearmand_st *gearmand)
{
  return gearman_queue_libtokyocabinet_deinit(gearmand_server(gearmand));
}

/*
 * Private definitions
 */

static gearmand_error_t _libtokyocabinet_add(gearman_server_st *server, void *context,
                                             const void *unique,
                                             size_t unique_size,
                                             const void *function_name,
                                             size_t function_name_size,
                                             const void *data, size_t data_size,
                                             gearmand_job_priority_t priority)
{
  (void)server;
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
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
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
   
  gearmand_log_debug("libtokyocabinet flush");

  if (!tcadbsync(queue->db))
     return GEARMAN_QUEUE_ERROR;
   
  return GEARMAN_SUCCESS;
}

static gearmand_error_t _libtokyocabinet_done(gearman_server_st *server, void *context,
                                              const void *unique,
                                              size_t unique_size, 
                                              const void *function_name,
                                              size_t function_name_size)
{
  (void)server;
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
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
  gearman_queue_libtokyocabinet_st *queue= (gearman_queue_libtokyocabinet_st *)context;
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
