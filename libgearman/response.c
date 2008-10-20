/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

gearman_return gearman_response(gearman_server_st *ptr, 
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

  if ((size_t)read_length != response_length)
    return GEARMAN_FAILURE;


  WATCHPOINT_ACTION(action);
  switch(action)
  {
    /* Return handle */
  case GEARMAN_JOB_CREATED:
    {
      if (response_length)
      {
        gearman_return rc;

        rc= gearman_result_value_store(result, buffer, response_length);
        assert(rc == GEARMAN_SUCCESS);
      }
      else
        return GEARMAN_FAILURE;

      return GEARMAN_SUCCESS;
    }

    /* Return value */
  case GEARMAN_WORK_COMPLETE:
    {
      gearman_return rc;
      ssize_t size_of_string;
      char *start_ptr;

      WATCHPOINT_ASSERT(result);

      start_ptr= buffer;

      /* Pull out handle */
      {
        size_of_string= strlen(start_ptr);
        rc= gearman_result_handle_store(result, start_ptr, size_of_string);
        assert(rc == GEARMAN_SUCCESS);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
      }

      /* Pull out actual result */
      rc= gearman_result_value_store(result, start_ptr, (size_t)((buffer + response_length) - start_ptr)); 
      assert(rc == GEARMAN_SUCCESS);

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_STATUS_RES: /* HANDLE[0]KNOWN[0]RUNNING[0]NUM[0]DENOM* */
    {
      gearman_return rc;
      size_t size_of_string;
      char *start_ptr;

      WATCHPOINT_ASSERT(result);

      start_ptr= buffer;

      {
        size_of_string= strlen(start_ptr);
        rc= gearman_result_handle_store(result, start_ptr, size_of_string);
        start_ptr+= size_of_string + 1; /* One additional for the NULL */
        assert(rc == GEARMAN_SUCCESS);
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
      gearman_return rc;
      WATCHPOINT_ASSERT(result);
      WATCHPOINT_STRING(buffer);
      rc= gearman_result_value_store(result, buffer, response_length);
      assert(rc == GEARMAN_SUCCESS);

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_NO_JOB:
    return GEARMAN_NOT_FOUND;
  case GEARMAN_JOB_ASSIGN: /* J->W: HANDLE[0]FUNC[0]ARG */
    {
      gearman_return rc;
      char *buffer_ptr;
      char *start_ptr;

      buffer_ptr= start_ptr= buffer;
      /* 
        In the future save handle?
        start_ptr, strlen(start_ptr)
      */

      for (; *buffer_ptr != 0; buffer_ptr++); /* duplicate of above */
      buffer_ptr++;
      start_ptr= buffer_ptr;

      for (; *buffer_ptr != 0; buffer_ptr++);
      buffer_ptr++;
      start_ptr= buffer_ptr;

      WATCHPOINT_ASSERT(result);
      rc= gearman_result_value_store(result, start_ptr, (size_t)((buffer + response_length) - start_ptr));
      assert(rc == GEARMAN_SUCCESS);

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_GRAB_JOB:
    {
      return GEARMAN_SUCCESS;
    }
  case GEARMAN_WORK_FAIL:
    {
      gearman_return rc;

      rc= gearman_result_value_store(result, buffer, response_length);
      assert(rc == GEARMAN_SUCCESS);

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
      gearman_result_handle_store(result, start_ptr, strlen(start_ptr));
      /* We move past the null terminator for the function and the function name */
      buffer_ptr+= gearman_result_handle_length(result) + 1;
      start_ptr= buffer_ptr;

      /* We don't handle unique just yet */
      buffer_ptr+= strlen(start_ptr) + 1;
      start_ptr= buffer_ptr;

      gearman_result_value_store(result, start_ptr, (size_t)((buffer + response_length) - start_ptr));
      WATCHPOINT_NUMBER(((buffer + response_length) - start_ptr));
      WATCHPOINT_STRING_LENGTH(start_ptr, (size_t)((buffer + response_length) - start_ptr));

      return GEARMAN_SUCCESS;
    }
  case GEARMAN_GET_STATUS:
    {
      gearman_return rc;

      rc= gearman_result_value_store(result, buffer, response_length);
      assert(rc == GEARMAN_SUCCESS);

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
