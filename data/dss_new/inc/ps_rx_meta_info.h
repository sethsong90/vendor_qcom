#ifndef PS_RX_META_INFO_H
#define PS_RX_META_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ R X _ M E T A _ I N F O . H

GENERAL DESCRIPTION
  This is a header file that contains the definition of the RX meta information
  that is passed up from the Mode handler/PPP layer to the IP layer.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_meta_info.h_v   1.2
  12 Feb 2003 20:35:38   omichael  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_rx_meta_info.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
12/28/07    pp     Created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "time_svc.h"
#include "qw.h"
#include "msg.h"

#include "ps_mem_ext.h"
#include "ps_iface_ipfltr.h"
#include "ps_acl.h"
#include "ps_in.h"
#include "ps_pkt_info.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            DEFINES AND TYPEDEFS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
- To aid in internal debugging following MACRO is introduced.
  - Current Level: MSG LOW
---------------------------------------------------------------------------*/
#define RX_META_INFO_DEBUG( sf, a, b, c )                                \
  /*lint -e{1534} */  MSG_LOW( sf, a, b, c )

/*---------------------------------------------------------------------------
TYPEDEF PS_RX_META_INFO_TYPE

DESCRIPTION
  This is used to pass per packet meta information through the stack.
  It moves along with the dsm item carrying the actual IP packet in RX path.

  Fields are:
    ipsec_info     : IPSEC inforamtion.
    pkt_info       : IP pkt info containing header and misc info
    fi_mask        : indicates if a particular filter client is already executed
    fi_result      : filtering result for each client
    routing_cache  : cached routing result
---------------------------------------------------------------------------*/
typedef struct
{
  ps_ipsec_info_type                ipsec_info;
  ip_pkt_info_type                  pkt_info;
  uint32                            fi_mask;
  ps_iface_ipfltr_result_type       fi_result[IP_FLTR_CLIENT_INPUT_MAX];
  void                            * routing_cache;
} ps_rx_meta_info_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            RX META INFO MACROS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
MACRO PS_RX_META_INFO_FREE()

DESCRIPTION
  This macro frees the PS mem buffer that is used for the RX meta info.

PARAMETERS
  rx_mi_ptr_ptr : pointer to a ps_rx_meta_info_type pointer.

RETURN VALUE
  *rx_mi_ptr_ptr will be set to Null.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_INFO_FREE ps_rx_meta_info_free
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_rx_meta_info_free
(
  ps_rx_meta_info_type ** rx_mi_ptr_ptr
)
{
  if (rx_mi_ptr_ptr != NULL && *rx_mi_ptr_ptr != NULL)
  {
    RX_META_INFO_DEBUG( "RX_META FREE ptr 0x%p", *rx_mi_ptr_ptr, 0, 0 );
    PS_MEM_FREE(*rx_mi_ptr_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_RX_META_INFO_DUP()

DESCRIPTION
  This macro duplicates the passed PS mem buffer that is used for the RX meta
  info.

PARAMETERS
  rx_mi_ptr_ptr : pointer to a ps_rx_meta_info_type pointer.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  Allocates new PS mem buffers for rx meta info and packet info if present
===========================================================================*/
#define PS_RX_META_INFO_DUP ps_rx_meta_info_dup
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_rx_meta_info_dup
(
  ps_rx_meta_info_type * rx_mi_ptr,
  ps_rx_meta_info_type ** dup_ptr_ptr
)
{
  if (rx_mi_ptr != NULL && dup_ptr_ptr != NULL)
  {
    RX_META_INFO_DEBUG( "RX_META DUP ptr 0x%p", rx_mi_ptr, 0, 0 );
    *dup_ptr_ptr = (ps_rx_meta_info_type *) ps_mem_dup(rx_mi_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_RX_META_GET_PKT_INFO()

DESCRIPTION
  This macro returns the pkt info stored inside RX meta info.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_GET_PKT_INFO(rx_mi_ptr)  (rx_mi_ptr)->pkt_info

/*===========================================================================
MACRO PS_RX_META_SET_PKT_INFO()

DESCRIPTION
  This macro copies the pkt info to the meta info passed.

PARAMETERS
  rx_mi_ptr   : pointer to a ps_rx_meta_info_type.
  mi_pkt_info : packet info

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_SET_PKT_INFO(rx_mi_ptr, mi_pkt_info)           \
  memcpy(&(rx_mi_ptr)->pkt_info, &(mi_pkt_info), sizeof(mi_pkt_info));

/*===========================================================================
MACRO PS_RX_META_IS_PKT_INFO_VALID()

DESCRIPTION
  This macro returns the pkt info validity

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  RX meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_IS_PKT_INFO_VALID(rx_mi_ptr)                    \
  (rx_mi_ptr)->pkt_info.is_pkt_info_valid


/*===========================================================================
MACRO PS_RX_META_INVALIDATE_PKT_INFO()

DESCRIPTION
  This macro invalidates the pkt info.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_INVALIDATE_PKT_INFO(rx_mi_ptr)                    \
  (rx_mi_ptr)->pkt_info.is_pkt_info_valid = FALSE

/*===========================================================================
MACRO PS_RX_META_IS_FILTER_MASK_SET()

DESCRIPTION
  This macro checks whether the filter mask associated with a particular
  client id is set

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_IS_FILTER_MASK_SET(rx_mi_ptr, client_id)         \
  ((rx_mi_ptr)->fi_mask & ((uint32) 1 << (uint32)(client_id)))

/*===========================================================================
MACRO PS_RX_META_GET_FILTER_MASK()

DESCRIPTION
  This macro gets the entire filter mask associated with the meta info.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.

DEPENDENCIES
  rx_mi_ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_GET_FILTER_MASK(rx_mi_ptr) (rx_mi_ptr)->fi_mask

/*===========================================================================
MACRO PS_RX_META_GET_FILTER_RESULT()

DESCRIPTION
  This macro returns the filter result associated with a particular client id
  The result will be NULL if the particular filter has not been executed or
  failed the match on the packet.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_GET_FILTER_RESULT(rx_mi_ptr, client_id)              \
  (((rx_mi_ptr)->fi_mask & ((uint32) 1 << (client_id)))                     \
     ? (rx_mi_ptr)->fi_result[client_id]                                    \
     : PS_IFACE_IPFLTR_NOMATCH)

/*===========================================================================
MACRO PS_RX_META_SET_FILTER_RESULT()

DESCRIPTION
  This macro sets the filter result associated with a particular client id.
  The corresponding filter mask bit is also set to indicate there is no need
  to execute filters for the client id since a resulting fi_result is already
  available.

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type
  client_id : IP filter client id
  fi_result : Filter result  to set (of type ps_iface_ipfltr_result_type)

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_SET_FILTER_RESULT(rx_mi_ptr, client_id, in_fi_result)       \
  (rx_mi_ptr)->fi_result[(client_id)] =                                     \
    (ps_iface_ipfltr_result_type)(in_fi_result);                         \
  (rx_mi_ptr)->fi_mask |= ((uint32) 1 << (uint32)(client_id));

/*===========================================================================
MACRO PS_RX_META_RESET_FILTER_RESULT()

DESCRIPTION
  This macro resets the filter result associated with a particular client id.
  The corresponding filter mask bit is also reset

PARAMETERS
  rx_mi_ptr   pointer to a ps_rx_meta_info_type
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_RESET_FILTER_RESULT(rx_mi_ptr, client_id)                   \
  (rx_mi_ptr)->fi_result[(client_id)] = PS_IFACE_IPFLTR_NOMATCH;            \
  (rx_mi_ptr)->fi_mask &= ~((uint32) 1 << (client_id));

/*===========================================================================
MACRO PS_RX_META_GET_IPSEC_INFO()

DESCRIPTION
  This macro returns ipsec information associated with a given RX meta info

PARAMETERS
  rx_mi_ptr : pointer to a ps_rx_meta_info_type.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_GET_IPSEC_INFO(rx_mi_ptr)  (rx_mi_ptr)->ipsec_info

/*===========================================================================
MACRO PS_RX_META_GET_ROUTING_CACHE()

DESCRIPTION
  This macro returns the routing cache associated with given RX meta info

PARAMETERS
  rx_mi_ptr : pointer to a ps_tx_meta_info_type.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_GET_ROUTING_CACHE(rx_mi_ptr) (rx_mi_ptr)->routing_cache

/*===========================================================================
MACRO PS_META_SET_ROUTING_CACHE()

DESCRIPTION
  This macro sets routing for a given RX meta info

PARAMETERS
  rx_mi_ptr        : pointer to a ps_rx_meta_info_type.
  mi_routing_cache : subset id to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RX_META_SET_ROUTING_CACHE(rx_mi_ptr, mi_routing_cache)             \
  (rx_mi_ptr)->routing_cache = (mi_routing_cache);

#endif /* PS_RX_META_INFO_H */
