/*==========================================================================*/
/*!
  @file 
  ds_Net_Init.cpp

  @brief
  This file provides functions that are used to perform initializations 
  of DSNET module.

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Init.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-12-01 aa  Removing usage of scratchpad. Using system heap instead.
  2009-08-17 pp  DS_Net module uses system heap memory via scratchpad.
  2009-02-27 hm  Added scratchpad memory for DSNET client.
  2008-05-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Net_Init.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Platform.h"
#include "ds_Net_NetworkFactory.h"

#include "ds_Net_INetworkFactory.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_ITechUMTS.h"
#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Utils_FullPrivSet.h"

using namespace ds::Net;

static INetworkFactory *INetworkFactoryPtr = 0;
static INetworkFactoryPriv *INetworkFactoryPrivPtr = 0;
static IPrivSet *      IPrivSetPtr = 0;

/*---------------------------------------------------------------------------
  Local declarations.
---------------------------------------------------------------------------*/

void DSNetPowerup
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Powerup init of DSNET", 0, 0, 0);
  MemoryManager::MemPoolInit();

  NetPlatform::Init();

  /*-------------------------------------------------------------------------
    Allocate FullPriv instance.
  -------------------------------------------------------------------------*/
  IPrivSetPtr = ds::Utils::FullPrivSet::Instance();

  /* create singletons and only release during powerdown */
  ASSERT(AEE_SUCCESS == DSNetCreateInstance(0,
                                            AEECLSID_CNetworkFactory,
                                            IPrivSetPtr,
                                            (void**)&INetworkFactoryPtr));
  ASSERT(AEE_SUCCESS == DSNetCreateInstance(0,
                                            AEECLSID_CNetworkFactoryPriv,
                                            IPrivSetPtr,
                                            (void**)&INetworkFactoryPrivPtr));

  ds::Net::EventManager::Init();

} /* DSNetPowerupInit() */

void DSNetPowerdown
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Powerdown init of DSNET", 0, 0, 0);
  
  (void)INetworkFactoryPtr->Release();
  (void)INetworkFactoryPrivPtr->Release();
  (void)IPrivSetPtr->Release();

  EventManager::Deinit();

} /* DSNetPowerdown() */


void DSNetInit
(
  void
)
{

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("After-task start init of DSNET", 0, 0, 0);

} /* DSNetInit() */

void DSNetDeinit
(
  void
)
{

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("After-task stop init of DSNET", 0, 0, 0);

} /* DSNetDeinit() */


