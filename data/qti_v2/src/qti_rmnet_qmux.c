/******************************************************************************

                        QTI_RMNET_QMUX.C

******************************************************************************/

/******************************************************************************

  @file    qti_rmnet_qmux.c
  @brief   Qualcomm Tethering Interface for RMNET tethering. This file
           has functions which interact with QXMUD APIs for RMNET tethering.

  DESCRIPTION
  Has functions which interact with QXMUD APIs for RMNET tethering.

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
#include "qmi_qmux_if.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEstd.h"
#include "wireless_data_administrative_service_v01.h"


static set_data_format_param_s param;
static set_data_format_status status;
static  qti_rmnet_state_config        * rmnet_state_config;

/*===========================================================================
                               FUNCTION DEFINITIONS
/*=========================================================================*/
/*===========================================================================

FUNCTION QTI_RMNET_QMUX_OPEN()

DESCRIPTION

  This function
  - sets up the interface between QTI and QMUX.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int qti_rmnet_qmux_open
(
  qmi_qmux_if_rx_msg_hdlr_type qti_qmux_rx_cb,
  qti_rmnet_state_config       * rmnet_state
)
{
  int qmi_err = QMI_NO_ERR;

  rmnet_state_config = rmnet_state;

/*------------------------------------------------------------------------*/

  LOG_MSG_INFO1("QTI %d:Get a QMUX handle for QTI", rmnet_state_config->ipa_lcid, 0, 0);

/*------------------------------------------------------------------------
  Initialize a QMUX client and obtain a handle
-------------------------------------------------------------------------*/
  qmi_err = qmi_qmux_if_pwr_up_init_ex(
            qti_qmux_rx_cb, NULL, NULL, &qti_qmux_qmi_handle,
            QMI_QMUX_IF_CLNT_MODE_RAW);

  if (qmi_err != QMI_NO_ERR)
  {
    LOG_MSG_ERROR("QTI %d:Failed to get a QMUX handle for QTI", rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }
  else
  {
    qti_qmux_qmi_handle_valid = TRUE;
    LOG_MSG_INFO1("QTI %d : Succeeded to get a QMUX handle for QTI", rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_SUCCESS;
  }
}

/*===========================================================================

FUNCTION QTI_QMUX_TO_QMI_CONN_ID()

DESCRIPTION

  This function
  - maps to the right connection id corresponding to the USB tethered
  data call

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
static int qti_qmux_to_qmi_conn_id
(
  qmi_connection_id_type *qmi_conn_id
)
{
  boolean success = TRUE;
/*-------------------------------------------------------------------------*/
  ds_assert(qmi_conn_id != NULL);

  *qmi_conn_id = rmnet_state_config->qmux_conn_id;
  return QTI_SUCCESS;
}


/*===========================================================================

FUNCTION QTI_QMUX_BUF_ALLOC()

DESCRIPTION

  This function
  - allocates buffer to store the QMI QMUX packet

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

static int qti_qmux_buf_alloc
(
  qti_qmux_buf_s *buf,
  uint32          size
)
{
  void *data_ptr;
/*-------------------------------------------------------------------------*/
  ds_assert(buf != NULL);

/*--------------------------------------------------------------------------
  Protect against memory leaks via successive calls to buf_alloc
--------------------------------------------------------------------------*/
  if (buf->data != NULL)
  {
    LOG_MSG_INFO1("QTI %d:Attempted double-alloc of buffer! Old size %d new size %d",
                  rmnet_state_config->ipa_lcid, buf->size, size);
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
  }

/*--------------------------------------------------------------------------
  Allocate data for storing QMI packet
--------------------------------------------------------------------------*/
  data_ptr = malloc(size);
  if (data_ptr == NULL)
  {
    LOG_MSG_ERROR("QTI %d:Could not allocate data_ptr ", rmnet_state_config->ipa_lcid, 0, 0);
    buf->size = 0;
    return QTI_FAILURE;
  }
  else
  {
    memset(data_ptr, 0, size);
    buf->data = data_ptr;
    buf->size = size;
  }

  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION SET_VALUE()

DESCRIPTION

  This function
  - allows modifying of values present in QMI message TLVs

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

static void set_value
(
  void * data,
  int bytes_processed,
  int size,
  uint32 change_value
)
{
  uint32 * value = NULL;
/*------------------------------------------------------------------------*/

  value = malloc(sizeof(uint32));
  if (value == NULL)
  {
    LOG_MSG_ERROR("QTI %d : Could not allocate memory in set_value of size %d ",
                  rmnet_state_config->ipa_lcid, sizeof(uint32), 0);
    return ;
  }
  *value = change_value;
  memcpy(((uint8 *)data + bytes_processed),(uint8 *)value,size);
  free(value);
  return;
}

/*===========================================================================

FUNCTION GET_VALUE()

DESCRIPTION

  This function
  - gets the value of TLV present in QMI message

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
static int get_value
(
  void * data,
  int bytes_processed,
  int size
)
{
  int value;
/*------------------------------------------------------------------------*/
  void * value_data = NULL;
  value_data = malloc(size);
  if (value_data == NULL)
  {
    LOG_MSG_ERROR("QTI %d : Could not allocate memory in get_value of size %d ",
                  rmnet_state_config->ipa_lcid, size, 0);
    return QTI_FAILURE ;
  }
  memcpy((uint8 *)value_data,((uint8 *)data + bytes_processed),size);
  value = *(int *)value_data;
  free(value_data);
  return value;
}

/*===========================================================================

FUNCTION GET_VALUE()

DESCRIPTION

  This function
  - calls the IOCTL to set link protocol in bridge driver

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
static int set_bridge_mode(enum teth_link_protocol_type link_protocol)
{
  int fd;
  int ret;
  struct teth_ioc_set_bridge_mode set_bridge_mode;
/*- - - -- - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - */

  LOG_MSG_INFO1("QTI %d : Set bridge mode to %d request",
                rmnet_state_config->ipa_lcid, link_protocol, 0);

  set_bridge_mode.link_protocol = link_protocol;
  set_bridge_mode.lcid = rmnet_state_config->ipa_lcid;

  if ((fd=open(IPA_BRIDGE_DRIVER_FILE,O_RDWR)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:Opening IPA bridge driver file failed error %d",
                  rmnet_state_config->ipa_lcid, errno, 0);
    return QTI_FAILURE;
  }

  if ((ret = ioctl(fd,
            TETH_BRIDGE_IOC_SET_BRIDGE_MODE,
            &set_bridge_mode)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:IOCTL to set bridge mode to %d failed",
                   rmnet_state_config->ipa_lcid, link_protocol, 0);
    close(fd);
    return QTI_FAILURE;
  }
  else
  {
    LOG_MSG_INFO1("QTI %d:IOCTL to set bridge mode to %d succeeded",
                  rmnet_state_config->ipa_lcid, link_protocol, 0);
    close(fd);
  }
  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION GET_AGGREGATION_PARAMETERS()

DESCRIPTION

  This function
  - calls the IOCTL to get aggregation parameters from bridge driver

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
static int get_aggregation_parameters
(
  struct teth_aggr_params * aggr_param
)
{
  int fd;
  int ret;
  struct teth_ioc_aggr_params teth_aggr_params;
/*----------------------------------------------------------------------- */

  LOG_MSG_INFO1("QTI %d:Get aggregation parameters", rmnet_state_config->ipa_lcid, 0, 0);

  teth_aggr_params.lcid = rmnet_state_config->ipa_lcid;

  if ((fd=open(IPA_BRIDGE_DRIVER_FILE,O_RDWR)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:Opening IPA bridge driver file failed error %d",
                  rmnet_state_config->ipa_lcid, errno, 0);
    return QTI_FAILURE;
  }

  if ((ret = ioctl(fd,
            TETH_BRIDGE_IOC_GET_AGGR_PARAMS,
            &teth_aggr_params)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:IOCTL to get aggregation parameters failed",
                  rmnet_state_config->ipa_lcid, 0, 0);
    close(fd);
    return QTI_FAILURE;
  }
  else
  {

    LOG_MSG_INFO1_6(" QTI %d :IOCTL to get aggr param succeeded. UL aggr prot %d ; DL aggr prot %d; DL max size %d ; DL max datagrams %d",
                   rmnet_state_config->ipa_lcid,
                   teth_aggr_params.aggr_params.ul.aggr_prot,
                   teth_aggr_params.aggr_params.dl.aggr_prot,
                   teth_aggr_params.aggr_params.dl.max_transfer_size_byte,
                   teth_aggr_params.aggr_params.dl.max_datagrams,
                   0);

    if (teth_aggr_params.aggr_params.ul.aggr_prot == TETH_AGGR_PROTOCOL_NONE)
    {
      aggr_param->ul.aggr_prot = WDA_UL_DATA_AGG_DISABLED_V01;
    }
    else if (teth_aggr_params.aggr_params.ul.aggr_prot == TETH_AGGR_PROTOCOL_TLP)
    {
      aggr_param->ul.aggr_prot = WDA_UL_DATA_AGG_TLP_ENABLED_V01;
    }
    else if (teth_aggr_params.aggr_params.ul.aggr_prot == TETH_AGGR_PROTOCOL_MBIM )
    {
      aggr_param->ul.aggr_prot = WDA_UL_DATA_AGG_QC_NCM_ENABLED_V01;
    }

    if (teth_aggr_params.aggr_params.dl.aggr_prot == TETH_AGGR_PROTOCOL_NONE)
    {
      aggr_param->dl.aggr_prot = WDA_DL_DATA_AGG_DISABLED_V01;
    }
    else if (teth_aggr_params.aggr_params.dl.aggr_prot == TETH_AGGR_PROTOCOL_TLP)
    {
      aggr_param->dl.aggr_prot = WDA_DL_DATA_AGG_TLP_ENABLED_V01;
    }
    else if (teth_aggr_params.aggr_params.dl.aggr_prot == TETH_AGGR_PROTOCOL_MBIM )
    {
      aggr_param->dl.aggr_prot = WDA_DL_DATA_AGG_QC_NCM_ENABLED_V01;
    }

    aggr_param->dl.max_transfer_size_byte = teth_aggr_params.aggr_params.dl.max_transfer_size_byte;
    aggr_param->dl.max_datagrams = teth_aggr_params.aggr_params.dl.max_datagrams;

    LOG_MSG_INFO1_6("QTI %d:WDA parameters. UL aggr prot %d ; DL aggr prot %d; DL max size %d ; DL max datagrams %d",
                   rmnet_state_config->ipa_lcid,
                   aggr_param->ul.aggr_prot,
                   aggr_param->dl.aggr_prot,
                   aggr_param->dl.max_transfer_size_byte,
                   aggr_param->dl.max_datagrams,
                   0);
    close(fd);
  }
  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION SET_AGGREGATION_PARAMETERS()

DESCRIPTION

  This function
  - calls the IOCTL to set aggregation parameters in bridge driver

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
static int set_aggregation_parameters
(
  enum teth_aggr_protocol_type ul,
  enum teth_aggr_protocol_type dl,
  uint8_t dl_max_datagrams,
  uint8_t dl_max_size
)
{
  struct teth_ioc_aggr_params teth_aggr_param;
  int fd;
  int ret;
/*------------------------------------------------------------------------*/

  memset(&teth_aggr_param, 0, sizeof(struct teth_ioc_aggr_params));

  LOG_MSG_INFO1_6("QTI %d:Set aggregation parameter request. UL aggr prot :%d; DL aggr prot :%d; DL max datagrams :%d; DL max size :%d ",
                  rmnet_state_config->ipa_lcid,
                  ul,
                  dl,
                  dl_max_datagrams,
                  dl_max_size,
                  0);

  teth_aggr_param.lcid = rmnet_state_config->ipa_lcid;
  teth_aggr_param.aggr_params.ul.aggr_prot = ul;
  teth_aggr_param.aggr_params.dl.aggr_prot = dl;
  teth_aggr_param.aggr_params.dl.max_transfer_size_byte = dl_max_size;
  teth_aggr_param.aggr_params.dl.max_datagrams = dl_max_datagrams;

  if ((fd=open(IPA_BRIDGE_DRIVER_FILE,O_RDWR)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:Opening IPA bridge driver file failed error %d ",
                  rmnet_state_config->ipa_lcid, errno, 0);
    status.aggr_param_status = FALSE;
    return QTI_FAILURE;
  }

  if ((ret = ioctl(fd,
            TETH_BRIDGE_IOC_SET_AGGR_PARAMS,
            &teth_aggr_param)) < 0)
  {
    LOG_MSG_ERROR("QTI %d:IOCTL to set aggr parameters failed",
                  rmnet_state_config->ipa_lcid, 0, 0);
    status.aggr_param_status = FALSE;
    close(fd);
    return QTI_FAILURE;
  }
  else
  {
    LOG_MSG_INFO1("QTI %d:IOCTL to set aggr parameters succeeded",
                  rmnet_state_config->ipa_lcid, 0, 0);
    close(fd);
  }
  return QTI_SUCCESS;

}


/*===========================================================================

FUNCTION QTI_SET_DTR()

DESCRIPTION

  This function
  - toggles DTR.

DEPENDENCIES
  None.

RETURN VALUE

SIDE EFFECTS
  None

/*=========================================================================*/
int qti_set_dtr(uint8_t set)
{
  int             fd = 0;
  int             dtr_sig;
  int             ret = QTI_FAILURE;
/*------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
  Open USB tethered SMD channel port
-------------------------------------------------------------------------*/
  fd = open(rmnet_state_config->smd_device_file, O_RDWR);
  if (fd < 0)
  {
    LOG_MSG_ERROR("QTI %d:Opening the device file failed errno %d",
                   rmnet_state_config->ipa_lcid, errno, 0);
    return ret;
  }

  LOG_MSG_INFO1("QTI %d:Request to set DTR %d", rmnet_state_config->ipa_lcid, set, 0);

/*-------------------------------------------------------------------------
  Set DTR high
-------------------------------------------------------------------------*/
  if (set == SET_DTR_HIGH)
  {
    dtr_sig = 0;
    dtr_sig |= ioctl(fd, TIOCMGET, &dtr_sig);
    if( dtr_sig >= 0  && !(dtr_sig & TIOCM_DTR))
    {
      LOG_MSG_INFO1("QTI %d:DTR bit not set..will set it to..dtr_sig:%d ",
                    rmnet_state_config->ipa_lcid, dtr_sig, 0);
      dtr_sig |= TIOCM_DTR;
      if((ioctl(fd, TIOCMSET, (void *)dtr_sig)) == -1)
      {
        LOG_MSG_ERROR("QTI %d:Ioctl call to set DTR bit failed errno %d",
                      rmnet_state_config->ipa_lcid, errno, 0);
      }
      else
      {
        dtr_sig = 0;
        dtr_sig |= ioctl(fd, TIOCMGET, &dtr_sig);
        if( dtr_sig >= 0  && (dtr_sig & TIOCM_DTR))
        {
          LOG_MSG_INFO1("QTI %d:DTR bit set:%d", rmnet_state_config->ipa_lcid, dtr_sig, 0);
          ret=QTI_SUCCESS;
        }
        else
        {
          LOG_MSG_ERROR("QTI %d:Unable to set DTR bit", rmnet_state_config->ipa_lcid, 0, 0);
        }
      }
    }
    else if (dtr_sig == -1)
    {
      LOG_MSG_ERROR("QTI %d:Failed to get DTR bits..exiting...failed errno %d",
                     rmnet_state_config->ipa_lcid, errno, 0);
    }
  }
/*-------------------------------------------------------------------------
  Set DTR low
-------------------------------------------------------------------------*/
  else
  {
    dtr_sig = 0;
    dtr_sig |=  ioctl(fd, TIOCMGET, &dtr_sig);
    if( dtr_sig >= 0 && (dtr_sig & TIOCM_DTR))
    {
      LOG_MSG_INFO1("QTI %d:Clearing DTR bit...", rmnet_state_config->ipa_lcid, 0, 0);
      dtr_sig &= (~TIOCM_DTR);
      if(ioctl(fd, TIOCMSET, (void *)dtr_sig) == -1)
      {
        LOG_MSG_ERROR("QTI %d : Set DTR bit failed.. errno %d",
                      rmnet_state_config->ipa_lcid, errno, 0);
      }
      else
      {
        dtr_sig = 0;
        dtr_sig |= ioctl(fd, TIOCMGET, &dtr_sig);
        if( dtr_sig >= 0  && !(dtr_sig & TIOCM_DTR))
        {
          LOG_MSG_INFO1("QTI %d : Successfully reset DTR bit", rmnet_state_config->ipa_lcid, 0, 0);
          ret = QTI_SUCCESS;
        }
        else
        {
          LOG_MSG_ERROR("QTI %d : Unable to Clear DTR bit", rmnet_state_config->ipa_lcid, 0, 0);
        }
      }
    }
    else if( dtr_sig == -1)
    {
      LOG_MSG_ERROR("QTI %d:Failed to get DTR bits..exiting...failed errno %d ",
                     rmnet_state_config->ipa_lcid, errno, 0);
    }
  }
  if(fd > 0)
  {
    close(fd);
    fd = 0;
  }
  return ret;
}


/*===========================================================================

FUNCTION PROCESS_USB_RESET()

DESCRIPTION

  This function
  - peeks into set data format message.
  - sends aggregation parameters to IPA
  - modifies set data format message to send to modem

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int process_usb_reset()
{
  int                             ret_val;
  qti_rmnet_autoconnect_config    config;
  int                             ret;
  int                             line_state;
/*-------------------------------------------------------------------------*/

    memset(&(config), 0, sizeof(qti_rmnet_autoconnect_config));

    LOG_MSG_INFO1("QTI %d:Got zero bytes from the dev file.Need to signal DTR high/low",
                  rmnet_state_config->ipa_lcid, 0, 0);
    ret = ioctl(rmnet_state_config->usb_fd, FRMNET_CTRL_GET_LINE_STATE, &line_state);
    if (ret)
    {
      LOG_MSG_INFO1("QTI %d:Couldn't get FRMNET LINE STATE from driver: %d ",
                     rmnet_state_config->ipa_lcid, errno, 0);
      line_state = -1;
    }
    else
    {
      LOG_MSG_INFO1("QTI %d:FRMNET LINE STATE: %d", rmnet_state_config->ipa_lcid, line_state, 0);
/*----------------------------------------------------------------------------
  USB cable plug in happened
----------------------------------------------------------------------------*/
      if(line_state == 1)
      {
        if(!rmnet_state_config->dtr_enabled)
        {
          ret_val = qti_read_xml(RMNET_CONFIG_FILE, &config);
          if(ret_val == QTI_SUCCESS)
          {
            LOG_MSG_INFO1("QTI %d:Autoconnect value %d ",
                           rmnet_state_config->ipa_lcid, config.auto_connect, 0);
            LOG_MSG_INFO1("QTI %d:Link protocol value %d ",
                           rmnet_state_config->ipa_lcid, config.link_prot, 0);
            LOG_MSG_INFO1("QTI %d:UL aggregation value %d ",
                           rmnet_state_config->ipa_lcid, config.ul_aggr_prot, 0);
            LOG_MSG_INFO1("QTI %d:DL aggregation value %d ",
                           rmnet_state_config->ipa_lcid, config.dl_aggr_prot, 0);
            if (config.auto_connect)
            {
              set_bridge_mode(config.link_prot);
              set_aggregation_parameters(config.ul_aggr_prot,
                                         config.dl_aggr_prot,
                                         0, 0);
            }
          }
          else
          {
            //parameters will be configured to default by bridge driver
          }

          if(qti_set_dtr(SET_DTR_HIGH)<0)
          {
            LOG_MSG_INFO1("QTI %d:QTI failed to signal DTR high ",
                           rmnet_state_config->ipa_lcid, 0, 0);
          }
          else
          {
            LOG_MSG_INFO1("QTI %d :QTI succeeded to signal DTR high",
                           rmnet_state_config->ipa_lcid, 0, 0);
          }

          memset(&(param), 0, sizeof(set_data_format_param_s));
          rmnet_state_config->dtr_enabled = 1;
	}
      }
/*----------------------------------------------------------------------------
  USB cable plug out happened
----------------------------------------------------------------------------*/
      else if(line_state == 0)
      {
        if(rmnet_state_config->dtr_enabled)
        {
          if(qti_set_dtr(SET_DTR_LOW)<0)
          {
            LOG_MSG_INFO1("QTI %d:QTI failed to signal DTR low",
                           rmnet_state_config->ipa_lcid, 0, 0);
          }
          else
          {
            LOG_MSG_INFO1("QTI %d:QTI succeeded to signal DTR low",
                          rmnet_state_config->ipa_lcid, 0, 0);
          }
          memset(&(param), 0, sizeof(set_data_format_param_s));
          rmnet_state_config->dtr_enabled = 0;
        }
      }
    }
}

/*===========================================================================

FUNCTION PROCESS_TX_TO_MODEM_SET_DATA_FORMAT_MESSAGE()

DESCRIPTION

  This function
  - peeks into set data format message.
  - sends aggregation parameters to IPA
  - modifies set data format message to send to modem

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/


static int process_tx_to_modem_set_data_format_message
(
  qti_qmux_msg_s *qmux_msg,
  uint32 qmux_msg_len
)
{
  int        count;
  void       * data = NULL;
  int        bytes_processed = 0;
  uint8      tlv_type;
  uint16     tlv_length;
  uint32     link_protocol = TETH_LINK_PROTOCOL_ETHERNET;
  uint32     ul_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
  uint32     ul_aggr_prot_tx;
  uint32     dl_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
  uint32     dl_aggr_prot_tx;
  uint32     ndp_signature;
  uint32     dl_max_datagrams;
  uint32     dl_max_size;
  uint32     ndp_signature_tx;
  uint32     dl_max_datagrams_tx;
  uint32     dl_max_size_tx;
  int        ret;
  boolean    link_protocol_set = FALSE;
/*----------------------------------------------------------------------------*/

  count = (uint16)qmux_msg->sdu.qmux.msg.msg_length;
  LOG_MSG_INFO1("QTI %d:Message length %d", rmnet_state_config->ipa_lcid, count, 0);
  data = malloc(count);
  if (data == NULL)
  {
    LOG_MSG_ERROR("QTI %d:Could not allocate data for WDA set data format message in TX ",
                   rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }
  memcpy((uint8 *)data,((uint8 *)qmux_msg + QTI_QMUX_SDU_MESSAGE_HDR_LEN_BYTES),count);

  while (bytes_processed < count)
  {
    tlv_type = (uint8)get_value(data, bytes_processed,sizeof(uint8));
    LOG_MSG_INFO1("QTI %d:TLV type rx %d", rmnet_state_config->ipa_lcid, tlv_type, 0);
    bytes_processed += sizeof(uint8);

    tlv_length = (uint16)get_value(data, bytes_processed,sizeof(uint16));
    LOG_MSG_INFO1("QTI%d: TLV length rx %d", rmnet_state_config->ipa_lcid, tlv_length, 0);
    bytes_processed += sizeof(uint16);

    switch (tlv_type)
    {
      case QMI_TYPE_QOS_DATA_FORMAT:
        param.qos_enable = (uint8)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:QOS enable rx %d", rmnet_state_config->ipa_lcid, param.qos_enable, 0);
        bytes_processed += tlv_length;
        break;

      case QMI_TYPE_LINK_PROTOCOL:
        param.link_protocol = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:Link protocol rx %d",
                       rmnet_state_config->ipa_lcid, param.link_protocol, 0);
        if (param.link_protocol == WDA_LINK_LAYER_ETHERNET_MODE_V01)
        {
          link_protocol = TETH_LINK_PROTOCOL_ETHERNET;
        }
        else if (param.link_protocol == WDA_LINK_LAYER_IP_MODE_V01)
        {
          link_protocol = TETH_LINK_PROTOCOL_IP;
        }

/*----------------------------------------------------------------------------
  Call IOCTL to set bridge mode in bridge driver
----------------------------------------------------------------------------*/
        ret = set_bridge_mode(link_protocol);
        if (ret == QTI_FAILURE)
        {
          LOG_MSG_ERROR("QTI%d:Set bridge mode to %d link protocol failed",
                         rmnet_state_config->ipa_lcid, link_protocol, 0);
          status.link_prot_ipa_status = FALSE;
          LOG_MSG_INFO1("QTI%d:Resetting bridge mode to default",
                         rmnet_state_config->ipa_lcid, 0, 0);
/*----------------------------------------------------------------------------
  Call IOCTL to set bridge mode to default in bridge driver
----------------------------------------------------------------------------*/
          ret = set_bridge_mode(TETH_LINK_PROTOCOL_ETHERNET);
          if (ret == QTI_FAILURE)
          {
            LOG_MSG_ERROR("QTI%d:Resetting bridge mode to default failed",
                           rmnet_state_config->ipa_lcid, 0, 0);
          }
          else if (ret == QTI_SUCCESS)
          {
            LOG_MSG_INFO1("QTI%d:Resetting bridge mode to default succeeded",
                           rmnet_state_config->ipa_lcid, 0, 0);
          }

          param.link_protocol = WDA_LINK_LAYER_ETHERNET_MODE_V01;
          LOG_MSG_INFO1("QTI%d:Sending default link protocol as %d to modem",
                         rmnet_state_config->ipa_lcid, param.link_protocol, 0);
          set_value(data, bytes_processed, tlv_length,param.link_protocol);
        }
        else if(ret == QTI_SUCCESS)
        {
          LOG_MSG_INFO1("QTI%d:Set bridge mode to %d link protocol suceeded",
                         rmnet_state_config->ipa_lcid, link_protocol, 0);
          status.link_prot_ipa_status = TRUE;
        }
        bytes_processed += tlv_length;
        link_protocol_set = TRUE;
        break;

      case QMI_TYPE_UL_DATA_AGG_PROTOCOL:
        param.ul_aggr_prot = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:UL aggr_prot rx %d",
                       rmnet_state_config->ipa_lcid, param.ul_aggr_prot, 0);

/*-----------------------------------------------------------------------------
  Send no aggregation to modem
------------------------------------------------------------------------------*/
        ul_aggr_prot_tx = DEFAULT_AGGR_PROTOCOL;
        set_value(data, bytes_processed, tlv_length,ul_aggr_prot_tx );
        LOG_MSG_INFO1("QTI%d:UL aggr_prot tx %d",
                       rmnet_state_config->ipa_lcid, ul_aggr_prot_tx, 0);
        bytes_processed += tlv_length;
        break;

      case QMI_TYPE_DL_DATA_AGG_PROTOCOL:
        param.dl_aggr_prot = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:DL aggr_prot rx %d",
                       rmnet_state_config->ipa_lcid, param.dl_aggr_prot, 0);

/*-----------------------------------------------------------------------------
  Send no aggregation to modem
------------------------------------------------------------------------------*/
        dl_aggr_prot_tx = DEFAULT_AGGR_PROTOCOL;
        set_value(data, bytes_processed, tlv_length,dl_aggr_prot_tx );
        LOG_MSG_INFO1("QTI%d:DL aggr_prot tx %d",
                       rmnet_state_config->ipa_lcid, dl_aggr_prot_tx, 0);
        bytes_processed += tlv_length;
        break;

      case QMI_TYPE_NDP_SIGNATURE:
        param.ndp_signature = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:ndp_signature rx %d",
                       rmnet_state_config->ipa_lcid, param.ndp_signature, 0);

/*-----------------------------------------------------------------------------
  Send no aggregation to modem
------------------------------------------------------------------------------*/
        ndp_signature_tx = DEFAULT_NDP_SIGNATURE;
        set_value(data, bytes_processed, tlv_length,ndp_signature_tx);
        LOG_MSG_INFO1("QTI%d:ndp signature tx %d ",
                       rmnet_state_config->ipa_lcid, ndp_signature_tx, 0);

        bytes_processed += tlv_length;
        break;

      case QMI_TYPE_DL_DATA_AGG_MAX_DATAGRAMS:
        param.dl_max_datagrams = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:dl_max_datagrams rx %d ",
                       rmnet_state_config->ipa_lcid, param.dl_max_datagrams, 0);

/*-----------------------------------------------------------------------------
  Send no aggregation to modem
------------------------------------------------------------------------------*/
        dl_max_datagrams_tx = DEFAULT_MAX_DATAGRAMS;
        set_value(data, bytes_processed, tlv_length,dl_max_datagrams_tx);
        LOG_MSG_INFO1("QTI%d:dl_max_datagrams_tx %d",
                       rmnet_state_config->ipa_lcid, dl_max_datagrams_tx, 0);
        bytes_processed += tlv_length;
        break;

      case QMI_TYPE_DL_DATA_AGG_MAX_SIZE:
        param.dl_max_size = (uint32)get_value(data, bytes_processed, tlv_length);
        LOG_MSG_INFO1("QTI%d:dl_max_size rx %d",
                       rmnet_state_config->ipa_lcid, param.dl_max_size, 0);

/*-----------------------------------------------------------------------------
  Send no aggregation to modem
------------------------------------------------------------------------------*/
        dl_max_size_tx = DEFAULT_MAX_SIZE;
        set_value(data, bytes_processed, tlv_length,dl_max_size_tx);
        LOG_MSG_INFO1("QTI%d:dl_max_size_tx %d", rmnet_state_config->ipa_lcid, dl_max_size_tx, 0);
        bytes_processed += tlv_length;
        break;

      default:
        break;
     }

  }

/*----------------------------------------------------------------------------
  If link protocol information is not present as part of set data format
  set the link protocol to default ie Ethernet mode.
----------------------------------------------------------------------------*/
  if(!link_protocol_set)
  {
    LOG_MSG_INFO1("QTI%d:Link protocol not present in WDA SET DATA FORMAT",
                   rmnet_state_config->ipa_lcid, 0, 0);
/*----------------------------------------------------------------------------
  Call IOCTL to set bridge mode to default in bridge driver
----------------------------------------------------------------------------*/
    ret = set_bridge_mode(TETH_LINK_PROTOCOL_ETHERNET);
    if (ret == QTI_FAILURE)
    {
      LOG_MSG_ERROR("QTI%d:Setting bridge mode to default failed",
                     rmnet_state_config->ipa_lcid, 0, 0);
    }
    else if (ret == QTI_SUCCESS)
    {
      LOG_MSG_INFO1("QTI%d:Setting bridge mode to default succeeded",
                     rmnet_state_config->ipa_lcid, 0, 0);
      status.link_prot_ipa_status = TRUE;
    }
    link_protocol_set = TRUE;
  }


  if (param.ul_aggr_prot == WDA_UL_DATA_AGG_TLP_ENABLED_V01)
  {
    ul_aggr_prot = TETH_AGGR_PROTOCOL_TLP;
  }
  else if (param.ul_aggr_prot == WDA_UL_DATA_AGG_QC_NCM_ENABLED_V01)
  {
    ul_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
  }
  else if (param.ul_aggr_prot == WDA_UL_DATA_AGG_DISABLED_V01)
  {
    ul_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
  }
  else
  {
    ul_aggr_prot = TETH_AGGR_PROTOCOL_MAX;
  }

  if (param.dl_aggr_prot == WDA_DL_DATA_AGG_TLP_ENABLED_V01)
  {
    dl_aggr_prot = TETH_AGGR_PROTOCOL_TLP;
  }
  else if (param.dl_aggr_prot == WDA_DL_DATA_AGG_QC_NCM_ENABLED_V01)
  {
    dl_aggr_prot = TETH_AGGR_PROTOCOL_MBIM;
  }
  else if (param.dl_aggr_prot == WDA_DL_DATA_AGG_DISABLED_V01)
  {
    dl_aggr_prot = TETH_AGGR_PROTOCOL_NONE;
  }
  else
  {
    dl_aggr_prot = TETH_AGGR_PROTOCOL_MAX;
  }

  if (ul_aggr_prot == TETH_AGGR_PROTOCOL_MAX ||
      dl_aggr_prot == TETH_AGGR_PROTOCOL_MAX)
  {
    LOG_MSG_ERROR("QTI%d:Aggregation protocol not supported by IPA. UL aggr: %d DL aggr: %d",
                  rmnet_state_config->ipa_lcid,
                  ul_aggr_prot,
                  dl_aggr_prot);
  }
  else
  {
/*-----------------------------------------------------------------------------
  Call IOCTL to send aggregation parameters to bridge driver
------------------------------------------------------------------------------*/
    ret = set_aggregation_parameters(ul_aggr_prot, dl_aggr_prot, param.dl_max_datagrams, param.dl_max_size);
    if (ret == QTI_FAILURE)
    {
       LOG_MSG_ERROR("QTI%d:Setting of aggregation parameters failed",
                      rmnet_state_config->ipa_lcid, 0, 0);
       status.aggr_param_status = FALSE;
       LOG_MSG_ERROR("QTI%d:Setting of aggregation parameters to default",
                      rmnet_state_config->ipa_lcid, 0, 0);
       set_aggregation_parameters(TETH_AGGR_PROTOCOL_NONE,TETH_AGGR_PROTOCOL_NONE,0,0);
       if (ret == QTI_FAILURE)
       {
         LOG_MSG_ERROR("QTI%d:Setting of aggregation parameters to default failed",
                        rmnet_state_config->ipa_lcid, 0, 0);
       }
       else if (ret == QTI_SUCCESS)
       {
         LOG_MSG_ERROR("QTI%d:Setting of aggregation parameters to default succeeded",
                        rmnet_state_config->ipa_lcid, 0, 0);
       }
    }
    else if (ret == QTI_SUCCESS)
    {
       LOG_MSG_INFO1("QTI%d:Setting of aggregation parameters succeeded",
                      rmnet_state_config->ipa_lcid, 0, 0);
       status.aggr_param_status = TRUE;
    }
  }

 // print_buffer((char *)data, count);

  memcpy(((uint8 *)qmux_msg + QTI_QMUX_SDU_MESSAGE_HDR_LEN_BYTES),(uint8 *)data,count);

  free(data);

  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION PROCESS_TX_TO_CLIENT_SET_DATA_FORMAT_MESSAGE()

DESCRIPTION

  This function
  - peeks into set data format message response.
  - if the setting of link protocol succeeds at modem and IPA, it returns
    success with aggregation protocol negotiated with IPA.
  - sends back the response back to client

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

static int process_tx_to_client_set_data_format_message
(
  qti_qmux_msg_s *qmux_msg,
  uint32 qmux_msg_len
)
{
   int                      count;
   void                     * data = NULL;
   int                      bytes_processed = 0;
   uint8                    tlv_type;
   uint16                   tlv_length;
   boolean                  qos_enable;
   uint32                   link_protocol;
   uint32                   ul_aggr_prot;
   uint32                   ul_aggr_prot_tx;
   uint32                   dl_aggr_prot;
   uint32                   dl_aggr_prot_tx;
   uint32                   ndp_signature;
   uint32                   dl_max_datagrams;
   uint32                   dl_max_size;
   uint8                    response_type;
   uint16                   response_length;
   uint16                   response_res;
   uint16                   response_err;
   boolean                  set_data_format_result;
   struct teth_aggr_params  aggr_param;
   int                      ret;

/*--------------------------------------------------------------------------*/

  //print_buffer((char *)qmux_msg, qmux_msg_len);

  count = (uint16)qmux_msg->sdu.qmux.msg.msg_length;
  LOG_MSG_INFO1("QTI%d:RX : Count is %d", rmnet_state_config->ipa_lcid, count, 0);

  data = malloc(count);
  if (data == NULL)
  {
    LOG_MSG_ERROR("QTI%d:Could not allocate data for WDA set data format message in RX ",
                   rmnet_state_config->ipa_lcid, 0, 0);
    return QTI_FAILURE;
  }
  memcpy((uint8 *)data,((uint8 *)qmux_msg + QTI_QMUX_SDU_MESSAGE_HDR_LEN_BYTES),count);

  //print_buffer((char *)data, count);

  response_type = (uint8)get_value(data, bytes_processed,sizeof(uint8));
  bytes_processed += sizeof(uint8);
  LOG_MSG_INFO1("QTI%d:response_type %d", rmnet_state_config->ipa_lcid, response_type, 0);

  response_length = (uint16)get_value(data, bytes_processed,sizeof(uint16));
  bytes_processed += sizeof(uint16);
  LOG_MSG_INFO1("QTI%d:response_length %d ", rmnet_state_config->ipa_lcid, response_length, 0);

  response_res = (uint16)get_value(data, bytes_processed,sizeof(uint16));
/*--------------------------------------------------------------------------
  Set data format message failed
---------------------------------------------------------------------------*/
  if (response_res == QMI_RESULT_FAILURE_V01)
  {

     LOG_MSG_ERROR("QTI%d:WDA Set data format message failed", rmnet_state_config->ipa_lcid, 0, 0);
     set_data_format_result = FALSE;
  }
  else
  {
     LOG_MSG_INFO1("QTI%d:WDA Set data format message succeeded", rmnet_state_config->ipa_lcid, 0, 0);
     set_data_format_result = TRUE;
  }
  bytes_processed += sizeof(uint16);
  LOG_MSG_INFO1("QTI%d:response_res %d", rmnet_state_config->ipa_lcid, response_res, 0);

  response_err = (uint16)get_value(data, bytes_processed,sizeof(uint16));
  bytes_processed += sizeof(uint16);
  LOG_MSG_INFO1("QTI%d:response_err %d", rmnet_state_config->ipa_lcid, response_err, 0);


  while (bytes_processed < count)
  {
     tlv_type = (uint8)get_value(data, bytes_processed,sizeof(uint8));
     bytes_processed += sizeof(uint8);
     LOG_MSG_INFO1("QTI%d:Tlv type %d ", rmnet_state_config->ipa_lcid, tlv_type, 0);

     tlv_length = (uint16)get_value(data, bytes_processed,sizeof(uint16));
     bytes_processed += sizeof(uint16);
     LOG_MSG_INFO1("QTI%d:Tlv length %d", rmnet_state_config->ipa_lcid, tlv_length, 0);

     switch (tlv_type)
     {
       case QMI_TYPE_QOS_DATA_FORMAT:
         qos_enable = (uint8)get_value(data, bytes_processed, tlv_length);
         bytes_processed += tlv_length;
         LOG_MSG_INFO1("QTI%d:qos enable rx from modem %d",
                        rmnet_state_config->ipa_lcid, qos_enable, 0);
         break;

       case QMI_TYPE_LINK_PROTOCOL:
         link_protocol = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d: link_protocol rx from modem %d",
                        rmnet_state_config->ipa_lcid, link_protocol, 0);
/*---------------------------------------------------------------------------
  Only if setting of link protocol at modem and IPA succeeds return success
  with aggregation parameters
----------------------------------------------------------------------------*/
         if ((link_protocol == param.link_protocol) && set_data_format_result && status.link_prot_ipa_status)
         {
            LOG_MSG_INFO1("QTI%d:Setting of link protocol at modem and IPA succeeded",
                           rmnet_state_config->ipa_lcid, 0, 0);
            ret = get_aggregation_parameters(&aggr_param);
            if (ret == QTI_FAILURE)
            {
               LOG_MSG_ERROR("QTI%d:Get of aggregation parameters failed",
                              rmnet_state_config->ipa_lcid, 0, 0);
            }
            else if (ret == QTI_SUCCESS)
            {
               LOG_MSG_INFO1("QTI%d:Get of aggregation parameters succeeded",
                              rmnet_state_config->ipa_lcid, 0, 0);
               param.dl_aggr_prot = aggr_param.dl.aggr_prot;
               param.ul_aggr_prot = aggr_param.ul.aggr_prot;
               param.dl_max_datagrams = aggr_param.dl.max_datagrams;
               param.dl_max_size = aggr_param.dl.max_transfer_size_byte;
            }

         }
/*---------------------------------------------------------------------------
  Else return failure of WDA set data format message and set IPA to default
  mode
----------------------------------------------------------------------------*/
         else
         {
            LOG_MSG_INFO1("QTI%d:Setting of link protocol at modem and IPA failed",
                           rmnet_state_config->ipa_lcid, 0, 0);
            LOG_MSG_INFO1("QTI%d:Will set default mode in IPA and return result as error",
                           rmnet_state_config->ipa_lcid, 0, 0);
            set_value(data,
                      WDA_SET_DATA_FORMAT_RESULT_OFFSET,
                      sizeof(uint16),
                      QMI_RESULT_FAILURE_V01);
            set_value(data,
                      WDA_SET_DATA_FORMAT_RESULT_OFFSET+sizeof(uint16),
                      sizeof(uint16),
                      QMI_ERR_UNKNOWN_V01);
            set_value(data, bytes_processed, tlv_length, WDA_LINK_LAYER_ETHERNET_MODE_V01 );

            set_bridge_mode(TETH_LINK_PROTOCOL_ETHERNET);
            if (ret == QTI_FAILURE)
            {
               LOG_MSG_ERROR("QTI%d:Setting of link protocol to default failed",
                              rmnet_state_config->ipa_lcid, 0, 0);
            }
            else if (ret == QTI_SUCCESS)
            {
               LOG_MSG_INFO1("QTI%d:Setting of link protocol to default succeeded",
                              rmnet_state_config->ipa_lcid, 0, 0);
            }

            set_aggregation_parameters(TETH_AGGR_PROTOCOL_NONE,TETH_AGGR_PROTOCOL_NONE,0,0);
            if (ret == QTI_FAILURE)
            {
               LOG_MSG_ERROR("QTI%d:Setting of aggregation protocol to default failed",
                              rmnet_state_config->ipa_lcid, 0, 0);
            }
            else if (ret == QTI_SUCCESS)
            {
               LOG_MSG_INFO1("QTI%d:Setting of aggregation protocol to default succeeded",
                              rmnet_state_config->ipa_lcid, 0, 0);
            }
         }
         bytes_processed += tlv_length;
         LOG_MSG_INFO1("QTI%d:link_protocol tx to client %d",
                        rmnet_state_config->ipa_lcid, link_protocol, 0);
         break;

       case QMI_TYPE_UL_DATA_AGG_PROTOCOL:
         ul_aggr_prot = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d: ul_aggr_prot rx from modem %d",
                        rmnet_state_config->ipa_lcid, ul_aggr_prot, 0);
         set_value(data, bytes_processed, tlv_length,param.ul_aggr_prot );
         LOG_MSG_INFO1("QTI%d: ul_aggr_prot tx to client %d",
                        rmnet_state_config->ipa_lcid, param.ul_aggr_prot, 0);
         bytes_processed += tlv_length;
         break;

       case QMI_TYPE_DL_DATA_AGG_PROTOCOL:
         dl_aggr_prot = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d:dl_aggr_prot rx from modem %d",
                        rmnet_state_config->ipa_lcid, dl_aggr_prot, 0);
         set_value(data, bytes_processed, tlv_length,param.dl_aggr_prot );
         LOG_MSG_INFO1("QTI%d:dl_aggr_prot tx to client %d",
                        rmnet_state_config->ipa_lcid, param.dl_aggr_prot, 0);
         bytes_processed += tlv_length;
         break;

       case QMI_TYPE_NDP_SIGNATURE:
         ndp_signature = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d:ndp_signature rx from modem %d",
                        rmnet_state_config->ipa_lcid, ndp_signature, 0);
         set_value(data, bytes_processed, tlv_length,param.ndp_signature );
         LOG_MSG_INFO1("QTI%d:ndp_signature tx to client %d",
                        rmnet_state_config->ipa_lcid, param.ndp_signature, 0);
         bytes_processed += tlv_length;
         break;

       case QMI_TYPE_DL_DATA_AGG_MAX_DATAGRAMS:
         dl_max_datagrams = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d:dl_max_datagrams rx from modem %d",
                        rmnet_state_config->ipa_lcid, dl_max_datagrams, 0);
         set_value(data, bytes_processed, tlv_length,param.dl_max_datagrams );
         LOG_MSG_INFO1("QTI%d:dl_max_datagrams tx to client %d",
                        rmnet_state_config->ipa_lcid, param.dl_max_datagrams, 0);
         bytes_processed += tlv_length;
         break;

       case QMI_TYPE_DL_DATA_AGG_MAX_SIZE:
         dl_max_size = (uint32)get_value(data, bytes_processed, tlv_length);
         LOG_MSG_INFO1("QTI%d:dl_max_size rx from modem %d ",
                        rmnet_state_config->ipa_lcid, dl_max_size, 0);
         set_value(data, bytes_processed, tlv_length,param.dl_max_size);
         LOG_MSG_INFO1("QTI%d:dl_max_size tx to client %d",
                        rmnet_state_config->ipa_lcid, param.dl_max_size, 0);
         bytes_processed += tlv_length;
         break;

       default:
         break;
     }
   }

  //print_buffer((char *)data, count);

  memcpy(((uint8 *)qmux_msg + QTI_QMUX_SDU_MESSAGE_HDR_LEN_BYTES),(uint8 *)data,count);
  free(data);
  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_QMUX_TX_TO_MODEM()

DESCRIPTION

  This function
  - sends all QMI messages from USB interface to modem.
  - processes WDA set data format message

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int qti_qmux_tx_to_modem
(
  qti_qmux_msg_s *qmux_msg,
  uint32          qmux_msg_len
)
{
  int                          rc;
  qti_qmux_buf_s               qmi_qmux_if_buf;
  qmi_connection_id_type       qmi_conn_id;
  boolean                      ret_val = QTI_FAILURE;
  int                          message_id, svc_type;
/*-------------------------------------------------------------------------*/

  ds_assert(qmux_msg != NULL);

  memset(&qmi_qmux_if_buf, 0, sizeof(qti_qmux_buf_s));

  if(!rmnet_state_config->dtr_enabled)
  {
    process_usb_reset();
  }

  if (qti_qmux_qmi_handle_valid == FALSE)
  {
    LOG_MSG_ERROR("QTI%d: Tried to send QMUX message to modem, but qmux handle is invalid",
                   rmnet_state_config->ipa_lcid, 0, 0);
  }
  else if (qmux_msg_len < QTI_QMUX_MIN_MSG_LEN_BYTES)
  {
    LOG_MSG_ERROR("QTI%d:Tried sending short QMUX message to the modem! Got %d bytes, min %d",
                  rmnet_state_config->ipa_lcid, qmux_msg_len, QTI_QMUX_MIN_MSG_LEN_BYTES);
  }
  else if (qti_qmux_to_qmi_conn_id(&qmi_conn_id)!= QTI_SUCCESS)
  {
    LOG_MSG_ERROR("QTI%d: Couldn't find connection ID", rmnet_state_config->ipa_lcid, 0, 0);
  }
  else if (qti_qmux_buf_alloc(&qmi_qmux_if_buf,
                              ((qmux_msg_len - QTI_QMUX_HDR_LEN_BYTES) +
                               QMI_QMUX_IF_MSG_HDR_SIZE)) != QTI_SUCCESS)
  {
    LOG_MSG_ERROR("QTI%d:Couldn't allocate buffer of size %d for qmi_qmux_if!",
                   rmnet_state_config->ipa_lcid, (qmux_msg_len + QMI_QMUX_IF_MSG_HDR_SIZE), 0);
  }
  else
  {
/*----------------------------------------------------------------------------
 Send QMI_CTL message
-----------------------------------------------------------------------------*/
    if (qmux_msg->qmux_hdr.svc_type == QTI_QMUX_SVC_TYPE_QMI_CTL)
    {
      LOG_MSG_INFO1("QTI %d:Will send a QMI CTL packet to modem",
                     rmnet_state_config->ipa_lcid, 0, 0);
      //print_buffer((char *)qmux_msg, qmux_msg_len);

/*----------------------------------------------------------------------------
  The qmi_qmux_if APIs expect to be passed a buffer that has
  QMI_QMUX_IF_MSG_HDR_SIZE bytes of free space available in front of the
  buffer containing the SDU.
-----------------------------------------------------------------------------*/
    memcpy(((uint8 *) qmi_qmux_if_buf.data + QMI_QMUX_IF_MSG_HDR_SIZE),
               &qmux_msg->sdu, (qmux_msg_len - QTI_QMUX_HDR_LEN_BYTES));
      rc = qmi_qmux_if_send_raw_qmi_cntl_msg(
        qti_qmux_qmi_handle, qmi_conn_id,
        ((unsigned char *) qmi_qmux_if_buf.data + QMI_QMUX_IF_MSG_HDR_SIZE),
        (qmux_msg_len - QTI_QMUX_HDR_LEN_BYTES));
    }
/*----------------------------------------------------------------------------
 Send QMI_SDU message
-----------------------------------------------------------------------------*/
    else
    {
       svc_type = (uint8)qmux_msg->qmux_hdr.svc_type;
       if (svc_type == WDA_SERVICE_ID)
       {
          LOG_MSG_INFO1("QTI%d:***** Got WDA message ****", rmnet_state_config->ipa_lcid, 0, 0);
          message_id = (uint16)qmux_msg->sdu.qmux.msg.msg_id;
          LOG_MSG_INFO1("QTI%d:Message ID is %d ", rmnet_state_config->ipa_lcid, message_id, 0);
          if (message_id == WDA_SET_DATA_FORMAT_MESSAGE_ID)
          {
             LOG_MSG_INFO1("QTI%d:**** Got WDA SET DATA FORMAT message ****",
                            rmnet_state_config->ipa_lcid, 0, 0);
             //print_buffer((char *)qmux_msg, qmux_msg_len);
/*----------------------------------------------------------------------------
 Process SET DATA FORMAT message
-----------------------------------------------------------------------------*/
             process_tx_to_modem_set_data_format_message(qmux_msg, qmux_msg_len);
             //print_buffer((char *)qmux_msg, qmux_msg_len);
          }
       }
       //print_buffer((char *)qmux_msg, qmux_msg_len);
       LOG_MSG_INFO1("QTI%d:Will send a QMI SDU packet to modem",
                      rmnet_state_config->ipa_lcid, 0, 0);

/*----------------------------------------------------------------------------
  The qmi_qmux_if APIs expect to be passed a buffer that has
  QMI_QMUX_IF_MSG_HDR_SIZE bytes of free space available in front of the
  buffer containing the SDU.
-----------------------------------------------------------------------------*/
       memcpy(((uint8 *) qmi_qmux_if_buf.data + QMI_QMUX_IF_MSG_HDR_SIZE),
               &qmux_msg->sdu, (qmux_msg_len - QTI_QMUX_HDR_LEN_BYTES));
       rc = qmi_qmux_if_send_qmi_msg(
                      qti_qmux_qmi_handle, qmi_conn_id,
                      (qmi_service_id_type) qmux_msg->qmux_hdr.svc_type,
                      (qmi_client_id_type) qmux_msg->qmux_hdr.client_id,
                      ((unsigned char *) qmi_qmux_if_buf.data + QMI_QMUX_IF_MSG_HDR_SIZE),
                      (qmux_msg_len - QTI_QMUX_HDR_LEN_BYTES));
    }

    if (rc != QMI_NO_ERR)
    {
      LOG_MSG_ERROR("QTI%d:Error %d when sending QMI message for svc %d",
                     rmnet_state_config->ipa_lcid, rc, qmux_msg->qmux_hdr.svc_type);
    }
    else
    {
      ret_val = QTI_SUCCESS;
    }
    free(qmi_qmux_if_buf.data);
    qmi_qmux_if_buf.data = NULL;
    qmi_qmux_if_buf.size = 0;
  }

  rmnet_state_config->qti_qmi_mess_state = QTI_RX_MODEM_WAIT;
  return ret_val;
}

/*===========================================================================

FUNCTION QTI_QMUX_RX_CB()

DESCRIPTION

  This function
  - receives all messages from modem and passes onto USB interface.
  - processes set data format response message.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/


void qti_qmux_rx_cb
(
  qmi_connection_id_type conn_id,
  qmi_service_id_type    service_id,
  qmi_client_id_type     client_id,
  unsigned char          control_flags,
  unsigned char         *rx_msg,
  int                    rx_msg_len
)
{

  int                    message_id;
  int                    svc_type ;
  qti_qmux_buf_s         buf;
  qti_qmux_msg_s         *qmux_msg;

  LOG_MSG_INFO1("QTI%d:Received QMUX data buffer with size %d on connection ID %d",
                 rmnet_state_config->ipa_lcid, rx_msg_len, conn_id);

  if (service_id == QMI_CTL_SERVICE &&
      rx_msg_len < sizeof(qti_qmux_qmi_ctl_sdu_s))
  {
    LOG_MSG_ERROR("QTI%d:Received short QMI payload: %d bytes (minimum %d)",
                   rmnet_state_config->ipa_lcid, rx_msg_len, sizeof(qti_qmux_qmi_ctl_sdu_s));
    return;
  }
  else if (service_id != QMI_CTL_SERVICE &&
           rx_msg_len < sizeof(qti_qmux_sdu_s))
  {
    LOG_MSG_ERROR("QTI%d:Received short regular QMUX payload: %d bytes (minimum %d)",
                   rmnet_state_config->ipa_lcid, rx_msg_len, sizeof(qti_qmux_sdu_s));
    return;
  }
  else if ( conn_id != rmnet_state_config->qmux_conn_id)
  {
    LOG_MSG_ERROR("QTI%d:Received message on invalid connection id %d",
                   rmnet_state_config->ipa_lcid, conn_id, 0);
    return;
  }
  else
  {
    memset(&buf, 0 , sizeof(qti_qmux_buf_s));
    if (qti_qmux_buf_alloc(&buf, (rx_msg_len + QTI_QMUX_HDR_LEN_BYTES)) != QTI_SUCCESS)
    {
      LOG_MSG_ERROR("QTI%d:Couldn't allocate buffer of size %d bytes for QMUX response!",
                     rmnet_state_config->ipa_lcid, (rx_msg_len + QTI_QMUX_HDR_LEN_BYTES), 0);
    }
    else
    {

/*----------------------------------------------------------------------------
  Add the QMUX header
---------------------------------------------------------------------------- */
      qmux_msg = (qti_qmux_msg_s *) buf.data;
      qmux_msg->if_type = QTI_QMUX_IF_TYPE_QMUX;
      qmux_msg->qmux_hdr.length    = (rx_msg_len + sizeof(qti_qmux_hdr_s));
      qmux_msg->qmux_hdr.ctl_flags = (uint8) control_flags;
      qmux_msg->qmux_hdr.svc_type  = (uint8) service_id;
      qmux_msg->qmux_hdr.client_id = (uint8) client_id;
      memcpy(&(qmux_msg->sdu), rx_msg, rx_msg_len);

      svc_type = (uint8)qmux_msg->qmux_hdr.svc_type;
      if (svc_type == WDA_SERVICE_ID)
      {
          LOG_MSG_INFO1("QTI%d:***** Got WDA message in RX ****", rmnet_state_config->ipa_lcid, 0, 0);
          message_id = (uint16)qmux_msg->sdu.qmux.msg.msg_id;
          LOG_MSG_INFO1("QTI%d: Message ID is %d in RX",
                         rmnet_state_config->ipa_lcid, message_id, 0);
          if (message_id == WDA_SET_DATA_FORMAT_MESSAGE_ID)
          {
              LOG_MSG_INFO1("QTI%d:**** Got WDA SET DATA FORMAT message in RX ****",
                             rmnet_state_config->ipa_lcid, 0, 0);
	      //print_buffer((char *)qmux_msg, rx_msg_len + sizeof(qti_qmux_hdr_s) +1);
/*----------------------------------------------------------------------------
  Process WDA SET DATA FORMAT response message
---------------------------------------------------------------------------- */
              process_tx_to_client_set_data_format_message(qmux_msg,rx_msg_len + sizeof(qti_qmux_hdr_s) +1);
              //print_buffer((char *)qmux_msg, rx_msg_len + sizeof(qti_qmux_hdr_s) +1);
          }
      }


      //print_buffer((char *)qmux_msg, rx_msg_len + sizeof(qti_qmux_hdr_s) +1);
/*----------------------------------------------------------------------------
  Write QMI message into USB interface
---------------------------------------------------------------------------- */
      qti_rmnet_usb_send_msg(buf.data,(rx_msg_len + QTI_QMUX_HDR_LEN_BYTES));

      free(buf.data);
      buf.data = NULL;
      buf.size = 0;
    }

    return;
  }
}
