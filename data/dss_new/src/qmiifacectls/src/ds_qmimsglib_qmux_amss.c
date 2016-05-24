/******************************************************************************
  @file    qmi_qmux_amss.c
  @brief   The QMI QMUX AMSS platform layer

  DESCRIPTION
  AMSS-based QMUX layer for the QMI user space driver

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2008 - 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


/******************************************************************************
******************************************************************************/
#include "comdef.h"
#include "customer.h"

#if defined( FEATURE_QMI_CLIENT ) && \
    defined( FEATURE_DATA_PS_CMI_REARCH ) && defined( IMAGE_APPS_PROC )

#define FEATURE_DSM_WM_CB

#include <stdio.h>
#include "assert.h"
#include "task.h"
#include "sio.h"
#include "smd.h"
#include "dsm.h"
#include "qmi_i.h"
#include "qmi_qmux.h"
#include "qmi_platform.h"
#include "ds_qmimsglib_task_sig.h"
#include "ps_crit_sect.h"


/* Data structure for connection object */
typedef struct
{
  sio_port_id_type      sio_port_id;
  sio_stream_id_type    sio_handle;
  dsm_watermark_type    rx_wm;
  q_type                rx_wm_q;
  dsm_watermark_type    tx_wm;
  q_type                tx_wm_q;
  boolean               active;
} qmi_qmux_amss_conn_info_type;


/* Data kept for each connection ID */
#define QMUX_RX_WM_LOW    (0)
#define QMUX_RX_WM_HIGH   (8192)
#define QMUX_RX_WM_DNE    (9216)
#define QMUX_TX_WM_LOW    (0)
#define QMUX_TX_WM_HIGH   (8192)
#define QMUX_TX_WM_DNE    (9216)


/*****************************************************************************
** Data declarations 
*****************************************************************************/

/* Connection meta information */
static qmi_qmux_amss_conn_info_type qmi_qmux_amss_conn_info[] =     
  {
    { QMI_SIO_PORT_RMNET_1, 0, {0,}, {0,}, {0,}, {0,}, FALSE }
   ,{ QMI_SIO_PORT_RMNET_2, 0, {0,}, {0,}, {0,}, {0,}, FALSE }
   ,{ QMI_SIO_PORT_RMNET_3, 0, {0,}, {0,}, {0,}, {0,}, FALSE }
   ,{ QMI_SIO_PORT_RMNET_4, 0, {0,}, {0,}, {0,}, {0,}, FALSE }
   ,{ QMI_SIO_PORT_RMNET_5, 0, {0,}, {0,}, {0,}, {0,}, FALSE }
  };

/*lint -restore */

/* Connection signal servicing elements */
static qmi_connection_id_type qmi_qmux_amss_service_conn = QMI_CONN_ID_FIRST;
QMI_PLATFORM_MUTEX_DATA_TYPE qmi_qmux_amss_service_conn_mutex;

/* Global function pointer to be called when Rx data packet is received */
qmi_qmux_io_platform_rx_cb_ptr qmi_qmux_amss_rx_cb = NULL;

/* Synchronous call timeout declarations */
#define QMI_PLATFORM_SYNC_MSG_TIMEOUT (5)  /* timout in sec */
static rex_timer_type qmi_qmux_wait_timer;

#define QMI_PLATFORM_DEVID_SIZ  (7)

/* QMI dev_id to conn_id mapping table */
static struct qmi_devid_x_connid_s 
{
  char                      dev_id[QMI_PLATFORM_DEVID_SIZ];
  qmi_connection_id_type    conn_id;  
} qmi_devid_x_connid_tbl[] = 
{
  { QMI_PORT_RMNET_1, QMI_CONN_ID_RMNET_0 },
  { QMI_PORT_RMNET_2, QMI_CONN_ID_RMNET_1 },
  { QMI_PORT_RMNET_3, QMI_CONN_ID_RMNET_2 },
  { QMI_PORT_RMNET_4, QMI_CONN_ID_RMNET_3 },
  { QMI_PORT_RMNET_5, QMI_CONN_ID_RMNET_4 },
};

#define QMI_DEVID_X_CONNID_TBL_SIZ \
        (sizeof(qmi_devid_x_connid_tbl)/sizeof(qmi_devid_x_connid_tbl[0]))


/*===========================================================================
  FUNCTION  qmi_qmux_amss_rx_msg
===========================================================================*/
/*!
@brief 
  Routine for handling QMI messages received from SMD QMI control port.
  This function will signal host task that new message is available.

@return 
  None.

@note

  Connection is assumed to be opened and valid data 

  - Side Effects
    - 
    
*/    
/*=========================================================================*/
static void qmi_qmux_amss_rx_msg
(
  dsm_watermark_type *wm_ptr,
  void *in_param
)
{
  qmi_connection_id_type  conn_id;

  (void)wm_ptr; /* unused */
  
  /* Input parameter is conn_id */
  conn_id = (qmi_connection_id_type) (uint32)in_param;
 
  /* Verify that conn_id was passed in correctly */
  if( (int)conn_id >= QMI_MAX_CONNECTIONS )
  {
    QMI_ERR_MSG_1("QMUX bad conn_id = %d!",conn_id);
    ASSERT(0);
    return;
  }

  /* Track first connection ready for round-robin servicing */
  if( QMI_CONN_ID_FIRST == qmi_qmux_amss_service_conn )
  {
    QMI_PLATFORM_MUTEX_LOCK( &qmi_qmux_amss_service_conn_mutex );
    qmi_qmux_amss_service_conn = conn_id;
    QMI_PLATFORM_MUTEX_UNLOCK( &qmi_qmux_amss_service_conn_mutex );
  }
  
  /* Notify QMI task of QMUX message arrival */
  (void)rex_set_sigs( &qmi_tcb, QMI_QMUX_RX_SIG ); 
  return;
}


/*===========================================================================
  FUNCTION  qmi_amss_qmux_io_pwr_up_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the qmi_qmux_amss_open_conn() 
  
@return 
  0 if function is successful, negative value if not.

@note

  - Side Effects
    - None.
    
*/    
/*=========================================================================*/
int
qmi_amss_qmux_io_pwr_up_init
(
  qmi_qmux_io_platform_rx_cb_ptr  rx_func
)
{
  int conn;

  /* Preserve the RX callback */  
  if( NULL != qmi_qmux_amss_rx_cb )
  {
    return QMI_INTERNAL_ERR;
  }
  qmi_qmux_amss_rx_cb = rx_func;

  /* Setup the datapath watermark queues */  
  for( conn=0; conn<QMI_MAX_CONNECTIONS; conn++ )
  {
    /* TX watermark */
    dsm_queue_init( &qmi_qmux_amss_conn_info[conn].tx_wm,
                    QMUX_TX_WM_DNE,
                    &qmi_qmux_amss_conn_info[conn].tx_wm_q );

    qmi_qmux_amss_conn_info[conn].tx_wm.lo_watermark = QMUX_TX_WM_LOW;
    qmi_qmux_amss_conn_info[conn].tx_wm.hi_watermark = QMUX_TX_WM_HIGH;
    
      
    /* RX watermark */
    dsm_queue_init( &qmi_qmux_amss_conn_info[conn].rx_wm,
                    QMUX_RX_WM_DNE,
                    &qmi_qmux_amss_conn_info[conn].rx_wm_q );

    qmi_qmux_amss_conn_info[conn].rx_wm.lo_watermark = QMUX_RX_WM_LOW;
    qmi_qmux_amss_conn_info[conn].rx_wm.hi_watermark = QMUX_RX_WM_HIGH;

    /* RX watermark enqueue callback */
    qmi_qmux_amss_conn_info[conn].rx_wm.each_enqueue_func_ptr = qmi_qmux_amss_rx_msg;
    qmi_qmux_amss_conn_info[conn].rx_wm.each_enqueue_func_data = (void*)conn;
  }

  QMI_PLATFORM_MUTEX_INIT( &qmi_qmux_amss_service_conn_mutex );

  /* Setup timer for synchronous call timeout */
  rex_def_timer( &qmi_qmux_wait_timer, rex_self(), QMI_PLATFORM_AMSS_TIMEOUT_SIGNAL );
  
  return QMI_NO_ERR;
}
 

/*===========================================================================
  FUNCTION  qmi_qmux_wait_on_signal
===========================================================================*/
/*!
@brief 
  Function to block calling task on the specified signal.  Any other signal
  set while waiting is left alone to be processed later.
  
@return 
  QMI_TIMEOUT_ERR if signal wait timeout occured 
  QMI_NO_ERR otherwise

@note

  - Side Effects
    - None.
    
*/    
/*=========================================================================*/
int
qmi_qmux_wait_on_signal
(
  QMI_PLATFORM_SIGNAL_DATA_TYPE * signal_ptr
)
{
  rex_sigs_type sigs = 0;
  int rc = QMI_NO_ERR;
  
  if( signal_ptr )
  {
    signal_ptr->tcb_ptr = rex_self();

    /* Loop for any signals set, exit only on requested signal. */
    do
    {
      sigs = rex_wait( QMI_PLATFORM_AMSS_SIGNALS );
    } while( 0 == (sigs & QMI_PLATFORM_AMSS_SIGNALS) );

    /* Clear the wait signal. */
    (void)rex_clr_sigs( signal_ptr->tcb_ptr, QMI_PLATFORM_AMSS_SIGNALS );

    signal_ptr->tcb_ptr = NULL;

    /* Check for wait timeout signal fired */
    if( 0 != (sigs & QMI_PLATFORM_AMSS_TIMEOUT_SIGNAL))
    {
      rc = QMI_TIMEOUT_ERR;
    }
  }

  return rc;
}


/*===========================================================================
  FUNCTION  qmi_amss_wait_for_sig_with_timeout
===========================================================================*/
/*!
@brief 
  Function to block calling task on the specified signal, and abort wait
  if timeout elapses.  Timeout must be in sec units.
  
@return 
  QMI_TIMEOUT_ERR if signal wait timeout occured 
  QMI_NO_ERR otherwise
  
@note

  - Side Effects
    - O/S timer is started/stopped.
    
*/    
/*=========================================================================*/
int
qmi_amss_wait_for_sig_with_timeout
(
  QMI_PLATFORM_SIGNAL_DATA_TYPE * signal_ptr,
  int                             timeout
)
{
  int rc = QMI_NO_ERR;

  /* Start the synchronous call wait timer.  Timeout is in msec units */
  (void) rex_set_timer( &qmi_qmux_wait_timer, (timeout*1000) );

  /* Perform the synchronous call */
  rc = qmi_qmux_wait_on_signal( signal_ptr );

  /* Stop the synchronous call wait timer */
  (void) rex_clr_timer( &qmi_qmux_wait_timer );
  
  return rc;
}


/*===========================================================================
  FUNCTION  qmi_qmux_amss_port_dsr_change_cb
===========================================================================*/
/*!
@brief 
  Callback for explicit control port open on Modem via DSR signal.
  
@return 
  None

@note
  - Side Effects
    - Posts signal to host task
    
*/    
/*=========================================================================*/
void qmi_qmux_amss_port_dsr_change_cb
(
  void * cb_data  
)
{
  QMI_PLATFORM_SIGNAL_DATA_TYPE signal;
  
  /* Notify host task the port DSR signal has changed. */
  QMI_PLATFORM_INIT_SIGNAL_DATA( &signal );
  signal.tcb_ptr = (rex_tcb_type*)cb_data;
  QMI_PLATFORM_SEND_SIGNAL( 0, &signal );
}

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
int
qmi_amss_qmux_io_open_conn 
(
  qmi_connection_id_type  conn_id
)
{ 
  qmi_qmux_amss_conn_info_type *conn_info_ptr;
  sio_open_type          sio_params;
  sio_ioctl_param_type  ioctl_param;
  boolean               dsr_asserted;
 
  /* Verifiy the conn_id parameter since it will be used to index
  ** into an array
  */
  if( conn_id >= QMI_MAX_CONNECTIONS )
  {
    QMI_ERR_MSG_1 ("qmi_qmux_amss.c: bad conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Assure that power-up init function has been called */
  if( NULL == qmi_qmux_amss_rx_cb )
  {
    QMI_ERR_MSG_0 ("qmi_qmux_amss.c: power-up init not called\n");
    return QMI_INTERNAL_ERR;
  }
  
  /* Set up pointer to connection info */
  conn_info_ptr = &qmi_qmux_amss_conn_info[conn_id];

  if( conn_info_ptr->active )
  {
    QMI_ERR_MSG_1 ("qmi_qmux_amss.c: connection already opened = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Open the SMD port */
  memset (&sio_params, 0, sizeof(sio_open_type));
  sio_params.stream_mode    = SIO_GENERIC_MODE;
  sio_params.rx_queue       = &conn_info_ptr->rx_wm;
  sio_params.tx_queue       = &conn_info_ptr->tx_wm;
  sio_params.rx_bitrate     = SIO_BITRATE_BEST;  
  sio_params.tx_bitrate     = SIO_BITRATE_BEST;  
  sio_params.port_id        = conn_info_ptr->sio_port_id;
  sio_params.tail_char_used = FALSE;
  sio_params.tail_char      = 0;
  sio_params.rx_func_ptr    = NULL;
  sio_params.rx_flow        = SIO_FCTL_BEST;
  sio_params.tx_flow        = SIO_FCTL_BEST;

  conn_info_ptr->sio_handle = sio_control_open( &sio_params );
  if( SIO_NO_STREAM_ID == conn_info_ptr->sio_handle )
  {
    QMI_ERR_MSG_0( "QMUX SIO control open failed!" );
    return FALSE;
  }

  /* Ensure Modem has opened port, wait here until it does. Note DSR and DTE share same signal. */
  ioctl_param.dte_ready_asserted = &dsr_asserted;
  sio_control_ioctl( conn_info_ptr->sio_handle, SIO_IOCTL_DTE_READY_ASSERTED, &ioctl_param ); 
  if( !dsr_asserted )
  {
    QMI_PLATFORM_SIGNAL_DATA_TYPE wait_signal;
    
    /* DSR not asserted, register callback for transition event */
    ioctl_param.enable_dte_ready_event_ext.cb_func = qmi_qmux_amss_port_dsr_change_cb;
    ioctl_param.enable_dte_ready_event_ext.cb_data = (void*)rex_self();
    sio_control_ioctl( conn_info_ptr->sio_handle, SIO_IOCTL_ENABLE_DTR_EVENT_EXT, &ioctl_param );

    /* Wait for signal set by CD callback */
    QMI_PLATFORM_INIT_SIGNAL_DATA( &wait_signal );
    (void) QMI_PLATFORM_WAIT_FOR_SIGNAL( 0, &wait_signal, QMI_PLATFORM_SYNC_MSG_TIMEOUT );

    /* Clear the DSR callback */
    sio_control_ioctl( conn_info_ptr->sio_handle, SIO_IOCTL_DISABLE_DTR_EVENT_EXT, &ioctl_param );
  }
  
  /* We are done, mark connection as active and return success code */
  conn_info_ptr->active = TRUE;
  return QMI_NO_ERR;
}


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
int
qmi_amss_qmux_io_send_qmi_msg
(
  qmi_connection_id_type  conn_id,
  unsigned char          *msg_ptr,
  int                     msg_len
)
{
  qmi_qmux_amss_conn_info_type *conn_info_ptr;
  dsm_item_type          *item_ptr = NULL;
  uint32                  len;
  
  /* Verifiy the conn_id parameter since it will be used to index into
  ** an array
  */
  if( (int)conn_id >= QMI_MAX_CONNECTIONS )
  {
    QMI_ERR_MSG_1 ("QMUX bad conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Set up pointer to connection info */
  conn_info_ptr = &qmi_qmux_amss_conn_info[conn_id];

  if( !conn_info_ptr->active )
  {
    QMI_ERR_MSG_1 ("QMUX connection not active conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Try to allocate DSM packet */
  item_ptr = dsm_new_buffer( DSM_DS_SMALL_ITEM_POOL );
  if( item_ptr )
  {
    /* Write QMUX message into DSM packet */
    len = dsm_pushdown_tail( &item_ptr, (void*)msg_ptr, msg_len,
                             DSM_DS_SMALL_ITEM_POOL );
    if( len != msg_len )
    {
      QMI_ERR_MSG_2( "QMUX length mismatch writing packet: %d != %d",
                     len,msg_len );
      return QMI_INTERNAL_ERR;
    }
    
    /* Send to proper SMD port */
    QMI_DEBUG_MSG_1("QMUX enqueue TX packet: size=%d",len);
    sio_control_transmit( conn_info_ptr->sio_handle, item_ptr );
  }

  return QMI_NO_ERR;
}


/*===========================================================================
  FUNCTION  qmi_qmux_amss_rx_msg
===========================================================================*/
/*!
@brief 
  Routine for processing QMI messages received from SMD QMI control
  ports.  This function is activated when the host task RX signal has
  been set.  It loops over all connections, starting with one which
  set signal, and reads a DSM item containing a message packet.  The
  message is extracted and processed.  If RX queue is not empty, the
  host task signal is set again to cause another processing pass.

@return 
  NULL

@note

  Connection is assumed to be opened and valid data 

  - Side Effects
    - Host task signal may be set.
    
*/    
/*=========================================================================*/
void*
qmi_qmux_amss_process_rx_msgs( void )
{
  /* Pointer to QMUX connection info */
  qmi_qmux_amss_conn_info_type *conn_info_ptr;
  qmi_connection_id_type  conn_id, start_conn_id;
  static unsigned char    rx_buf[QMI_MAX_MSG_SIZE];
  unsigned char          *rx_buf_ptr;
  dsm_item_type          *item_ptr = NULL;
  uint16                  item_len, len;
  int32                   i;
  
  /* Ensure RX processing callback has been registered, ignore otherwise. */
  if( NULL == qmi_qmux_amss_rx_cb )
  {
    return NULL;
  }
  
  /* Offset the RX buffer by QMI_QMUX_IF_HDR_SIZE - QMI_QMUX_HDR_SIZE 
   * QMUX needs this buffer to be able to prepend qmi_qmux_if_msg_hdr_type */
  rx_buf_ptr = rx_buf + QMI_QMUX_IF_HDR_SIZE - QMI_QMUX_HDR_SIZE;
  
  /* Get and clear first connection to service flag */
  QMI_PLATFORM_MUTEX_LOCK( &qmi_qmux_amss_service_conn_mutex );
  start_conn_id = qmi_qmux_amss_service_conn; 
  qmi_qmux_amss_service_conn = QMI_CONN_ID_FIRST;
  QMI_PLATFORM_MUTEX_UNLOCK( &qmi_qmux_amss_service_conn_mutex );

  /* Loop over all connections, starting with first signalled */
  for( i = (int)start_conn_id;
       i < ((int)start_conn_id+QMI_MAX_CONNECTIONS);
       i++ )
  {
    /* Determine actual connection to service */
    conn_id = (qmi_connection_id_type)(i % (int)QMI_MAX_CONNECTIONS);
    conn_info_ptr = &qmi_qmux_amss_conn_info[ conn_id ];
    
    /* Dequeue the new packet and check integrity */
    item_ptr = dsm_dequeue( &conn_info_ptr->rx_wm );
    if( item_ptr )
    {
      if( QMI_MAX_MSG_SIZE <
          (item_len = (uint16)dsm_length_packet( item_ptr )) )
      {
        QMI_ERR_MSG_1("QMUX packet dropped, exceeds maximum size: %d", item_len );
        dsm_free_packet( &item_ptr );
      }

      /* Extract the QMUX message into buffer */
      len = dsm_pullup( &item_ptr, rx_buf_ptr, item_len );
      if( len != item_len )
      {
        QMI_ERR_MSG_0("QMUX packet dropped, error reading packet" );
        dsm_free_packet( &item_ptr );
      }
      QMI_DEBUG_MSG_1("QMUX dequeue RX packet: size=%d",len);

      /* Process QMUX message */
      qmi_qmux_amss_rx_cb( conn_id, rx_buf_ptr, len );

      /* Check for more messages in this watermark, and set signal if present.
       * This ensure host task makes another pass after this one completes. */
      if( !dsm_is_wm_empty( &conn_info_ptr->rx_wm ) )
      {
        qmi_qmux_amss_rx_msg( &conn_info_ptr->rx_wm, (void*)conn_id );
      }
    }
  }
  
  return NULL;

} /* qmi_qmux_amss_rx_msg */

/*===========================================================================
  FUNCTION  qmi_amss_get_conn_id_by_name
===========================================================================*/
/*!
@brief 

@return 
  qmi_connection_id_type

@note
    
*/    
/*=========================================================================*/
qmi_connection_id_type 
qmi_amss_get_conn_id_by_name 
(
  const char *dev_id
)
{
  uint8 i;
  
  /* Loop over mapping table */
  for( i=0; i<QMI_DEVID_X_CONNID_TBL_SIZ; i++ )
  {
    /* Compare dev_id values */
    if( 0 == strcmp( dev_id, qmi_devid_x_connid_tbl[i].dev_id ) )
    {
      /* Match found, return conn_id */
      return qmi_devid_x_connid_tbl[i].conn_id;
    }
  }

  return QMI_CONN_ID_INVALID;
}

#endif /* FEATURE_QMI_CLIENT */
