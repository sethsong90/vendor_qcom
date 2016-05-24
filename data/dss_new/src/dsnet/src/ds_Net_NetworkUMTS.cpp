/*===========================================================================
  FILE: NetworkUMTS.cpp

  OVERVIEW: This file provides implementation of the NetworkUMTS class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkUMTS.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-04-06 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_NetworkUMTS.h"
#include "ds_Net_Platform.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
NetworkUMTS::NetworkUMTS
(
  Network* pParent
)
: mpParent (pParent),
  refCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

} /* NetworkUMTS() */

NetworkUMTS::~NetworkUMTS
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);
  mpParent = NULL;

} /* ~NetworkUMTS() */


/*---------------------------------------------------------------------------
  Functions inherited from INetworkUMTS
---------------------------------------------------------------------------*/
int NetworkUMTS::GetIMCNFlag
(
  UMTSIMCNFlagType* imcnFlag
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);
  return IfaceIoctlNonNullArg (mpParent->GetHandle(),
                               IFACE_IOCTL_UMTS_GET_IM_CN_FLAG,
                               (void *)imcnFlag);

} /* GetIMCNFlag() */

int NetworkUMTS::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
  case AEEIID_INetwork:
    *ppo = static_cast <INetwork *> (mpParent);
    (void) mpParent->AddRef();
    break;

  case AEEIID_INetworkUMTS:
    *ppo = static_cast <INetworkUMTS *>(this);
    (void) AddRef();
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    (void) AddRef();
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  return AEE_SUCCESS;
}
