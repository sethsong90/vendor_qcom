/*===========================================================================
  FILE: PhysLinkHelper.cpp

  OVERVIEW: This file is a helper wrapping a PhysLink and providing helper
  functions to be used with QTF testcases.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/
  test/PhysLinkHelper.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- -------- ------------------------------------------------------------
  2008-05-14 gnamasiv  Created module

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "PhysLinkHelper.h"

using namespace PS::QTF;

/**
 @brief Constructor for initialising and creation of physLinks
  
 The function initializes the physLink by assigning it callback 
 functions. 
  
 @param[in] pPhysLink physLinkPtr The physLink pointer t
 @param[in] physLinkUpFnPtr : callback function for physLinkUp 
       event
 @param[in] physLinkDownFnPtr : callback function for 
       physLinkDown event
 @param[in] PhysLinkNullFnPtr : callback function for 
       physLinkNull event
 @returnval : None
*/
PhysLinkHelper::PhysLinkHelper
(
  ps_phys_link_type          * pPhysLink,
  ps_phys_link_cmd_f_ptr_type  physLinkUpFnPtr,
  ps_phys_link_cmd_f_ptr_type  physLinkDownFnPtr,
  ps_phys_link_cmd_f_ptr_type  PhysLinkNullFnPtr
)
{
  physLink = pPhysLink;
  physLink->phys_link_up_cmd_f_ptr = (physLinkUpFnPtr == NULL) ? 
                   PSIfacePhysLinkUpCmdCB : physLinkUpFnPtr; 
  physLink->phys_link_down_cmd_f_ptr = (physLinkDownFnPtr == NULL) ? 
                   PSIfacePhysLinkDownCmdCB : physLinkDownFnPtr; 
  physLink->phys_link_go_null_cmd_f_ptr = (PhysLinkNullFnPtr == NULL) ? 
                   STAPSIfacePhysLinkGoNullCmdCB : PhysLinkNullFnPtr;
}


ps_phys_link_type * PhysLinkHelper::GetPhysLinkPtr
(
  void
)
{
  return physLink;
}

int PhysLinkHelper::PSIfacePhysLinkUpCmdCB
(
  ps_phys_link_type * physLinkPtr,
  void              * clientData
)
{
  PosixTask::PostMessage( PS::QTF::Event::PHYS_LINK_UP, physLinkPtr );
  return 0;
}

int PhysLinkHelper::PSIfacePhysLinkUpCmdDoNothingCB
(
  ps_phys_link_type * physLinkPtr,
  void              * clientData
)
{
  return 0;
}

int PhysLinkHelper::PSIfacePhysLinkDownCmdCB
(
  ps_phys_link_type * physLinkPtr,
  void              * clientData
)
{
  PosixTask::PostMessage( PS::QTF::Event::PHYS_LINK_DOWN, physLinkPtr );
  return 0;
}


int PhysLinkHelper::STAPSIfacePhysLinkGoNullCmdCB
(
  ps_phys_link_type * physLinkPtr,
  void              * clientData
)                                                                         
{
  PosixTask::PostMessage( PS::QTF::Event::PHYS_LINK_GONE, physLinkPtr );
  return 0;
}

void PhysLinkHelper::PostPhysLinkMessage
(
  PS::QTF::EventType eventType
)
{
  PosixTask::PostMessage( eventType, physLink );
}
