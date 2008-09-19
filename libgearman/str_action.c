#include "common.h"

char *gearman_straction(gearman_st *ptr, gearman_action action)
{
  /* do something with the ptr, set and ignore for now (compile warnings) */
  ptr = NULL;

  switch (action)
  {
  case GEARMAN_JOB_CREATED:
    return "GEARMAN_JOB_CREATED";
  case GEARMAN_WORK_COMPLETE:
    return "GEARMAN_WORK_COMPLETE";
  case GEARMAN_CAN_DO:
    return "GEARMAN_CAN_DO";
  case GEARMAN_CAN_DO_TIMEOUT:
    return "GEARMAN_CAN_DO_TIMEOUT";
  case GEARMAN_CANT_DO:
    return "GEARMAN_CANT_DO";
  case GEARMAN_RESET_ABILITIES:
    return "GEARMAN_RESET_ABILITIES";
  case GEARMAN_SET_CLIENT_ID:
    return "GEARMAN_SET_CLIENT_ID";
  case GEARMAN_PRE_SLEEP:
    return "GEARMAN_PRE_SLEEP";
  case GEARMAN_NOOP:
    return "GEARMAN_NOOP";
  case GEARMAN_SUBMIT_JOB:
    return "GEARMAN_SUBMIT_JOB";
  case GEARMAN_SUBMIT_JOB_HIGH:
    return "GEARMAN_SUBMIT_JOB_HIGH";
  case GEARMAN_SUBMIT_JOB_BJ:
    return "GEARMAN_SUBMIT_JOB_BJ";
  case GEARMAN_GRAB_JOB:
    return "GEARMAN_GRAB_JOB";
  case GEARMAN_NO_JOB:
    return "GEARMAN_NO_JOB";
  case GEARMAN_JOB_ASSIGN:
    return "GEARMAN_JOB_ASSIGN";
  case GEARMAN_WORK_STATUS:
    return "GEARMAN_WORK_STATUS";
  case GEARMAN_WORK_FAIL:
    return "GEARMAN_WORK_FAIL";
  case GEARMAN_GET_STATUS:
    return "GEARMAN_GET_STATUS";
  case GEARMAN_STATUS_RES:
    return "GEARMAN_STATUS_RES";
  case GEARMAN_ECHO_REQ:
    return "GEARMAN_ECHO_REQ";
  case GEARMAN_ECHO_RES:
    return "GEARMAN_ECHO_RES";
  case GEARMAN_ERROR:
    return "GEARMAN_ERROR";
  default:
    return "UNKNOWN ACTION TYPE";
  };
}
