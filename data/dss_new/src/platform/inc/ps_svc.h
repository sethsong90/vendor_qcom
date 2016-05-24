#ifndef PS_SVC_H
#define PS_SVC_H
/*===========================================================================

                        P S _ S V C . H

DESCRIPTION
  This is the header file for the Data protocol Task. Contained herein are
  all the definitions, functions, and structures for other software tasks
  to communicate with the Data Protocol Task.

EXTERNALIZED FUNCTIONS

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

$Header:$

when        who    what, where, why
--------    ---    ---------------------------------------------------------- 

===========================================================================*/
/*--------------------------------------------------------------------------
NOTE: No Platform indepdent header inclusion...eg: rex.h
--------------------------------------------------------------------------*/
#include "comdef.h"
#include "dcc_task_defs.h"
#include "ds_util.h"
#include "ds_Utils_DebugMsg.h"

#undef  MSG_ERROR
#define MSG_ERROR( fmtString, x, y, z)     LOG_MSG_ERROR( fmtString, x, y, z)
#undef  MSG_HIGH
#define MSG_HIGH( fmtString, x, y, z)      LOG_MSG_INFO1( fmtString, x, y, z)
#undef  MSG_MED
#define MSG_MED( fmtString, x, y, z)       LOG_MSG_INFO2( fmtString, x, y, z)
#undef  MSG_LOW
#define MSG_LOW( fmtString, x, y, z)       LOG_MSG_INFO3( fmtString, x, y, z)

/* Map absent PS task symbols to DCC thread */
#define  PS_DSNET_PROCESS_GENERIC_EVENT_CMD   DCC_DSNET_PROCESS_GENERIC_EVENT_CMD
#define  PS_DSSOCK_PROCESS_GENERIC_EVENT_CMD  DCC_DSSOCK_PROCESS_GENERIC_EVENT_CMD
#define  PS_DSSOCK_PROCESS_DOS_ACK_EVENT_CMD  DCC_DSSOCK_PROCESS_DOS_ACK_EVENT_CMD
#define  PS_DSS_NET_MGR_NET_UP_CMD            DCC_DSS_NET_MGR_NET_UP_CMD
#define  PS_DSS_NET_MGR_NET_DOWN_CMD          DCC_DSS_NET_MGR_NET_DOWN_CMD
#define  PS_DNS_RESOLVE_CMD                   DCC_DNS_RESOLVE_CMD
#define  PS_DNS_IO_MGR_SOCK_EVENT_CMD         DCC_DNS_IO_MGR_SOCK_EVENT_CMD
#define  PS_DNS_RESOLVER_TIMEOUT_CMD          DCC_DNS_RESOLVER_TIMEOUT_CMD
#define  PS_DNS_DELETE_SESSION_CMD            DCC_DNS_DELETE_SESSION_CMD
#define  PS_TIMER_CALLBACK_CMD                DCC_TIMER_CALLBACK_CMD
#define  PS_STAT_INST_GET_DESC_CMD            DCC_STAT_INST_GET_DESC_CMD                
#define  PS_STAT_INST_GET_STAT_CMD            DCC_STAT_INST_GET_STAT_CMD                
#define  PS_STAT_INST_RESET_STAT_CMD          DCC_STAT_INST_RESET_STAT_CMD


#define ps_cmd_enum_type      dcc_cmd_enum_type
#define ps_cmd_handler_type   dcc_cmd_handler_type

#define ps_set_cmd_handler    dcc_set_cmd_handler



/*===========================================================================

FUNCTION PS_SEND_CMD()

DESCRIPTION
  This function posts a cmd for processing in PS task context.  The cmd is
  processed by calling the registered cmd handler, if any.

  NOTE: The passed command will be copied to a PS task
  command buffer local to the PS Task.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
INLINE void ps_send_cmd
(
  ps_cmd_enum_type cmd,          /* Actual command to be processed         */
  void *user_data_ptr            /* Command specific user parameters       */
)
{
  dcc_send_cmd( DCC_DSNET_PROCESS_GENERIC_EVENT_CMD, user_data_ptr );
}

#endif /* PS_SVC_H */
