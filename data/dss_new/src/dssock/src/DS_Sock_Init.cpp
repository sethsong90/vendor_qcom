/*===========================================================================
  FILE: DS_Sock_Init.cpp

  OVERVIEW: This file provides functions that are used to perform DSSock module
  initialization

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_Init.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "DS_Sock_Init.h"
#include "DS_Sock_MemManager.h"
#include "DS_Sock_EventManager.h"
#include "DS_Utils_DebugMsg.h"


/*===========================================================================

                           EXTERNAL FUNCTIONS

===========================================================================*/
void DSSockPowerup
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "", 0, 0, 0);

  DS::Sock::MemManager::Init();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);

  return;
} /* DSPowerupInit() */


void DSSockInit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "", 0, 0, 0);

  DS::Sock::EventManager::Init();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);

  return;
} /* DSSockInit() */
