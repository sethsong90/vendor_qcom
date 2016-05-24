#ifndef QMI_PLATFORM_H
#define QMI_PLATFORM_H

/******************************************************************************
  @file    qmi_platform.h
  @brief   The QMI QMUX generic platform layer hearder file

  DESCRIPTION
  Interface definition for QMI QMUX platform layer.  This file will pull in
  the appropriate platform header file.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

#include <memory.h>
#include <stdlib.h>
#include "rex.h"
#include "task.h"
#include "data_msg.h"
#include "dcc_task_defs.h"
#include "AEEstd.h"
#if defined(TEST_FRAMEWORK) && defined(FEATURE_QUBE)
#include "qube.h"
#endif
#include "ps_crit_sect.h"
#include "DS_Utils_DebugMsg.h"

/* Turn on/off MULTI-PD feature here */
#undef QMI_MSGLIB_MULTI_PD 

/* Debug and error messages */
#define QMI_MSG0(xx_ss_mask, xx_fmt)                                     \
    PRINT_MSG( xx_ss_mask, xx_fmt, 0, 0, 0 );
#define QMI_MSG1(xx_ss_mask, xx_fmt, xx_arg1)                            \
    PRINT_MSG( xx_ss_mask, xx_fmt, xx_arg1, 0, 0 );
#define QMI_MSG2(xx_ss_mask, xx_fmt, xx_arg1, xx_arg2)                   \
    PRINT_MSG( xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, 0 );
#define QMI_MSG3(xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3)          \
    PRINT_MSG( xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3 );
#define QMI_MSG4(xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3, xx_arg4) \
    PRINT_MSG_6( xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3, xx_arg4, 0, 0);

#define QMI_ERR_MSG_0(str)                         QMI_MSG0(MSG_LVL_ERROR,str) 
#define QMI_ERR_MSG_1(str,arg1)                    QMI_MSG1(MSG_LVL_ERROR,str,arg1)
#define QMI_ERR_MSG_2(str,arg1,arg2)               QMI_MSG2(MSG_LVL_ERROR,str,arg1,arg2)
#define QMI_ERR_MSG_3(str,arg1,arg2,arg3)          QMI_MSG3(MSG_LVL_ERROR,str,arg1,arg2,arg3)
#define QMI_ERR_MSG_4(str,arg1,arg2,arg3,arg4)     QMI_MSG4(MSG_LVL_ERROR,str,arg1,arg2,arg3,arg4)

#define QMI_DEBUG_LVL MSG_LVL_MED
#define QMI_DEBUG_MSG_0(str)                       QMI_MSG0(QMI_DEBUG_LVL,str)
#define QMI_DEBUG_MSG_1(str,arg1)                  QMI_MSG1(QMI_DEBUG_LVL,str,arg1)
#define QMI_DEBUG_MSG_2(str,arg1,arg2)             QMI_MSG2(QMI_DEBUG_LVL,str,arg1,arg2)
#define QMI_DEBUG_MSG_3(str,arg1,arg2,arg3)        QMI_MSG3(QMI_DEBUG_LVL,str,arg1,arg2,arg3)
#define QMI_DEBUG_MSG_4(str,arg1,arg2,arg3,arg4)   QMI_MSG4(QMI_DEBUG_LVL,str,arg1,arg2,arg3,arg4)

#define QMI_STRCPY  std_strlcpy

/* Suppress missing symbols in production builds */
#ifndef LIST_DEBUG
#define list_is_valid(ptr)          (1)
#define ordered_list_is_valid(ptr)  (1)
#endif


/* Data structure for waiting task */
typedef struct
{
  rex_tcb_type  * tcb_ptr;          /* Waiting task  */
  rex_sigs_type   sig;              /* Wait signal   */
} qmi_signal_info_type;

#define QMI_PLATFORM_SIGNAL_DATA_TYPE qmi_signal_info_type

#define QMI_PLATFORM_AMSS_WAIT_SIGNAL    (1<<DCC_QMUX_WAIT_SIGNAL)
#define QMI_PLATFORM_AMSS_TIMEOUT_SIGNAL (1<<DCC_QMUX_TIMEOUT_SIGNAL)
#define QMI_PLATFORM_AMSS_SIGNALS \
        ( QMI_PLATFORM_AMSS_WAIT_SIGNAL | QMI_PLATFORM_AMSS_TIMEOUT_SIGNAL )

/* Macro to initialize signal data */
#define QMI_PLATFORM_INIT_SIGNAL_DATA(signal_ptr)          \
  (signal_ptr)->sig     = QMI_PLATFORM_AMSS_WAIT_SIGNAL;   \
  (signal_ptr)->tcb_ptr = rex_self();                          

/* Macro to destroy signal data */
#define QMI_PLATFORM_DESTROY_SIGNAL_DATA(signal_ptr)       \
  (signal_ptr)->sig    = 0;                                \
  (signal_ptr)->tcb_ptr = NULL;                         


#define QMI_PLATFORM_INIT_SIGNAL_FOR_WAIT(conn_id,signal_ptr) \
  QMI_PLATFORM_INIT_SIGNAL_DATA(signal_ptr);               \
  (void)rex_clr_sigs( (signal_ptr)->tcb_ptr, (signal_ptr)->sig )

/* Macro to wait on specified task signal */
#define QMI_PLATFORM_WAIT_FOR_SIGNAL(conn_id, signal_ptr, timeout_secs) \
  qmi_amss_wait_for_sig_with_timeout( signal_ptr, timeout_secs )

/* Macro to post signal */
#define QMI_PLATFORM_SEND_SIGNAL(conn_id,signal_ptr)       \
  if( (signal_ptr)->tcb_ptr )                              \
  {                                                        \
    (void)rex_set_sigs( (signal_ptr)->tcb_ptr,             \
			(signal_ptr)->sig );               \
  }


/* Mutex related defines */
typedef ps_crit_sect_type qmi_cube_cs_type;
#define QMI_PLATFORM_MUTEX_DATA_TYPE qmi_cube_cs_type

#define QMI_PLATFORM_MUTEX_INIT(mutex_ptr) \
  PS_INIT_CRIT_SECTION( mutex_ptr );

#define QMI_PLATFORM_MUTEX_CREATE_AND_INIT_STATIC_MUTEX(mutex_ptr) \
  QMI_PLATFORM_MUTEX_DATA_TYPE (mutex_ptr);

#define QMI_PLATFORM_MUTEX_INIT_RECURSIVE(mutex_ptr) \
  QMI_PLATFORM_MUTEX_INIT( mutex_ptr );

#define QMI_PLATFORM_MUTEX_DESTROY(mutex_ptr) \
  /* Not applicable to REXL4 */

#define QMI_PLATFORM_MUTEX_LOCK(mutex_ptr) \
  PS_ENTER_CRIT_SECTION( mutex_ptr );

#define QMI_PLATFORM_MUTEX_UNLOCK(mutex_ptr) \
  PS_LEAVE_CRIT_SECTION( mutex_ptr );


#define QMI_PLATFORM_DEV_NAME_TO_CONN_ID(dev_id) \
   qmi_amss_get_conn_id_by_name (dev_id)



/*===========================================================================
  FUNCTION  qmi_amss_wait_for_sig
===========================================================================*/
/*!
@brief 
  Function to block calling task on the specified signal.  Any other signal
  set while waiting is left alone to be processed later.
  
@return 
  None.

@note

  - Side Effects
    - None.
    
*/    
/*=========================================================================*/
void
qmi_amss_wait_for_sig
(
  QMI_PLATFORM_SIGNAL_DATA_TYPE * signal_ptr
);

/*===========================================================================
  FUNCTION  qmi_amss_wait_for_sig_with_timeout
===========================================================================*/
/*!
@brief 

@return 
  int

@note
    
*/    
/*=========================================================================*/
extern int
qmi_amss_wait_for_sig_with_timeout
(
  QMI_PLATFORM_SIGNAL_DATA_TYPE * signal_ptr,
  int                             timeout_secs
);



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
extern qmi_connection_id_type 
qmi_amss_get_conn_id_by_name 
(
  const char *dev_id
);


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
extern void*
qmi_qmux_amss_process_rx_msgs( void );


#endif  /* QMI_PLATFORM_H */

