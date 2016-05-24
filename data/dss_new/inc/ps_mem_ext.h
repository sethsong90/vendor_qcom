#ifndef PS_MEM_EXT_H
#define PS_MEM_EXT_H
/*===========================================================================

   P R O T O T O C O L   S E R V I C E S   D Y N A M I C   M E M O R Y

                     E X  T E R N A L   H E A D E R   F I L E

GENERAL DESCRIPTION
  This module implements a simple dynamic memory management mechanism for
  the protocol services.

EXTERNALIZED FUNCTIONS
  ps_ext_mem_get_buf()   - Allocates a memory buffer for the specified poolid
  ps_mem_free()          - Free the allocated memory, like free.
  ps_mem_dup()           - Dup the allocated memory. 
 
Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_mem_ext.h#1 $
  $DateTime: 2007/05/18 18:56:57

when        who    what, where, why
--------    ---    ---------------------------------------------------------- 
08/26/10    pp     Added PS_EXT_MEM_TX_META_INFO_TYPE pool for external 
                   clients.
06/09/10    pp     Added two EXT pools, cleaned up comments in this header.
08/31/09   mga     Merged from eHRPD branch
07/23/09    mt     New pool for DNS session-specific server buffers. 
02/19/09    am     Added new pool ID for DCC task cmd buffers.
12/14/08    pp     Created module as part of Common Modem Interface:
                   Public/Private API split.
3/17/09     en     API changes for better BCL memory management.
===========================================================================*/

#include "comdef.h"

/*===========================================================================

                         PUBLIC DEFINITIONS AND DATA

===========================================================================*/

/*---------------------------------------------------------------------------
  The following enum is used to identify the "external" poolid associated 
  with different external memory buffers.
  This enum is primarily required by modules outside CommonData as well as 
  some internal modules. This enum is also used by DATA Modules running on
  various HLOSs.
  Every external memory pool have an internal mapping.

  Current supported external memory pools are:
         External Enum            mapping    Internal Enum value
  - PS_EXT_MEM_META_INFO_TYPE_EX:   =>    PS_MEM_META_INFO_TYPE_EX
  - PS_EXT_MEM_RT_META_INFO_TYPE:   =>    PS_MEM_RT_META_INFO_TYPE
  - PS_EXT_MEM_PKT_META_INFO_TYPE:  =>    PS_MEM_PKT_META_INFO_TYPE
  - PS_EXT_MEM_TX_META_INFO_TYPE:   =>    PS_MEM_TX_META_INFO_TYPE

  Please refer to ps_mem_pool_enum_type [defined in ps_mem.h] for all internal
  pools.
---------------------------------------------------------------------------*/
#define PS_EXT_MEM_POOLS
typedef enum
{
  PS_EXT_MEM_DUMMY_POOL_TYPE              = 0, /* A dummy pool */
  PS_EXT_MEM_META_INFO_TYPE_EX            = 1,
  PS_EXT_MEM_RT_META_INFO_TYPE            = 2,
  PS_EXT_MEM_PKT_META_INFO_TYPE           = 3,
  PS_EXT_MEM_TX_META_INFO_TYPE            = 4,
  PS_EXT_MEM_MAX_POOLS
} ps_ext_mem_pool_enum_type;

/*===========================================================================
MACRO PS_MEM_FREE (buf)

DESCRIPTION
  This function is used for deallocating the memory.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_MEM_FREE(buf)                                                    \
{                                                                           \
  ps_mem_free((void *) buf);                                            \
  buf = NULL;                                                               \
}                              

#ifdef __cplusplus
extern "C" {
#endif
/*===========================================================================

                           PUBLIC FUNCTIONS

===========================================================================*/

/*===========================================================================
FUNCTION     PS_EXT_MEM_GET_BUF()

DESCRIPTION  This is the external API to get buffer for External Memory Pools.
             Current external memory pools are:
             - PS_EXT_MEM_META_INFO_TYPE_EX
             - PS_EXT_MEM_RT_META_INFO_TYPE
             - PS_EXT_MEM_PKT_META_INFO_TYPE  
             - PS_EXT_MEM_TX_META_INFO_TYPE  

RETURN VALUE If successful, a pointer to the allocated memory block is
             returned. Otherwise, a NULL pointer is returned.

DEPENDENCIES None

NOTES: The regular APIs should be used to reclaim/dup the buffers.
       No speacial APIs exists for free/dup calls - use ps_mem_free/dup.
===========================================================================*/
void *ps_ext_mem_get_buf
(
  ps_ext_mem_pool_enum_type poolid
);

/*===========================================================================
FUNCTION     PS_MEM_FREE()

DESCRIPTION  This function is used for deallocating the memory. It also
             checks for the possible memory corruptions.

RETURN VALUE None

DEPENDENCIES None
===========================================================================*/
void ps_mem_free
(
  void *buf
);

/*===========================================================================
FUNCTION     PS_MEM_DUP()

DESCRIPTION  This function is used to DUP a PS memory item.

RETURN VALUE None

DEPENDENCIES None
===========================================================================*/
void* ps_mem_dup
(
  void *buf
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PS_MEM_H */
