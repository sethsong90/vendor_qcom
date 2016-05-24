/******************************************************************************

               D S _ F M C _ A P P _ D A T A _ M D M _ I F . C

******************************************************************************/

/******************************************************************************
  @file    ds_fmc_app_data_mdm_if.c
  @brief   The DS_FMC_APP Data Modem interface

  DESCRIPTION
  The DS_FMC_APP data modem interface

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

#include "ds_fmc_app.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_data_if.h"
#include "ds_fmc_app_data_mdm_if.h"


/* Data kept for each connection ID */
typedef struct
{
  const char            *port_id_name;
  pthread_t             th_id;
  int                   f_desc;
  unsigned char         *rx_buf; 
  unsigned long         active;
} ds_fmc_app_data_mdm_conn_info_type;


/* Global function pointer to be called when data is received */
LOCAL ds_fmc_app_data_mdm_rx_cb_ptr ds_fmc_app_data_mdm_rx_cb = NULL;

/*****************************************************************************
** Data declarations 
*****************************************************************************/

LOCAL ds_fmc_app_data_mdm_conn_info_type ds_fmc_app_data_mdm_conn_info[DS_FMC_APP_MAX_CONNECTIONS] =     
  {
    {
      "/dev/smd22",
      0,
      -1,
      NULL,
      FALSE
    },
  };

/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_rx_data
===========================================================================*/
/*!
@brief 
  Routine for handling DS_FMC_APP messages received from SMD DS_FMC_APP control port.
  This function will be called by a thread that is spawned for each 
  DS_FMC_APP connection to handle receive messages. 
  
@return 
  NULL... but this function should never return

@note
  Connection is assumed to be opened and valid data 

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
LOCAL void *
ds_fmc_app_data_mdm_rx_data
(
  void *in_param
)
{
  /* Pointer to QMUX connection info */
  ds_fmc_app_data_mdm_conn_info_type *conn_info;
  int                                 conn_id;

  /* Input parameter is conn_id */
  conn_id = (int) in_param;
 
  /* Verify that conn_id was passed in correctly */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS) 
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_rx_data: Conn ID invalid %d\n",
                         conn_id);
    return NULL;
  }

  conn_info = (ds_fmc_app_data_mdm_conn_info_type*)
              &ds_fmc_app_data_mdm_conn_info[(int) conn_id];

  if (!conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_rx_data: connection"
                          " not active conn_id = %d!\n",conn_id);
    return NULL;
  }

  /* Assuming that conn_info structure is valid at this point! */

  /* Loop forever */
  while (1)
  {
    int num_read;

    /* Blocking read call */
    num_read = read (conn_info->f_desc,
                     (void *) conn_info->rx_buf,
                     (size_t) DS_FMC_APP_MAX_MSG_SIZE);

    if (num_read < 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_mdm_rx_data: Got invalid "
                            "number of bytes from read = %d\n",num_read);
      return NULL;
    }
    else if(num_read == 0)
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_mdm_rx_data: Peer "
                     "closed file descriptor, num_read %d\n",num_read);
      return NULL;
    }

    /* Make rx callback with newly read data */
    if (ds_fmc_app_data_mdm_rx_cb != NULL)
    {
      ds_fmc_app_data_mdm_rx_cb (conn_id, conn_info->rx_buf, num_read);
    }
    else
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_mdm_rx_data: NULL "
                  "ds_fmc_app_data_mdm_rx_cb, num_read %d\n",num_read);
      return NULL;
    }

  } /* while (1) */

  /* This will never be hit */
  return NULL;

} /* ds_fmc_app_data_mdm_rx_data */


/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_send_data
===========================================================================*/
/*!
@brief 
  Function to send data to SMD to be sent across to the modem
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Connection must have been previously opened.

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_mdm_send_data
(
  int                      conn_id,
  unsigned char           *msg_ptr,
  int                      msg_len
)
{
  ds_fmc_app_data_mdm_conn_info_type *conn_info = NULL;
  int                                 rc = DS_FMC_APP_FAILURE;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_send_data: bad conn_id = %d!\n",
                          conn_id);
    return rc;
  }

  /* Set up pointer to connection info */
  conn_info = (ds_fmc_app_data_mdm_conn_info_type*)
               &ds_fmc_app_data_mdm_conn_info[conn_id];

  if (!conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_send_data: connection"
                          " not active conn_id = %d!\n",conn_id);
    return rc;
  }

  /* Assuming that conn_info structure is valid at this point! */

  /* Send to proper SMD port... 
  ** no need to have any mutex here... writing will be
  ** made serial by above layer
  */
  if (write (conn_info->f_desc, 
             (void *) msg_ptr, 
              msg_len) < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_send_data: Write failed, "
                        " len %d\n", msg_len);
    rc = DS_FMC_APP_FAILURE;
  }
  else
  {
#ifdef _DS_FMC_APP_DEBUG
    ds_fmc_app_log_high ("ds_fmc_app_data_mdm_send_data: Write succeeded,"
                        " len %d\n", msg_len);
#endif /*_DS_FMC_APP_DEBUG*/

    rc = DS_FMC_APP_SUCCESS;
  }

  return rc;
} /* ds_fmc_app_data_mdm_send_data */

 
/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_open_conn
===========================================================================*/
/*!
@brief 
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Opens up SMD port and spawns a thread for RX handling.
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_mdm_open_conn 
(
  int                  conn_id,
  unsigned char       *rx_buf
)
{
  ds_fmc_app_data_mdm_conn_info_type  *conn_info;
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= DS_FMC_APP_MAX_CONNECTIONS)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: bad conn_id = %d!\n",
                  conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  if (rx_buf == NULL)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: NULL rxbuf pointer\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Assure that power-up init function has been called */
  if (ds_fmc_app_data_mdm_rx_cb == NULL)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: power-up init not called\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set up pointer to connection info */
  conn_info = (ds_fmc_app_data_mdm_conn_info_type *)
              &ds_fmc_app_data_mdm_conn_info[conn_id];

  if (conn_info->active)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: connection"
                          " already opened = %d!\n",conn_id);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* Set the RX buffer in connection info */
  conn_info->rx_buf = rx_buf;

  /* Open the SMD port */
  conn_info->f_desc = open (conn_info->port_id_name,O_RDWR);

  if (conn_info->f_desc < 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: Unable to "
                       "open port id %s\n", conn_info->port_id_name);
    conn_info->rx_buf = NULL;
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE; 
  }

  /* Spawn RX thread and pass to it the QMUX connection ID */
  if ((pthread_create (&conn_info->th_id,
                       NULL,
                       ds_fmc_app_data_mdm_rx_data,
                       (void *) conn_id)) != 0)
  {
    ds_fmc_app_log_err ("ds_fmc_app_data_mdm_open_conn: can't create RX thread");
    close( conn_info->f_desc );
    conn_info->f_desc = -1;
    conn_info->rx_buf = NULL;
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  /* We are done, mark connection as active and return success code */
  conn_info->active = TRUE;

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_data_mdm_open_conn */

/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_close_conn
===========================================================================*/
/*!

@brief 
  Function used to close a connection. This function must 
  be called when the controlling process gets killed . 
  
@return 
  - void
@note
  - Side Effects
    - Closes the SMD channel and corresponding threads assoc. with
      RX handling.
    
*/    
/*=========================================================================*/
void
ds_fmc_app_data_mdm_close_conn 
(
  void
)
{
  ds_fmc_app_data_mdm_conn_info_type  *conn_info;
  int                              conn_id;

  /* Verify the conn_id parameter since it will be used to index
  ** into an array
  */
  for (conn_id = 0; conn_id < DS_FMC_APP_MAX_CONNECTIONS; conn_id++)
  {

    /* Set up pointer to connection info */
    conn_info = (ds_fmc_app_data_mdm_conn_info_type* )
                &ds_fmc_app_data_mdm_conn_info[conn_id];

    if (!conn_info->active)
    {
      ds_fmc_app_log_err ("ds_fmc_app_data_mdm_close_conn: connection"
                            " already closed = %d!\n",conn_id);
      DS_FMC_APP_LOG_FUNC_EXIT;
      continue ;
    }

    /* Set the RX buffer in connection info */
    conn_info->rx_buf = NULL;

    /* Need not reset port_id_name, as that remains the same */
    if( close( conn_info->f_desc ) < 0 )
    {
      ds_fmc_app_log_err("ds_fmc_app_data_mdm_close_conn: "
                         "cannot close SMD channel\n");
    }

    conn_info->f_desc = -1;

    /* We are done, mark connection as in-active and return success code */
    conn_info->active = FALSE;

    /* Stop the socket listener thread */
    if( pthread_kill( conn_info->th_id, SIGUSR1 ) != 0 ) 
    {
      ds_fmc_app_log_err("ds_fmc_app_data_mdm_close_conn: Error "
                         "cannot stop sock listener thread\n");
    }

  }

  return ;
} /* ds_fmc_app_data_mdm_close_conn */


/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the ds_fmc_app_data_mdm_open_conn() 
  
@return 
  - void

@note

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
void
ds_fmc_app_data_mdm_init
(
  ds_fmc_app_data_mdm_rx_cb_ptr  rx_func
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;

  atexit(ds_fmc_app_data_mdm_close_conn);

  if (ds_fmc_app_data_mdm_rx_cb == NULL)
  {
    ds_fmc_app_data_mdm_rx_cb = rx_func;
  }
  else
  {
    DS_FMC_APP_STOP("ds_fmc_app_data_mdm_init: Error reiniting an already"
                      " initialized data MDM entity\n" );
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return ;
} /* ds_fmc_app_data_mdm_init */ 
