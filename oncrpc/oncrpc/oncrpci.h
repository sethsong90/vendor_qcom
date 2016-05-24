/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C I . H

GENERAL DESCRIPTION
  This is the internal header file and should only be included by ONCRPC files.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

 Copyright (c) 2005-2006, 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpci.h#5 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
07/10/07    ptm    Remove featurization.
08/23/06    ptm    Remove proxy log events and move DSM pool selection to
                   oncrpc_dsm.h.
06/27/06    ptm    Add new SMEM log event for std async call.
05/29/06    ptm    Add new SMEM log events.
05/07/06    ptm    Added ONCRPC_LOG_EVENT for modem standalone builds.
04/26/06    ddh    Added ONCRPC_LOG_EVENT 
03/23/06    ptm    Add support for 6 data value smem_log API.
03/20/06    ptm    Add proxy trace events.
04/22/05    ptm    Add symbol for logging BOTH WAIT and APIS INITED events.
04/19/05    ptm    Add symbol for logging RPC WAIT event.
03/16/05    ptm    Initial release.
===========================================================================*/
#ifndef ONCRPCI_H
#define ONCRPCI_H

#include <string.h>

#include "msg.h"
#include "assert.h"

/* Define the size of a DSM item for oncrpc*/
#define ONCRPC_DSM_ITEM_SIZ                   512
#include "dsm.h"
#include "queue.h"

#ifdef ONCRPC_SYSLOG_DEBUG
#include <utils/Log.h>
#include "common_log.h"

#define LOG_TAG "ONCRPC"
#define ONCRPC_SYSTEM_LOGI(...)	SLOGI(__VA_ARGS__)
#define ONCRPC_SYSTEM_LOGE(...)	LOGE(__VA_ARGS__)

#else

#define ONCRPC_SYSTEM_LOGI(...) do{} while(0)
#define ONCRPC_SYSTEM_LOGE(...) do{} while(0)

#endif /* ONCRPC_SYSLOG_DEBUG */


/* Specify how to output debuging */
#define RPC_MSG_HIGH( m, a1, a2, a3 ) MSG_HIGH( m, a1, a2, a3 )
#define RPC_MSG_MED( m, a1, a2, a3 )  MSG_MED( m, a1, a2, a3 )
#define RPC_MSG_LOW( m, a1, a2, a3 )  MSG_LOW( m, a1, a2, a3 )

#ifdef FEATURE_SMEM_LOG
/* Define the IDs to be used with smem_log (shared memory event logging for
   multiprocessor debugging) */

#include "smem_log.h"
#define ONCRPC_LOG_EVENT_SMD_WAIT           (SMEM_LOG_ONCRPC_EVENT_BASE +  0)
#define ONCRPC_LOG_EVENT_RPC_WAIT           (SMEM_LOG_ONCRPC_EVENT_BASE +  1)
#define ONCRPC_LOG_EVENT_RPC_BOTH_WAIT      (SMEM_LOG_ONCRPC_EVENT_BASE +  2)
#define ONCRPC_LOG_EVENT_RPC_INIT           (SMEM_LOG_ONCRPC_EVENT_BASE +  3)
#define ONCRPC_LOG_EVENT_RUNNING            (SMEM_LOG_ONCRPC_EVENT_BASE +  4)
#define ONCRPC_LOG_EVENT_APIS_INITED        (SMEM_LOG_ONCRPC_EVENT_BASE +  5)
#define ONCRPC_LOG_EVENT_AMSS_RESET         (SMEM_LOG_ONCRPC_EVENT_BASE +  6)
#define ONCRPC_LOG_EVENT_SMD_RESET          (SMEM_LOG_ONCRPC_EVENT_BASE +  7)
#define ONCRPC_LOG_EVENT_ONCRPC_RESET       (SMEM_LOG_ONCRPC_EVENT_BASE +  8)
#define ONCRPC_LOG_EVENT_CB                 (SMEM_LOG_ONCRPC_EVENT_BASE +  9)
#define ONCRPC_LOG_EVENT_STD_CALL           (SMEM_LOG_ONCRPC_EVENT_BASE + 10)
#define ONCRPC_LOG_EVENT_STD_REPLY          (SMEM_LOG_ONCRPC_EVENT_BASE + 11)
#define ONCRPC_LOG_EVENT_STD_CALL_ASYNC     (SMEM_LOG_ONCRPC_EVENT_BASE + 12)

#define ONCRPC_SMEM_DEBUG

#ifdef ONCRPC_SMEM_DEBUG
#define ONCRPC_LOG_EVENT(event, data1, data2, data3)    \
        SMEM_LOG_EVENT( event, data1, data2, data3 )
#define ONCRPC_LOG_EVENT6(identifier, data1, data2, data3, \
                          data4, data5, data6) \
        SMEM_LOG_EVENT6(identifier, data1, data2, data3, \
                        data4, data5, data6) \

#else
#define ONCRPC_LOG_EVENT(event, data1, data2, data3)    \
        do{} while(0)
#define ONCRPC_LOG_EVENT6(identifier, data1, data2, data3, \
                          data4, data5, data6) \
        do{} while(0)
#endif /* ONCRPC_SMEM_DEBUG */

#else

#define SMEM_LOG_EVENT( ID, D1, D2, D3 ) \
        RPC_MSG_LOW( #ID " %x %x %x", D1, D2, D3 )

#define SMEM_LOG_EVENT6( ID, D1, D2, D3, D4, D5, D6 ) \
        RPC_MSG_LOW( #ID " %x %x %x", D1, D2, D3 ); \
        RPC_MSG_LOW( #ID " %x %x %x", D4, D5, D6 )

#define ONCRPC_LOG_EVENT( ID, D1, D2, D3 ) \
        RPC_MSG_LOW( #ID " %x %x %x", D1, D2, D3 )

#define ONCRPC_LOG_EVENT6( ID, D1, D2, D3, D4, D5, D6 ) \
        RPC_MSG_LOW( #ID " %x %x %x", D1, D2, D3 ); \
        RPC_MSG_LOW( #ID " %x %x %x", D4, D5, D6 )

#endif /* FEATURE_SMEM_LOG */
#endif /* ONCRPCI_H */
