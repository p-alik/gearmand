#include "common.h"

gearman_return gearman_dispatch(gearman_server_st *ptr, 
                                gearman_action action,
                                giov_st *giov,
                                uint8_t with_flush)
{
  size_t sent_length;
  size_t total_length;
  uint32_t tmp_store;
  gearman_return rc;

  if ((rc= gearman_connect(ptr)) != GEARMAN_SUCCESS)
    return rc;

  /* Header, aka pick the right one! */
  if (ptr->type == GEARMAN_SERVER_TYPE_INTERNAL)
    sent_length= gearman_io_write(ptr, "\0RES", 4, 0);
  else
    sent_length= gearman_io_write(ptr, "\0REQ", 4, 0);

  /* Action */
  tmp_store= htonl(action);
  sent_length= gearman_io_write(ptr, (char *)&tmp_store, sizeof(uint32_t), 0);
  
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
      (void)gearman_io_write(ptr, (char *)&total_length, sizeof(uint32_t), 0);
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
      sent_length= gearman_io_write(ptr, (char *)&tmp_store, sizeof(uint32_t), 0);
      sent_length= gearman_io_write(ptr, giov->arg, giov->arg_length, 0);

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
      sent_length= gearman_io_write(ptr, (char *)&tmp_store, sizeof(uint32_t), 0);


      sent_length= gearman_io_write(ptr, giov->arg, giov->arg_length, 0);
      sent_length+= gearman_io_write(ptr, "\0", 1, 0);

      if ((giov + 1)->arg_length)
        sent_length+= gearman_io_write(ptr, (giov + 1)->arg, (giov + 1)->arg_length, 0);

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
      sent_length= gearman_io_write(ptr, (char *)&tmp_store, sizeof(uint32_t), 0);

      /* Strings */
      for (x= 0, sent_length= 0; x < 3; x++)
      {
        if ((giov + x)->arg_length)
          sent_length+= gearman_io_write(ptr, (giov + x)->arg, (giov + x)->arg_length, 0);
        if (x != 2)
          sent_length+= gearman_io_write(ptr, "\0", 1, 0);
      }

      break;
    }
    /* Five arguements */
  case GEARMAN_STATUS_RES: //HANDLE[0]KNOWN[0]RUNNING[0]NUM[0]DENOM*
    {
      uint8_t x;

      /* Length of package */
      for (total_length= 0, x= 0; x < 5; x++)
      {
        total_length+= (giov + x)->arg_length + 1;   /* unique */
      }
      total_length--;
      tmp_store= htonl(total_length);
      sent_length= gearman_io_write(ptr, (char *)&tmp_store, sizeof(uint32_t), 0);

      /* Strings */
      for (x= 0, sent_length= 0; x < 5; x++)
      {
        if ((giov + x)->arg_length)
          sent_length+= gearman_io_write(ptr, (giov + x)->arg, (giov + x)->arg_length, 0);
        if (x != 4)
          sent_length+= gearman_io_write(ptr, "\0", 1, 0);
      }

      break;
    }
  default:
  case GEARMAN_SUBMIT_JOB_SCHEDULUED:
  case GEARMAN_SUBMIT_JOB_FUTURE:
    return GEARMAN_FAILURE;
  }

  (void)gearman_io_write(ptr, NULL, 0, with_flush);

  /* We should eventually fix this */
  if (sent_length != total_length)
    return GEARMAN_WRITE_FAILURE;

  return GEARMAN_SUCCESS;
}
