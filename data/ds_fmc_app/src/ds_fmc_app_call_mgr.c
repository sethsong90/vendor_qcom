/******************************************************************************

          D S _ F M C _ A P P _ C A L L _ M G R . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_call_mgr.c
  @brief   DS_FMC_APP Call Mgr entity Interface

  DESCRIPTION
  Implementation of DS_FMC_APP call mgr entity interface.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010-2012 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
08/28/12   op         Dynamic permission change of the call manager socket
02/18/10   scb        Initial version

******************************************************************************/

#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "ds_fmc_app_util.h"
#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_call_mgr.h"

#define DS_FMC_APP_CALL_MGR_BUF_SIZE  4

/* File descriptor sets */
LOCAL fd_set cm_master_fd_set, cm_read_fd_set;

/* Listener File descriptor */
LOCAL int cm_listener_fd = -1;

/* maximum file descriptor number */
LOCAL int cm_max_fd = -1;

/* Call manager client fd, which connects to listener sock */
LOCAL int cm_client_fd = -1;

/* Thread associated with the call manager listener sock */
LOCAL pthread_t cm_thrd_id;

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_init_listener
===========================================================================*/
/*!
@brief
  The function to accept a client socket to interact with.

@return
  DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE on failure

@note

  - Dependencies
    - None  

  - Side Effects
    - The cm_master_fd_set now contains the new cm_listener_fd
    - cm_max_fd value is updated if needed
*/
/*=========================================================================*/

LOCAL int
ds_fmc_app_call_mgr_init_listener
(
  void
)
{
  int len, rc;
  struct sockaddr_un addr;
  int    path_len;

  DS_FMC_APP_LOG_FUNC_ENTRY;

 /* Get the connection listener socket */
  if ((cm_listener_fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
    ds_fmc_app_log_err ("%s: unable to open listener socket,"
                        " rc = %d\n",__FILE__,cm_listener_fd);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Unlink socket path name just in case.... */
  unlink (DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH);

  /* setup for bind */
  memset (&addr,0, sizeof (struct sockaddr_un));
  path_len = strlen (DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH);
  addr.sun_family = AF_UNIX;
  memcpy (&addr.sun_path[0],DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH,path_len);
  addr.sun_path[path_len] = '\0';

  len = offsetof (struct sockaddr_un, sun_path) + path_len;

  ds_fmc_app_log_high ("addr path = %s, len = %d\n",
                        addr.sun_path,len);

  /* Bind socket to address */
  if ((rc = bind (cm_listener_fd, (struct sockaddr *)&addr, len)) < 0)
  {
    ds_fmc_app_log_err ("%s: unable to bind to listener socket,"
                        " rc = %d\n",__FILE__,rc);
    close (cm_listener_fd);
    cm_listener_fd = -1;
    unlink (DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Make socket a listener */
  if ((rc = listen (cm_listener_fd,0)) < 0)
  {
    ds_fmc_app_log_err("%s: unable to listen with listener socket,"
                       " rc = %d, errno=%d\n",__FILE__,rc,errno);
    close (cm_listener_fd);
    cm_listener_fd = -1;
    unlink (DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Change permissions on socket */
  chmod(DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH, 0666);  
  
  /* Set the cm_max_fd and the bit in the master_fd */
  if (cm_listener_fd > cm_max_fd)
  {
    cm_max_fd = cm_listener_fd;
  }

  FD_SET (cm_listener_fd,&cm_master_fd_set);

  ds_fmc_app_log_high ("%s: Added cm_listener_fd %d, cm_max_fd=%d\n",
                        __FILE__, cm_listener_fd, cm_max_fd);

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;

} /* ds_fmc_app_call_mgr_init_listener */

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_process_client_msg
===========================================================================*/
/*!
@brief
  Processes the client message received and takes adequate action appr.

@return
    - None
@note

  - Dependencies
    - None  

  - Side Effects
    - Triggers events to help SM transition
*/
/*=========================================================================*/
void
ds_fmc_app_call_mgr_process_client_msg
(
  int fd
)
{
  ssize_t buf_size;
  ds_fmc_app_exec_cmd_t * cmd = NULL;
  ds_fmc_app_fmc_bearer_status_type_t  buffer;

  DS_FMC_APP_LOG_FUNC_ENTRY;
   
  /* Validate fd */
  if( fd < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_process_client_msg: fd=%d"
                        " Invalid\n",fd);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }

  /* Call the recv function to receive the buffer from client */
  if ((buf_size = recv (fd,
                        (void *)&buffer,
                        sizeof(ds_fmc_app_fmc_bearer_status_type_t),
                        0)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_process_client_msg :"
                        " recv returned an error = %d\n", (int)buf_size);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }
  else if (buf_size == 0)
  {

    /* Peer successfully closed the socket. Perform appr. closure
       on the server side. */

    FD_CLR (fd, &cm_master_fd_set);

    /* Find new cm_max_fd */
    if (fd == cm_max_fd)
    {
      int i;
      for (i = cm_listener_fd; i < fd; i++)
      {
        if (FD_ISSET(i,&cm_master_fd_set))
        {
          cm_max_fd = i;
        }
      }
    }

    /* Close the fd  and reset the cm_client_fd stored */
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_process_client_msg: "
                         "Closing fd=%d, cm_max_fd = %d\n",fd,cm_max_fd);
    close (fd);
    fd = cm_client_fd = -1;
  }
  else
  {

    ds_fmc_app_log_high ("ds_fmc_app_call_mgr_process_client_msg:"
                         " Received %d bytes on fd = %d\n",(int)buf_size,fd);

    /* Allocate a command object */
    cmd = ds_fmc_app_exec_get_cmd();
    DS_FMC_APP_ASSERT(cmd);

    /* Data received, will be required to parse it and verify if it is 
       a trigger enable or disable indication.
       Set command object parameters depending upon trigger type */

    if(DS_FMC_APP_FMC_BEARER_ENABLED == buffer)
    {
      cmd->data.type = DS_FMC_APP_EXT_TRIG_ENABLE_EV;
      ds_fmc_app_log_high("Sending DS_FMC_APP_EXT_TRIG_ENABLE_EV event\n");
    }
    else if (DS_FMC_APP_FMC_BEARER_DISABLED == buffer)
    {
      cmd->data.type = DS_FMC_APP_EXT_TRIG_DISABLE_EV;
      ds_fmc_app_log_high("Sending DS_FMC_APP_EXT_TRIG_DISABLE_EV event\n");
    }
    else
    {
      ds_fmc_app_log_err ("ds_fmc_app_call_mgr_process_client_msg :"
                          " recv returned invalid cmd = %c\n",buffer);
      ds_fmc_app_exec_release_cmd(cmd);
      DS_FMC_APP_LOG_FUNC_EXIT;
      return;
    }

    /* Post command for processing in the command thread context */
    if( DS_FMC_APP_SUCCESS != ds_fmc_app_exec_put_cmd( cmd ) ) 
    {
       DS_FMC_APP_STOP("ds_fmc_app_exec_put_cmd: failed to put commmand\n");
    }
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
} /* ds_fmc_app_call_mgr_process_client_msg */


/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_init_client
===========================================================================*/
/*!
@brief
  The function to accept a client socket to interact with.

@return
  DS_FMC_APP_SUCCESS - On success, DS_FMC_APP_FAILURE - On failure

@note

  - Dependencies
    - None  

  - Side Effects
    - The cm_master_fd_set now contains the new client_fd
    - cm_max_fd value is updated if needed
*/
/*=========================================================================*/
LOCAL int ds_fmc_app_call_mgr_init_client
(
  void
)
{
  int rc;
  socklen_t len;
  struct stat stats;
  struct sockaddr_un addr;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  memset (&addr,0,sizeof(struct sockaddr_un));
  len = sizeof(struct sockaddr_un);

  if ((cm_client_fd = accept(
          cm_listener_fd, (struct sockaddr *)&addr, &len)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_init_client: unable to"
                        " accept on listener socket, rc = %d\n",
                         cm_client_fd);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }
#if 0
  /* With the ICS release, this verification code stopped working. The address
   * as well as the address length returned as a part of the accept() call was
   * invalid due to kernel updation changes potentially. Since this is a good
   * to have, but not a must have fix, the validation checks have been
   * temporarily commented out.
   */

  len -= offsetof (struct sockaddr_un, sun_path);
  addr.sun_path[len] = '\0';

  if (( rc = stat (addr.sun_path, &stats)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_init_client: unable"
                        " to stat client socket file \"%s\","
                        " rc = %d\n",addr.sun_path,rc);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  if (S_ISSOCK (stats.st_mode) == 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_call_mgr_init_client: client"
                        " socket file not a socket file, rc = %d\n",rc);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }
#endif
  /* No longer need the temp file */
  unlink (addr.sun_path);

  /* Add the new fd to the master fd set */
  FD_SET (cm_client_fd,&cm_master_fd_set);

  if (cm_client_fd > cm_max_fd)
  {
    cm_max_fd = cm_client_fd;
  }

  ds_fmc_app_log_high ("ds_fmc_app_call_mgr_init_client: added new client,"
                       " fd=%d, cm_max_fd=%d\n", cm_client_fd, cm_max_fd);

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;

} /* ds_fmc_app_call_mgr_init_client */

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_socklthrd_main
===========================================================================*/
/*!
@brief
  The main function of a Socket Listener thread.

@return
  void * - Does not return

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void *
ds_fmc_app_call_mgr_socklthrd_main 
(
  void *arg
)
{
  int ret;

  DS_FMC_APP_LOG_FUNC_ENTRY;
  (void)arg;

  for (;;) 
  {
    cm_read_fd_set = cm_master_fd_set;

    /* Call select to block on incoming message on all registered fds */
    if ((ret = select(cm_max_fd+1, &cm_read_fd_set,NULL,NULL,NULL)) < 0)
    {
      DS_FMC_APP_STOP("ds_fmc_app_call_mgr_socklthrd_main:"
                       " select failed!\n");
      return NULL;
    }

    /* Process new client connection request if we get one */
    if (FD_ISSET(cm_listener_fd, &cm_read_fd_set))
    {
      /* Only one outstanding client at a time supported 
         currently, verify if client conn. already exists */

      if ( FD_ISSET(cm_client_fd, &cm_master_fd_set) && 
          (0 <= cm_client_fd) )
      {
        DS_FMC_APP_STOP("ds_fmc_app_call_mgr_socklthrd_main:"
                         " cm_client_fd already set in"
                         " cm_master_fd_set\n");
        return NULL;
      }

      /* Will be called only once for the CnE client in case of
         Android */
      if( ds_fmc_app_call_mgr_init_client() < 0) 
      {
        DS_FMC_APP_STOP("ds_fmc_app_call_mgr_socklthrd_main:"
                         " ds_fmc_app_call_mgr_init_client failed!\n");
        return NULL;
      }

      FD_CLR (cm_listener_fd, &cm_read_fd_set);
    }
    else
    {
      int fd;
      /* Loop through all client FD's and process any with messages */
      for (fd = cm_listener_fd + 1; fd <= cm_max_fd; fd++)
      {
        if (FD_ISSET (fd, &cm_read_fd_set))
        {
          /* Ensure that the peer is the same as the cm_client_fd 
             stored */

          if (cm_client_fd != fd)
          {
            ds_fmc_app_log_err ("ds_fmc_app_call_mgr_process_client_msg: fd=%d"
                                " cm_client_fd=%d mismatch\n",fd, cm_client_fd);
            FD_CLR (fd, &cm_read_fd_set);
            continue;
          }

          ds_fmc_app_call_mgr_process_client_msg(fd);

          FD_CLR (fd, &cm_read_fd_set);

        } /* if */
      }  /* for */
    } /* else */

  } /* end of for(;;) */

  DS_FMC_APP_LOG_FUNC_EXIT;
} /* ds_fmc_app_call_mgr_socklthrd_main */


/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_socklthrd_start
===========================================================================*/
/*!
@brief
  Starts the socket listener thread and associates it with the specified 
  handle.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - Spawns a pthread for reading data received on associated sockets.
*/
/*=========================================================================*/
int ds_fmc_app_call_mgr_socklthrd_start 
(
  void
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  
  /* Create and start listener thread */
  if (pthread_create(&cm_thrd_id, NULL,
                     ds_fmc_app_call_mgr_socklthrd_main, NULL) != 0) 
  {
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;

  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_call_mgr_socklthrd_start */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_get_cm_client_fd
===========================================================================*/
/*!
@brief
  The function returns the client fd to the caller module

@return
  int - cm_client_fd

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_cm_get_cm_client_fd
(
  void
)
{
  ds_fmc_app_log_high("ds_fmc_app_cm_get_cm_client_fd: returning "
                      "client fd=%d\n",cm_client_fd);

  return cm_client_fd;
}

/*===========================================================================
  FUNCTION  ds_fmc_app_cm_tx_msg
===========================================================================*/
/*!
@brief
  The function to send out messages via the listener fd to a client
@return
  DS_FMC_APP_SUCCESS - On Success DS_FMC_APP_FAILURE - On failure

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_cm_tx_msg
(
  int fd,
  unsigned char *msg,
  int  msg_len
)
{
  int  rc;
  DS_FMC_APP_LOG_FUNC_ENTRY;

  if(fd < 0 || msg_len <= 0 || NULL == msg)
  {
    ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_tx_msg:"
                       " Invalid parameteres: fd=%d, len=%d\n",
                        fd, msg_len);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high("server: Sending message on fd=%d, len=%d\n",fd,msg_len);

  if ((rc = send (fd,
                 (void *) msg,
                 (size_t) msg_len,
                 0)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_cm_tx_msg: msgsnd error = %d\n",rc);

    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high ("ds_fmc_app_cm_tx_msg: sent %d bytes to client %d\n", rc, fd);

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_cm_tx_msg */

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_init
===========================================================================*/
/*!
@brief
  Starts the socket listener thread and associates it with the specified 
  handle.

@return
  - void
@note

  - Dependencies - None  

  - Side Effects
    - Spawns a pthread for reading data received on associated sockets.
*/
/*=========================================================================*/
void ds_fmc_app_call_mgr_init
(
  void
)
{

  DS_FMC_APP_LOG_FUNC_ENTRY;
  /* Initialize file desciptor sets */
  FD_ZERO (&cm_master_fd_set);
  FD_ZERO (&cm_read_fd_set);
  cm_max_fd = 2;

  atexit(ds_fmc_app_call_mgr_close);

  /* Set up listener socket */
  if(DS_FMC_APP_SUCCESS != ds_fmc_app_call_mgr_init_listener())
  {
    DS_FMC_APP_STOP("ds_fmc_app_call_mgr_init: "
                     "ds_fmc_app_call_mgr_init_listener\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }
  
  /* Spawn a pthread for reading data from associated sockets*/
  if(DS_FMC_APP_SUCCESS != ds_fmc_app_call_mgr_socklthrd_start())
  {
    DS_FMC_APP_STOP("ds_fmc_app_call_mgr_init: "
                     "ds_fmc_app_call_mgr_socklthrd_start\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }

  ds_fmc_app_log_high ("Server: added listener, fd=%d, cm_max_fd=%d\n",
                        cm_listener_fd,cm_max_fd);
  DS_FMC_APP_LOG_FUNC_EXIT;
} /* ds_fmc_app_call_mgr_init */

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_close
===========================================================================*/
/*!
@brief
  Closes resources associated with the DS_FMC_APP call mgr module.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies 
    - None.

  - Side Effects
    - None.
*/
/*=========================================================================*/
void ds_fmc_app_call_mgr_close
(
  void
)
{

  /* Close the cm_client_fd previously created for the call_mgr */
  if((0 <= cm_client_fd) && (close(cm_client_fd) < 0))
  {
    ds_fmc_app_log_err("ds_fmc_app_call_mgr_close: "
                     "error while closing cm_client_fd\n");
  }

  /* Close the listener socket previously created for the call_mgr */
  if(close(cm_listener_fd) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_call_mgr_close: "
                     "error while closing cm_listener_fd\n");
  }

  cm_listener_fd = cm_max_fd = cm_client_fd = -1;

  unlink (DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH);

  /* Re-initialize file desciptor sets */
  FD_ZERO (&cm_master_fd_set);
  FD_ZERO (&cm_read_fd_set);

  /* Kill pthread associated with the call_mgr */
  if(pthread_kill(cm_thrd_id, SIGUSR1) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_call_mgr_close: "
                     "Error while killing thrd_id %d\n", cm_thrd_id);
  }

} /* ds_fmc_app_call_mgr_close */
