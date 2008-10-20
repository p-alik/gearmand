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
#include <libgearman/dispatch.h>
#include <libgearman/server.h>
#include <libgearman/connect.h>

gearman_return gearman_dispatch(gearman_server_st *server, 
                                gearman_action action,
                                giov_st *giov,
                                bool with_flush)
{
  size_t sent_length;
  size_t total_length;
  uint32_t tmp_store;
  gearman_return rc;

  if ((rc= gearman_connect(server)) != GEARMAN_SUCCESS)
    return rc;

  /* Header, aka pick the right one! */
  if (server->type == GEARMAN_SERVER_TYPE_INTERNAL)
    sent_length= gearman_io_write(server, "\0RES", 4, false);
  else
    sent_length= gearman_io_write(server, "\0REQ", 4, false);

  /* Action */
  tmp_store= htonl(action);
  sent_length= gearman_io_write(server, (char *)&tmp_store, sizeof(uint32_t), false);
  
  /* Value! */
  switch(action)
  {
    /* No arguments */
  case GEARMAN_RESET_ABILITIES:
  case GEARMAN_PRE_SLEEP:
  case GEARMAN_NOOP:
  case GEARMAN_NO_JOB:
  case GEARMAN_GRAB_JOB:
    {
      total_length= sent_length= 0;
      (void)gearman_io_write(server, (char *)&total_length, sizeof(uint32_t), false);
      break;
    }
    /* One Argument */
  case GEARMAN_SET_CLIENT_ID:
    {
      WATCHPOINT_STRING("sending id");
      /* Create code to test for white space */
    }
  case GEARMAN_WORK_FAIL:
  case GEARMAN_GET_STATUS:
  case GEARMAN_ECHO_RES:
  case GEARMAN_JOB_CREATED:
  case GEARMAN_ECHO_REQ:
  case GEARMAN_CAN_DO:
  case GEARMAN_CANT_DO:
    {
      total_length= giov->arg_length;
      tmp_store= htonl(total_length);
      sent_length= gearman_io_write(server, (char *)&tmp_store, sizeof(uint32_t), false);
      sent_length= gearman_io_write(server, giov->arg, giov->arg_length, false);

      break;
    }
    /* Two arguments */
  case GEARMAN_WORK_COMPLETE:
  case GEARMAN_ERROR:
  case GEARMAN_CAN_DO_TIMEOUT:
    {
      /* Length of package */
      total_length= giov->arg_length + 1;
      total_length+= (giov + 1)->arg_length;

      tmp_store= htonl(total_length);
      sent_length= gearman_io_write(server, (char *)&tmp_store, sizeof(uint32_t), false);


      sent_length= gearman_io_write(server, giov->arg, giov->arg_length, false);
      sent_length+= gearman_io_write(server, "\0", 1, false);

      if ((giov + 1)->arg_length)
        sent_length+= gearman_io_write(server, (giov + 1)->arg, (giov + 1)->arg_length, false);

      break;
    }
    /* Threee arguments */
  case GEARMAN_WORK_STATUS:
  case GEARMAN_SUBMIT_JOB:
  case GEARMAN_SUBMIT_JOB_HIGH:
  case GEARMAN_SUBMIT_JOB_BJ:
  case GEARMAN_JOB_ASSIGN: /* J->W: HANDLE[0]FUNC[0]ARG */
    {
      uint8_t x;

      /* Length of package */
      for (total_length= 0, x= 0; x < 3; x++)
      {
        total_length+= (giov + x)->arg_length + 1;   /* unique */
      }
      total_length--; /* Remove final NULL */
      tmp_store= htonl(total_length);
      sent_length= gearman_io_write(server, (char *)&tmp_store, sizeof(uint32_t), false);

      /* Strings */
      for (x= 0, sent_length= 0; x < 3; x++)
      {
        if ((giov + x)->arg_length)
          sent_length+= gearman_io_write(server, (giov + x)->arg, (giov + x)->arg_length, false);
        if (x != 2)
          sent_length+= gearman_io_write(server, "\0", 1, false);
      }

      break;
    }
    /* Five arguements */
  case GEARMAN_STATUS_RES:  /* HANDLE[0]KNOWN[0]RUNNING[0]NUM[0]DENOM* */
    {
      uint8_t x;

      /* Length of package */
      for (total_length= 0, x= 0; x < 5; x++)
      {
        total_length+= (giov + x)->arg_length + 1;   /* unique */
      }
      total_length--;
      tmp_store= htonl(total_length);
      sent_length= gearman_io_write(server, (char *)&tmp_store, sizeof(uint32_t), false);

      /* Strings */
      for (x= 0, sent_length= 0; x < 5; x++)
      {
        if ((giov + x)->arg_length)
          sent_length+= gearman_io_write(server, (giov + x)->arg, (giov + x)->arg_length, false);
        if (x != 4)
          sent_length+= gearman_io_write(server, "\0", 1, false);
      }

      break;
    }
  default:
  case GEARMAN_SUBMIT_JOB_SCHEDULUED:
  case GEARMAN_SUBMIT_JOB_FUTURE:
    return GEARMAN_FAILURE;
  }

  (void)gearman_io_write(server, NULL, 0, with_flush);

  /* We should eventually fix this */
  if (sent_length != total_length)
    return GEARMAN_WRITE_FAILURE;

  return GEARMAN_SUCCESS;
}
