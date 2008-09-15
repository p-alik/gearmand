/*
  Gearman library

  gearman_response() is used to determine the return result
  from an issued command.
*/

#include "common.h"

gearman_return gearman_response(gearman_server_st *ptr, 
                                gearman_byte_array_st *handle,
                                gearman_result_st *result)
{
  char buffer[HUGE_STRING_LEN];
  ssize_t read_length;
  uint32_t tmp_number;
  gearman_action action;
  size_t response_length; /* While we use size_t, protocol is uint32_t */

  WATCHPOINT;
  read_length= gearman_io_read(ptr, buffer, PACKET_HEADER_LENGTH);
  WATCHPOINT_NUMBER(read_length);

  if (read_length == 0)
    return GEARMAN_READ_FAILURE;
#ifdef CRAP
  int x;

  for (x= 0; x < 4; x++)
    printf("\t%u -> %u (%c)\n", x, buffer[x], buffer[x]);
#endif

  if (ptr->type == GEARMAN_SERVER_TYPE_INTERNAL)
  {
    if (memcmp(buffer, "\0REQ", 4) != 0)
      return GEARMAN_FAILURE;
  }
  else
  {
    if (memcmp(buffer, "\0RES", 4) != 0)
      return GEARMAN_FAILURE;
  }

  memcpy(&tmp_number, buffer + 4, sizeof(uint32_t));
  action= ntohl(tmp_number);

  if (result)
    result->action= action;

  memcpy(&tmp_number, buffer + 8, sizeof(uint32_t));
  response_length= ntohl(tmp_number);

  /* Now we read the body */
  memset(buffer, 0, HUGE_STRING_LEN);
  read_length= gearman_io_read(ptr, buffer, response_length);
#ifdef CRAP
  for (x= 0; x < response_length; x++)
    printf("\t%u -> %u (%c)\n", x, buffer[x], buffer[x]);
#endif

  if (read_length != response_length)
    return GEARMAN_FAILURE;


  WATCHPOINT_ACTION(action);
  switch(action)
  {
    /* Return handle */
  case GEARMAN_JOB_CREATED:
    {
      if (response_length)
      {
        gearman_byte_array_store(handle, buffer, response_length);
      }
      else
        return GEARMAN_FAILURE;

      return GEARMAN_SUCCESS;
    }

    /* Return value */
  case GEARMAN_WORK_COMPLETE:
    {
      size_t size_of_string;
      char *start_ptr;

      WATCHPOINT_ASSERT(result);

      start_ptr= buffer;

      /* Pull out handle */
      {
        size_of_string= strlen(start_ptr);
        gearman_result_set_handle(result, start_ptr, size_of_string);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
      }

      /* Pull out actual result */
      gearman_byte_array_store(&(result->value), start_ptr, (size_t)((buffer + response_length) - start_ptr));

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_STATUS_RES: //HANDLE[0]KNOWN[0]RUNNING[0]NUM[0]DENOM*
    {
      gearman_return rc;
      size_t size_of_string;
      char *start_ptr;

      WATCHPOINT_ASSERT(result);

      start_ptr= buffer;

      {
        size_of_string= strlen(start_ptr);
        gearman_result_set_handle(result, start_ptr, size_of_string);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
      }

      /* Put in error logic for case where job not known */
      {
        size_of_string= strlen(start_ptr);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
      }

      {
        size_of_string= strlen(start_ptr);

        /* YEs, this is crap, original gearman looked for string value of 0 */
        if (*start_ptr == 48)
          return GEARMAN_SUCCESS;
        else
          rc= GEARMAN_STILL_RUNNING;

        start_ptr+= size_of_string + 1; /* One additional for the NULL */
      }

      {
        size_of_string= strlen(start_ptr);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
        result->numerator= strtol(start_ptr, (char **) NULL, 10);
      }

      {
        size_of_string= strlen(start_ptr);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
        result->denominator= strtol(start_ptr, (char **) NULL, 10);
      }

      return rc;
    }
  case GEARMAN_NOOP:
      return GEARMAN_SUCCESS;
  case GEARMAN_CAN_DO:
  case GEARMAN_CAN_DO_TIMEOUT:
  case GEARMAN_CANT_DO:
  case GEARMAN_ECHO_REQ:
  case GEARMAN_ECHO_RES:
    {
      WATCHPOINT_ASSERT(result);
      gearman_byte_array_store(&(result->value), buffer, response_length);

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_NO_JOB:
    return GEARMAN_NOT_FOUND;
  case GEARMAN_JOB_ASSIGN: /* J->W: HANDLE[0]FUNC[0]ARG */
    {
      char *buffer_ptr;
      char *start_ptr;

      buffer_ptr= start_ptr= buffer;
      gearman_result_set_handle(result, start_ptr, strlen(start_ptr));

      for (; *buffer_ptr != 0; buffer_ptr++); /* duplicate of above */
      buffer_ptr++;
      start_ptr= buffer_ptr;

      for (; *buffer_ptr != 0; buffer_ptr++);
      buffer_ptr++;
      start_ptr= buffer_ptr;

      gearman_result_set_value(result, start_ptr, (size_t)((buffer + response_length) - start_ptr));

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_GRAB_JOB:
    {
      return GEARMAN_SUCCESS;
    }
  case GEARMAN_WORK_FAIL:
    {
      gearman_byte_array_store(&(result->handle), buffer, response_length);

      return GEARMAN_FAILURE;
    }
  case GEARMAN_SUBMIT_JOB:
  case GEARMAN_SUBMIT_JOB_HIGH:
  case GEARMAN_SUBMIT_JOB_BJ:
    {
      char *buffer_ptr;
      char *start_ptr;

      buffer_ptr= start_ptr= buffer;
      WATCHPOINT_STRING(start_ptr);
      gearman_result_set_handle(result, start_ptr, strlen(start_ptr));
      /* We move past the null terminator for the function and the function name */
      buffer_ptr+= gearman_result_handle_length(result) + 1;
      start_ptr= buffer_ptr;

      /* We don't handle unique just yet */
      buffer_ptr+= strlen(start_ptr) + 1;
      start_ptr= buffer_ptr;

      gearman_result_set_value(result, start_ptr, (size_t)((buffer + response_length) - start_ptr));
      WATCHPOINT_NUMBER(((buffer + response_length) - start_ptr));
      WATCHPOINT_STRING_LENGTH(start_ptr, (size_t)((buffer + response_length) - start_ptr));

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_GET_STATUS:
    {
      gearman_result_set_handle(result, buffer, response_length);
      return GEARMAN_SUCCESS;
    }
  case GEARMAN_RESET_ABILITIES:
  case GEARMAN_SET_CLIENT_ID:
  case GEARMAN_PRE_SLEEP:
  case GEARMAN_WORK_STATUS:
  case GEARMAN_SUBMIT_JOB_SCHEDULUED:
  case GEARMAN_SUBMIT_JOB_FUTURE:
  case GEARMAN_ERROR:
  default:
    return GEARMAN_PROTOCOL_ERROR;
  }
}
