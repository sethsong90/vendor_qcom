/*===========================================================================
  FILE: Factory.cpp

  OVERVIEW: This file implements the ds::Utils::Factory class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_Factory.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_Factory.h"

using namespace ds::Utils;


/*===========================================================================

                       CONSTRUCTOR/DESTRUCTOR

===========================================================================*/
Factory::Factory
(
  void
)
throw()
: _list()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Creating Factory 0x%p", this, 0, 0);

} /* Factory() */

Factory::~Factory
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Deleting Factory 0x%p", this, 0, 0);

  DeleteAllItems();

} /* ~Factory() */

/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
int32 Factory::AddItem
(
  INode* item
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Fac 0x%p, item 0x%p", this, item, 0);

  return _list.PushBack(item);

} /* PushBack() */

void Factory::RemoveItem
(
  INode*   item
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Fac 0x%p, item 0x%p", this, 0, 0);

  _list.RemoveItem(item);

} /* RemoveItem() */


void Factory::DeleteAllItems
(
  void
)
throw()
{
  INode *pItem;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Factory 0x%p", this, 0, 0);

  while (NULL != (pItem = _list.PopFront()))
  {
    (void) pItem->Release();
  }

} /* DeleteAllItems() */


boolean Factory::Traverse
(
  void* userDataPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Fac 0x%p", this, 0, 0);

  return _list.Traverse (userDataPtr);

} /* Traverse() */

//Harsh: This is very rough and ugly implementation for iterator.
//It is currently very inefficient. We need to clean this up.
void Factory::First 
(
  void 
)
throw()
{
  _list.First();
}

void Factory::Next 
(
  void 
)
throw()
{
  _list.Next();
}

INode * Factory::CurrentItem 
(
  void 
)
throw()
{
  return _list.CurrentItem();
}

boolean Factory::IsDone 
(
  void 
)
throw()
{
  return _list.IsDone();
}

