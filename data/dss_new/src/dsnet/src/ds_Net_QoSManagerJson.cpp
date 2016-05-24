/*===========================================================================
  @file ds_Net_QoSManagerJson.cpp

  This file provides implementation of the QoSManagerJson class using Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSManagerJson.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-08-02 en  Created module.

===========================================================================*/
/*===========================================================================

  INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "ds_Net_QoSManagerJson.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_Platform.h"
#include "ds_Net_QoSJson2PS.h"
#include "ds_Net_QoSSecondariesOutput.h"
#include "ds_Net_QoSSecondary.h"
#include "AEEIPrivSet.h"
#include "ps_system_heap.h"

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

QoSManagerJson::QoSManagerJson
(
  int32     ifaceHandle,
  IPrivSet* pPrivSet,
  NetworkModeType networkMode
)
 : mHandle(ifaceHandle),
   mpPrivSet(pPrivSet), 
   mNetworkMode(networkMode),
   refCnt (1)
{
  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x", this, ifaceHandle, 0);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

} /* QoSManagerJson() */


void QoSManagerJson::Destructor
(
  void 
)
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);

} /* QoSManagerJson::Destructor() */

QoSManagerJson::~QoSManagerJson()
throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPrivSet = NULL;

} /* QoSManagerJson::~QoSManagerJson() */

ds::ErrorType QoSManagerJson::Request
(
  const char* specs, 
  QoSRequestOpCodeType opCode, 
  IQoSSecondariesOutput** qosSessions, 
  char* errSpec, 
  int errSpecLen,   
  int* errSpecLenReq
)
{
  int32                             result;
  int32                             index;
  QoSRequestExType                  qosRequestExInfo;
  QoSSecondary**                    ppQoSSecondary = NULL;
  int                               specsLen;
  QoSSecondariesOutput*             pQoSSecondariesOutput;

  //TODO see if this variable is needed after the implementation of QoSJSon2PS
  QoSJSon2PS                        lQoSJson2PSConverter;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);
  
  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == specs ||
      NULL == qosSessions ||
      NULL == errSpec ||
      NULL == errSpecLenReq ||
      0    >= errSpecLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }
  
  /*-------------------------------------------------------------------------
    Construct QOS_REQUEST_EX_IOCTL arguments.
  -------------------------------------------------------------------------*/
  memset (&qosRequestExInfo, 0, sizeof (QoSRequestExType));

  result = lQoSJson2PSConverter.ConvertJSon2PSQoSSpec(specs, &qosRequestExInfo.qos_specs_ptr ,&specsLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }
  
  qosRequestExInfo.num_qos_specs = (uint8) specsLen;
  qosRequestExInfo.subset_id = 0;
  qosRequestExInfo.qos_control_flags = 0;

  if (opCode == QoSRequestOpCode::QDS_REQUEST)
  {
    qosRequestExInfo.opcode = PS_IFACE_IOCTL_QOS_REQUEST_OP;
  }
  else
  {
    qosRequestExInfo.opcode = PS_IFACE_IOCTL_QOS_CONFIGURE_OP;
  }


  /*-------------------------------------------------------------------------
    Allocate memory for flow_ptrs array. Here we are allocating memory
    for holding only the pointers to ps flows and not the entire
    flow structures.
  -------------------------------------------------------------------------*/
  qosRequestExInfo.flows_ptr =
    (ps_flow_type **)
    ps_system_heap_mem_alloc
    (
      sizeof (ps_flow_type *) * specsLen
    );
  if (NULL == qosRequestExInfo.flows_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Fill the filter and flow specs into the bundled IOCTL request arg.
  -------------------------------------------------------------------------*/
  for (index = 0; index < specsLen; index++) 
  {
    qosRequestExInfo.flows_ptr[index] = NULL;
  }

  /*-------------------------------------------------------------------------
    Issue a QOS_REQUEST_EX_IOCTL to request for a bundle of flows.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mHandle,
                       IFACE_IOCTL_QOS_REQUEST_EX,
                       static_cast <void *> (&qosRequestExInfo));

  /*-------------------------------------------------------------------------
    Get the err mask set in the QoS Request info argument.
  -------------------------------------------------------------------------*/
  
  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }
  
  result = lQoSJson2PSConverter.GenerateJSonErrFromPSSpec(qosRequestExInfo.qos_specs_ptr, specsLen, errSpec);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
  Allocate memory for secondary QoS objects array. Here we are allocating
  memory for only the array of pointers and not the QoSSecondary objects
  themselves.
  -------------------------------------------------------------------------*/
  pQoSSecondariesOutput = new QoSSecondariesOutput();

  if (NULL == qosSessions)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  ppQoSSecondary = 
    (QoSSecondary**)
    ps_system_heap_mem_alloc
    (
    sizeof (QoSSecondary *) * specsLen
    );
  if (NULL == ppQoSSecondary)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  memset (ppQoSSecondary, 0, specsLen * sizeof(QoSSecondary *));

  for (index = 0;
    index < qosRequestExInfo.num_qos_specs && index < specsLen;
    index++)
  {
    ppQoSSecondary[index] =
      new QoSSecondary (mHandle,
                        (int32) qosRequestExInfo.flows_ptr[index],
                        mpPrivSet,
                        mNetworkMode);
    if (NULL == ppQoSSecondary[index])
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }

    /*-----------------------------------------------------------------------
      Populate the out parameter with the IQoSSecondary interface.
    -----------------------------------------------------------------------*/
    result = pQoSSecondariesOutput->AddQoSSecondary(static_cast <IQoSSecondary *> (ppQoSSecondary[index]));
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }
  } /* for */
  
  *qosSessions = static_cast <IQoSSecondariesOutput *> (pQoSSecondariesOutput);

  // Release the reference to the QoSSecondaries. The QoSSecondariesOutput object
  // already holds a reference to the QoSSecondaries.
  if (NULL != ppQoSSecondary)
  {
     for (index = 0;
        index < qosRequestExInfo.num_qos_specs && index < specsLen;
        index++)
     {
        DS_UTILS_RELEASEIF (ppQoSSecondary[index]);
     }
  }
  
  result = AEE_SUCCESS;

  /*-------------------------------------------------------------------------
  ***FALL-THROUGH***
  -------------------------------------------------------------------------*/
bail:
  for (index = 0;
    index < qosRequestExInfo.num_qos_specs && index < specsLen;
    index++)
  {
    (void) CleanupPSQoSRequestSpec (&qosRequestExInfo.qos_specs_ptr[index]);
  }

  PS_SYSTEM_HEAP_MEM_FREE (qosRequestExInfo.flows_ptr);
  PS_SYSTEM_HEAP_MEM_FREE (qosRequestExInfo.qos_specs_ptr);
  PS_SYSTEM_HEAP_MEM_FREE (ppQoSSecondary);
  return result;
 
}/* Request() */