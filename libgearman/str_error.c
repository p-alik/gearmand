#include "common.h"

char *gearman_strerror(gearman_st *ptr, gearman_return rc)
{
  /* do something with the ptr, set and ignore for now (compile warnings) */
  ptr = NULL;

  switch (rc)
  {
  case GEARMAN_SUCCESS:
    return "SUCCESS";
  case GEARMAN_NOT_FOUND:
    return "NOT FOUND";
  case GEARMAN_FAILURE:
    return "FAILURE";
  case GEARMAN_HOST_LOOKUP_FAILURE:
    return "HOSTNAME LOOKUP FAILURE";
  case GEARMAN_CONNECTION_FAILURE:
    return "CONNECTION FAILURE";
  case GEARMAN_CONNECTION_BIND_FAILURE:
    return "CONNECTION BIND FAILURE";
  case GEARMAN_READ_FAILURE:
    return "READ FAILURE";
  case GEARMAN_UNKNOWN_READ_FAILURE:
    return "UNKNOWN READ FAILURE";
  case GEARMAN_PROTOCOL_ERROR:
    return "PROTOCOL ERROR";
  case GEARMAN_CLIENT_ERROR:
    return "CLIENT ERROR";
  case GEARMAN_SERVER_ERROR:
    return "SERVER ERROR";
  case GEARMAN_WRITE_FAILURE:
    return "WRITE FAILURE";
  case GEARMAN_CONNECTION_SOCKET_CREATE_FAILURE:
    return "CONNECTION SOCKET CREATE FAILURE";
  case GEARMAN_MEMORY_ALLOCATION_FAILURE:
    return "MEMORY ALLOCATION FAILURE";
  case GEARMAN_PARTIAL_READ:
    return "PARTIAL READ";
  case GEARMAN_SOME_ERRORS:
    return "SOME ERRORS WERE REPORTED";
  case GEARMAN_NO_SERVERS:
    return "NO SERVERS DEFINED";
  case GEARMAN_ERRNO:
    return "SYSTEM ERROR";
  case GEARMAN_NOT_SUPPORTED:
    return "ACTION NOT SUPPORTED";
  case GEARMAN_FETCH_NOTFINISHED:
    return "FETCH WAS NOT COMPLETED";
  case GEARMAN_NO_KEY_PROVIDED:
    return "A KEY LENGTH OF ZERO WAS PROVIDED";
  case GEARMAN_BUFFERED:
    return "ACTION QUEUED";
  case GEARMAN_STILL_RUNNING:
    return "JOB STILL RUNNING";
  case GEARMAN_TIMEOUT:
    return "A TIMEOUT OCCURRED";
  case GEARMAN_BAD_KEY_PROVIDED:
    return "A BAD KEY WAS PROVIDED/CHARACTERS OUT OF RANGE";
  case GEARMAN_MAXIMUM_RETURN:
    return "Gibberish returned!";
  default:
    return "Gibberish returned!";
  };
}
