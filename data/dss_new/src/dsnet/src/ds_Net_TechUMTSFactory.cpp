/*===========================================================================
  FILE: TechUMTSFactory.cpp

  OVERVIEW: This file provides implementation of the TechUMTSFactory class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_TechUMTSFactory.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-10-23 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Net_Utils.h"
#include "ds_Net_TechUMTSFactory.h"
#include "ds_Net_Platform.h"
#include "ds_Net_MTPDReg.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_Policy.h"


using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;

TechUMTSFactory * TechUMTSFactory::_instance = NULL;

TechUMTSFactory::TechUMTSFactory
(
  void
)
throw()
: refCnt (0)
{

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

} /* TechUMTSFactory() */

TechUMTSFactory::~TechUMTSFactory
(
  void 
)
throw()
{

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* ~TechUMTSFactory() */

/*---------------------------------------------------------------------------
  Public Function Definitions
---------------------------------------------------------------------------*/
TechUMTSFactory * TechUMTSFactory::Instance
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  if (0 == _instance)
  {
    _instance = new TechUMTSFactory();
  }

  return _instance;

} /* Instance() */

/*-------------------------------------------------------------------------
  Functions inherited from ITechUMTS interface
-------------------------------------------------------------------------*/
ds::ErrorType TechUMTSFactory::RegMTPD
(
  IPolicy* argPolicy, 
  ISignal* signalObj, 
  IQI**    mtpdRegObj
)
{
  MTPDReg*            pMTPDReg = NULL;
  int32               ifaceHandle; 
  int                 result;
  MTRegCBType         mtRegInfo;
  ACLPolicyInfoType   aclPolicy;
  IQI*                tmpRegObj;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NULL == argPolicy || NULL == signalObj || NULL == mtpdRegObj)
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Perform a network lookup before issuing the IOCLTL.
  -------------------------------------------------------------------------*/
  result = IfaceLookUpByPolicy (argPolicy, &ifaceHandle);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Get the ACL policy for the given IPolicy.
  -------------------------------------------------------------------------*/
  result = NetPlatform::GenerateAclPolicy(argPolicy, &aclPolicy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  memset (&mtRegInfo, 0, sizeof(mtRegInfo));
  mtRegInfo.acl_pol_ptr = &aclPolicy;
  mtRegInfo.handle = 0;

  result = IfaceIoctl (ifaceHandle, 
                       IFACE_IOCTL_MT_REG_CB,
                       &mtRegInfo);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Create the MTPDReg object depending on the MT handle returned.
  -------------------------------------------------------------------------*/
  pMTPDReg = new MTPDReg(ifaceHandle, (int32)mtRegInfo.handle);
  if (NULL == pMTPDReg)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  *mtpdRegObj = reinterpret_cast <IQI *> (pMTPDReg);

  /*-------------------------------------------------------------------------
    Register for MT events on the object created.
  -------------------------------------------------------------------------*/
  
  tmpRegObj = pMTPDReg->GetRegObj();

  result = pMTPDReg->OnStateChange (signalObj, QDS_EV_MTPD, &tmpRegObj);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  pMTPDReg->SetRegObj(tmpRegObj);

  return AEE_SUCCESS;

bail:
  if (NULL != pMTPDReg)
  {
    (void) pMTPDReg->Release();
    pMTPDReg = NULL;
    *mtpdRegObj = NULL;
  }
  LOG_MSG_ERROR ("Err %d reg for MTPD", result, 0, 0);
  return result;

} /* RegMTPD() */
