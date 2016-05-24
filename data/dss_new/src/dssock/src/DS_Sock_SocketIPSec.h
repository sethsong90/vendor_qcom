#ifndef DS_SOCK_SOCKETIPSEC_H
#define DS_SOCK_SOCKETIPSEC_H
/*===========================================================================
  @file DS_Sock_SocketIPSec.h

  This file defines a set of helper routines for IPSec related functionality.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketIPSec.h#1 $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#include "DS_Sock_Socket.h"
#include "ps_rt_meta_info.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    class SocketIPSec
    {                                                         /*lint !e578 */
      public:
        static bool IsHandleInIfaceList
        (
          int32                   ifaceHandle,
          ps_rt_meta_info_type *  rtMetaInfoPtr
        );

        static bool IsIfaceListInReqState
        (
          ps_rt_meta_info_type *  rtMetaInfoPtr,
          uint32                  reqIfaceState
        );

        static void ClearIfaceList
        (
          ps_rt_meta_info_type *  rtMetaInfoPtr
        );

        static void TearDownIfaceList
        (
          ps_rt_meta_info_type *  rtMetaInfoPtr
        );

        static bool IsIfaceListFlowEnabled
        (
          ps_rt_meta_info_type *  rtMetaInfoPtr
        );

        static void FltrIPSecClient
        (
          DS::Sock::Socket *      sockPtr,
          ps_rt_meta_info_type *  newRtMetaInfoPtr,
          ps_rt_meta_info_type *  oldRtMetaInfoPtr
        );
    }; /* class SocketIPSec */

  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_SOCKETIPSEC_H */
