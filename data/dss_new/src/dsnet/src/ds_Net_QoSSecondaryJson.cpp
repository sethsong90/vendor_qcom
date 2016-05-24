/*===========================================================================
  @file ds_Net_QoSSecondaryJson.cpp

  This file provides implementation of the QoSSecondaryJson class using Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSSecondaryJson.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-08-02 en  Created module.

===========================================================================*/
/*===========================================================================

  INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "ds_Net_QoSSecondaryJson.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_Platform.h"
#include "ds_Net_QoSJson2PS.h"
#include "ds_Net_QoSSecondariesOutput.h"
#include "ds_Net_QoSSecondary.h"
#include "AEEIPrivSet.h"

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

QoSSecondaryJson::QoSSecondaryJson
(
  int32     ifaceHandle
)
 : mHandle(ifaceHandle),
   refCnt (1)
{
  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x", this, ifaceHandle, 0);

} /* QoSManagerJson() */


void QoSSecondaryJson::Destructor
(
  void 
)
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* QoSManagerJson::Destructor() */


ds::ErrorType QoSSecondaryJson::Modify 
(
   const char* requestedSpec,
   QoSModifyMaskType modifyMask, 
   char* errSpec, 
   int errSpecLen, 
   int* errSpecLenReq
)
{
  QoSModifyType                     modifyInfo;
  int32                             result;
  PSQoSSpecType                     localQoSSpec;
  QoSJSon2PS                        localQoSJson2PS;
  int                               requestedSpecLen;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);


  if (NULL == requestedSpec)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create IOCTL argument for QOS_MODIFY
  -------------------------------------------------------------------------*/
  memset (&modifyInfo, 0, sizeof (QoSModifyType));
  modifyInfo.subset_id = 0;

  memset (&localQoSSpec, 0, sizeof (localQoSSpec));
  modifyInfo.qos_ptr = &localQoSSpec;

  /*-------------------------------------------------------------------------
    Create Spec according to the mask
  -------------------------------------------------------------------------*/

  /* All other masks are set inside the conversion function */
  result = localQoSJson2PS.ConvertJSon2PSQoSSpec(requestedSpec, &modifyInfo.qos_ptr, &requestedSpecLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If a RX_SPEC_DELETE mask is set then we should set QOS_MASK_RX_FLOW mask
    and IPFLOW_MASK_NONE should be set in the flow.
    No need to check the min_req_flow and
    aux flow specs here as this will checked by lower layers.
  -------------------------------------------------------------------------*/

  if (QoSModifyMask::QDS_RX_SPEC_DELETE & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MASK_RX_FLOW;
    if ( (uint32) IPFLOW_MASK_NONE != 
      modifyInfo.qos_ptr->rx.flow_template.req_flow.field_mask ) 
    {
      return QDS_EFAULT;
    }
    modifyInfo.qos_ptr->rx.flow_template.req_flow.field_mask = 
      (uint32) IPFLOW_MASK_NONE;
    modifyInfo.qos_ptr->rx.flow_template.num_aux_flows = 0;
  }

  if (QoSModifyMask::QDS_TX_SPEC_DELETE & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MASK_TX_FLOW;
    if ( (uint32) IPFLOW_MASK_NONE != 
      modifyInfo.qos_ptr->tx.flow_template.req_flow.field_mask ) 
    {
      return QDS_EFAULT;
    }
    modifyInfo.qos_ptr->tx.flow_template.num_aux_flows = 0;
  }

  /*-------------------------------------------------------------------------
    If the RX_FILTER is set then we should modify the RX filters, thus
    a QOS_MODIFY_MASK_RX_FLTR_MODIFY must be set in the spec
    that is passed to PS according to the semantics all the
    filters (new and old) should be provided.
  -------------------------------------------------------------------------*/
  if (QoSModifyMask::QDS_RX_FILTER & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MODIFY_MASK_RX_FLTR_MODIFY;
  }

  /*-------------------------------------------------------------------------
    Same for the case of TX
  -------------------------------------------------------------------------*/

  if (QoSModifyMask::QDS_TX_FILTER & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MODIFY_MASK_TX_FLTR_MODIFY;
  }

  /*-------------------------------------------------------------------------
    Call the flow IOCTL to modify the Qos session.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (mHandle,
                      FLOW_IOCTL_QOS_MODIFY,
                      static_cast <void *> (&modifyInfo));

  /*-------------------------------------------------------------------------
    Get the err mask set in the QoS Request info argument.
  -------------------------------------------------------------------------*/
  localQoSJson2PS.GenerateJSonErrFromPSSpec(modifyInfo.qos_ptr, requestedSpecLen, errSpec);

  if ((AEE_SUCCESS != result) && (AEE_EWOULDBLOCK != result))
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Cleanup the dynamic memories allocated for the Qos specs.
  -------------------------------------------------------------------------*/
  (void) CleanupPSQoSRequestSpec (modifyInfo.qos_ptr);
  return AEE_SUCCESS;

bail:
  (void) CleanupPSQoSRequestSpec (modifyInfo.qos_ptr);
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* Modify() */