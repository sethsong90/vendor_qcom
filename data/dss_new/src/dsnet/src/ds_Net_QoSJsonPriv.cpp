/*===========================================================================
  @file ds_Net_QoSJsonPriv.cpp

  This file provides implementation of the QoSJson class for the 
  modem, when there is no use of Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSJsonPriv.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-08-11 en  Created module.

===========================================================================*/
/*===========================================================================

  INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "ds_Net_QoSJson.h"
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

QoSJson::QoSJson
(
  int32     ifaceHandle
)
 : mHandle(ifaceHandle),
   refCnt (1)
{
  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x", this, ifaceHandle, 0);

} /* QoSJson() */

void QoSJson::Destructor
(
  void
)
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* QoSJson::Destructor() */

QoSJson::~QoSJson()
throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* QoSJSon::~QoSJson() */


ds::ErrorType QoSJson::GetGrantedFlowSpec 
(
  NetPlatform::FlowIoctlEnumType  ioctlKind,
  char* rxFlow, 
  int rxFlowLen, 
  int* rxFlowLenReq, 
  char* txFlow, 
  int txFlowLen, 
  int* txFlowLenReq
)
{
  return AEE_EUNSUPPORTED;
}/* GetGrantedFlowSpec() */

