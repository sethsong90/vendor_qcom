#ifndef QMI_QMUX_IF_PLATFORM_H
#define QMI_QMUX_IF_PLATFORM_H
/******************************************************************************
  @file    qmi_qmux.c
  @brief   The QMI QMUX layer

  DESCRIPTION
  Interface definition for QMI QMUX layer

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

#include "qmi_i.h"

typedef struct
{
  int total_msg_size;
  int qmux_client_id;
} qmi_amss_qmux_if_platform_hdr_type;


#define QMI_QMUX_IF_PLATFORM_SPECIFIC_HDR_SIZE  sizeof(qmi_amss_qmux_if_platform_hdr_type)

#define QMI_QMUX_IF_CONN_SOCKET_PATH   ""
#define QMI_QMUX_IF_CLIENT_SOCKET_PATH ""

#define QMI_QMUX_IF_PLATFORM_CLIENT_INIT(client,rx_buf,buf_siz) \
     qmi_amss_qmux_if_client_init (client,rx_buf,buf_siz)

#define QMI_QMUX_IF_PLATFORM_CLIENT_RELEASE(client) \
     qmi_amss_qmux_if_client_release (client)

#define QMI_QMUX_IF_PLATFORM_TX_MSG(client,msg,msg_size) \
     qmi_amss_qmux_if_client_tx_msg (client,msg,msg_len)

#define QMI_QMUX_IF_PLATFORM_RX_MSG(client,msg,msg_size) \
     qmi_amss_qmux_if_server_tx_msg (client,msg,msg_len)



/*===========================================================================
  FUNCTION  qmi_amss_qmux_if_client_init
===========================================================================*/
/*!
@brief 

@return 
  int

@note
    
*/    
/*=========================================================================*/
extern int 
qmi_amss_qmux_if_client_init
(
  int *qmux_client_id,
  unsigned char *rx_buf,
  int           rx_buf_size
);


/*===========================================================================
  FUNCTION  qmi_amss_qmux_if_client_release
===========================================================================*/
/*!
@brief 

@return 
  int

@note
    
*/    
/*=========================================================================*/
extern int
qmi_amss_qmux_if_client_release
(
  int qmux_client_id
);


/*===========================================================================
  FUNCTION  qmi_amss_qmux_if_client_tx_msg
===========================================================================*/
/*!
@brief 

@return 
  int

@note
    
*/    
/*=========================================================================*/
extern int 
qmi_amss_qmux_if_client_tx_msg
(
  int qmux_client_id,
  unsigned char *msg,
  int  msg_len
);


/*===========================================================================
  FUNCTION  qmi_amss_qmux_if_server_tx_msg
===========================================================================*/
/*!
@brief 

@return 
  int

@note
    
*/    
/*=========================================================================*/
extern int 
qmi_amss_qmux_if_server_tx_msg
(
  int qmux_client_id,
  unsigned char *msg,
  int  msg_len
);
 

#endif                                                                                                            
