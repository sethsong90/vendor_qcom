#ifndef PS_PKT_META_INFO_H
#define PS_PKT_META_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ P K T _ M E T A _ I N F O . H

GENERAL DESCRIPTION
  This is a header file that contains the definition of the per packet meta
  information that is passed down from the sockets layer all the way down to the
  interface.

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
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_pkt_meta_info.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/26/10    pp     Added PS_EXT_PKT_META_INFO_GET API.
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
12/28/07    pp     Created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#include "time_svc.h"
#include "qw.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "msg.h"

#include "ps_mem_ext.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                            DEFINES AND TYPEDEFS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
- To aid in internal debugging following MACRO is introduced.
  - Current Level: MSG LOW
---------------------------------------------------------------------------*/
#define PKT_META_INFO_DEBUG( sf, a, b, c )                               \
  /*lint -e{1534} */  MSG_LOW( sf, a, b, c )

/*---------------------------------------------------------------------------
TYPEDEF PS_PKT_META_INFO_TYPE

DESCRIPTION
  This is used to pass per packet meta information through the stack.
  It moves along with the dsm item carrying the actual IP packet.

  This is used in transmit path of the IP packet.

  Fields are:
    timestamp      : time when this packet is sent by the application
    tx_flags       : flags associated with each IP packet (defined in
                     dssocket.h)
    dos_ack_handle : socket which generated this packet
---------------------------------------------------------------------------*/
typedef struct
{
  time_type                         timestamp;
  uint32                            tx_flags;
  int32                             dos_ack_handle;
} ps_pkt_meta_info_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         PACKET META INFO MACROS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
MACRO PS_PKT_META_INFO_FREE()

DESCRIPTION
  This macro frees the PS mem buffer that is used for the PACKET meta info.

PARAMETERS
  pkt_mi_ptr_ptr : pointer to a ps_pkt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  pkt_mi_ptr will be set to NULL.
===========================================================================*/
#define PS_PKT_META_INFO_FREE ps_pkt_meta_info_free
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_pkt_meta_info_free
(
  ps_pkt_meta_info_type ** pkt_mi_ptr_ptr
)
{
  if (pkt_mi_ptr_ptr != NULL && *pkt_mi_ptr_ptr != NULL)
  {
    PKT_META_INFO_DEBUG( "PKT_META FREE ptr 0x%p", *pkt_mi_ptr_ptr, 0, 0 );
    PS_MEM_FREE(*pkt_mi_ptr_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_PKT_META_INFO_DUP()

DESCRIPTION
  This macro duplicates the passed PS mem buffer that is used for the PACKET
  meta info.

PARAMETERS
  pkt_mi_ptr : pointer to a ps_pkt_meta_info_type.
  dup_ptr_ptr: dup double pointer to a ps_pkt_meta_info_type.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
#define PS_PKT_META_INFO_DUP ps_pkt_meta_info_dup
#ifdef __cplusplus
extern "C" {
#endif
INLINE void ps_pkt_meta_info_dup
(
  ps_pkt_meta_info_type * pkt_mi_ptr,
  ps_pkt_meta_info_type ** dup_ptr_ptr
)
{
  if  (pkt_mi_ptr != NULL && dup_ptr_ptr != NULL)
  {
    PKT_META_INFO_DEBUG( "PKT_META DUP ptr 0x%p", pkt_mi_ptr, 0, 0 );
    *(dup_ptr_ptr) = (ps_pkt_meta_info_type *)ps_mem_dup(pkt_mi_ptr);
  }
}
#ifdef __cplusplus
}
#endif

/*===========================================================================
MACRO PS_PKT_META_INFO_IS_NULL()

DESCRIPTION
  This macro checks for NULL

PARAMETERS
  pkt_mi_ptr : pointer to a ps_pkt_meta_info_type.

RETURN VALUE
  TRUE / FALSE

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
#define PS_PKT_META_INFO_IS_NULL(pkt_mi_ptr) ((pkt_mi_ptr) == NULL)

/*===========================================================================
MACRO PS_PKT_META_GET_TX_FLAGS()

DESCRIPTION
  This macro returns Tx flags associated with a given PACKET meta info

PARAMETERS
  pkt_mi_ptr : pointer to a ps_pkt_meta_info_type.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_GET_TX_FLAGS(pkt_mi_ptr)  (pkt_mi_ptr)->tx_flags

/*===========================================================================
MACRO PS_PKT_META_SET_TX_FLAGS()

DESCRIPTION
  This macro sets Tx flags for a given PACKET meta info

PARAMETERS
  pkt_mi_ptr   : pointer to a ps_pkt_meta_info_type.
  tx_flags     : Tx flags to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_SET_TX_FLAGS(pkt_mi_ptr, mi_tx_flags)                       \
  (pkt_mi_ptr)->tx_flags = (mi_tx_flags);

/*===========================================================================
MACRO PS_PKT_META_GET_DOS_ACK_HANDLE()

DESCRIPTION
  This macro returns dos_ack_handle associated with a given PACKET meta info

PARAMETERS
  pkt_mi_ptr : pointer to a ps_pkt_meta_info_type.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_GET_DOS_ACK_HANDLE(pkt_mi_ptr)                  \
  (pkt_mi_ptr)->dos_ack_handle

/*===========================================================================
MACRO PS_PKT_META_SET_DOS_ACK_HANDLE()

DESCRIPTION
  This macro sets dos_ack_handle for a given PACKET meta info

PARAMETERS
  pkt_mi_ptr     : pointer to a ps_pkt_meta_info_type.
  dos_ack_handle : dos_ack_handle to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_SET_DOS_ACK_HANDLE(pkt_mi_ptr, mi_dos_ack_handle) \
  (pkt_mi_ptr)->dos_ack_handle = (mi_dos_ack_handle);

/*===========================================================================
MACRO PS_PKT_META_GET_TIMESTAMP()

DESCRIPTION
  This macro returns timestamp associated with a given PACKET meta info

PARAMETERS
  pkt_mi_ptr : pointer to a ps_pkt_meta_info_type pointer.

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_GET_TIMESTAMP(pkt_mi_ptr)  (pkt_mi_ptr)->timestamp

/*===========================================================================
MACRO PS_PKT_META_SET_TIMESTAMP()

DESCRIPTION
  This macro sets timestamp for a given PACKET meta info

PARAMETERS
  pkt_mi_ptr   : pointer to a ps_tx_meta_info_type pointer.
  mi_timestamp : timestamp to be set

DEPENDENCIES
  meta info ptr must be valid.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_PKT_META_SET_TIMESTAMP(pkt_mi_ptr, mi_timestamp)          \
  qw_equ( (pkt_mi_ptr)->timestamp, mi_timestamp);

/*===========================================================================
MACRO PS_EXT_PKT_META_INFO_GET()

DESCRIPTION
  This macro allocates a PS mem buffer that is used for the PACKET meta info.
  - Should be used by External Clients only.
  - Internal clients should use PS_PKT_META_INFO_GET

PARAMETERS
  pkt_mi_ptr : pointer that will have the ps_pkt_meta_info_type memory assigned to it.

RETURN VALUE
  pkt_mi_ptr will be NULL on failure, or point to the ps_pkt_meta_info_type data

DEPENDENCIES
  None

SIDE EFFECTS
  Initializes the allocated meta info block to all 0s.
===========================================================================*/
#define PS_EXT_PKT_META_INFO_GET(pkt_mi_ptr)                                   \
  (pkt_mi_ptr) = (ps_pkt_meta_info_type *)ps_ext_mem_get_buf(PS_EXT_MEM_PKT_META_INFO_TYPE); \
  if ((pkt_mi_ptr) != NULL)                                                    \
  {                                                                            \
    memset((pkt_mi_ptr), 0, sizeof(ps_pkt_meta_info_type));                    \
  }

#endif /* PS_PKT_META_INFO_H */
