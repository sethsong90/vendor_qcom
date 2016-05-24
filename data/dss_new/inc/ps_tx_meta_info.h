#ifndef PS_TX_META_INFO_H
#define PS_TX_META_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         P S _ T X _ M E T A _ I N F O . H

GENERAL DESCRIPTION
  This is a header file that contains the definition of the TX [transmit]
  meta information that is passed down from the sockets layer all the way
  down to the interface.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/PS_TX_META_info.h_v   1.2
  12 Feb 2003 20:35:38   omichael  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_tx_meta_info.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/26/10    pp     Added PS_EXT_TX_META_INFO_GET_ALL.
06/21/10    pp     PS_RT_META_INVALIDATE_PKT_INFO typo correction.
01/05/10    am     Removed rt_cache copy from Rx -> Tx metainfo copy.
12/14/08    pp     Common Modem Interface: Public/Private API split.
10/03/08    pp     Macro PS_TX_META_INFO_GET_ALL introduced.
03/06/08    pp     Created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "amssassert.h"
#include "comdef.h"
#include "ps_pkt_meta_info.h"
#include "ps_rt_meta_info.h"
#include "ps_rx_meta_info.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            DEFINES AND TYPEDEFS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
- To aid in internal debugging following MACRO is introduced.
  - Current Level: MSG_LOW
---------------------------------------------------------------------------*/
#define TX_META_INFO_DEBUG( sf, a, b, c )                                \
    /*lint -e{1534} */  MSG_LOW( sf, a, b, c )

/*---------------------------------------------------------------------------
TYPEDEF PS_TX_META_INFO_TYPE

DESCRIPTION
  This is used to pass TX META information through the stack.
  It moves along with the dsm item carrying the actual IP packet.

  Fields are:
   pkt_meta_info_ptr : ptr to PACKET meta info [defined in ps_pkt_meta_info.h].
   rt_meta_info_ptr  : ptr to ROUTING meta info [defined in ps_rt_meta_info.h].
---------------------------------------------------------------------------*/
typedef struct
{
  ps_pkt_meta_info_type           * pkt_meta_info_ptr;
  ps_rt_meta_info_type            * rt_meta_info_ptr;
} ps_tx_meta_info_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            PS TX META INFO MACROS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
MACRO PS_TX_META_GET_RT_META_INFO_PTR()

DESCRIPTION
  This macro gets ROUTING meta info pointer associated with TX meta info

PARAMETERS
  tx_mi_ptr : pointer to ps_tx_meta_info_type.

RETURN VALUE
  returns rt_meta_info_ptr

DEPENDENCIES
  tx_mi_ptr must be valid.

SIDE EFFECTS
  None.
===========================================================================*/
#define PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr)                    \
  (tx_mi_ptr)->rt_meta_info_ptr

/*===========================================================================
MACRO PS_TX_META_GET_PKT_META_INFO_PTR()

DESCRIPTION
  This macro gets PACKET meta info pointer associated with TX meta info.

PARAMETERS
  tx_mi_ptr : pointer to ps_tx_meta_info_type.

RETURN VALUE
  returns pkt_meta_info_ptr

DEPENDENCIES
  tx_mi_ptr must be valid.

SIDE EFFECTS
  None.
===========================================================================*/
#define PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr)                   \
  (tx_mi_ptr)->pkt_meta_info_ptr

/*===========================================================================
MACRO PS_TX_META_INFO_FREE()

DESCRIPTION
  This macro de-allocates the PS mem buffer that is used for the TX meta info.
  - This first deallocates PACKET meta info, ROUTING meta info buffers inside
    the TX meta info and then frees memory associated with the
    TX meta info buffer.

PARAMETERS
  tx_mi_ptr : ps_tx_meta_info_type pointer that will freed.

RETURN VALUE
  *tx_mi_ptr_ptr will be set to NULL.

DEPENDENCIES
  None

SIDE EFFECTS
  None.

NOTE:
  This is the single macro to free entire meta info buffer.
===========================================================================*/
#define PS_TX_META_INFO_FREE ps_tx_meta_info_free
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_tx_meta_info_free
(
  ps_tx_meta_info_type ** tx_mi_ptr_ptr
)
{
  ps_pkt_meta_info_type * pkt_mi_ptr;
  ps_rt_meta_info_type  * rt_mi_ptr;

  if (tx_mi_ptr_ptr != NULL && *tx_mi_ptr_ptr != NULL)
  {
    pkt_mi_ptr = PS_TX_META_GET_PKT_META_INFO_PTR(*tx_mi_ptr_ptr);
    PS_PKT_META_INFO_FREE(&pkt_mi_ptr);

    rt_mi_ptr  = PS_TX_META_GET_RT_META_INFO_PTR(*tx_mi_ptr_ptr);
    PS_RT_META_INFO_FREE(&rt_mi_ptr);

    TX_META_INFO_DEBUG( "TX_META FREE ptr 0x%p", *tx_mi_ptr_ptr, 0, 0 );
    PS_MEM_FREE(*tx_mi_ptr_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_TX_META_INFO_DUP()

DESCRIPTION
  This macro duplicates the passed PS mem buffer that is used for the TX meta
  info.

PARAMETERS
  tx_mi_ptr   : ps_tx_meta_info_type pointer that will dup'd.
  dup_ptr_ptr : pointer to a ps_tx_meta_info_type pointer.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  This macro dup's ROUTING meta info, PACKET meta info along with TX meta info.
===========================================================================*/
#define PS_TX_META_INFO_DUP ps_tx_meta_info_dup
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_tx_meta_info_dup
(
  ps_tx_meta_info_type * tx_mi_ptr,
  ps_tx_meta_info_type ** dup_ptr_ptr
)
{
  if (tx_mi_ptr != NULL && dup_ptr_ptr != NULL)
  {
    TX_META_INFO_DEBUG( "PS_TX_META_INFO_DUP ptr 0x%p", tx_mi_ptr, 0, 0 );

    *dup_ptr_ptr = (ps_tx_meta_info_type *) ps_mem_dup(tx_mi_ptr);
    PS_PKT_META_INFO_DUP(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr),
                         &(PS_TX_META_GET_PKT_META_INFO_PTR(*dup_ptr_ptr)));
    PS_RT_META_INFO_DUP(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),
                        &(PS_TX_META_GET_RT_META_INFO_PTR(*dup_ptr_ptr)));
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_TX_META_INFO_COPY()

DESCRIPTION
  This macro copies the meta info buffer details into passed copy pointer buffer.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type
  copy_ptr  : pointer to a ps_tx_meta_info_type [copy buffer],
RETURN VALUE
  None

DEPENDENCIES
  Both tx_mi_ptr, copy_ptr should be valid.

SIDE EFFECTS
  Copies ROUTING and PACKET meta info buffers from tx_mi_ptr to copy_ptr buffer.
===========================================================================*/
#define PS_TX_META_INFO_COPY(tx_mi_ptr, copy_ptr)                   \
  {                                                                      \
    if(((tx_mi_ptr) != NULL) && ((copy_ptr) != NULL))                 \
    {                                                                    \
      TX_META_INFO_DEBUG( "TX_META COPY ptr f:0x%p t:0x%p ", tx_mi_ptr, copy_ptr, 0 );  \
      if((PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr) != NULL) &&            \
         (PS_TX_META_GET_RT_META_INFO_PTR(copy_ptr) != NULL))            \
      {                                                                  \
        memcpy(PS_TX_META_GET_RT_META_INFO_PTR(copy_ptr),             \
                PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),               \
                sizeof(ps_rt_meta_info_type));                           \
      }                                                                  \
      if((PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr) != NULL) &&          \
         (PS_TX_META_GET_PKT_META_INFO_PTR(copy_ptr) != NULL))          \
      {                                                                  \
        memcpy(PS_TX_META_GET_PKT_META_INFO_PTR(copy_ptr),             \
                PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr),               \
                sizeof(ps_pkt_meta_info_type));                           \
      }                                                                  \
    }                                                                    \
  }

/*===========================================================================
MACRO PS_TX_META_INFO_IS_NULL()
      PS_TX_AND_RT_META_INFO_IS_NULL()
      PS_TX_META_INFO_ALL_IS_NULL()

DESCRIPTION
  The NULL check MACROs.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type

RETURN VALUE
  TRUE or FALSE

DEPENDENCIES
  None
  
SIDE EFFECTS
  None.
===========================================================================*/

#define PS_TX_META_INFO_IS_NULL(tx_mi_ptr)                           \
   ((tx_mi_ptr == NULL))

#define PS_TX_AND_RT_META_INFO_IS_NULL(tx_mi_ptr)                           \
      ((tx_mi_ptr == NULL) ||                                           \
    (PS_RT_META_INFO_IS_NULL(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))))

#define PS_TX_META_INFO_ALL_IS_NULL(tx_mi_ptr)                           \
   ((tx_mi_ptr == NULL) ||                                           \
    (PS_RT_META_INFO_IS_NULL(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))) || \
    (PS_PKT_META_INFO_IS_NULL(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr))))

/*===========================================================================
MACRO PS_TX_META_GET_PKT_INFO()

DESCRIPTION
  This macro returns the pointer to the pkt info stored inside TX meta info.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_PKT_INFO(tx_mi_ptr)                                    \
  PS_RT_META_GET_PKT_INFO(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_PKT_INFO()

DESCRIPTION
  This macro sets the Pkt-info to the pkt-info stored inside TX meta info.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_PKT_INFO(tx_mi_ptr, mi_pkt_info)                      \
  PS_RT_META_SET_PKT_INFO(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr), mi_pkt_info)

/*===========================================================================
MACRO PS_TX_META_IS_PKT_INFO_VALID()

DESCRIPTION
  This macro returns whether Pkt-info inside TX meta info is valid or not. Returns is_pkt_info_valid flag.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_IS_PKT_INFO_VALID(tx_mi_ptr)                      \
  PS_RT_META_IS_PKT_INFO_VALID(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_INVALIDATE_PKT_INFO()

DESCRIPTION
  This macro invalidates the Pkt-info inside TX meta info. This is used to regenate pkt_info using
  ps_pkt_info utility functions.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_INVALIDATE_PKT_INFO(tx_mi_ptr)                      \
  PS_RT_META_INVALIDATE_PKT_INFO(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_GET_FILTER_MASK()

DESCRIPTION
  This macro gets the entire filter mask associated with the meta info.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_FILTER_MASK(tx_mi_ptr)                          \
  PS_RT_META_GET_FILTER_MASK(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_IS_FILTER_MASK_SET()

DESCRIPTION
  This macro checks whether the filter mask associated with a particular
  client id is set

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.
  client_id : IP filter client id

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_IS_FILTER_MASK_SET(tx_mi_ptr, client_id)                    \
  PS_RT_META_IS_FILTER_MASK_SET(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr), \
                                    client_id)

/*===========================================================================
MACRO PS_TX_META_GET_FILTER_RESULT()

DESCRIPTION
  This macro returns the filter result associated with a particular client id
  The result will be NULL if the particular filter has not been executed or
  failed the match on the packet.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.
  client_id : IP filter client id

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_FILTER_RESULT(tx_mi_ptr, client_id)                     \
  PS_RT_META_GET_FILTER_RESULT(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr), \
                                   client_id)

/*===========================================================================
MACRO PS_TX_META_SET_FILTER_RESULT()

DESCRIPTION
  This macro sets the filter result associated with a particular client id.
  The corresponding filter mask bit is also set to indicate there is no need
  to execute filters for the client id since a resulting fi_result is already
  available.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type
  client_id : IP filter client id
  fi_result : Filter result  to set (of type ps_iface_ipfltr_result_type)

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_FILTER_RESULT(tx_mi_ptr, client_id, in_fi_result)    \
  PS_RT_META_SET_FILTER_RESULT(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr), \
                                   client_id, in_fi_result)

/*===========================================================================
MACRO PS_TX_META_RESET_FILTER_RESULT()

DESCRIPTION
  This macro resets the filter result associated with a particular client id.
  The corresponding filter mask bit is also reset

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.
  client_id : IP filter client id

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_RESET_FILTER_RESULT(tx_mi_ptr, client_id)                   \
  PS_RT_META_RESET_FILTER_RESULT(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr), \
                                     client_id)

/*===========================================================================
MACRO PS_TX_META_GET_SUBSET_ID()

DESCRIPTION
  This macro returns the subset id associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_SUBSET_ID(tx_mi_ptr)                                   \
  PS_RT_META_GET_SUBSET_ID(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_SUBSET_ID()

DESCRIPTION
  This macro sets subset id for a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.
  subset_id : subset id to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_SUBSET_ID( tx_mi_ptr, mi_subset_id)                   \
  PS_RT_META_SET_SUBSET_ID(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),     \
                              mi_subset_id)

/*===========================================================================
MACRO PS_TX_META_GET_ROUTING_CACHE()

DESCRIPTION
  This macro returns routing cache associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_ROUTING_CACHE(tx_mi_ptr)                          \
  PS_RT_META_GET_ROUTING_CACHE(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_ROUTING_CACHE()

DESCRIPTION
  This macro sets routing cache for a given TX meta info

PARAMETERS
  tx_mi_ptr     : pointer to a ps_tx_meta_info_type.
  routing_cache : routing cache to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_ROUTING_CACHE(tx_mi_ptr, mi_routing_cache)             \
  PS_RT_META_SET_ROUTING_CACHE(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),\
                                  mi_routing_cache)

/*===========================================================================
MACRO PS_TX_META_GET_IPSEC_INFO()

DESCRIPTION
  This macro returns ipsec information associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_IPSEC_INFO(tx_mi_ptr)                                 \
  PS_RT_META_GET_IPSEC_INFO(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_IPSEC_INFO()

DESCRIPTION
  This macro sets ipsec information for a given TX meta info

PARAMETERS
  tx_mi_ptr  : pointer to a ps_tx_meta_info_type.
  ipsec_info : ipsec information to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_IPSEC_INFO( tx_mi_ptr, mi_ipsec_info)                \
  PS_RT_META_SET_IPSEC_INFO(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),   \
                               (mi_ipsec_info));

/*===========================================================================
MACRO PS_TX_META_GET_IP_ADDR_SCOPE()

DESCRIPTION
  This macro returns IP address scope associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type pointer.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_IP_ADDR_SCOPE(tx_mi_ptr)                               \
  PS_RT_META_GET_IP_ADDR_SCOPE(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_IP_ADDR_SCOPE()

DESCRIPTION
  This macro sets IP address scope for a given TX meta info

PARAMETERS
  tx_mi_ptr     : pointer to a ps_tx_meta_info_type pointer.
  ip_addr_scope : IP address scope to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_IP_ADDR_SCOPE(tx_mi_ptr, mi_ip_addr_scope)             \
  PS_RT_META_SET_IP_ADDR_SCOPE(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),\
                                  mi_ip_addr_scope)

/*===========================================================================
MACRO PS_TX_META_GET_NEXT_HOP_ADDR()

DESCRIPTION
  This macro returns next hop address associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type pointer.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_NEXT_HOP_ADDR(tx_mi_ptr)                             \
  PS_RT_META_GET_NEXT_HOP_ADDR(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_NEXT_HOP_ADDR()

DESCRIPTION
  This macro sets next hop address for a given TX meta info

PARAMETERS
  tx_mi_ptr     : pointer to a ps_tx_meta_info_type.
  next_hop_addr : next hop address to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_NEXT_HOP_ADDR(tx_mi_ptr, mi_next_hop_addr)             \
  PS_RT_META_SET_NEXT_HOP_ADDR(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr),\
                                  mi_next_hop_addr)

/*===========================================================================
MACRO PS_TX_META_GET_TX_FLAGS()

DESCRIPTION
  This macro returns Tx flags associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->pkt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_TX_FLAGS(tx_mi_ptr)                                \
  PS_PKT_META_GET_TX_FLAGS(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_SET_TX_FLAGS()

DESCRIPTION
  This macro sets Tx flags for a given TX meta info

PARAMETERS
  tx_mi_ptr: pointer to a ps_tx_meta_info_type.
  tx_flags : Tx flags to be set

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->pkt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_SET_TX_FLAGS(tx_mi_ptr, mi_tx_flags)                       \
  PS_PKT_META_SET_TX_FLAGS(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr),    \
                              mi_tx_flags);

/*===========================================================================
MACRO PS_TX_META_GET_DOS_ACK_HANDLE()

DESCRIPTION
  This macro returns dos_ack_handle associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->pkt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_DOS_ACK_HANDLE(tx_mi_ptr)                            \
  PS_PKT_META_GET_DOS_ACK_HANDLE(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_GET_TIMESTAMP()

DESCRIPTION
  This macro returns timestamp associated with a given TX meta info

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  tx_mi_ptr, tx_mi_ptr->pkt_meta_info_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_TX_META_GET_TIMESTAMP(tx_mi_ptr)                              \
  PS_PKT_META_GET_TIMESTAMP(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr))

/*===========================================================================
MACRO PS_TX_META_INFO_COPY_FROM_RX_META_INFO()

DESCRIPTION
  This macro copies contents from RX meta info buffer into TX meta info buffer.
  - Main fields copied are: ipsec_info, subset_id, routing_cache, pkt_info.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
  tx_mi_ptr will be NULL on failure, or point to the meta_info data

DEPENDENCIES
  rx_tx_mi_ptr, tx_mi_ptr, tx_mi_ptr->rt_meta_info_ptr must be valid.

SIDE EFFECTS
  None.

NOTES:
  - Before calling this MACRO the tx_mi_ptr must be allocated using
    PS_TX_META_INFO_GET().
===========================================================================*/
#define PS_TX_META_INFO_COPY_FROM_RX_META_INFO(rx_mi_ptr, tx_mi_ptr ) \
  if (((tx_mi_ptr) != NULL) &&                                             \
       (PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr) != NULL) && \
      (rx_mi_ptr) != NULL)                                         \
  {                                                                 \
    ps_rt_meta_info_type *rt_mi_ptr = PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr); \
    TX_META_INFO_DEBUG( "TX_META COPY ptr 0x%p", tx_mi_ptr, 0, 0 );\
    /* Copy RX meta info fields to TX meta info here */              \
    PS_RT_META_SET_IPSEC_INFO(rt_mi_ptr,                          \
                                 PS_RX_META_GET_IPSEC_INFO(rx_mi_ptr)); \
    PS_RT_META_SET_PKT_INFO(rt_mi_ptr,                             \
                               PS_RX_META_GET_PKT_INFO(rx_mi_ptr)); \
  }

/*===========================================================================
MACRO PS_TX_META_SET_PKT_META_INFO_PTR()

DESCRIPTION
  This macro attaches a PACKET META INFO pointer to a given TX meta info.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
..None.

DEPENDENCIES
  None

SIDE EFFECTS
  tx_mi_ptr->pkt_meta_info_ptr will be updated
===========================================================================*/
#define PS_TX_META_SET_PKT_META_INFO_PTR(tx_mi_ptr, pkt_mi_ptr)  \
  (tx_mi_ptr)->pkt_meta_info_ptr = (pkt_mi_ptr);


/*===========================================================================
MACRO PS_TX_META_SET_RT_META_INFO_PTR()

DESCRIPTION
  This macro attaches a ROUTING META INFO pointer to a given TX meta info.

PARAMETERS
  tx_mi_ptr : pointer to a ps_tx_meta_info_type.

RETURN VALUE
..None.

DEPENDENCIES
  None

SIDE EFFECTS
  tx_mi_ptr->pkt_meta_info_ptr will be updated
===========================================================================*/
#define PS_TX_META_SET_RT_META_INFO_PTR(tx_mi_ptr, rt_mi_ptr)  \
  (tx_mi_ptr)->rt_meta_info_ptr = (rt_mi_ptr);

/*===========================================================================
MACRO PS_EXT_TX_META_INFO_GET_ALL()

DESCRIPTION
  This macro allocates a PS mem buffer that is used for the TX meta info.
  - Should be used by External clients.
  - Internal clients should use PS_TX_META_INFO_GET_ALL
  - This also allocates ROUTING meta info buffer inside the TX meta info.
  - This also allocates PACKET meta info buffer inside the TX meta info.

PARAMETERS
  tx_mi_ptr : pointer that will be have the ps_tx_meta_info memory assigned to it.

RETURN VALUE
  tx_mi_ptr will be NULL on failure, or point to the ps_tx_meta_info_type buffer.
  tx_mi_ptr->rt_meta_info_ptr will be NULL on failure, or point to the
                           rt_meta_info_type buffer.
  tx_mi_ptr->pkt_meta_info_ptr will be NULL. on failure, or point to the
                           pkt_meta_info_type buffer.

DEPENDENCIES
  None

SIDE EFFECTS
  Initializes the allocated meta info block to all 0s.

NOTE:
  This always creates ALL meta info buffers inside the TX meta info,
===========================================================================*/
#define PS_EXT_TX_META_INFO_GET_ALL(tx_mi_ptr)               \
  (tx_mi_ptr) = (ps_tx_meta_info_type *)ps_ext_mem_get_buf(PS_EXT_MEM_TX_META_INFO_TYPE); \
  if ( (tx_mi_ptr) != NULL )                                              \
  {                                                                         \
    TX_META_INFO_DEBUG( "TX_META EXT GET ptr 0x%p", tx_mi_ptr, 0, 0 );   \
    memset( (tx_mi_ptr), 0, sizeof(ps_tx_meta_info_type) );               \
    /* Create ROUTING meta info */                                  \
    PS_EXT_RT_META_INFO_GET(PS_TX_META_GET_RT_META_INFO_PTR(tx_mi_ptr)); \
    /* Create PACKET meta info */                                  \
    PS_EXT_PKT_META_INFO_GET(PS_TX_META_GET_PKT_META_INFO_PTR(tx_mi_ptr)); \
  }

#endif /* PS_TX_META_INFO_H */
