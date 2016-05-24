/*===========================================================================
  @file ds_Net_QoSManagerJsonPriv.cpp

  This file provides implementation of the QoSManagerJson class for the modem
  when there is no use of Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSManagerJsonPriv.cpp#1 $
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
#include "AEEIPrivSet.h"
#include "ds_Net_Utils.h"

using namespace ds::Net;
using namespace ds::Error;

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
  return AEE_EUNSUPPORTED;
}


