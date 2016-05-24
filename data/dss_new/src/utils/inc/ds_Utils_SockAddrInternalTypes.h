#ifndef DS_SOCK_ADDRTYPESHELPERS_H
#define DS_SOCK_ADDRTYPESHELPERS_H
/*===========================================================================
  @file ds_Utils_SockAddrInternalTypes.h

  This file defines structures and helper routines/macros for use with Address
  Related definitions in QCM API Socket IDL implementations.

  *IMPORTANT* - Please use this header and not DS_AddrTypesHelpers for implementations inside dssock/dsnet/utils etc

  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ds_Utils_SockAddrInternalTypes.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

INCLUDE FILES FOR MODULE

===========================================================================*/
#include "ds_Addr_Def.h"

/*===========================================================================

PUBLIC DATA DECLARATIONS

===========================================================================*/

namespace ds
{
  typedef struct SockAddrINInternalType SockAddrINInternalType;            /* Internet style IPv4 socket address in Network Byte order */
  struct SockAddrINInternalType
  {
    AddrFamilyType   family;         /* Address family - AF_INET */
    unsigned short   port;           /* transport layer port number */
    uint32           addr;           /* IPv4 address */
  };
 
  typedef struct SockAddrIN6InternalType SockAddrIN6InternalType;            /* Internet style IPv6 socket address in Network Byte order */
  struct SockAddrIN6InternalType
  {
    AddrFamilyType   family;         /* Address family - AF_INET6 */
    unsigned short   port;           /* transport layer port number */
    uint32           flowInfo;       /* IPv6 flow information */
    INAddr6Type      addr;           /* IPv6 address */
    uint32           scopeId;        /* Set of interface for a scope */
  };

  union SockAddrInternalType
  {
    SockAddrINInternalType v4;
    SockAddrIN6InternalType v6;
    uint64 alignmentField;    // members of SockAddrInternalType are used as input to PS functions that assume alignment of 8 bytes
  };
} /* namespace ds */
#endif /* DS_SOCK_ADDRTYPESHELPERS_H */
