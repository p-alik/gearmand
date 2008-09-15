#include "common.h"

/*
  We send a string to the server and do a "pass or fail" on whether or not we got it back.
*/

static gearman_return echo_server(gearman_server_st *ptr, char *message, size_t message_length)
{
  gearman_return rc;
  gearman_result_st result;
  gearman_result_st *result_ptr;
  giov_st giov;


  result_ptr= gearman_result_create(ptr->root, &result);

  if (result_ptr == 0)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  giov.arg= message;
  giov.arg_length= message_length;
  rc= gearman_dispatch(ptr, GEARMAN_ECHO_REQ, &giov, 1); 


  if (rc != GEARMAN_SUCCESS)
    return rc;

  rc= gearman_response(ptr, NULL, result_ptr);

  if (rc != GEARMAN_SUCCESS)
    return rc;

  if (result_ptr->action != GEARMAN_ECHO_RES)
    return GEARMAN_PROTOCOL_ERROR;

  if ((gearman_result_length(result_ptr) != message_length) ||
      (memcmp(gearman_result_value(result_ptr), message, message_length) != 0))
    return GEARMAN_FAILURE;

  return GEARMAN_SUCCESS;
}

gearman_return gearman_echo(gearman_st *ptr, char *message, size_t message_length, char *function)
{
  if (function)
  {
    uint32_t server_key;

    server_key= find_server(ptr);

    return echo_server(&(ptr->hosts[server_key]), message, message_length);
  }
  else if (ptr->hosts && ptr->number_of_hosts)
  {
    uint32_t x;

    for (x= 0; x < ptr->number_of_hosts; x++)
    {
      gearman_return rc;

      rc= echo_server(&ptr->hosts[x], message, message_length);

      if (rc != GEARMAN_SUCCESS)
        return rc;
    }
  }
  else
    return GEARMAN_NO_SERVERS;


  return GEARMAN_SUCCESS;
}
