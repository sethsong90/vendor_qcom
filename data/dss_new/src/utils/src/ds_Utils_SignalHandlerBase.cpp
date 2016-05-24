/**
   @file
   ds_Utils_SignalHandlerBase.cpp

   @details
   This file implements the ds::Utils::SignalHandlerBase class.

   @see
   ds_Utils_SignalHandlerBase.h

   Copyright (c) 2010 Qualcomm Technologies, Inc.
   All Rights Reserved.
   Qualcomm Technologies Confidential and Proprietary
*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalHandlerBase.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-01-11  mt Created the module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#include "ds_Utils_SignalHandlerBase.h"

#include "ds_Utils_DebugMsg.h"
#include "ps_mem_ext.h"

using namespace ds::Utils;

SignalHandlerBase::SignalHandlerBase() :
   refCnt(1), weakRefCnt(1)
{
   LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);
   signalHandler.Init(this);

} /* SignalHandlerBase::SignalHandlerBase() */

void SignalHandlerBase::Destructor() throw()
{
   LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);
   (void)signalHandler.Release();

} /* SignalHandlerBase::Destructor() */

SignalHandlerBase::~SignalHandlerBase() throw() {}

void SignalHandlerBase::operator delete
(
   void *  bufPtr
) throw()
{
   PS_MEM_FREE(bufPtr);

} /* SignalHandlerBase::operator delete() */
