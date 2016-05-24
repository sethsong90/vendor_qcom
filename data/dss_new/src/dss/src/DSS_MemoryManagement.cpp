/*===========================================================================
//   FILE: DSS_MemoryManagement.cpp 
//
//   OVERVIEW: This file provides implementation of the Socket class.
//
//   DEPENDENCIES: None
//
//                 Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
//                 All Rights Reserved.
//                 Qualcomm Technologies Confidential and Proprietary
//===========================================================================*/

/*===========================================================================

            INCLUDE FILES FOR MODULE

===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MemoryManagement.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "ps_mem.h"
#include "dss_config.h"
#include "ds_Utils_DebugMsg.h"
#include "DSS_Globals.h"
#include "DSS_NetMCastMBMSCtrl.h"
#include "DSS_EventHandlerNetApp.h"
#include "DSS_EventHandlerQoS.h"
#include "DSS_MemoryManagement.h"
#include "DSS_SecondaryNetApp.h"
#include "DSS_EventHandler.h"

#include "DSS_MCast.h"

#include "ds_Net_MemConfig.h"

#include "DSS_PrivIpv6Addr.h"

#include "err.h"


#include "ps_iface_defs.h"
 
/**********************************************/
/**********************************************/
/* PS_MEM */
/**********************************************/
/**********************************************/

/*---------------------------------------------------------------------------
Macros for sizes of objects of these classes.
---------------------------------------------------------------------------*/
#define GLOBALS_SIZE                        (( sizeof( DSSGlobals) + 3) & ~3)
#define PRIMARY_NET_APP_SIZE                (( sizeof( DSSPrimaryNetApp) + 3) & ~3)
#define SECONDARY_NET_APP_SIZE              (( sizeof( DSSSecondaryNetApp) + 3) & ~3)
#define SOCKET_SIZE                         (( sizeof( DSSSocket) + 3) & ~3)
#define FILTER_REG_INFO_SIZE                (( sizeof( FilterRegInfo) + 3) & ~3)
#define NET_QOS_DEFAULT_SIZE                (( sizeof( DSSNetQoSDefault) + 3) & ~3)
#define NET_QOS_SECONDARY_SIZE              (( sizeof( DSSNetQoSSecondary) + 3) & ~3)
#define MCAST_SIZE                          (( sizeof( DSSMCast) + 3) & ~3)
#define NET_MCAST_MBMS_CTRL_SIZE            (( sizeof( DSSNetMCastMBMSCtrl) + 3) & ~3)
#define IPV6_PRIV_SIZE                      (( sizeof( DSSPrivIpv6Addr) + 3) & ~3) 


/*---------------------------------------------------------------------------
Macros for number of buffers needed, high and low watermarks.
These are valid for both high end and low end targets.
---------------------------------------------------------------------------*/
#ifdef FEATURE_DATACOMMON_PACKAGE_BMP /* APPLICATION */

#define GLOBALS_NUM_BUF                                                 1
#define PRIMARY_NET_APP_NUM_BUF                                         DSS_MAX_APPS 
//#define SECONDARY_NET_APP_NUM_BUF                                     (PRIMARY_NET_APP_NUM_BUF * (MAX_SYSTEM_IFACES - 1))
#define SECONDARY_NET_APP_NUM_BUF                                       (PRIMARY_NET_APP_NUM_BUF + 1) /* additional one for temporary DSSNetApp objects */
#define SOCKET_NUM_BUF                                                  DSS_MAX_SOCKS
#define FILTER_REG_INFO_NUM_BUF                                         20
#define NET_QOS_DEFAULT_NUM_BUF                                         MAX_QOS_DEFAULT_OBJS
#define NET_QOS_SECONDARY_NUM_BUF                                       MAX_QOS_SECONDARY_OBJS
#define MCAST_NUM_BUF                                                   MAX_MCAST_OBJS
#define NET_MCAST_MBMS_CTRL_NUM_BUF                                     MAX_NETWORK_MBMS_OBJS 
#define IPV6_PRIV_NUM_BUF                                               20 // TODO replace with define , consult with Harsh on that
#define EVENT_HANDLER_NUM_BUF                                           100 

#else
#ifdef FEATURE_DATACOMMON_PACKAGE_MODEM /* MODEM */

#define GLOBALS_NUM_BUF                                                 1
#define PRIMARY_NET_APP_NUM_BUF                                         ( DSS_MAX_APPS + 1 )
//#define SECONDARY_NET_APP_NUM_BUF                                     (PRIMARY_NET_APP_NUM_BUF * (MAX_SYSTEM_IFACES - 1))
#define SECONDARY_NET_APP_NUM_BUF                                       (PRIMARY_NET_APP_NUM_BUF + 1) /* additional one for temporary DSSNetApp objects */
#define SOCKET_NUM_BUF                                                  DSS_MAX_SOCKS
#define FILTER_REG_INFO_NUM_BUF                                         20
#define NET_QOS_DEFAULT_NUM_BUF                                         MAX_QOS_DEFAULT_OBJS
#define NET_QOS_SECONDARY_NUM_BUF                                       MAX_QOS_SECONDARY_OBJS
#define MCAST_NUM_BUF                                                   MAX_MCAST_OBJS
#define NET_MCAST_MBMS_CTRL_NUM_BUF                                     MAX_NETWORK_MBMS_OBJS 
#define IPV6_PRIV_NUM_BUF                                               20 // TODO replace with define , consult with Harsh on that
#define EVENT_HANDLER_NUM_BUF                                           50 

#else /* DEFAULT */

#define GLOBALS_NUM_BUF                                                 1
#define PRIMARY_NET_APP_NUM_BUF                                         ( DSS_MAX_APPS + 1 )
//#define SECONDARY_NET_APP_NUM_BUF                                     (PRIMARY_NET_APP_NUM_BUF * (MAX_SYSTEM_IFACES - 1))
#define SECONDARY_NET_APP_NUM_BUF                                       (PRIMARY_NET_APP_NUM_BUF + 1) /* additional one for temporary DSSNetApp objects */
#define SOCKET_NUM_BUF                                                  DSS_MAX_SOCKS
#define FILTER_REG_INFO_NUM_BUF                                         20
#define NET_QOS_DEFAULT_NUM_BUF                                         MAX_QOS_DEFAULT_OBJS
#define NET_QOS_SECONDARY_NUM_BUF                                       MAX_QOS_SECONDARY_OBJS
#define MCAST_NUM_BUF                                                   MAX_MCAST_OBJS
#define NET_MCAST_MBMS_CTRL_NUM_BUF                                     MAX_NETWORK_MBMS_OBJS 
#define IPV6_PRIV_NUM_BUF                                               20 // TODO replace with define , consult with Harsh on that
#define EVENT_HANDLER_NUM_BUF                                           50 

#endif 
#endif




#define GLOBALS_HIGH_WM                          GLOBALS_NUM_BUF
#define PRIMARY_NET_APP_HIGH_WM                  PRIMARY_NET_APP_NUM_BUF
#define SECONDARY_NET_APP_HIGH_WM                SECONDARY_NET_APP_NUM_BUF
#define SOCKET_HIGH_WM                           SOCKET_NUM_BUF
#define FILTER_REG_INFO_HIGH_WM                  FILTER_REG_INFO_NUM_BUF
#define NET_QOS_DEFAULT_HIGH_WM                  NET_QOS_DEFAULT_NUM_BUF
#define NET_QOS_SECONDARY_HIGH_WM                NET_QOS_SECONDARY_NUM_BUF
#define MCAST_HIGH_WM                            MCAST_NUM_BUF
#define NET_MCAST_MBMS_CTRL_HIGH_WM              NET_MCAST_MBMS_CTRL_NUM_BUF

#define EVENT_HANDLER_HIGH_WM                    EVENT_HANDLER_NUM_BUF
#define IPV6_PRIV_HIGH_WM                        IPV6_PRIV_NUM_BUF 


#define GLOBALS_LOW_WM                           0
#define PRIMARY_NET_APP_LOW_WM                   0
#define SECONDARY_NET_APP_LOW_WM                 0
#define SOCKET_LOW_WM                            0
#define FILTER_REG_INFO_LOW_WM                   0
#define NET_QOS_DEFAULT_LOW_WM                   0
#define NET_QOS_SECONDARY_LOW_WM                 0
#define MCAST_LOW_WM                             0
#define NET_MCAST_MBMS_CTRL_LOW_WM               0

#define EVENT_HANDLER_LOW_WM                     0
#define IPV6_PRIV_LOW_WM                         0 


/*---------------------------------------------------------------------------
Allocate memory to hold different ds Net objects along with ps_mem header.
---------------------------------------------------------------------------*/
static int globalsBuf[PS_MEM_GET_TOT_SIZE_OPT( GLOBALS_NUM_BUF,
                                               GLOBALS_SIZE)];

static int primaryNetAppBuf[PS_MEM_GET_TOT_SIZE_OPT( PRIMARY_NET_APP_NUM_BUF,
                                                     PRIMARY_NET_APP_SIZE)];

static int secondaryNetAppBuf[PS_MEM_GET_TOT_SIZE_OPT( SECONDARY_NET_APP_NUM_BUF,
                                                       SECONDARY_NET_APP_SIZE)];

static int socketBuf[PS_MEM_GET_TOT_SIZE_OPT( SOCKET_NUM_BUF,
                                              SOCKET_SIZE)];

static int filterRegInfoBuf[PS_MEM_GET_TOT_SIZE_OPT( FILTER_REG_INFO_NUM_BUF,
                                                     FILTER_REG_INFO_SIZE)];

static int netQosDefaultBuf[PS_MEM_GET_TOT_SIZE_OPT( NET_QOS_DEFAULT_NUM_BUF,
                                                     NET_QOS_DEFAULT_SIZE)];

static int netQosSecondaryBuf[PS_MEM_GET_TOT_SIZE_OPT( NET_QOS_SECONDARY_NUM_BUF,
                                                       NET_QOS_SECONDARY_SIZE)];

static int mcastBuf[PS_MEM_GET_TOT_SIZE_OPT( MCAST_NUM_BUF,
                                             MCAST_SIZE)];

static int netMcastMBMSCtrlBuf[PS_MEM_GET_TOT_SIZE_OPT( NET_MCAST_MBMS_CTRL_NUM_BUF,
                                                        NET_MCAST_MBMS_CTRL_SIZE)];

static int eventHandlerBuf [PS_MEM_GET_TOT_SIZE( EVENT_HANDLER_NUM_BUF,
                                                           EVENT_HANDLER_SIZE)];

static int ipv6PrivBuf      [PS_MEM_GET_TOT_SIZE( IPV6_PRIV_NUM_BUF,
                                                 IPV6_PRIV_SIZE)];


#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*---------------------------------------------------------------------------
Array of pointers used to facilitate easy debugging.  The first one points
to the ps_mem header and the latter points to actual object array.
---------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type             *  globalsHdrPtr[ GLOBALS_NUM_BUF ];
static DSSGlobals                      *  globalsBufPtr[ GLOBALS_NUM_BUF ];

static ps_mem_buf_hdr_type             *  primaryNetAppHdrPtr[ PRIMARY_NET_APP_NUM_BUF ];
static DSSPrimaryNetApp                *  primaryNetAppBufPtr[ PRIMARY_NET_APP_NUM_BUF ];

static ps_mem_buf_hdr_type             *  secondaryNetAppHdrPtr[ SECONDARY_NET_APP_NUM_BUF ];
static DSSSecondaryNetApp              *  secondaryNetAppBufPtr[ SECONDARY_NET_APP_NUM_BUF ];

static ps_mem_buf_hdr_type             *  socketHdrPtr[ SOCKET_NUM_BUF ];
static DSSSocket                       *  socketBufPtr[ SOCKET_NUM_BUF ];

static ps_mem_buf_hdr_type             *  filterRegInfoHdrPtr[ FILTER_REG_INFO_NUM_BUF ];
static DSSSecondaryNetApp              *  filterRegInfoBufPtr[ FILTER_REG_INFO_NUM_BUF ];

static ps_mem_buf_hdr_type             *  netQosDefaultHdrPtr[ NET_QOS_DEFAULT_NUM_BUF ];
static DSSNetQoSDefault                *  netQosDefaultBufPtr[ NET_QOS_DEFAULT_NUM_BUF ];

static ps_mem_buf_hdr_type             *  netQosSecondaryHdrPtr[ NET_QOS_SECONDARY_NUM_BUF ];
static DSSNetQoSSecondary              *  netQosSecondaryBufPtr[ NET_QOS_SECONDARY_NUM_BUF ];

static ps_mem_buf_hdr_type             *  mcastHdrPtr[ MCAST_NUM_BUF ];
static DSSMCast                        *  mcastBufPtr[ MCAST_NUM_BUF ];


static ps_mem_buf_hdr_type              *  netMcastMBMSCtrlHdrPtr[ NET_MCAST_MBMS_CTRL_NUM_BUF ];
static DSSNetMCastMBMSCtrl              *  netMcastMBMSCtrlBufPtr[ NET_MCAST_MBMS_CTRL_NUM_BUF ];

static ps_mem_buf_hdr_type              *  eventHandlerHdrPtr[ EVENT_HANDLER_NUM_BUF ];
static DSSEventHandlerNetApp            *  eventHandlerBufPtr[ EVENT_HANDLER_NUM_BUF ];

static ps_mem_buf_hdr_type              *  ipv6PrivHdrPtr[ IPV6_PRIV_NUM_BUF ];
static DSSPrivIpv6Addr                  *  ipv6PrivBufPtr[ IPV6_PRIV_NUM_BUF ];

#endif /* FEATURE_DATA_PS_MEM_DEBUG */


/*===========================================================================

PUBLIC MEMBER FUNCTIONS

===========================================================================*/
void DSSmem_pool_init()
{
   /* PS_MEM_DSAL_GLOBALS_TYPE INIT */  
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_GLOBALS_TYPE,
      globalsBuf,
      GLOBALS_SIZE,
      GLOBALS_NUM_BUF,
      GLOBALS_HIGH_WM,
      GLOBALS_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *) globalsHdrPtr,
      (int *) globalsBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
         LOG_MSG_FATAL_ERROR( "Can't init pool %d", (int32)PS_MEM_DSAL_GLOBALS_TYPE, 0, 0);
      }
      /* PS_MEM_DSAL_GLOBALS_TYPE INIT */

   /* PS_MEM_DSAL_PRIMARY_NET_APP_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_PRIMARY_NET_APP_TYPE,
      primaryNetAppBuf,
      PRIMARY_NET_APP_SIZE,
      PRIMARY_NET_APP_NUM_BUF,
      PRIMARY_NET_APP_HIGH_WM,
      PRIMARY_NET_APP_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *) primaryNetAppHdrPtr,
      (int *) primaryNetAppBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_PRIMARY_NET_APP_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_PRIMARY_NET_APP_TYPE INIT */

   /* PS_MEM_DSAL_SECONDARY_NET_APP_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_SECONDARY_NET_APP_TYPE,
      secondaryNetAppBuf,
      SECONDARY_NET_APP_SIZE,
      SECONDARY_NET_APP_NUM_BUF,
      SECONDARY_NET_APP_HIGH_WM,
      SECONDARY_NET_APP_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *) secondaryNetAppHdrPtr,
      (int *) secondaryNetAppBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_SECONDARY_NET_APP_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_SECONDARY_NET_APP_TYPE INIT */

   /* PS_MEM_DSAL_SOCKET_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_SOCKET_TYPE,
      socketBuf,
      SOCKET_SIZE,
      SOCKET_NUM_BUF,
      SOCKET_HIGH_WM,
      SOCKET_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *) socketHdrPtr,
      (int *) socketBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_SOCKET_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_SOCKET_TYPE INIT */

   /* PS_MEM_DSAL_FILTER_REG_INFO_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_FILTER_REG_INFO_TYPE,
      filterRegInfoBuf,
      FILTER_REG_INFO_SIZE,
      FILTER_REG_INFO_NUM_BUF,
      FILTER_REG_INFO_HIGH_WM,
      FILTER_REG_INFO_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  filterRegInfoHdrPtr,
      (int *)  filterRegInfoBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_FILTER_REG_INFO_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_FILTER_REG_INFO_TYPE INIT */

   /* PS_MEM_DSAL_NET_QOS_DEFAULT_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_NET_QOS_DEFAULT_TYPE,
      netQosDefaultBuf,
      NET_QOS_DEFAULT_SIZE,
      NET_QOS_DEFAULT_NUM_BUF,
      NET_QOS_DEFAULT_HIGH_WM,
      NET_QOS_DEFAULT_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  netQosDefaultHdrPtr,
      (int *)  netQosDefaultBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_NET_QOS_DEFAULT_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_NET_QOS_DEFAULT_TYPE INIT */

   /* PS_MEM_DSAL_NET_QOS_SECONDARY_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_NET_QOS_SECONDARY_TYPE,
      netQosSecondaryBuf,
      NET_QOS_SECONDARY_SIZE,
      NET_QOS_SECONDARY_NUM_BUF,
      NET_QOS_SECONDARY_HIGH_WM,
      NET_QOS_SECONDARY_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  netQosSecondaryHdrPtr,
      (int *)  netQosSecondaryBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_NET_QOS_SECONDARY_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_NET_QOS_SECONDARY_TYPE INIT */

   /* PS_MEM_DSAL_MCAST_TYPE INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_MCAST_TYPE,
      mcastBuf,
      MCAST_SIZE,
      MCAST_NUM_BUF,
      MCAST_HIGH_WM,
      MCAST_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  mcastHdrPtr,
      (int *)  mcastBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_MCAST_TYPE, 0, 0);
   }
   /* PS_MEM_DSAL_MCAST_TYPE INIT */


   /* PS_MEM_DSAL_NET_MCAST_MBMS_CTRL INIT */
   if (PS_MEM_POOL_INIT_OPT( PS_MEM_DSAL_NET_MCAST_MBMS_CTRL,
      netMcastMBMSCtrlBuf,
      NET_MCAST_MBMS_CTRL_SIZE,
      NET_MCAST_MBMS_CTRL_NUM_BUF,
      NET_MCAST_MBMS_CTRL_HIGH_WM,
      NET_MCAST_MBMS_CTRL_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  netMcastMBMSCtrlHdrPtr,
      (int *)  netMcastMBMSCtrlBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_NET_MCAST_MBMS_CTRL, 0, 0);
   }
   /* PS_MEM_DSAL_NET_MCAST_MBMS_CTRL INIT */

   /* PS_MEM_DSAL_EVENT_HANDLER INIT */
   if (ps_mem_pool_init( PS_MEM_DSAL_EVENT_HANDLER,
      eventHandlerBuf,
      EVENT_HANDLER_SIZE,
      EVENT_HANDLER_NUM_BUF,
      EVENT_HANDLER_HIGH_WM,
      EVENT_HANDLER_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *)  eventHandlerHdrPtr,
      (int *)  eventHandlerBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_EVENT_HANDLER, 0, 0);
   }
   /* PS_MEM_DSAL_EVENT_HANDLER INIT */

   /* PS_MEM_DSAL_PRIV_IPV6_ADDR INIT */
   if (ps_mem_pool_init( PS_MEM_DSAL_PRIV_IPV6_ADDR,
      ipv6PrivBuf,
      IPV6_PRIV_SIZE,
      IPV6_PRIV_NUM_BUF,
      IPV6_PRIV_HIGH_WM,
      IPV6_PRIV_LOW_WM,
      NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
      (int *) ipv6PrivHdrPtr,
      (int *) ipv6PrivBufPtr
#else
      NULL,
      NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
      ) == -1) {
      LOG_MSG_FATAL_ERROR( "Can't init pool %d", (uint32)PS_MEM_DSAL_PRIV_IPV6_ADDR, 0, 0);
     }
     /* PS_MEM_DSAL_PRIV_IPV6_ADDR INIT */

} /* ps_mem_init */

#endif /* FEATURE_DATA_PS */


