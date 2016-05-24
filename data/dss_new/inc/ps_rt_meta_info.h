#ifndef PS_RT_META_INFO_H
#define PS_RT_META_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ R T _ M E T A _ I N F O . H

GENERAL DESCRIPTION
  This is a header file that contains the definition of the ROUTING META
  INFORMATION that is passed down from the sockets layer all the way down
  to the interface.

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
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_rt_meta_info.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/26/10    pp     Added PS_EXT_RT_META_INFO_GET API.
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
10/08/08    pp     Metainfo fixes.
12/28/07    pp     Created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "msg.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#include "ps_pkt_info.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "ps_mem_ext.h"
#include "ps_iface_ipfltr.h"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            DEFINES AND TYPEDEFS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
- To aid in internal debugging following MACRO is introduced.
  - Current Level: MSG LOW
---------------------------------------------------------------------------*/
#define RT_META_INFO_DEBUG( sf, a, b, c )                                \
  /*lint -e{1534} */  MSG_LOW( sf, a, b, c )

/*---------------------------------------------------------------------------
TYPEDEF IP_ADDR_SCOPE_ENUM_TYPE

DESCRIPTION
  An enum that defines the scope of a given IP address.

---------------------------------------------------------------------------*/
typedef enum
{
  IP_ADDR_UNICAST      = 0,
  IP_ADDR_MULTICAST    = 1,
  IP_ADDR_BROADCAST    = 2,
  IP_ADDR_INVALID_SCOPE
} ip_addr_scope_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF ps_rt_meta_info_type

DESCRIPTION
  This is used to pass ROUTING META information through the stack.
  It moves along with the dsm item carrying the actual IP packet.

  This meta info is used in TX [Transmit] path.

  Fields are:
    ipsec_info     : IPSEC related info
    pkt_info       : IP pkt info containing header and misc info
    fi_mask        : indicates if a particular filter client
                     is already executed
    fi_result      : filtering result for each output client
    subset_id      : unique ID for each filter set
    next_hop_addr  : IP address of the next hop
    ip_addr_scope  : Unicast/multicast/bcast scope
    routing_cache  : cached routing result [iface pointer]
---------------------------------------------------------------------------*/
typedef struct
{
  ps_ipsec_info_type                ipsec_info;
  ip_pkt_info_type                  pkt_info;
  uint32                            fi_mask;
#define IP_FLTR_CLIENT_OUTPUT_MAX ((uint32)IP_FLTR_CLIENT_MAX - (uint32)IP_FLTR_CLIENT_INPUT_MAX)
  ps_iface_ipfltr_result_type       fi_result[IP_FLTR_CLIENT_OUTPUT_MAX];
  ps_iface_ipfltr_subset_id_type    subset_id;
  ps_ip_addr_type                   next_hop_addr;
  ip_addr_scope_enum_type           ip_addr_scope;
  void                            * routing_cache;
} ps_rt_meta_info_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            ROUTING META INFO MACROS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
MACRO PS_RT_META_INFO_FREE()

DESCRIPTION
  This macro frees the PS mem buffer that is used for the ROUTING meta info.

PARAMETERS
  rt_mi_ptr_ptr : pointer to a ps_rt_meta_info_type pointer.

RETURN VALUE
  *rt_mi_ptr_ptr will be set to NULL.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_INFO_FREE ps_rt_meta_info_free
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_rt_meta_info_free
(
  ps_rt_meta_info_type ** rt_mi_ptr_ptr
)
{
  if(rt_mi_ptr_ptr != NULL && *rt_mi_ptr_ptr != NULL)
  {
    RT_META_INFO_DEBUG( "RT_META_INFO_FREE ptr 0x%p", *rt_mi_ptr_ptr, 0, 0);
    PS_MEM_FREE(*rt_mi_ptr_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_RT_META_INFO_DUP()

DESCRIPTION
  This macro duplicates the passed PS mem buffer that is used for the ROUTING meta
  info.

PARAMETERS
  rt_mi_ptr_ptr : pointer to a ps_rt_meta_info_type pointer.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  Allocates new PS mem buffers for ROUTING meta info.
===========================================================================*/
#define PS_RT_META_INFO_DUP ps_rt_meta_info_dup
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_rt_meta_info_dup
(
  ps_rt_meta_info_type * rt_mi_ptr,
  ps_rt_meta_info_type ** dup_ptr_ptr
)
{
  if(rt_mi_ptr != NULL && dup_ptr_ptr != NULL)
  {
    RT_META_INFO_DEBUG( "RT_META_INFO_DUP ptr 0x%p", rt_mi_ptr, 0, 0 );
    *dup_ptr_ptr = (ps_rt_meta_info_type *) ps_mem_dup(rt_mi_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_RT_META_INFO_IS_NULL()

DESCRIPTION
  This macro checks for NULL

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type.

RETURN VALUE
  TRUE / FALSE

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
#define PS_RT_META_INFO_IS_NULL(rt_mi_ptr) ((rt_mi_ptr) == NULL)

/*===========================================================================
MACRO PS_RT_META_GET_PKT_INFO()

DESCRIPTION
  This macro returns the pointer to the pkt info stored inside ROUTING meta info.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_PKT_INFO(rt_mi_ptr)  (rt_mi_ptr)->pkt_info

/*===========================================================================
MACRO PS_RT_META_SET_PKT_INFO()

DESCRIPTION
  This macro returns the pointer to the pkt info stored inside ROUTING meta info.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_PKT_INFO(rt_mi_ptr,mi_pkt_info)                    \
  memcpy(&((rt_mi_ptr)->pkt_info), &(mi_pkt_info), sizeof(mi_pkt_info))

/*===========================================================================
MACRO PS_RT_META_IS_PKT_INFO_VALID()

DESCRIPTION
  This macro returns the pointer to the pkt info stored inside ROUTING meta info.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_IS_PKT_INFO_VALID(rt_mi_ptr)                    \
  (rt_mi_ptr)->pkt_info.is_pkt_info_valid
  

/*===========================================================================
MACRO PS_RT_META_INVALIDATE_PKT_INFO()

DESCRIPTION
  This macro returns the pointer to the pkt info stored inside ROUTING meta info.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_INVALIDATE_PKT_INFO(rt_mi_ptr)                    \
  (rt_mi_ptr)->pkt_info.is_pkt_info_valid = FALSE

/*===========================================================================
MACRO GET_INDEX_FROM_CLIENT_ID()

DESCRIPTION
  This macro gets the fi_result array index from client id passed, used to access
  fi_result array, also used to set the bit in filter mask..

PARAMETERS
  client_id : IP output filter client id

DEPENDENCIES
  client_id must be valid and it should be output client id.

SIDE EFFECTS
  None

NOTE:
  Output client ids are defined between IP_FLTRR_CLIENT_INPUT_MAX and
  IP_FLTR_CLIENT_MAX in ps_iface_ipfltr_client_id_enum_type.
  To get the correct index from the client id following rule is applied:
  (index) = (client_id) - IP_FLTR_CLIENT_INPUT_MAX
===========================================================================*/
#define GET_INDEX_FROM_CLIENT_ID(client_id)                         \
  ((uint8)(client_id) - (uint8)IP_FLTR_CLIENT_INPUT_MAX)

/*===========================================================================
MACRO PS_RT_META_IS_FILTER_MASK_SET()

DESCRIPTION
  This macro checks whether the filter mask associated with a particular
  client id is set

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_IS_FILTER_MASK_SET(rt_mi_ptr, client_id)                    \
  ((rt_mi_ptr)->fi_mask & ((uint32) 1 << (GET_INDEX_FROM_CLIENT_ID(client_id))))

/*===========================================================================
MACRO PS_RT_META_GET_FILTER_MASK()

DESCRIPTION
  This macro checks gets the entire filter mask associated with the meta info.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_FILTER_MASK(rt_mi_ptr) (rt_mi_ptr)->fi_mask

/*===========================================================================
MACRO PS_RT_META_GET_FILTER_RESULT()

DESCRIPTION
  This macro returns the filter result associated with a particular client id
  The result will be NULL if the particular filter has not been executed or
  failed the match on the packet.

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_FILTER_RESULT(rt_mi_ptr, client_id)            \
  (((rt_mi_ptr)->fi_mask & ((uint32) 1 << (GET_INDEX_FROM_CLIENT_ID(client_id)))) \
     ? (rt_mi_ptr)->fi_result[(GET_INDEX_FROM_CLIENT_ID(client_id))]  \
     : PS_IFACE_IPFLTR_NOMATCH)

/*===========================================================================
MACRO PS_RT_META_SET_FILTER_RESULT()

DESCRIPTION
  This macro sets the filter result associated with a particular client id.
  The corresponding filter mask bit is also set to indicate there is no need
  to execute filters for the client id since a resulting fi_result is already
  available.

PARAMETERS
  rt_mi_ptr    : pointer to a ps_rt_meta_info_type pointer
  client_id    : IP filter client id
  in_fi_result : Filter result  to set (of type ps_iface_ipfltr_result_type)

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_FILTER_RESULT(rt_mi_ptr, client_id, in_fi_result) \
  (rt_mi_ptr)->fi_result[(GET_INDEX_FROM_CLIENT_ID(client_id))] =    \
    (ps_iface_ipfltr_result_type)(in_fi_result);                         \
  (rt_mi_ptr)->fi_mask |= ((uint32) 1 << (GET_INDEX_FROM_CLIENT_ID(client_id)));

/*===========================================================================
MACRO PS_RT_META_RESET_FILTER_RESULT()

DESCRIPTION
  This macro resets the filter result associated with a particular client id.
  The corresponding filter mask bit is also reset

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer
  client_id : IP filter client id

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_RESET_FILTER_RESULT(rt_mi_ptr, client_id)       \
  (rt_mi_ptr)->fi_result[(GET_INDEX_FROM_CLIENT_ID(client_id))] =   \
                                               PS_IFACE_IPFLTR_NOMATCH;   \
  (rt_mi_ptr)->fi_mask &= ~((uint32) 1 << (GET_INDEX_FROM_CLIENT_ID(client_id)));

/*===========================================================================
MACRO PS_RT_META_GET_SUBSET_ID()

DESCRIPTION
  This macro returns the subset id associated with given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_SUBSET_ID(rt_mi_ptr)  (rt_mi_ptr)->subset_id

/*===========================================================================
MACRO PS_RT_META_SET_SUBSET_ID()

DESCRIPTION
  This macro sets subset id for a given meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.
  subset_id : subset id to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_SUBSET_ID(rt_mi_ptr, mi_subset_id)                     \
  (rt_mi_ptr)->subset_id = (mi_subset_id);

/*===========================================================================
MACRO PS_RT_META_GET_IPSEC_INFO()

DESCRIPTION
  This macro returns ipsec information associated with a given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_IPSEC_INFO(rt_mi_ptr)  (rt_mi_ptr)->ipsec_info

/*===========================================================================
MACRO PS_RT_META_SET_IPSEC_INFO()

DESCRIPTION
  This macro assigns ipsec information to the given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_IPSEC_INFO(rt_mi_ptr, mi_ipsec_info)              \
  memcpy(&((rt_mi_ptr)->ipsec_info), &(mi_ipsec_info), sizeof(mi_ipsec_info))

/*===========================================================================
MACRO PS_RT_META_GET_ROUTING_CACHE()

DESCRIPTION
  This macro sets routing cache for a given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_ROUTING_CACHE(rt_mi_ptr)  (rt_mi_ptr)->routing_cache

/*===========================================================================
MACRO PS_RT_META_SET_ROUTING_CACHE()

DESCRIPTION
  This macro sets routing cache for a given meta info

PARAMETERS
  rt_mi_ptr     : pointer to a ps_rt_meta_info_type pointer.
  routing_cache : routing cache to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_ROUTING_CACHE(rt_mi_ptr, mi_routing_cache)    \
  (rt_mi_ptr)->routing_cache = (mi_routing_cache);

/*===========================================================================
MACRO PS_RT_META_GET_IP_ADDR_SCOPE()

DESCRIPTION
  This macro returns IP address scope associated with a given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_IP_ADDR_SCOPE(rt_mi_ptr) (rt_mi_ptr)->ip_addr_scope

/*===========================================================================
MACRO PS_RT_META_SET_IP_ADDR_SCOPE()

DESCRIPTION
  This macro sets IP address scope for a given ROUTING meta info

PARAMETERS
  rt_mi_ptr     : pointer to a ps_rt_meta_info_type pointer.
  ip_addr_scope : IP address scope to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_IP_ADDR_SCOPE(rt_mi_ptr, mi_ip_addr_scope)    \
  (rt_mi_ptr)->ip_addr_scope = (mi_ip_addr_scope);

/*===========================================================================
MACRO PS_RT_META_GET_NEXT_HOP_ADDR()

DESCRIPTION
  This macro returns next hop address associated with a given ROUTING meta info

PARAMETERS
  rt_mi_ptr : pointer to a ps_rt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_GET_NEXT_HOP_ADDR(rt_mi_ptr)  (rt_mi_ptr)->next_hop_addr

/*===========================================================================
MACRO PS_RT_META_SET_NEXT_HOP_ADDR()

DESCRIPTION
  This macro sets next hop address for a given ROUTING meta info

PARAMETERS
  rt_mi_ptr     : pointer to a ps_tx_meta_info_type pointer.
  next_hop_addr : next hop address to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_RT_META_SET_NEXT_HOP_ADDR(rt_mi_ptr, mi_next_hop_addr)       \
  (rt_mi_ptr)->next_hop_addr = (mi_next_hop_addr);

/*===========================================================================
MACRO PS_EXT_RT_META_INFO_GET()

DESCRIPTION
  This macro allocates a PS mem buffer that is used for the ROUTING meta info.
  - Should be used by External Clients ONLY.
  - Internal clients must use PS_RT_META_INFO_GET

PARAMETERS
  rt_mi_ptr : pointer that will have the ps_rt_meta_info_type memory assigned
              to it.

RETURN VALUE
  rt_mi_ptr will be NULL on failure, or point to the ps_rt_meta_info_type data

DEPENDENCIES
  None

SIDE EFFECTS
  Initializes the allocated meta info block to all 0s.
===========================================================================*/
#define PS_EXT_RT_META_INFO_GET(rt_mi_ptr)                                  \
  (rt_mi_ptr) = (ps_rt_meta_info_type *) ps_ext_mem_get_buf(PS_EXT_MEM_RT_META_INFO_TYPE); \
  if ((rt_mi_ptr) != NULL)                                                 \
  {                                                                        \
    memset((rt_mi_ptr), 0, sizeof(ps_rt_meta_info_type));                  \
  }

#endif /* PS_RT_META_INFO_H */
