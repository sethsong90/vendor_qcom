#ifndef DS_SOCK_ROUTINGMANAGER_H
#define DS_SOCK_ROUTINGMANAGER_H
/*===========================================================================
  @file DS_Sock_RoutingManager.h

  This file declares a class which provides a set of helper methods useful
  in routing and filtering.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_RoutingManager.h#1 $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#include "DS_Utils_CSSupport.h"
#include "DS_Net_IPolicy.h"
#include "DS_Sock_Socket.h"
#include "ps_rt_meta_info.h"
#include "ps_iface_defs.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    class RoutingManager
    {                                                         /*lint !e578 */
      public:
        static DS::ErrorType RoutePacket
        (
          DS::Sock::Socket *      sockPtr,
          bool                    isSystemSocket,
          DS::Net::IPolicy *      policyPtr,
          ps_rt_meta_info_type *  newRtMetaInfoPtr
        );

        static void FltrClient
        (
          DS::Sock::Socket *                   sockPtr,
          ps_iface_ipfltr_client_id_enum_type  fltrClient,
          ps_rt_meta_info_type *               newRtMetaInfoPtr
        );
    };

  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_ROUTINGMANAGER_H */
