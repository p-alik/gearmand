/*
  Main GearmanDC server
 */

#define SERVER_PORT "7003"
#define MAX_MSG 100

#define SUCCESS 0
#define ERROR   1

#include <signal.h>
#include "server_common.h"

static struct event_base *main_base;

/* Prototypes */
void server_read(int fd, short event, void *arg);
bool server_response(gearman_connection_st *gear_conn);

int add_listener(int fd, int client)
{
  gearman_connection_st *gear_conn;

  gear_conn= gearman_connection_create(NULL);
  assert(gear_conn);

  (void)gearman_connection_add_fd(gear_conn, fd);

  WATCHPOINT_NUMBER(client);
  if (client)
    gear_conn->state= GEARMAN_CONNECTION_STATE_READ;

  /* Initalize one event */
  WATCHPOINT_NUMBER(fd);
  WATCHPOINT_ASSERT(fd);
  event_set(&gear_conn->evfifo, fd, EV_READ | EV_PERSIST, server_read, (void *)gear_conn);

  event_base_set(main_base, &gear_conn->evfifo);

  /* Add it to the active events, without a timeout */
  if (event_add(&gear_conn->evfifo, NULL) == -1)
  {
    WATCHPOINT;
    perror("event_add");
    WATCHPOINT;
    exit(1);
  }

  return 0;
}

int startup(void)
{
  int fd;
  struct addrinfo *ai;
  struct addrinfo *next;
  struct addrinfo hints;
  int arg;

  memset(&hints, 0, sizeof (hints));

  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  hints.ai_socktype = SOCK_STREAM;

  int e= getaddrinfo(NULL, SERVER_PORT, &hints, &ai);

  if (e != 0)
    exit(1);

  if (ai == NULL)
  {
    fprintf(stderr, "no hosts found\n");
    exit(1);
  }

  next= ai;
  do 
  {
    fd= socket(next->ai_family, next->ai_socktype,
               next->ai_protocol);
    
    arg= 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&arg, sizeof(int));

    if (bind(fd, next->ai_addr, next->ai_addrlen) != 0)
    {
      if (errno != EADDRINUSE) 
      {
        perror("bind()");
        exit(1);
      }
      close(fd);
    }
    else if (listen(fd, 100) != 0)
    {
      exit(1);
    }
    else
    {
      add_listener(fd, 0);
      WATCHPOINT_NUMBER(fd);
    }

  } while ((next= next->ai_next) != NULL);

  freeaddrinfo(ai);

  return 0;
}

void server_read(int fd, short event, void *arg)
{
  gearman_connection_st *gear_conn;
  gear_conn= (gearman_connection_st *)arg;
  bool run= true;


  fprintf(stderr, "server_read called with fd: %d, event: %d, arg: %p\n", fd, event, arg);

  while (run)
  {
    WATCHPOINT_NUMBER(gear_conn->state);
    switch (gear_conn->state)
    {
    case GEARMAN_CONNECTION_STATE_LISTENING:
      {
        struct sockaddr_storage addr;
        socklen_t addrlen;
        int client_fd;

        addrlen = sizeof(addr);
        if ((client_fd= accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) 
        {
          perror("accept");
          return;
        }
        assert(client_fd);

        add_listener(client_fd, 1);
        run= false;
      }
      break;
    case GEARMAN_CONNECTION_STATE_READ:
      WATCHPOINT;
      run= server_response(gear_conn);
      WATCHPOINT;
      break;
    case GEARMAN_CONNECTION_STATE_WRITE:
      assert(0);
      break;
    case GEARMAN_CONNECTION_STATE_CLOSE:
      {
        WATCHPOINT_STRING("CLOSING DOWN BABY");
        gearman_connection_free(gear_conn);
        return;
      }
    default:
      assert(0);
    }
    
    if (run == false && gearman_connection_buffered(gear_conn))
      run= true;
  }

  /* Reschedule this event */
  WATCHPOINT_STRING("rescheduling");
  event_add(&gear_conn->evfifo, NULL);
}

bool server_response(gearman_connection_st *gear_conn)
{
  gearman_return rc;
  bool run;

  rc= gearman_response(&gear_conn->server, NULL, &gear_conn->result); 
  WATCHPOINT_ERROR(rc);

  if (rc == GEARMAN_FAILURE || rc == GEARMAN_PROTOCOL_ERROR || rc == GEARMAN_READ_FAILURE)
  {
    gear_conn->state= GEARMAN_CONNECTION_STATE_CLOSE;
    return true;
  } 
  else if (rc != GEARMAN_SUCCESS)
  {
    WATCHPOINT_ERROR(rc);
    assert(0);
  }

  WATCHPOINT_ERROR(rc);
  WATCHPOINT_NUMBER(gearman_result_length(&gear_conn->result));

  WATCHPOINT_ACTION(gear_conn->result.action);
  /* Value! */
  switch(gear_conn->result.action)
  {
  case GEARMAN_ECHO_REQ:
    {
      giov_st giov;
      gearman_return rc;

      giov.arg= gearman_result_value(&gear_conn->result);
      giov.arg_length= gearman_result_length(&gear_conn->result);
      WATCHPOINT;
      rc= gearman_dispatch(&gear_conn->server, GEARMAN_ECHO_RES, &giov, 1); 
      WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);
      WATCHPOINT;
      run= false;
      break;
    }
  case GEARMAN_CAN_DO:
  case GEARMAN_CAN_DO_TIMEOUT:
    {
      /* We should be storing this + client into a hash table for lookup */
      run= false;
      break;
    }
  case GEARMAN_SET_CLIENT_ID:
    {
      /* Update hash structure storage with CLIENT ID */
      run= false;
      break;
    }
  case GEARMAN_RESET_ABILITIES:
    {
      /* We should remove the worker from the queue at this point */ 
      run= false;
      break;
    }
  case GEARMAN_GRAB_JOB:
    {
      gearman_return rc;

      /* Find a job that is available or return no_job */
      rc= gearman_dispatch(&gear_conn->server, GEARMAN_NO_JOB, NULL, 1); 
      WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);

      /* Once send the job, we will wait for the worker to tell us when it is done */
      run= false;

      break;
    }
  case GEARMAN_SUBMIT_JOB:
  case GEARMAN_SUBMIT_JOB_HIGH:
  case GEARMAN_SUBMIT_JOB_BJ:
    {
      giov_st giov;
      gearman_return rc;

      giov.arg= "foomanchoo";
      giov.arg_length= strlen("foomanchoo");
      rc= gearman_dispatch(&gear_conn->server, GEARMAN_JOB_CREATED, &giov, 1); 
      WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);

      run= false;
      break;
    }
  case GEARMAN_PRE_SLEEP:
  case GEARMAN_NOOP:
  case GEARMAN_NO_JOB:
  case GEARMAN_WORK_FAIL:
  case GEARMAN_GET_STATUS:
    {
      gearman_return rc;

      /* We should look up work here, and dispatch as appropriate */
      if (1)
      {
        giov_st giov[2];

        giov[0].arg= "foomanchoo";
        giov[0].arg_length= strlen("foomanchoo");

        giov[1].arg= gearman_result_value(&gear_conn->result);
        giov[1].arg_length= gearman_result_length(&gear_conn->result);

        rc= gearman_dispatch(&gear_conn->server, GEARMAN_WORK_COMPLETE, giov, 1); 
        WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);
      }
      else
      {
        rc= gearman_dispatch(&gear_conn->server, GEARMAN_WORK_FAIL, NULL, 1); 
        WATCHPOINT_ASSERT(rc == GEARMAN_SUCCESS);
      }

      run= false;
      break;
    }
  case GEARMAN_ECHO_RES:
  case GEARMAN_JOB_CREATED:
  case GEARMAN_CANT_DO:
  case GEARMAN_WORK_COMPLETE:
  case GEARMAN_ERROR:
  case GEARMAN_WORK_STATUS:
  case GEARMAN_JOB_ASSIGN:
  case GEARMAN_STATUS_RES:
  case GEARMAN_SUBMIT_JOB_SCHEDULUED:
  case GEARMAN_SUBMIT_JOB_FUTURE:
    WATCHPOINT_ACTION(gear_conn->result.action);
    WATCHPOINT_ASSERT(0);
    run= false;
  }

  return run;
}

int main (int argc, char **argv)
{
  struct sigaction sa;

  /*
   * ignore SIGPIPE signals; we can use errno==EPIPE if we
   * need that information
 */
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(SIGPIPE, &sa, 0) == -1) 
  {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }


  /* Initalize the event library */
  main_base= event_init();

  /* create socket */
  startup();

  //event_dispatch();
  event_base_loop(main_base, 0);

  return (0);
}
