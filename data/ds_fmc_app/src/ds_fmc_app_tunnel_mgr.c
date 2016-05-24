/******************************************************************************

          D S _ F M C _ A P P _ T U N N E L _ M G R . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_tunnel_mgr.c
  @brief   DS_FMC_APP Tunnel Mgmt entity Interface

  DESCRIPTION
  Implementation of DS_FMC_APP tunnel mgmt entity interface.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 Qualcomm Technologies, Inc. All Rights Reserved

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
02/14/10   scb        Initial version

******************************************************************************/

#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#ifdef _DS_FMC_APP_DEBUG
#include <arpa/inet.h>
#endif /*_DS_FMC_APP_DEBUG*/
#include "ds_fmc_app.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_qmi.h"
#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_data_if.h"
#include "ds_fmc_app_tunnel_mgr.h"
#include "ds_fmc_app_data_ext_if.h"

#define DS_FMC_APP_TUNNEL_MGR_MAX_CONNECT_TRIES 5

/*===========================================================================

                         LOCAL VARIABLES

===========================================================================*/

/*! @brief Variables internal to module ds_fmc_app_sm_stub.c
*/
LOCAL ds_fmc_app_tunnel_mgr_ds_type ds_fmc_app_tunnel_mgr_ds;

/* Client file descriptor */
LOCAL int tunnel_mgr_client_fd = -1;

/* DS_FMC_APP Tunnel Manager thread to receive msgs from Tunnel Mgr */
LOCAL pthread_t ds_fmc_app_tunnel_mgr_th_id;

/* Mutex to be used, to prevent concurrent access to 
   ds_fmc_app_tunnel_mgr_ds
*/
LOCAL pthread_mutex_t tunnel_mgr_mutex;
/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_config_tunnel_params
===========================================================================*/
/*!
@brief
  This function calls the QMI set tunnel params function to set tunnel
  parameters

@return
  int - DS_FMC_APP_SUCCESS on Success DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_config_tunnel_params
(
  ds_fmc_app_tunnel_mgr_ds_type *ds_fmc_app_tunnel_mgr_ds
)
{

  DS_FMC_APP_LOG_FUNC_ENTRY;
  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_tunnel_params\n");

  if(ds_fmc_app_tunnel_mgr_ds == NULL)
  {
    ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                        "Tunnel Information invalid\n");
    return DS_FMC_APP_FAILURE;
  }

  /*If is_tunnel_set is set to 1, then call set_tunnel_params,
    else call clear_tunnel_params appr.*/
  if(ds_fmc_app_tunnel_mgr_ds->is_tunnel_set == DS_FMC_APP_TUNNEL_STATUS_SET)
  {
    if(ds_fmc_app_qmi_wds_set_tunnel_params(ds_fmc_app_tunnel_mgr_ds) < 0)
    {
      ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                          "ds_fmc_app_qmi_wds_set_tunnel_params Failure\n");
      return DS_FMC_APP_FAILURE;
    }

    ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                       "ds_fmc_app_qmi_wds_set_tunnel_params Success\n");
  }
  else if(ds_fmc_app_tunnel_mgr_ds->is_tunnel_set == 
          DS_FMC_APP_TUNNEL_STATUS_RESET)
  {
    if(ds_fmc_app_qmi_wds_clear_tunnel_params() < 0)
    {
      ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                        "ds_fmc_app_qmi_wds_clear_tunnel_params Failure\n");
      return DS_FMC_APP_FAILURE;
    }

    ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                       "ds_fmc_app_qmi_wds_clear_tunnel_params Success\n");
  }

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_tunnel_params: "
                      "Successful.\n");

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_tunnel_mgr_config_tunnel_params */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_config_ext_data_path
===========================================================================*/
/*!
@brief
  The function opens/closes a WLAN socket with appropriate tunnel params.
  parameters

@return
  int - DS_FMC_APP_SUCCESS on Success, DS_FMC_APP_FAILURE on Failure

@note

  - Dependencies
    - None  

  - Side Effects
    - Populates the tunnel_dest_ip appr. into the OUT param, before
      return
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_config_ext_data_path
(
  ds_fmc_app_fmc_bearer_status_type_t bearer_status,
  struct sockaddr_storage             *addr
)
{
#ifdef _DS_FMC_APP_DEBUG
  char buffer[INET_ADDR_MAX_BUF_SIZE];

  struct sockaddr_in *inet_addr; 
#endif /*_DS_FMC_APP_DEBUG*/

  DS_FMC_APP_LOG_FUNC_ENTRY;

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                       "bearer_status %d\n", bearer_status);

  DS_FMC_APP_ASSERT(addr);

#ifdef _DS_FMC_APP_DEBUG
  memset(buffer, 0, INET_ADDR_MAX_BUF_SIZE);

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                       "addr->ss_family %d\n", 
                  ds_fmc_app_tunnel_mgr_ds.tunnel_dest_ip.ss_family);


  switch(ds_fmc_app_tunnel_mgr_ds.tunnel_dest_ip.ss_family)
  {
    case AF_INET:

      inet_addr = (struct sockaddr_in*) 
                  &(ds_fmc_app_tunnel_mgr_ds.tunnel_dest_ip);

      inet_ntop(AF_INET, &inet_addr->sin_addr, buffer, 
                         INET_ADDR_MAX_BUF_SIZE);

      ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                          "inet_addr->sin_family %d "
                          "orig inet_addr->sin_addr %d "
                          "inet_addr->sin_addr %s\n", inet_addr->sin_family,
                           inet_addr->sin_addr.s_addr, buffer);
      break;
    default:
      ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                          "in default!\n");

      break;
  }
#endif /*_DS_FMC_APP_DEBUG*/

  switch(bearer_status)
  {
    case DS_FMC_APP_FMC_BEARER_DISABLED:
      /* Close connection to the external datapath entity */
      if(ds_fmc_app_data_ext_close_conn (DS_FMC_APP_DATA_CONN_ID_0) < 0)
      { 
        /* Nothing much can be done on failure of closure. Proceed
           to send a success */
        ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                         "ds_fmc_app_data_ext_close_conn() failed\n");
      }

      break;

    case DS_FMC_APP_FMC_BEARER_ENABLED:
      /* Initiate connection to the external datapath entity */
       
      pthread_mutex_lock(&tunnel_mgr_mutex);
     
      if(ds_fmc_app_data_ext_open_conn (
         DS_FMC_APP_DATA_CONN_ID_0, 
         (struct sockaddr*)&(ds_fmc_app_tunnel_mgr_ds.tunnel_dest_ip),
         sizeof(struct sockaddr_storage),
         ds_fmc_app_data_ext_rx_buf ) < 0)
      {
        ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                         "ds_fmc_app_data_ext_open_conn() failed");

        pthread_mutex_unlock(&tunnel_mgr_mutex);
        DS_FMC_APP_LOG_FUNC_EXIT;
        return DS_FMC_APP_FAILURE;
      }

      /* Copy the destination address back into the OUT param, to be
         conveyed back to the Call Manager. */
      memcpy(addr, &(ds_fmc_app_tunnel_mgr_ds.tunnel_dest_ip),
              sizeof(struct sockaddr_storage));
#ifdef _DS_FMC_APP_DEBUG
      inet_addr = (struct sockaddr_in*) addr;

      inet_ntop(AF_INET, &inet_addr->sin_addr, buffer, 
                         16);

      ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                          "inet_addr->sin_family %d "
                          "orig inet_addr->sin_addr %d "
                          "inet_addr->sin_addr %s\n", inet_addr->sin_family,
                           inet_addr->sin_addr.s_addr, buffer);
#endif /*_DS_FMC_APP_DEBUG*/
      pthread_mutex_unlock(&tunnel_mgr_mutex);

      break;

    default:
      ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                         "Invalid bearer status %d\n", bearer_status);
      DS_FMC_APP_LOG_FUNC_EXIT;
      return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                      "Successful.\n");

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_tunnel_mgr_config_ext_data_path */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd
===========================================================================*/
/*!
@brief
  The function returns the client fd to the caller module

@return
  int - tunnel_mgr_client_fd

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd
(
  void
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd: returning "
                  "client fd=%d\n",tunnel_mgr_client_fd);

  DS_FMC_APP_LOG_FUNC_EXIT;
  return tunnel_mgr_client_fd;
} /* ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_init
===========================================================================*/
/*!
@brief
  Tunnel Manager initialization function to initialize the mutex.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_fmc_app_tunnel_mgr_init
(
  void
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  ds_fmc_app_log_high ("ds_fmc_app_tunnel_mgr_init Success\n");
  pthread_mutex_init(&tunnel_mgr_mutex, NULL);
  DS_FMC_APP_LOG_FUNC_EXIT;
  return ;
} /* ds_fmc_app_tunnel_mgr_init */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_rx_msg
===========================================================================*/
/*!
@brief
  Processes a client message that was received on the tunnel_mgr_client_fd.

@return
  void* 

@note

  - Dependencies
    - None  

  - Side Effects
    - Parses message and posts an appropriate command to initiate
      SM transition. Commands posted can be : DS_FMC_APP_TUNNEL_OPENED_EV
      or DS_FMC_APP_TUNNEL_CLOSED_EV
*/
/*=========================================================================*/

LOCAL void *
ds_fmc_app_tunnel_mgr_rx_msg
(
  void *arg
)
{
  ssize_t buf_size;
  ds_fmc_app_exec_cmd_t * cmd = NULL;
  ds_fmc_app_sm_events_t  cmd_type;
  
  DS_FMC_APP_LOG_FUNC_ENTRY;
  (void)arg;

  if(tunnel_mgr_client_fd < 0)
  {
    DS_FMC_APP_STOP("ds_fmc_app_tunnel_mgr_rx_msg: Invalid "
                      "tunnel_mgr_client_fd %d\n", tunnel_mgr_client_fd);
    return NULL;
  }

  for (;;)
  {
    /* Allocate a command object */
    cmd = ds_fmc_app_exec_get_cmd();
    DS_FMC_APP_ASSERT(cmd);

    /* Clear the receive buffer */
    memset (&cmd->data.ds_fmc_app_tunnel_mgr_ds, 0, 
             sizeof(cmd->data.ds_fmc_app_tunnel_mgr_ds));

    /* Block waiting on recv on previously connected tunnel_mgr_client_fd */
       
    if ((buf_size = recv (tunnel_mgr_client_fd,
                          (void *)&cmd->data.ds_fmc_app_tunnel_mgr_ds,
                          sizeof(cmd->data.ds_fmc_app_tunnel_mgr_ds),
                          0)) < 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_rx_msg: recv"
          " returns error %d\n",(int)buf_size);
      ds_fmc_app_exec_release_cmd(cmd);
      continue;
    }

    /* Set the connection open status of the external datapath entity
       in the command structure */

    cmd->data.ext_data_path_conn_status = 
      ds_fmc_app_data_ext_conn_open_status(DS_FMC_APP_DATA_CONN_ID_0);

    if ( buf_size == 0 )
    {
      ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_rx_msg: "
            "Received Tunnel Mgr Server closure, buf_size = %d\n", 
            buf_size);
      cmd->data.type = DS_FMC_APP_TUNNEL_CLOSED_EV;
    }
    else
    {
      /* Copy the latest and greatest tunnel mgmt structure into
         our internal structure */

      pthread_mutex_lock(&tunnel_mgr_mutex);

      memset (&ds_fmc_app_tunnel_mgr_ds, 0, 
              sizeof(cmd->data.ds_fmc_app_tunnel_mgr_ds));

      memcpy( &ds_fmc_app_tunnel_mgr_ds, 
              &cmd->data.ds_fmc_app_tunnel_mgr_ds, 
              sizeof(cmd->data.ds_fmc_app_tunnel_mgr_ds) );

      pthread_mutex_unlock(&tunnel_mgr_mutex);

      ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_rx_msg: is_tunnel_set %d\n",
      cmd->data.ds_fmc_app_tunnel_mgr_ds.is_tunnel_set);

      /* Set command object parameters */
      if(cmd->data.ds_fmc_app_tunnel_mgr_ds.is_tunnel_set == 
          DS_FMC_APP_TUNNEL_STATUS_SET)
      {
        cmd->data.type = DS_FMC_APP_TUNNEL_OPENED_EV;
      }
      else if(cmd->data.ds_fmc_app_tunnel_mgr_ds.is_tunnel_set == 
              DS_FMC_APP_TUNNEL_STATUS_RESET)
      {
        cmd->data.type = DS_FMC_APP_TUNNEL_CLOSED_EV;
      }
    }

    cmd_type = cmd->data.type;

    /* Post command for processing in the command thread context */
    if( DS_FMC_APP_SUCCESS != ds_fmc_app_exec_put_cmd( cmd ) ) 
    {
      DS_FMC_APP_STOP("ds_fmc_app_tunnel_mgr_rx_msg: failed to put"
                      " commmand\n");
    }

    if(cmd_type == DS_FMC_APP_TUNNEL_CLOSED_EV)
    {
      ds_fmc_app_tunnel_mgr_close_conn(tunnel_mgr_client_fd);
    }

  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return NULL;
} /* ds_fmc_app_tunnel_mgr_rx_msg */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_client_tx_msg
===========================================================================*/
/*!
@brief
  Sends a message to the corresponding server.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_client_tx_msg
(
  int            client_fd,
  unsigned char *msg,
  int            msg_len
)
{
  int  rc;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  if(client_fd < 0 || msg_len <= 0 || NULL == msg)
  {
    ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_tx_msg:"
                       " Invalid parameteres: fd=%d, len=%d\n",
                        client_fd, msg_len);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high ("Sending %d bytes on fd = %d\n",
      msg_len,client_fd); 

  if ((rc = send (client_fd,
                 (void *) msg,
                 (size_t) msg_len,
                  0)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_tx_msg:"
                        " send error = %d\n",rc);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_tunnel_mgr_client_tx_msg */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_open_conn
===========================================================================*/
/*!
@brief
 Opens a tunnel manager connection. 

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - populates the tunnel_mgr_client_fd with relevant file descriptor
      information
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_open_conn
(
  void
)
{
  int len, rc;
  int ret = DS_FMC_APP_SUCCESS;
  struct sockaddr_un server_addr, client_addr;
  int i;

  tunnel_mgr_client_fd = -1;
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Initialize the addr variables */
  memset (&server_addr,0,sizeof (struct sockaddr_un));
  memset (&client_addr,0,sizeof (struct sockaddr_un));

  /* Create a client tunnel mgr socket */
  if ((tunnel_mgr_client_fd = socket (AF_UNIX,SOCK_STREAM,0)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_init: unable"
    " to open client socket, rc = %d\n", tunnel_mgr_client_fd);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  client_addr.sun_family = AF_UNIX;
  snprintf (client_addr.sun_path, sizeof(client_addr.sun_path), "%s",DS_FMC_APP_TUNNEL_MGR_CLIENT_SOCKET_PATH);
  len = offsetof (struct sockaddr_un, sun_path) + strlen (client_addr.sun_path);

  /* Delete file in case it exists */
  unlink (client_addr.sun_path);

  /* Bind socket to address */
  if ((rc = bind (tunnel_mgr_client_fd, 
      (struct sockaddr *)&client_addr, len)) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_init: unable"
                        " to bind to client socket, rc = %d\n",rc);
    ret = DS_FMC_APP_FAILURE;
    goto init_exit;
  }

  server_addr.sun_family = AF_UNIX;
  snprintf (server_addr.sun_path, sizeof(server_addr.sun_path), "%s", DS_FMC_APP_TUNNEL_MGR_CONN_SOCKET_PATH);
  len = offsetof (struct sockaddr_un, sun_path) + strlen (server_addr.sun_path);

  /* Connect to the server's connection socket */
  for (i = 0; i < DS_FMC_APP_TUNNEL_MGR_MAX_CONNECT_TRIES; i++)
  {

    if ((rc = connect (tunnel_mgr_client_fd,
        (struct sockaddr *) &server_addr, len)) < 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_init: unable"
                          " to connect to server, rc = %d\n", rc);
      sleep (1);
    }
    else
    {
      break;
    }
  }

  if (rc < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_init: unable"
                        " to connect to server after %d tries"
                        " ... giving up\n", i);
    ret = DS_FMC_APP_FAILURE;
    goto init_exit;
  }

  /* Spawn receive message thread */
  if ((pthread_create (&ds_fmc_app_tunnel_mgr_th_id,
                       NULL,
                       ds_fmc_app_tunnel_mgr_rx_msg,
                       (void *) NULL)) != 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_tunnel_mgr_client_init: "
    "can't create RX thread for client %d\n", tunnel_mgr_client_fd);
    ret = DS_FMC_APP_FAILURE;
    goto init_exit;
  }

init_exit:
  if (ret != DS_FMC_APP_SUCCESS)
  {
    /* Close tunnel_mgr_client_fd and delete file in case it exists */
    close(tunnel_mgr_client_fd);
    tunnel_mgr_client_fd = -1;
    unlink (client_addr.sun_path);
  }

  DS_FMC_APP_LOG_FUNC_EXIT;

  return ret;
} /* ds_fmc_app_tunnel_mgr_open_conn */

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_close_conn
===========================================================================*/
/*!
@brief
  Closes the file descriptor pointed to by client_fd.

@return
 - void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

void
ds_fmc_app_tunnel_mgr_close_conn
(
  int client_fd
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_close_conn: "
    " pthread_self() %d, tunnel_mgr_thrd %d\n", 
     pthread_self(), ds_fmc_app_tunnel_mgr_th_id);

  /* Close the client_fd previously created for the tunnel_mgr */
  if(close(client_fd) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_tunnel_mgr_close_conn: "
                     "error while closing client_fd\n");
  }

  ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_close_conn: "
  "Success while closing client_fd %d\n", client_fd);

  /* Kill pthread associated with the tunnel mgr */
  pthread_kill(ds_fmc_app_tunnel_mgr_th_id, SIGUSR1);

  DS_FMC_APP_LOG_FUNC_EXIT;

  return ;
} /* ds_fmc_app_tunnel_mgr_close_conn */
