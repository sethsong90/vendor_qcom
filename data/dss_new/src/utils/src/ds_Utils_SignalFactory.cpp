/*==========================================================================*/
/*!
  @file
  ds_Utils_SignalFactory.cpp

  @brief
  This file provides implementation for ds::Utils::SignalFactory class.

  @see
  ds_Utils_SignalFactory.h

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalFactory.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-11-26 mt  Branched file from SignalFactory.cpp

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"

#include "ds_Utils_SignalFactory.h"
#include "ds_Utils_SignalCtl.h"

using namespace ds::Utils;
using namespace ds::Error;


/*===========================================================================

                       CONSTRUCTOR/DESTRUCTOR

===========================================================================*/
/**
  @brief      This is the constructor of the SignalFactory object.

              This is the constructor of the SignalFactory object.

  @param      None.

  @see        No external dependencies

  @return     None.
*/

SignalFactory::SignalFactory
(
  void
)
: Factory()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Creating SignalFactory obj 0x%p", this, 0, 0);

} /* SignalFactory() */


/**
  @brief      This is the destructor of the SignalFactory object.

              This is the destructor of the SignalFactory object.

  @param      None.

  @see        No external dependencies

  @return     None.
*/

SignalFactory::~SignalFactory
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Deleting SignalFactory obj 0x%p", this, 0, 0);

  /* By default the factory class's destructor would be called, which will
     pop each item and delete them */

} /* ~SignalFactory() */

/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/**
  @brief      This function creates a Signal object and adds it to the
              factory.

              This function creates a Signal object and adds it to the
              factory.

  @param      None.

  @see        No external dependencies

  @return     None.
*/
int CDECL SignalFactory::CreateSignal
(
  ISignalHandler * piHandler,
  uint32           uArgA,
  uint32           uArgB,
  ISignal**        ppiSig,
  ISignalCtl**     ppiSigCtl
)
throw()
{
  Signal*         signalPtr = NULL;
  SignalCtl*      signalCtlPtr = NULL;

#if 0
  int32           result;
#endif

/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("CreateSignal() called on signal fac 0x%p", this, 0, 0);

  if (NULL == piHandler || 0 == uArgA || NULL == ppiSig || NULL == ppiSigCtl)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  signalPtr = Signal::CreateInstance (piHandler, uArgA, uArgB);
  if (NULL == signalPtr)
  {
    LOG_MSG_ERROR ("Cant create signal", 0, 0, 0);
    goto bail;
  }

  signalCtlPtr = SignalCtl::CreateInstance (signalPtr);
  if (NULL == signalCtlPtr)
  {
    LOG_MSG_ERROR ("Cant create signalctl", 0, 0, 0);
    goto bail;
  }

  *ppiSig = static_cast <ISignal *> (signalPtr);
  *ppiSigCtl = static_cast <ISignalCtl *> (signalCtlPtr);
  return AEE_SUCCESS;

bail:
  DS_UTILS_RELEASEIF(signalPtr);
  DS_UTILS_RELEASEIF(signalCtlPtr);
  *ppiSig = NULL;
  *ppiSigCtl = NULL;
  return AEE_ENOMEMORY;

} /* CreateSignal() */


/*-------------------------------------------------------------------------
    CreateInstance method for SignalFactory.
  -------------------------------------------------------------------------*/
static SignalFactory *instance;

int SignalFactory::CreateInstance
(
  void*    env,
  AEECLSID clsid,
  void*    privset,
  void**   newObj
)

{
  (void) privset;
  (void) env;

  if (AEECLSID_CSignalFactory == clsid)
  {
    if (0 == instance)
    {
      instance = new SignalFactory();
      if (0 == instance)
      {
        LOG_MSG_ERROR ("Cant create SignalFactory singleton", 0, 0, 0);
        return AEE_ENOMEMORY;
      }
    }

    *newObj = (ISignalFactory *)instance;
    return AEE_SUCCESS;
  }
  return AEE_ECLASSNOTSUPPORT;
} /* CreateInstance() */

