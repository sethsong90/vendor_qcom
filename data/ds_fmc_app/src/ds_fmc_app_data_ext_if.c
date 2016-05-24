/******************************************************************************

              D S _ F M C _ A P P _ D A T A _ E X T _ I F . C

******************************************************************************/

/******************************************************************************
  @file    ds_fmc_app_data_ext_if.c
  @brief   The DS_FMC_APP DATA_EXT interface

  DESCRIPTION
  The DS_FMC_APP DATA_EXT interface

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
02/24/10   scb        Initial version

******************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ds_fmc_app.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_data_ext_if.h"


/* Data kept for each connection ID */
typedef struct
{
  pthread_t             th_id;
  int                   sock_fd;
  unsigned char         *rx_buf; 
  boolean                active;
} ds_fmc_app_data_ext_conn_info_type;


/* Global function pointer to be called when data is received */
LOCAL ds_fmc_app_data_ext_rx_cb_ptr ds_fmc_app_data_ext_rx_cb = NULL;

/*****************************************************************************
** Data declarations 
*****************************************************************************/

LOCAL ds_fmc_app_data_ext_conn_info_type ds_fmc_app_data_ext_conn_info[DS_FMC_APP_MAX_CONNECTIONS] =     
  {
    {
      0,
      -1,
      NULL,
      FALSE
    }
  };

/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_rx_data
===========================================================================*/
/*!
@brief 
  Routine for handling DS_FMC_APP messages received from 
  DATA_EXT socket i/f.

  This function will be called by a thread that is spawned for each 
  DS_FMC_APP connection (1 as of now) to handle receive messages. 
  
@return 
  NULL... but this function should never return

@note
  Connection is assumed to be opened with valid data being transferred

  - Side Effects
    - None
    
*/    

/*=========================================================================*/
LOCAL void *
ds_fmc_app_data_ext_rx_data
(
  void *in_param
)
{
  /* Pointer to QMUX connection info */
  ds_fmc_app_data_ext_conn_info_type *conn_info;
  int                             conn_id;

  /* Input parameter is conn_id */
  conn_id = (int) in_param;
 
  /* Verify that conn_id was passed in correctly */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS) 
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_rx_data: Conn ID invalid %d\n",
                         conn_id);
    return NULL;
  }

  conn_info = &ds_fmc_app_data_ext_conn_info[conn_id];

  if (!conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_rx_data: socket"
                        " not active for conn_id = %d!\n",conn_id);
    return NULL;
  }

  /* Assuming that conn_info structure is valid at this point and that
     checking additional variables in the conn_info struct is an overkill
   */

  /* Loop forever */
  while (1)
  {
    int num_read;

    /* Blocking read call */
    num_read = recv (conn_info->sock_fd,
                     (void *) conn_info->rx_buf,
                     (size_t) DS_FMC_APP_MAX_MSG_SIZE, 0);

    if (num_read < 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_ext_rx_data: Got invalid "
                            "number of bytes from read = %d\n",num_read);
      return NULL;
    }
    else if(num_read == 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_ext_rx_data: Peer "
                          "closed socket, num_read %d\n",num_read);
      return NULL;
    }

    /* Make rx callback with newly read data */
    if (ds_fmc_app_data_ext_rx_cb != NULL)
    {
      ds_fmc_app_data_ext_rx_cb (conn_id, conn_info->rx_buf, num_read);
    }
    else
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_ext_rx_data: NULL "
                  "ds_fmc_app_data_ext_rx_cb, num_read %d\n",num_read);
      return NULL;
    }

  } /* while (1) */

  /* This will never be hit */
  return NULL;

} /* ds_fmc_app_data_ext_rx_data */


/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_send_data
===========================================================================*/
/*!
@brief 
  Function to send data over DATA_EXT sockets to be sent over the air
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Connection must have been previously opened.

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_ext_send_data
(
  int                      conn_id,
  unsigned char           *msg_ptr,
  int                      msg_len
)
{
  ds_fmc_app_data_ext_conn_info_type *conn_info = NULL;
  int                                 rc = DS_FMC_APP_FAILURE;

  /* Verifiy the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_send_data: bad conn_id = %d!\n",
                          conn_id);
    return rc;
  }

  /* Set up pointer to connection info */
  conn_info = &ds_fmc_app_data_ext_conn_info[conn_id];

  if (!conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_send_data: Failed, connection"
                          " not active conn_id = %d\n",conn_id);
    return rc;
  }

  /* Assuming that conn_info structure is valid at this point! */

  if (send (conn_info->sock_fd, 
            (void *) msg_ptr, 
             msg_len, 0) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_send_data: Send failed"
                          "msg_len %d\n", msg_len);
  }
  else
  {
#ifdef _DS_FMC_APP_DEBUG
    ds_fmc_app_log_high ("ds_fmc_app_data_ext_send_data: Send success"
                          "msg_len %d\n", msg_len);
#endif /*_DS_FMC_APP_DEBUG*/
    rc = DS_FMC_APP_SUCCESS;
  }

  return rc;
} /* ds_fmc_app_data_ext_send_data */

/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the ds_fmc_app_data_ext_open_conn() 
  
@return 
  - void

@note

  - Connection is assumed to be opened with valid data before this 
  function starts to execute

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
void
ds_fmc_app_data_ext_init
(
  ds_fmc_app_data_ext_rx_cb_ptr  rx_func
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  if (ds_fmc_app_data_ext_rx_cb == NULL)
  {
    ds_fmc_app_data_ext_rx_cb = rx_func;
  }
  else
  {
    DS_FMC_APP_STOP("ds_fmc_app_data_ext_init: Error reiniting an already"
                      " initialized data EXT entity\n" );
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return ;
} /* ds_fmc_app_data_ext_init */ 
 
/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_open_conn
===========================================================================*/
/*!
@brief 
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Opens up a DATA_EXT socket and spawns a thread for RX handling.
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_ext_open_conn 
(
  int                 conn_id,
  struct sockaddr    *addr, 
  socklen_t           addrlen,
  unsigned char       *rx_buf
)
{
  ds_fmc_app_data_ext_conn_info_type  *conn_info = NULL;
  int                              rc;
  struct sockaddr_in                  *inet_addr = NULL; 
#ifdef _DS_FMC_APP_DEBUG
  char buffer[INET_ADDR_MAX_BUF_SIZE];

#endif /*_DS_FMC_APP_DEBUG*/
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: bad conn_id = %d!\n",
                  conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  if (rx_buf == NULL)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: NULL rxbuf pointer\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  if (addr == NULL || addrlen <=0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: NULL addr pointer"
                         " or invalid addrlen %d\n", addrlen);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Assure that power-up init function has been called */
  if (ds_fmc_app_data_ext_rx_cb == NULL)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: power-up init not called\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set up pointer to connection info */
  conn_info = (ds_fmc_app_data_ext_conn_info_type  *) 
              &ds_fmc_app_data_ext_conn_info[conn_id];

  if (conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: connection"
                          " already opened = %d!\n",conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set the RX buffer in connection info */
  conn_info->rx_buf = rx_buf;

  /* Open the DATA_EXT socket and proceed to connect to it*/

  conn_info->sock_fd =  socket(
                ((struct sockaddr_storage *)(addr))->ss_family,
                 SOCK_DGRAM, 0);

  if (conn_info->sock_fd < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: Unable to "
                          "open socket\n");
    conn_info->rx_buf = NULL;
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high("ds_fmc_app_data_ext_open_conn: "
                       "addr->ss_family %d\n", 
                  ((struct sockaddr_storage *)(addr))->ss_family);


  switch(((struct sockaddr_storage *)(addr))->ss_family)
  {
    case AF_INET:

      inet_addr = (struct sockaddr_in*) addr;
#ifdef _DS_FMC_APP_DEBUG
      memset(buffer, 0, INET_ADDR_MAX_BUF_SIZE);
      inet_ntop(AF_INET, &inet_addr->sin_addr, buffer, INET_ADDR_MAX_BUF_SIZE);

      ds_fmc_app_log_high("ds_fmc_app_data_ext_open_conn: "
                          "inet_addr->sin_family %d "
                          "orig inet_addr->sin_addr %d "
                          "dest port inet_addr->sin_port %d "
                          "inet_addr->sin_addr %s\n", inet_addr->sin_family,
                           inet_addr->sin_addr.s_addr, 
                           inet_addr->sin_port, buffer);
#endif /*_DS_FMC_APP_DEBUG*/
      /* Swap the sin_port to network byte order before connecting. */
      inet_addr->sin_port = htons(inet_addr->sin_port);
      break;

    default:
      ds_fmc_app_log_high("ds_fmc_app_data_ext_open_conn: "
                          "in default!\n");

      break;
  }

  /* Connect to the destination IP address           */
  rc = connect(conn_info->sock_fd, (struct sockaddr*)addr, addrlen);

  if (rc < 0) 
  {
    ds_fmc_app_log_err("ds_fmc_app_data_ext_open_conn: connect() failed\n");
    goto init_exit;
  }

  /* Spawn RX thread and pass to it the QMUX connection ID */
  if ((pthread_create (&conn_info->th_id,
                       NULL,
                       ds_fmc_app_data_ext_rx_data,
                       (void *) conn_id)) != 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_open_conn: can't create RX thread");
    goto init_exit;
  }

  /* We are done, mark connection as active and return success code */
  conn_info->active = TRUE;

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;

  /* Process error condition label. Close sock_fd and reset other variables
     appropriately */
init_exit:
  close(conn_info->sock_fd);
  conn_info->sock_fd = -1;
  conn_info->rx_buf = NULL;
  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_FAILURE;

} /* ds_fmc_app_data_ext_open_conn */

 
/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_close_conn
===========================================================================*/
/*!
@brief 
  Function used to close a connection.  This function must be called
  once data transfer is complete and we are ready to initiate closure 
  of the DATA_EXT socket. 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Closes the DATA_EXT socket and corresponding threads assoc. with
      RX handling.
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_ext_close_conn 
(
  int                 conn_id
)
{
  ds_fmc_app_data_ext_conn_info_type  *conn_info;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_close_conn: bad conn_id = %d!\n",
                  conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set up pointer to connection info */
  conn_info = &ds_fmc_app_data_ext_conn_info[conn_id];

  if (!conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_ext_close_conn: connection"
                          " already closed = %d!\n",conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set the RX buffer in connection info */
  conn_info->rx_buf = NULL;

  if( close( conn_info->sock_fd ) < 0 )
  {
    ds_fmc_app_log_err("ds_fmc_app_data_ext_close_conn: "
                       "cannot close DATA_EXT sock\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  conn_info->sock_fd = -1;

  /* We are done, mark connection as in-active and return success code */
  conn_info->active = FALSE;

  /* Stop the socket listener thread */
  if( pthread_kill( conn_info->th_id, SIGUSR1 ) != 0 ) 
  {
    ds_fmc_app_log_err("ds_fmc_app_data_ext_close_conn: "
                       "cannot stop sock listener thread\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_data_ext_close_conn */

/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_conn_open_status
===========================================================================*/
/*!
@brief 
  Function used to query connection status. This function is invoked
  when a Tunnel closure event is received by the tunnel mgr module.
  
@return 
  Returns value of the conn_info assc. with conn_id.

@note
  - Side Effects
    - None.
    
*/    
/*=========================================================================*/
boolean
ds_fmc_app_data_ext_conn_open_status 
(
  int                 conn_id
)
{
  ds_fmc_app_data_ext_conn_info_type  *conn_info;
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    DS_FMC_APP_STOP ("ds_fmc_app_data_ext_close_conn: bad conn_id = %d!\n",
                  conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set up pointer to connection info */
  conn_info = (ds_fmc_app_data_ext_conn_info_type*)
              &ds_fmc_app_data_ext_conn_info[conn_id];

  DS_FMC_APP_LOG_FUNC_EXIT;
  return(conn_info->active);
} /* ds_fmc_app_data_ext_conn_open_status */
