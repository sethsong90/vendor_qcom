/*===========================================================================
  @file ds_Net_QoSDefaultJson.cpp

  This file provides implementation of the QoSDefaultJson class using Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSDefaultJson.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-08-02 en  Created module.

===========================================================================*/
/*===========================================================================

  INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "ds_Net_QoSDefaultJson.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_Platform.h"
#include "ds_Net_QoSJson2PS.h"
#include "ds_Net_QoSSecondariesOutput.h"
#include "ds_Net_QoSSecondary.h"
#include "AEEIPrivSet.h"
#include "ds_Net_Privileges_Def.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

/*===========================================================================

PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/

QoSDefaultJson::QoSDefaultJson
(
  int32     ifaceHandle,
  IPrivSet * pPrivSet
)
 : mHandle(ifaceHandle),
   mpPrivSet(pPrivSet),
   refCnt (1)
{
  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x", this, ifaceHandle, 0);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

} /* QoSManagerJson() */


void QoSDefaultJson::Destructor
(
  void 
)
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);

} /* QoSManagerJson::Destructor() */

QoSDefaultJson::~QoSDefaultJson()
throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPrivSet = NULL;

} /* QoSManagerJson::~QoSManagerJson() */


ds::ErrorType QoSDefaultJson::Modify 
(
   const char* requestedSpec,
   QoSModifyMaskType modifyMask, 
   char* errSpec, 
   int errSpecLen, 
   int* errSpecLenReq
)
{
  int32                          result;
  PrimaryQoSModifyType           modifyInfo;
  PrimaryQoSModifySpecType       modifySpec;
  QoSJSon2PS                     localQoSJson2PS;
  NetPlatform::PSQoSSpecType*    pPSQoSFlowSpecArray = NULL;
  int                            piSpecArrayLen;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                (AEEPRIVID *)&AEEPRIVID_PNetSystem,
                                sizeof(AEEPRIVID_PNetSystem)
                                )
      ||
      AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                (AEEPRIVID *)&AEEPRIVID_PrimaryQoSSession,
                                sizeof(AEEPRIVID_PrimaryQoSSession)
                                )
     )
  {
    LOG_MSG_ERROR( "No privilege for Modify operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    Create arguments for PRIMARY_QOS_MODIFY ioctl.
  -------------------------------------------------------------------------*/
  result = localQoSJson2PS.ConvertJSon2PSQoSSpec(requestedSpec, &pPSQoSFlowSpecArray, &piSpecArrayLen);  
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  memset (&modifyInfo, 0, sizeof (modifyInfo));
  
  /*-------------------------------------------------------------------------
    Taking the first spec in the array returned by the conversion
    from Json string
  -------------------------------------------------------------------------*/
  modifySpec.field_mask = modifyMask;
  (void) memcpy (&(modifySpec.rx_flow_template), 
                 &(pPSQoSFlowSpecArray[0].rx.flow_template),
                 sizeof (ip_flow_spec_type));

  (void) memcpy (&(modifySpec.tx_flow_template), 
                 &(pPSQoSFlowSpecArray[0].tx.flow_template),
                 sizeof (ip_flow_spec_type));

  modifyInfo.primary_qos_spec_ptr = &modifySpec;  

  /*-----------------------------------------------------------------------
    Call the flow IOCTL to modify the primary QoS flow.
  -----------------------------------------------------------------------*/
  result = FlowIoctl  (mHandle,
                       FLOW_IOCTL_PRIMARY_QOS_MODIFY,
                       static_cast <void *> (&modifyInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  Conversion::CleanupPSQoSRequestSpec(&pPSQoSFlowSpecArray[0]);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  if(NULL != pPSQoSFlowSpecArray)
  {
    Conversion::CleanupPSQoSRequestSpec(&pPSQoSFlowSpecArray[0]);
  }
  return result;
} /* Modify() */