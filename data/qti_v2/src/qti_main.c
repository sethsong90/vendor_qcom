/******************************************************************************

                        QTI_MAIN.C

******************************************************************************/

/******************************************************************************

  @file    qti_main.c
  @brief   Qualcomm Tethering Interface Module

  DESCRIPTION
  Implementation of Qualcomm Tethering Interface.

  ---------------------------------------------------------------------------
  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------


******************************************************************************/


/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/15/12   sb         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "qti.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================
                              VARIABLE DECLARATIONS
===========================================================================*/

static qti_conf_t                qti_conf;


/*===========================================================================
                              FUNCTION DEFINITIONS
===========================================================================*/
/*===========================================================================

FUNCTION QTI_NL_SOCK_LISTENER_START()

DESCRIPTION

  This function
  - calls the select system call and listens to netlink events coming on
    netlink socket and any data coming on device file used for Rmnet
	tethering
  - once the netlink event is got or data is received on the Rmnet tethering
    device file corresponding registered functions for that socket are
	called

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

==========================================================================*/
static int qti_listener_start
(
  qti_nl_sk_fd_set_info_t    * sk_fd_set
)
{
  int            i,ret;

/*-------------------------------------------------------------------------*/
  LOG_MSG_INFO1("Starting Netlink listener",0,0,0);

/*--------------------------------------------------------------------------
  Update QTI state
---------------------------------------------------------------------------*/
  qti_conf.state = QTI_LINK_UP_WAIT;

  while(TRUE)
  {

    for(i = 0; i < sk_fd_set->num_fd; i++ )
    {
      FD_SET(sk_fd_set->sk_fds[i].sk_fd, &(sk_fd_set->fdset));
    }

/*--------------------------------------------------------------------------
    Call select system function which will listen to netlink events
    coming on netlink socket which we would have opened during
    initialization
--------------------------------------------------------------------------*/
    if((ret = select(sk_fd_set->max_fd+1,
                     &(sk_fd_set->fdset),
                     NULL,
                     NULL,
                     NULL)) < 0)
    {
      LOG_MSG_ERROR("qti_nl select failed", 0, 0, 0);
    }
    else
    {
      for(i = 0; i < sk_fd_set->num_fd; i++ )
      {
        if( FD_ISSET(sk_fd_set->sk_fds[i].sk_fd,
                     &(sk_fd_set->fdset) ) )
        {
          if(sk_fd_set->sk_fds[i].read_func)
          {
            if( QTI_SUCCESS != ((sk_fd_set->sk_fds[i].read_func)(sk_fd_set->sk_fds[i].sk_fd)) )
            {
              LOG_MSG_ERROR("Error on read callback[%d] fd=%d",
                            i,
                            sk_fd_set->sk_fds[i].sk_fd,
                            0);
            }
            else
            {
              FD_CLR(sk_fd_set->sk_fds[i].sk_fd, &(sk_fd_set->fdset));
            }
          }
          else
          {
            LOG_MSG_ERROR("No read function",0,0,0);
          }
        }
      }
    }
  }
  return QTI_SUCCESS;
}


/*==========================================================================

FUNCTION MAIN()

DESCRIPTION

  The main function for QTI which is first called when QTI gets started on
  boot up.

DEPENDENCIES
  None.

RETURN VALUE
  0 on SUCCESS
  -1 on FAILURE

SIDE EFFECTS
  None

==========================================================================*/
int main(int argc, char ** argv)
{
  int                       ret_val;
  qti_nl_sk_fd_set_info_t   sk_fdset;
  qti_rmnet_state_config    rmnet_state_param;

/*------------------------------------------------------------------------*/
    /* Initializing Diag for QXDM loga*/
  if (TRUE != Diag_LSM_Init(NULL))
  {
     printf("Diag_LSM_Init failed !!");
  }

  LOG_MSG_INFO1("Start QTI", 0, 0, 0);

/*-----------------------------------------------------------------------
    Initialize QTI variables
------------------------------------------------------------------------*/
  memset(&sk_fdset, 0, sizeof(qti_nl_sk_fd_set_info_t));
  memset(&qti_conf, 0, sizeof(qti_conf_t));
  memset(&rmnet_state_param, 0, sizeof(qti_rmnet_state_config));

  switch(argc)
  {
    /* If command line parameters were entered, ...*/
    case 3:
      printf("Got 3 arguements from main \n");

      if ((0 == strncasecmp(argv[1],
                            RMNET_USB_DEV_FILE_PATH,
                            strlen(RMNET_USB_DEV_FILE_PATH))) &&
          (0 == strncasecmp(argv[2],
                            USB_TETHERED_SMD_CH,
                            strlen(USB_TETHERED_SMD_CH))))
      {
        rmnet_state_param.multi_rmnet_enabled = FALSE;
        rmnet_state_param.qmux_conn_id = QMI_CONN_ID_RMNET_8;
        rmnet_state_param.ipa_lcid = IPA_LCID_USB_TETHERED;
        memcpy(rmnet_state_param.usb_device_file, argv[1], strlen(RMNET_USB_DEV_FILE_PATH));
        memcpy(rmnet_state_param.smd_device_file, argv[2], strlen(USB_TETHERED_SMD_CH));
      }
      else if ((0 == strncasecmp(argv[1],
                                 MULTI_RMNET_USB_DEV_FILE_PATH_1,
                                 strlen(MULTI_RMNET_USB_DEV_FILE_PATH_1))) &&
               (0 == strncasecmp(argv[2],
                                 MULTI_RMNET_SMD_CH_1,
                                 strlen(MULTI_RMNET_SMD_CH_1))))
      {
        rmnet_state_param.multi_rmnet_enabled = TRUE;
        rmnet_state_param.qmux_conn_id = QMI_CONN_ID_RMNET_9;
        rmnet_state_param.ipa_lcid = IPA_LCID_MULTI_RMNET_1;
        memcpy(rmnet_state_param.usb_device_file, argv[1], strlen(MULTI_RMNET_USB_DEV_FILE_PATH_1));
        memcpy(rmnet_state_param.smd_device_file, argv[2], strlen(MULTI_RMNET_SMD_CH_1));
      }
      else if((0 == strncasecmp(argv[1],
                                MULTI_RMNET_USB_DEV_FILE_PATH_2,
                                strlen(MULTI_RMNET_USB_DEV_FILE_PATH_2))) &&
              (0 == strncasecmp(argv[2],
                                MULTI_RMNET_SMD_CH_2,
                                strlen(MULTI_RMNET_SMD_CH_2))))
      {
        rmnet_state_param.multi_rmnet_enabled = TRUE;
        rmnet_state_param.qmux_conn_id = QMI_CONN_ID_RMNET_10;
        rmnet_state_param.ipa_lcid = IPA_LCID_MULTI_RMNET_2;
        memcpy(rmnet_state_param.usb_device_file,argv[1], strlen(MULTI_RMNET_USB_DEV_FILE_PATH_2));
        memcpy(rmnet_state_param.smd_device_file, argv[2], strlen(MULTI_RMNET_SMD_CH_2));
      }

      else if((0 == strncasecmp(argv[1],
                                MULTI_RMNET_USB_DEV_FILE_PATH_3,
                                strlen(MULTI_RMNET_USB_DEV_FILE_PATH_3))) &&
              (0 == strncasecmp(argv[2],
                                MULTI_RMNET_SMD_CH_3,
                                strlen(MULTI_RMNET_SMD_CH_3))))
      {
        rmnet_state_param.multi_rmnet_enabled = TRUE;
        rmnet_state_param.qmux_conn_id = QMI_CONN_ID_RMNET_11;
        rmnet_state_param.ipa_lcid = IPA_LCID_MULTI_RMNET_3;
        memcpy(rmnet_state_param.usb_device_file,argv[1], strlen(MULTI_RMNET_USB_DEV_FILE_PATH_3));
        memcpy(rmnet_state_param.smd_device_file, argv[2], strlen(MULTI_RMNET_SMD_CH_3));
      }
      else
      {
        printf("\n Exiting main \n");
        exit(1);
      }
      break;
    default:
      printf("Incorrect paramaters entered. Enter qti <USB dev file> <SMD dev file>");
      exit(1);
      break;
  }
/*-----------------------------------------------------------------------
    Initialize QTI interfaces for netlink events
------------------------------------------------------------------------*/
  if(rmnet_state_param.multi_rmnet_enabled == FALSE)
  {

    ret_val = qti_netlink_init(&qti_conf);

    if(ret_val != QTI_SUCCESS)
    {
      LOG_MSG_ERROR("QTI %d : Failed to initialize netlink interfaces for QTI",
                     rmnet_state_param.ipa_lcid, 0, 0);
      return QTI_FAILURE;
    }

/*-----------------------------------------------------------------------
    Initialize QTI to be a client of QCMAP
------------------------------------------------------------------------*/
    ret_val = qti_qcmap_init(&qti_conf);
    if(ret_val != QTI_SUCCESS)
    {
      LOG_MSG_ERROR("QTI %d: Failed to initialize QTI as a QCMAP client",
                     rmnet_state_param.ipa_lcid, 0, 0);
      return QTI_FAILURE;
    }

/*---------------------------------------------------------------------
   Initialize QTI command queue
----------------------------------------------------------------------*/
    qti_cmdq_init();

/*---------------------------------------------------------------------
  Call into the netlink listener init function which sets up QTI to
  listen to netlink events
---------------------------------------------------------------------*/
    ret_val = qti_nl_listener_init(NETLINK_ROUTE,
                                   RTMGRP_LINK,
                                   &sk_fdset,
                                   qti_nl_recv_msg);
    if(ret_val != QTI_SUCCESS)
    {
      LOG_MSG_ERROR("QTI %d : Failed to initialize QTI netlink event listener",
                     rmnet_state_param.ipa_lcid, 0, 0);
      return QTI_FAILURE;
    }

  }
/*---------------------------------------------------------------------
  Call into the rmnet listener init function which sets up QTI to
  listen to QMI packets coming in from the USB device file for Rmnet
---------------------------------------------------------------------*/
  ret_val = qti_rmnet_listener_init(&rmnet_state_param,
                                    &sk_fdset,
                                    qti_rmnet_usb_recv_msg);
  if(ret_val != QTI_SUCCESS)
  {
    LOG_MSG_ERROR(" QTI %d : Failed to initialize QTI Rmnet QMI message listener",
                  rmnet_state_param.ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }

/*---------------------------------------------------------------------
  Initialize QMUXD handle for sending QMUX packets
---------------------------------------------------------------------*/

  ret_val = qti_rmnet_qmux_open(qti_qmux_rx_cb, &rmnet_state_param);

  if(ret_val != QTI_SUCCESS)
  {
    LOG_MSG_ERROR("QTI %d:Failed to initialize QMUXD interface",
                  rmnet_state_param.ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }

/*--------------------------------------------------------------------
  Start the listener which listens to netlink events and QMI packets
  coming on USB-Rmnet device file
---------------------------------------------------------------------*/
  ret_val = qti_listener_start(&sk_fdset);

  if(ret_val != QTI_SUCCESS)
  {
    LOG_MSG_ERROR("QTI %d:Failed to start NL listener",
                   rmnet_state_param.ipa_lcid, 0, 0);
  }

/*--------------------------------------------------------------------
  Wait for the QTI command queue to finish before exiting QTI
---------------------------------------------------------------------*/
  if(rmnet_state_param.multi_rmnet_enabled == FALSE)
  {
    qti_cmdq_wait();

    qti_qcmap_exit();
  }

  return QTI_SUCCESS;
}


