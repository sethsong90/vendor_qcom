#ifndef QMI_QMUX_IO_PLATFORM_H
#define QMI_QMUX_IO_PLATFORM_H

/******************************************************************************
  @file    qmi_platform_qmux_io.h
  @brief   The QMI QMUX generic platform layer hearder file

  DESCRIPTION
  Interface definition for QMI QMUX platform layer.  This file will pull in
  the appropriate platform header file(s).

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

#include "qmi_i.h"
#include "qmi_qmux.h" 

/*===========================================================================
  FUNCTION  qmi_amss_qmux_io_send_qmi_msg
===========================================================================*/
/*!
@brief 
  Function to send a QMUX PDU on the Linux platform
  
@return 
  0 if function is successful, negative value if not.

@note

  - Connection must have been previously opened.

  - Side Effects
    - Sends a QMUX PDU to modem processor
    
*/    
/*=========================================================================*/
extern int
qmi_amss_qmux_io_send_qmi_msg
(
  qmi_connection_id_type  conn_id,
  unsigned char           *msg_ptr,
  int                     msg_len
);


/*===========================================================================
  FUNCTION  qmi_amss_qmux_io_open_conn
===========================================================================*/
/*!
@brief 
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications 
  
@return 
  0 if function is successful, negative value if not.

@note
  - Side Effects
    - Opens up SMD port and spawns a thread for RX handling.
    
*/    
/*=========================================================================*/
extern int
qmi_amss_qmux_io_open_conn 
(
  qmi_connection_id_type  conn_id,
  unsigned char           *rx_buf
);

/*===========================================================================
  FUNCTION  qmi_amss_qmux_io_pwr_up_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the qmi_amss_qmux_io_open_conn() 
  
@return 
  0 if function is successful, negative value if not.

@note

  - Connection is assumed to be opened with valid data before this 
  function starts to execute

  - Side Effects
    - 
    
*/    
/*=========================================================================*/
extern int
qmi_amss_qmux_io_pwr_up_init
(
  qmi_qmux_io_platform_rx_cb_ptr  rx_func
);



/* These macros are used in QMUX */ 
#define QMI_QMUX_IO_PLATFORM_SEND_QMI_MSG(conn_id,msg_buf,len) \
  qmi_amss_qmux_io_send_qmi_msg (conn_id,msg_buf,len)

#define QMI_QMUX_IO_PLATFORM_OPEN_CONN(conn_id, rx_buf) \
  qmi_amss_qmux_io_open_conn (conn_id,rx_buf)

#define QMI_QMUX_IO_PLATFORM_PWR_UP_INIT(qmi_qmux_rx_msg_cb) \
  qmi_amss_qmux_io_pwr_up_init (qmi_qmux_rx_msg_cb)




#endif /* QMI_QMUX_IO_PLATFORM_H */
