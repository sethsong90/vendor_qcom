/*===========================================================================
  FILE: DS_Sock_MemManager.cpp

  OVERVIEW: This file provides implementation of the Socket class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_MemManager.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_RecvIFInfo.h"
#include "DS_Sock_ICMPErrInfo.h"
#include "DS_Sock_TCPSocket.h"
#include "DS_Sock_UDPSocket.h"
#include "DS_Sock_ICMPSocket.h"
#include "DS_Sock_SocketFactory.h"
#include "DS_Sock_SocketFactoryPriv.h"
#include "DS_Sock_MemManager.h"
#include "DS_Sock_SocketDef.h"
#include "DS_Utils_DebugMsg.h"
#include "ps_mem.h"
#include "ps_rt_meta_info.h"
#include "ps_pkt_meta_info.h"


using namespace DS::Sock;
using namespace DS::Utils;


/*===========================================================================

                         INTERNAL DATA DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Macros for sizes of objects of these classes.
---------------------------------------------------------------------------*/
#define RECV_IF_INFO_SIZE                (( sizeof( RecvIFInfo) + 3) & ~3)
#define ICMP_ERR_INFO_SIZE               (( sizeof( ICMPErrInfo) + 3) & ~3)
#define TCP_SOCKET_SIZE                  (( sizeof( TCPSocket) + 3) & ~3)
#define UDP_SOCKET_SIZE                  (( sizeof( UDPSocket) + 3) & ~3)
#define ICMP_SOCKET_SIZE                 (( sizeof( ICMPSocket) + 3) & ~3)
#define SOCKET_FACTORY_SIZE              (( sizeof( SocketFactory) + 3) & ~3)
#define SOCKET_FACTORY_PRIV_SIZE   (( sizeof( SocketFactoryPriv) + 3) & ~3)
#define PS_RT_META_INFO_BUF_SIZE   (( sizeof( ps_rt_meta_info_type) + 3) & ~3)
#define PS_PKT_META_INFO_BUF_SIZE  (( sizeof( ps_pkt_meta_info_type) + 3) & ~3)

/*---------------------------------------------------------------------------
  Macros for number of bufferes needed, high and low watermarks.
  These are valid for both high end and low end targets.
---------------------------------------------------------------------------*/
#define TCP_SOCKET_NUM_BUF                 MAX_TCB
#define UDP_SOCKET_NUM_BUF                 MAX_UDP_SOCK
#define ICMP_SOCKET_NUM_BUF                MAX_ICMP_SOCK
#define SOCKET_FACTORY_NUM_BUF             MAX_SOCKET_FACTORY
#define SOCKET_FACTORY_PRIV_NUM_BUF        MAX_SOCKET_FACTORY_PRIV

#define TCP_SOCKET_HIGH_WM                 ( MAX_TCB - 5)
#define UDP_SOCKET_HIGH_WM                 ( MAX_UDP_SOCK - 5)
#define ICMP_SOCKET_HIGH_WM                1
#define SOCKET_FACTORY_HIGH_WM             1
#define SOCKET_FACTORY_PRIV_HIGH_WM  1

#define TCP_SOCKET_LOW_WM                  0
#define UDP_SOCKET_LOW_WM                  0
#define ICMP_SOCKET_LOW_WM                 0
#define SOCKET_FACTORY_LOW_WM              0
#define SOCKET_FACTORY_PRIV_LOW_WM   0


#ifndef IMAGE_MODEM_PROC

  #define RECV_IF_INFO_NUM_BUF               10
  #define ICMP_ERR_INFO_NUM_BUF              10

  #define RECV_IF_INFO_HIGH_WM               8
  #define ICMP_ERR_INFO_HIGH_WM              8

  #define RECV_IF_INFO_LOW_WM                2
  #define ICMP_ERR_INFO_LOW_WM               2

#else

  #define RECV_IF_INFO_NUM_BUF               4
  #define ICMP_ERR_INFO_NUM_BUF              4

  #define RECV_IF_INFO_HIGH_WM               3
  #define ICMP_ERR_INFO_HIGH_WM              3

  #define RECV_IF_INFO_LOW_WM                1
  #define ICMP_ERR_INFO_LOW_WM               1

#endif /* ifndef IMAGE_MODEM_PROC */



#if !(defined(FEATURE_DATA_PS_LOW_MEM_CHIPSET) || defined (IMAGE_MODEM_PROC))

#define PS_RT_META_INFO_BUF_NUM      50
#define PS_RT_META_INFO_BUF_HIGH_WM  45
#define PS_RT_META_INFO_BUF_LOW_WM    5

#define PS_PKT_META_INFO_BUF_NUM      50
#define PS_PKT_META_INFO_BUF_HIGH_WM  45
#define PS_PKT_META_INFO_BUF_LOW_WM    5

#else

#define PS_RT_META_INFO_BUF_NUM       25
#define PS_RT_META_INFO_BUF_HIGH_WM   20
#define PS_RT_META_INFO_BUF_LOW_WM     5

#define PS_PKT_META_INFO_BUF_NUM       25
#define PS_PKT_META_INFO_BUF_HIGH_WM   20
#define PS_PKT_META_INFO_BUF_LOW_WM     5
#endif /* !(defined(FEATURE_DATA_PS_LOW_MEM_CHIPSET) || defined (IMAGE_MODEM_PROC)) */

/*---------------------------------------------------------------------------
  Allocate memory to hold different DS Net objects along with ps_mem header.
---------------------------------------------------------------------------*/
static int recvIFInfoBuf[ PS_MEM_GET_TOT_SIZE( RECV_IF_INFO_NUM_BUF,
                                               RECV_IF_INFO_SIZE)];

static int icmpErrInfoBuf[ PS_MEM_GET_TOT_SIZE( ICMP_ERR_INFO_NUM_BUF,
                                                ICMP_ERR_INFO_SIZE)];

static int tcpSocketBuf[ PS_MEM_GET_TOT_SIZE_OPT( TCP_SOCKET_NUM_BUF,
                                                  TCP_SOCKET_SIZE)];

static int udpSocketBuf[ PS_MEM_GET_TOT_SIZE_OPT( UDP_SOCKET_NUM_BUF,
                                                  UDP_SOCKET_SIZE)];

static int icmpSocketBuf[ PS_MEM_GET_TOT_SIZE_OPT( ICMP_SOCKET_NUM_BUF,
                                                   ICMP_SOCKET_SIZE)];

static int socketFactoryBuf[ PS_MEM_GET_TOT_SIZE( SOCKET_FACTORY_NUM_BUF,
                                                  SOCKET_FACTORY_SIZE)];

static int socketFactoryPrivBuf[ PS_MEM_GET_TOT_SIZE
                                  (
                                    SOCKET_FACTORY_PRIV_NUM_BUF,
                                    SOCKET_FACTORY_PRIV_SIZE
                                  )];

static int ps_rt_meta_info_buf_mem[ PS_MEM_GET_TOT_SIZE
                                    (
                                      PS_RT_META_INFO_BUF_NUM,
                                      PS_RT_META_INFO_BUF_SIZE
                                    )];

static int ps_pkt_meta_info_buf_mem[ PS_MEM_GET_TOT_SIZE
                                     (
                                       PS_PKT_META_INFO_BUF_NUM,
                                       PS_PKT_META_INFO_BUF_SIZE
                                     )];


#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*---------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to actual object array.
---------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type *  recvIFInfoBufHdrPtr[ RECV_IF_INFO_NUM_BUF];
static RecvIFInfo          *  recvIFInfoBufPtr[ RECV_IF_INFO_NUM_BUF];

static ps_mem_buf_hdr_type *  icmpErrInfoBufHdrPtr[ ICMP_ERR_INFO_NUM_BUF];
static ICMPErrInfo         *  icmpErrInfoBufPtr[ ICMP_ERR_INFO_NUM_BUF];

static ps_mem_buf_hdr_type *  tcpSocketBufHdrPtr[ TCP_SOCKET_NUM_BUF];
static TCPSocket           *  tcpSocketBufPtr[ TCP_SOCKET_NUM_BUF];

static ps_mem_buf_hdr_type *  udpSocketBufHdrPtr[ UDP_SOCKET_NUM_BUF];
static UDPSocket           *  udpSocketBufPtr[ UDP_SOCKET_NUM_BUF];

static ps_mem_buf_hdr_type *  icmpSocketBufHdrPtr[ ICMP_SOCKET_NUM_BUF];
static ICMPSocket          *  icmpSocketBufPtr[ ICMP_SOCKET_NUM_BUF];

static ps_mem_buf_hdr_type *  socketFactoryBufHdrPtr[ SOCKET_FACTORY_NUM_BUF];
static SocketFactory       *  socketFactoryBufPtr[ SOCKET_FACTORY_NUM_BUF];

static ps_mem_buf_hdr_type *  socketFactoryPrivBufHdrPtr[ SOCKET_FACTORY_PRIV_NUM_BUF];
static SocketFactoryPriv   *  socketFactoryPrivBufPtr[ SOCKET_FACTORY_PRIV_NUM_BUF];

static ps_mem_buf_hdr_type   * ps_rt_meta_info_buf_hdr[PS_RT_META_INFO_BUF_NUM];
static ps_rt_meta_info_type  * ps_rt_meta_info_buf_ptr[PS_RT_META_INFO_BUF_NUM];

static ps_mem_buf_hdr_type   * ps_pkt_meta_info_buf_hdr[PS_PKT_META_INFO_BUF_NUM];
static ps_pkt_meta_info_type * ps_pkt_meta_info_buf_ptr[PS_PKT_META_INFO_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/
void MemManager::Init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1( "Initing Socket Lib mem pools", 0, 0, 0);

  if (ps_mem_pool_init( PS_MEM_RECV_IF_INFO_TYPE,
                        recvIFInfoBuf,
                        RECV_IF_INFO_SIZE,
                        RECV_IF_INFO_NUM_BUF,
                        RECV_IF_INFO_HIGH_WM,
                        RECV_IF_INFO_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) recvIFInfoBufHdrPtr,
                        (int *) recvIFInfoBufPtr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_RECV_IF_INFO_TYPE, 0, 0);
  }

  if (ps_mem_pool_init( PS_MEM_ICMP_ERR_INFO_TYPE,
                        icmpErrInfoBuf,
                        ICMP_ERR_INFO_SIZE,
                        ICMP_ERR_INFO_NUM_BUF,
                        ICMP_ERR_INFO_HIGH_WM,
                        ICMP_ERR_INFO_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) icmpErrInfoBufHdrPtr,
                        (int *) icmpErrInfoBufPtr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_ICMP_ERR_INFO_TYPE, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT( PS_MEM_TCP_SOCKET_TYPE,
                            tcpSocketBuf,
                            TCP_SOCKET_SIZE,
                            TCP_SOCKET_NUM_BUF,
                            TCP_SOCKET_HIGH_WM,
                            TCP_SOCKET_LOW_WM,
                            NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                            (int *) tcpSocketBufHdrPtr,
                            (int *) tcpSocketBufPtr
#else
                            NULL,
                            NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_TCP_SOCKET_TYPE, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT( PS_MEM_UDP_SOCKET_TYPE,
                            udpSocketBuf,
                            UDP_SOCKET_SIZE,
                            UDP_SOCKET_NUM_BUF,
                            UDP_SOCKET_HIGH_WM,
                            UDP_SOCKET_LOW_WM,
                            NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                            (int *) udpSocketBufHdrPtr,
                            (int *) udpSocketBufPtr
#else
                            NULL,
                            NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_UDP_SOCKET_TYPE, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT( PS_MEM_ICMP_SOCKET_TYPE,
                            icmpSocketBuf,
                            ICMP_SOCKET_SIZE,
                            ICMP_SOCKET_NUM_BUF,
                            ICMP_SOCKET_HIGH_WM,
                            ICMP_SOCKET_LOW_WM,
                            NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                            (int *) icmpSocketBufHdrPtr,
                            (int *) icmpSocketBufPtr
#else
                            NULL,
                            NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_ICMP_SOCKET_TYPE, 0, 0);
  }

  if (ps_mem_pool_init( PS_MEM_SOCKET_FACTORY_TYPE,
                        socketFactoryBuf,
                        SOCKET_FACTORY_SIZE,
                        SOCKET_FACTORY_NUM_BUF,
                        SOCKET_FACTORY_HIGH_WM,
                        SOCKET_FACTORY_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) socketFactoryBufHdrPtr,
                        (int *) socketFactoryBufPtr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL( "Can't init pool %d", PS_MEM_SOCKET_FACTORY_TYPE, 0, 0);
  }

  if (ps_mem_pool_init( PS_MEM_SOCKET_FACTORY_PRIV_TYPE,
                        socketFactoryPrivBuf,
                        SOCKET_FACTORY_PRIV_SIZE,
                        SOCKET_FACTORY_PRIV_NUM_BUF,
                        SOCKET_FACTORY_PRIV_HIGH_WM,
                        SOCKET_FACTORY_PRIV_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) socketFactoryPrivBufHdrPtr,
                        (int *) socketFactoryPrivBufPtr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL( "Can't init pool %d",
               PS_MEM_SOCKET_FACTORY_PRIV_TYPE, 0, 0);
  }

  if (ps_mem_pool_init( PS_MEM_RT_META_INFO_TYPE,
                        ps_rt_meta_info_buf_mem,
                        PS_RT_META_INFO_BUF_SIZE,
                        PS_RT_META_INFO_BUF_NUM,
                        PS_RT_META_INFO_BUF_HIGH_WM,
                        PS_RT_META_INFO_BUF_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) ps_rt_meta_info_buf_hdr,
                        (int *) ps_rt_meta_info_buf_ptr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL("Can't init the module", 0, 0, 0);
  }

  if (ps_mem_pool_init( PS_MEM_PKT_META_INFO_TYPE,
                        ps_pkt_meta_info_buf_mem,
                        PS_PKT_META_INFO_BUF_SIZE,
                        PS_PKT_META_INFO_BUF_NUM,
                        PS_PKT_META_INFO_BUF_HIGH_WM,
                        PS_PKT_META_INFO_BUF_LOW_WM,
                        NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                        (int *) ps_pkt_meta_info_buf_hdr,
                        (int *) ps_pkt_meta_info_buf_ptr
#else
                        NULL,
                        NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    ERR_FATAL("Can't init the module", 0, 0, 0);
  }

} /* MemManager::Init() */

#endif /* FEATURE_DATA_PS */
