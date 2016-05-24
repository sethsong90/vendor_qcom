/******************************************************************************

                        QTI_RMNET_USB.C

******************************************************************************/

/******************************************************************************

  @file    qti_rmnet_usb.c
  @brief   Qualcomm Tethering Interface for RMNET tethering. This file contains
           QTI interaction with USB for RMNET tethering

  DESCRIPTION
  Implementation file for QTI inteaction with USB for RMnet tethering.

  ---------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------


******************************************************************************/


/******************************************************************************

                      EDIT HISTORY FOR FILE

when       who        what, where, why
--------   ---        -------------------------------------------------------
12/14/12   sb         Initial version

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <fcntl.h>

#include "qti.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEstd.h"


static qti_rmnet_state_config        * rmnet_state_config;

/*===========================================================================
                          FUNCTION DEFINITIONS
============================================================================*/
/*===========================================================================

FUNCTION PRINT_BUFFER()

DESCRIPTION

  This function
  - prints QMI message.

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
void print_buffer
(
  char    *buf,
  int      size
)
{
  int i;

  for(i=0; i < size; i++)
  {
    if(i%8 == 0)
      printf("\n%02X ", buf[i]);
    else
      printf("%02X ", buf[i]);
  }
  printf("\n");
}

/*===========================================================================

FUNCTION QTI_RMNET_USB_RECV_MSG()

DESCRIPTION

  This function
  - receives QMI messages from USB interface

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/


int qti_rmnet_usb_recv_msg
(
   int rmnet_usb_fd
)
{
  int        ret;
  char      usb_rx_buf[RMNET_USB_MAX_TRANSFER_SIZE];


 /*-------------------------------------------------------------------------*/

  ret = read(rmnet_usb_fd, usb_rx_buf, RMNET_USB_MAX_TRANSFER_SIZE);
  LOG_MSG_INFO1("QTI%d: QTI read %d bytes from USB file", rmnet_state_config->ipa_lcid, ret, 0);
  if (ret < 0)
  {
    LOG_MSG_ERROR("QTI%d:Failed to read from the dev file. errno : %d ",
                   rmnet_state_config->ipa_lcid, errno, 0);
  }
/*----------------------------------------------------------------------------
  USB cable plug out/ plug in happened
----------------------------------------------------------------------------*/
  else if (ret == 0)
  {
    process_usb_reset();
  }
  else
  {
    //print_buffer(usb_rx_buf, ret);
/*----------------------------------------------------------------------------
  Process and send QMI message to modem
----------------------------------------------------------------------------*/
    qti_qmux_tx_to_modem((qti_qmux_msg_s *) &usb_rx_buf,
                         (uint32) ret);
  }

  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_RMNET_USB_FILE_OPEN()

DESCRIPTION

  This function
  - opens the device file which is used for interfacing with USB

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/


static int qti_rmnet_usb_file_open
(
   char * rmnet_usb_dev_file,
   int * usb_fd
)
{
  int qti_rmnet_fd;
/*--------------------------------------------------------------------------*/
  qti_rmnet_fd = open(rmnet_usb_dev_file, O_RDWR);
  if(qti_rmnet_fd == -1)
  {
    LOG_MSG_ERROR("QTI%d:Could not open device file, errno : %d ",
                   rmnet_state_config->ipa_lcid, errno, 0);
    return QTI_FAILURE;
  }
  else
  {
    LOG_MSG_INFO1("QTI%d:Successfully opened USB device file. FD is %d",
                   rmnet_state_config->ipa_lcid, qti_rmnet_fd, 0);
    *usb_fd = qti_rmnet_fd;
    rmnet_state_config->qti_qmi_mess_state = QTI_RX_CLIENT_WAIT ;
    return QTI_SUCCESS;
  }
}

/*===========================================================================

FUNCTION QTI_RMNET_MAP_FD_READ()

DESCRIPTION

  This function
  - adds the USB fd to the list of FD on which select call listens
  - maps the read fucntion for the USB fd

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

static int qti_rmnet_map_fd_read
(
   qti_nl_sk_fd_set_info_t *fd_set,
   int                     usb_fd,
   qti_sock_thrd_fd_read_f read_f
)
{
  if( fd_set->num_fd < MAX_NUM_OF_FD )
  {
    FD_SET(usb_fd, &(fd_set->fdset));
/*--------------------------------------------------------------------------
  Add fd to fdmap array and store read handler function ptr
-------------------------------------------------------------------------- */
    fd_set->sk_fds[fd_set->num_fd].sk_fd = usb_fd;
    fd_set->sk_fds[fd_set->num_fd].read_func = read_f;
    LOG_MSG_INFO1("QTI%d:Added read function for fd %d", rmnet_state_config->ipa_lcid, usb_fd, 0);

/*--------------------------------------------------------------------------
  Increment number of fds stored in fdmap
--------------------------------------------------------------------------*/
    fd_set->num_fd++;
    if(fd_set->max_fd < usb_fd)
    {
      LOG_MSG_INFO1("QTI%d:Updating USB max fd %d", rmnet_state_config->ipa_lcid, usb_fd, 0);
      fd_set->max_fd = usb_fd;
    }
  }
  else
  {
    LOG_MSG_ERROR("QTI%d:Exceeds maximum num of FD", rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }

	return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_RMNET_LISTENER_INIT()

DESCRIPTION

  This function
  - opens the USB device file
  - adds the USB fd to wait on select call

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

int qti_rmnet_listener_init
(
  qti_rmnet_state_config  * rmnet_state,
  qti_nl_sk_fd_set_info_t * fd_set,
  qti_sock_thrd_fd_read_f read_f
)
{
  int ret_val;
/*-------------------------------------------------------------------------*/
  LOG_MSG_INFO1("Open USB file to receive QMI messages", 0, 0, 0);
  rmnet_state_config = rmnet_state;
  ret_val = qti_rmnet_usb_file_open(rmnet_state->usb_device_file, &(rmnet_state->usb_fd));

  LOG_MSG_INFO1("QTI%d:Opened file's fd is %d",
                 rmnet_state_config->ipa_lcid, rmnet_state->usb_fd, 0);
  if(ret_val == QTI_FAILURE)
  {
    LOG_MSG_ERROR("QTI%d:Failed to open USB device file. Abort",
                   rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }
  else
  {
    ret_val = qti_rmnet_map_fd_read(fd_set,rmnet_state->usb_fd, read_f);
    if(ret_val == QTI_FAILURE)
    {
      LOG_MSG_ERROR("QTI%d:Failed to map fd with the read function",
                     rmnet_state_config->ipa_lcid, 0, 0);
      return QTI_FAILURE;
    }
  }
  return QTI_SUCCESS;
}


/*===========================================================================

FUNCTION QTI_RMNET_USB_SEND_MSG()

DESCRIPTION

  This function
  - send QMI message to USB

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/

void qti_rmnet_usb_send_msg
(
   void      *data,
   uint32     len
)
{
  int ret;
/*-----------------------------------------------------------------------*/
  LOG_MSG_INFO1("QTI%d: Will write message of size %d to USB file",
                 rmnet_state_config->ipa_lcid, len, 0);

  //print_buffer((char*)data, len);
  ret = write(rmnet_state_config->usb_fd, (char*)data, len);
  if (ret == -1)
  {
    LOG_MSG_ERROR("QTI%d:Couldn't send message to host: %d",
                   rmnet_state_config->ipa_lcid, errno, 0);
  }
  else if (ret != len)
  {
    LOG_MSG_ERROR_6("QTI%d: Unexpected return value when writing to device file: got %d, "
                  "expected %d (errno %d)",
                   rmnet_state_config->ipa_lcid,
                   ret,
                   len,
                   errno, 0 , 0);
  }
  else
  {
    LOG_MSG_INFO1("QTI%d:Successfully sent message to host\n", rmnet_state_config->ipa_lcid, 0, 0);
    rmnet_state_config->qti_qmi_mess_state = QTI_RX_CLIENT_WAIT;
  }
}
