/*=========================================================================*/
/*!
  @file
  BearerTech.cpp

  @brief
  This file provides implementation of the ds::Net::BearerTech class.

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_BearerTech.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-08-20 hm  Use crit sect interface instead of direct objects.
  2008-04-04 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_BearerTech.h"
#include "ds_Net_BearerTech.h"
#include "ds_Net_Utils.h"
#include "AEECCritSect.h"

using namespace ds::Net;
using namespace ds::Error;

/*===========================================================================

                     PRIVATE FUNCTION DEFINITIONS

===========================================================================*/
ds::ErrorType BearerTech::GetParams
(
  ParamsType paramName,
  void *     pOut,
  void *     pIn,
  int        len
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pOut)
  {
    LOG_MSG_INVALID_INPUT ("NULL out arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  memcpy (pOut, pIn, len);
  mpICritSect->Leave();

  LOG_MSG_INFO1 ("Obj 0x%p, param %d, val %d",
    this, paramName, *((int *)pOut));

  return AEE_SUCCESS;

} /* GetParams() */

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
BearerTech::BearerTech
(
  ds::Net::IfaceNameType                   networkType,
  uint32                                   cdmaMask,
  uint32                                   cdmaServiceOptionMask,
  uint32                                   umtsMask,
  BearerTechRateType*                pDataBearerRate
)
: mNetworkType (networkType),
  mCdmaTypeMask (cdmaMask),
  mCdmaServiceOptionMask (cdmaServiceOptionMask),
  mUmtsTypeMask (umtsMask),
  refCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }

  if (NULL != pDataBearerRate)
  {
    memcpy (&mBearerRate, pDataBearerRate, sizeof(BearerTechRateType));
  }


} /* BearerTech() */

BearerTech::~BearerTech
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */

} /* ~BearerTech() */

/*---------------------------------------------------------------------------
  Functions inherited from IBearerInfo
---------------------------------------------------------------------------*/
ds::ErrorType BearerTech::GetNetwork
(
  ds::Net::IfaceNameType* pNetworkType
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return GetParams (DS_NET_BEARER_TECH_PARAMS_NETWORK_TYPE,
                    pNetworkType,
                    &mNetworkType,
                    sizeof (ds::Net::IfaceNameType));

} /* GetNetwork() */


ds::ErrorType BearerTech::GetCDMATypeMask
(
  unsigned int* pCDMATypeMask
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return GetParams (DS_NET_BEARER_TECH_PARAMS_CDMA_TYPE_MASK,
                    pCDMATypeMask,
                    &mCdmaTypeMask,
                    sizeof (unsigned int));

} /* GetCDMATypeMask() */

ds::ErrorType BearerTech::GetCDMAServiceOptionsMask
(
  unsigned int* pCDMAServiceOptionsMask
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return GetParams (DS_NET_BEARER_TECH_PARAMS_CDMA_SERVICE_OPTION_MASK,
                    pCDMAServiceOptionsMask,
                    &mCdmaServiceOptionMask,
                    sizeof (unsigned int));

} /* GetCDMAServiceOptionsMask() */


ds::ErrorType BearerTech::GetUMTSTypeMask
(
  unsigned int* pUMTSTypeMask
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return GetParams (DS_NET_BEARER_TECH_PARAMS_UMTS_TYPE_MASK,
                    pUMTSTypeMask,
                    &mUmtsTypeMask,
                    sizeof (unsigned int));

} /* GetUMTSTypeMask() */

ds::ErrorType BearerTech::GetRate
(
  BearerTechRateType* pDataBearerRate
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   return GetParams (DS_NET_BEARER_TECH_PARAMS_BAERER_RATE,
                     pDataBearerRate,
                     &mBearerRate,
                     sizeof (BearerTechRateType));

} /* GetRate() */

ds::ErrorType BearerTech::GetBearerIsNull
(
  boolean*  BearerIsNull
)
{
  ds::Net::IfaceNameType  networkType;
  ds::ErrorType           dsErrno;
  unsigned int            typeMask;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (0 == BearerIsNull)
  {
    LOG_MSG_INVALID_INPUT ("NULL out arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  dsErrno = GetNetwork(&networkType);
  if (AEE_SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3 ("GetNetwork failed (%d)", dsErrno, 0, 0);
    return dsErrno;
  }

  switch (networkType)
  {
    case IfaceName::IFACE_CDMA_SN:
    case IfaceName::IFACE_CDMA_AN:
      dsErrno = GetCDMATypeMask(&typeMask);
      if (AEE_SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3 ("GetCDMATypeMask failed (%d)", dsErrno, 0, 0);
        return dsErrno;
      }

      *BearerIsNull = (0 != (typeMask ==
                             BearerTechCDMA::SUBTECH_NULL));
      break;

    case IfaceName::IFACE_UMTS:
      dsErrno = GetUMTSTypeMask(&typeMask);
      if (AEE_SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3 ("GetUMTSTypeMask failed (%d)", dsErrno, 0, 0);
        return dsErrno;
      }

      *BearerIsNull = (0 != (typeMask ==
                             BearerTechUMTS::SUBTECH_NULL));
      break;

    default:
      *BearerIsNull = FALSE;
  }

  LOG_MSG_FUNCTION_EXIT ("Obj 0x%p, BearerIsNull %d", this, *BearerIsNull, 0);
  return AEE_SUCCESS;

} /* GetBearerIsNull() */

