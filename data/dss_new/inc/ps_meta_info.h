#ifndef PS_META_INFO_H
#define PS_META_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         P S _ M E T A _ I N F O . H

GENERAL DESCRIPTION
  This is a header file that contains the definition of the meta information
  that is passed down from the sockets layer all the way down to the
  interface.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2002-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_meta_info.h_v   1.2
  12 Feb 2003 20:35:38   omichael  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_meta_info.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
09/12/08    pp     Metainfo optimizations. New ps_*_meta_info.h files created.
05/29/08    ssh    Added PS_META_INFO_COPY
03/16/07    scb    Fixed Klocwork High errors
12/15/06    msr    Using time_type for timestamping packets
11/17/06    msr    Added support for timestamping packets
07/31/06    rt     Route Scope modifications.
07/07/06    msr    Added PS_META_IS_FILTER_MASK_SET()
12/15/05    msr    Added PS_META_RESET_FILTER_RESULT()
09/02/05    msr    Allocating pkt_info in PS_META_INFO_GET()
05/11/05    sv     Lint changes.
05/10/05    vp     PS_META_INFO_DUP and PS_META_INFO_FREE to increment the
                   refcnt instead of allocating a new buffer.
05/02/05    msr    Including ps_iface_ipfltr.h instead of ps_iface_defs.h and
                   changed PS_META_FILTER_ID and PS_META_SET_FILTER_ID to
                   PS_META_SET_FILTER_RESULT and PS_META_SET_FILTER_RESULT,
04/18/05    vp     Addition of route_scope to meta_info.
04/17/05    msr    Added subset ID to support subset based filtering.
01/10/05    sv     Merged IPSEC changes.
12/23/04    lyr    Added next hop addr & packet scope (e.g. unicast) metainfo
10/01/04    ifk    Added meta_info_dup macro
06/15/04    sv     Added flow label to metainfo structure.
04/27/04    usb    Added support for filtering/pkt info in metainfo, removed
                   unused member ipsec_required, cleaned up existing macro
                   definitions, defined new macros.
03/10/04    aku    Call ps_mem_free() only if meta_info_ptr is not NULL.
02/02/04    mvl    Added #define for kind field - used in ps-ds interface.
01/31/04    usb    Removed policy info from meta info.
08/25/03    aku    ps_mem_free() takes pointer-to-pointer as arg.
08/15/03    aku    Added support to use PS memory allocation schemes instead
                   of DSM
08/01/03    ss     Removed ttl and sock_opts from meta info
06/06/03    om     Added ipsec_required flag.
04/24/03    mvl    Added some missing parentheses in macros.
02/12/03    om     Added policy-info-is-valid flag and UDP routing info.
09/24/02    om     Added socket options mask.
04/01/02    mvl    created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "ps_tx_meta_info.h"
#include "ps_rx_meta_info.h"
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            DEFINES AND TYPEDEFS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*---------------------------------------------------------------------------
TYPEDEF ps_meta_info_type

DESCRIPTION
  This is aliased to ps_tx_meta_info_type.

NOTE:
  The usage of this defintion is now obsoleted, please use:
  ps_tx_meta_info_type instead.
---------------------------------------------------------------------------*/
typedef ps_tx_meta_info_type ps_meta_info_type;

/*---------------------------------------------------------------------------
TYPEDEF ps_meta_info_type_ex

DESCRIPTION
  This is used to pass both TX & RX META information. this is mainly used in
  ps_iface_input scenario.

  Fields are:
   tx_meta_info_ptr : ptr to TX meta info [defined in ps_tx_meta_info.h].
   rx_meta_info_ptr : ptr to RX meta info [defined in ps_rx_meta_info.h].

---------------------------------------------------------------------------*/
typedef struct
{
  ps_tx_meta_info_type           * tx_meta_info_ptr;
  ps_rx_meta_info_type           * rx_meta_info_ptr;
} ps_meta_info_type_ex;

/*===========================================================================
MACRO PS_META_INFO_GET_EX()

DESCRIPTION
  This macro allocates a PS mem buffer that is used for the ps_meta_info_type_ex.

PARAMETERS
  mi_ptr : pointer that will be have the ps_meta_info_ex memory assigned to it.

RETURN VALUE
  mi_ptr will be NULL on failure, or point to the ps_meta_info_type_ex buffer.

DEPENDENCIES
  None

SIDE EFFECTS
  Initializes the allocated meta info block to all 0s.

===========================================================================*/
#define PS_META_INFO_GET_EX(mi_ptr)                                 \
  (mi_ptr) = (ps_meta_info_type_ex *)ps_ext_mem_get_buf(PS_EXT_MEM_META_INFO_TYPE_EX); \
  if ( (mi_ptr) != NULL )                                              \
  {                                                                     \
    memset( (mi_ptr), 0, sizeof(ps_meta_info_type_ex) );               \
  }

/*===========================================================================
MACRO PS_META_INFO_FREE_EX()

DESCRIPTION
  This macro de-allocates the PS mem buffer that is used for the
  ps_meta_info_type_ex.

PARAMETERS
  mi_ptr : ps_meta_info_type_ex pointer that will freed.

RETURN VALUE
  *mi_ptr_ptr will be set to NULL.

DEPENDENCIES
  None

SIDE EFFECTS
  None.

===========================================================================*/
#define PS_META_INFO_FREE_EX ps_meta_info_free_ex
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_meta_info_free_ex
(
  ps_meta_info_type_ex ** mi_ptr_ptr
)
{
  if(mi_ptr_ptr != NULL && *mi_ptr_ptr != NULL)
  {
    PS_MEM_FREE(*mi_ptr_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*---------------------------------------------------------------------------
                             META INFO/TX META INFO

  Following definitions are kept to NOT to impact mode handlers!
---------------------------------------------------------------------------*/
/*===========================================================================
MACRO PS_META_INFO_IS_NULL()

DESCRIPTION
  This macro checks for NULL.

ALIAS PS_TX_META_INFO_IS_NULL
===========================================================================*/
#define PS_META_INFO_IS_NULL PS_TX_META_INFO_IS_NULL

/*===========================================================================
MACRO PS_META_INFO_FREE()

DESCRIPTION
  This macro frees the PS mem buffer that is used for the meta info.

ALIAS PS_TX_META_INFO_FREE
===========================================================================*/
#define PS_META_INFO_FREE PS_TX_META_INFO_FREE

/*===========================================================================
MACRO PS_META_GET_IPSEC_INFO()

DESCRIPTION
  This macro returns ipsec information associated with a given meta info

ALIAS PS_TX_META_GET_IPSEC_INFO
===========================================================================*/
#define PS_META_GET_IPSEC_INFO PS_TX_META_GET_IPSEC_INFO

/*===========================================================================
MACRO PS_META_GET_TX_FLAGS()

DESCRIPTION
  This macro returns Tx flags associated with a given meta info

ALIAS PS_TX_META_GET_TX_FLAGS
===========================================================================*/
#define PS_META_GET_TX_FLAGS PS_TX_META_GET_TX_FLAGS

/*===========================================================================
MACRO PS_META_GET_DOS_ACK_HANDLE()

DESCRIPTION
  This macro returns dos_ack_handle associated with a given meta info

ALIAS PS_TX_META_GET_DOS_ACK_HANDLE
===========================================================================*/
#define PS_META_GET_DOS_ACK_HANDLE PS_TX_META_GET_DOS_ACK_HANDLE

/*===========================================================================
MACRO PS_META_GET_ROUTING_CACHE ()

DESCRIPTION
  This macro returns routing_cache associated with a given TX meta info

ALIAS PS_TX_META_GET_ROUTING_CACHE
===========================================================================*/
#define PS_META_GET_ROUTING_CACHE PS_TX_META_GET_ROUTING_CACHE

/*===========================================================================
MACRO PS_META_GET_TIMESTAMP()

DESCRIPTION
  This macro returns timestamp associated with a given meta info

ALIAS PS_TX_META_GET_TIMESTAMP
===========================================================================*/
#define PS_META_GET_TIMESTAMP PS_TX_META_GET_TIMESTAMP

/*===========================================================================
  PS_META* version of MACROs to access TX_META fields
===========================================================================*/
/*===========================================================================
MACRO PS_META_GET_RT_META_INFO_PTR()

DESCRIPTION
  This macro gets ROUTING meta info pointer associated with TX meta info

ALIAS PS_TX_META_GET_RT_META_INFO_PTR
===========================================================================*/
#define PS_META_GET_RT_META_INFO_PTR PS_TX_META_GET_RT_META_INFO_PTR

/*===========================================================================
MACRO PS_META_GET_PKT_META_INFO_PTR()

DESCRIPTION
  This macro gets PACKET meta info pointer associated with TX meta info

ALIAS PS_TX_META_GET_PKT_META_INFO_PTR
===========================================================================*/
#define PS_META_GET_PKT_META_INFO_PTR PS_TX_META_GET_PKT_META_INFO_PTR

#endif /* PS_META_INFO_H */
