/*===========================================================================

  FILE: ds_Net_NatManager.cpp



  OVERVIEW: This file provides implementation of the NatManager class.



  DEPENDENCIES: None



  Copyright (c) 2010 Qualcomm Technologies, Inc.

  All Rights Reserved.

  Qualcomm Technologies Confidential and Proprietary

===========================================================================*/



/*===========================================================================

  EDIT HISTORY FOR MODULE



  Please notice that the changes are listed in reverse chronological order.



  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NatManager.cpp#1 $

  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $



  when       who what, where, why

  ---------- --- ------------------------------------------------------------

  2010-10-04 dm  Created module.



===========================================================================*/



/*===========================================================================



                     INCLUDE FILES FOR MODULE



===========================================================================*/



#include "comdef.h"


#include "ds_Errors_Def.h"

#include "ds_Utils_DebugMsg.h"

#include "ds_Utils_Conversion.h"

#include "ds_Utils_CreateInstance.h"

#include "ds_Net_Utils.h"

#include "ds_Net_NatManager.h"

#include "ds_Net_Platform.h"

#include "ds_Net_Conversion.h"

#include "AEECCritSect.h"



using namespace ds::Error;

using namespace ds::Net;

using namespace ds::Net::Conversion;

using namespace NetPlatform;



/*---------------------------------------------------------------------------

  CONSTRUCTOR/DESTRUCTOR

---------------------------------------------------------------------------*/

NatManager::NatManager

(

  const IPolicy   *pIPolicy

)

: mIfaceHandle(0),

  mpNatHandle(0),

  refCnt(1)

{

  ds::ErrorType result;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/



  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,

                                              AEECLSID_CCritSect,

                                              NULL,

                                              (void **) &mpICritSect))

  {

    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);

    ASSERT (0);

  }





  mpIPolicy = const_cast<ds::Net::IPolicy *> (pIPolicy);

  if (NULL != mpIPolicy)

  {

    (void) mpIPolicy->AddRef();

  }



  result = NetPlatform::IfaceLookUpByPolicy (mpIPolicy,

                                             &mIfaceHandle);

  if (AEE_SUCCESS != result || 0 == mIfaceHandle)

  {

    LOG_MSG_INFO1 ("Route lookup failed for NAT iface", 0, 0, 0);

    mIfaceHandle = 0;

  }



  LOG_MSG_INFO1 ("Created NatSession obj 0x%p, if handle 0x%x",

                  this, mIfaceHandle, 0);



} /* NatManager::NatManager() */



NatManager::~NatManager

(

  void 

)

throw()

{

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/



  LOG_MSG_INFO1 ("Deleteing NatManager 0x%p if handle 0x%x", 

                  this, mIfaceHandle, 0);



  mIfaceHandle = 0;

  mpNatHandle  = NULL;



  DSNET_RELEASEIF(mpICritSect);                       /*lint !e1550 !e1551 */



  DSNET_RELEASEIF(mpIPolicy);                         /*lint !e1550 !e1551 */



} /* NatManager::~NatManager() */





ds::ErrorType NatManager::Enable

(

  void 

)

{

  ds::ErrorType result;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/



  mpNatHandle = NetPlatform::EnableNat(mpIPolicy, &result);

  if (NULL == mpNatHandle)

  {

    LOG_MSG_ERROR ("Error enabling NAT", 0, 0, 0);

    return result;

  }



  LOG_MSG_INFO2 ("Enabling NAT", 0, 0, 0);



  return AEE_SUCCESS;



} /* NatManager::Enable() */



ds::ErrorType NatManager::Disable

(

  void 

)

{

  ds::ErrorType result;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/



  result = NetPlatform::DisableNat(mpNatHandle);



  LOG_MSG_INFO2 ("Disabling NAT, handle 0x%x, result 0x%x", 

                  mpNatHandle, result, 0);



  return result;



} /* NatManager::Disable() */

ds::ErrorType NatManager::EnableRoamingAutoconnect
(
  void 
)
{
  ds::ErrorType result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Enabling NAT Roaming Autoconnect", 0, 0, 0);
  result = NetPlatform::EnableRoamingAutoconnect();

  return result;
} /* NatManager::EnableRoamingAutoconnect() */


ds::ErrorType NatManager::DisableRoamingAutoconnect
(
  void 
)
{
  ds::ErrorType result;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2 ("Disabling NAT Roaming Autoconnect", 0, 0, 0);
  result = NetPlatform::DisableRoamingAutoconnect();

  return result;
} /* NatManager::DisableRoamingAutoconnect() */





