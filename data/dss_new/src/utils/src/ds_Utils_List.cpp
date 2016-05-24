/*===========================================================================
  FILE: List.cpp

  OVERVIEW: This file implements the ds::Utils::List class.

  DEPENDENCIES: None

  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_List.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-05-10 hm  Added support for cloning the list before traversal.
  2008-08-29 hm  Added Iterator implementation.
  2008-05-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_List.h"

#include "ds_Utils_CSSupport.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECCritSect.h"
#ifdef FEATURE_DSS_LINUX
#include "ds_sl_list.h"
#else
#include "list.h"
#endif

#include "ps_system_heap.h"

using namespace ds::Error;
using namespace ds::Utils;

/*===========================================================================

                     INTERNAL CLASSES AND FUNCTION DEFINITIONS

===========================================================================*/

typedef struct _list_node
{
  list_link_type        _list_link;
  ds::Utils::INode*     _item_ptr;
} _list_node_type;

namespace ds
{
namespace Utils
{

class ListInternal
{
public:
  ListInternal() throw();
  virtual ~ListInternal() throw();

  list_type     _list;
  int           _current;

  static void* operator new
  (
    unsigned int numBytes
  )
  throw();

  static void operator delete
  (
    void* bufPtr
  )
  throw();

}; /* class ListInternal */

}  // namespace Utils
}  // namespace ds

/*---------------------------------------------------------------------------
  Namespace declarations. This is done after ListInternal class is
  defined to avoid lint error 578.
---------------------------------------------------------------------------*/
using namespace ds::Error;
using namespace ds::Utils;

ListInternal::ListInternal
(
  void
)
throw()
: _current(0)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset (&_list, 0, sizeof(_list));
  list_init (&_list);

} /* ListInternal() */

ListInternal::~ListInternal
(
  void
)
throw()
{
  _list_node_type *  _list_node_ptr;
  INode *            _item_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  while (NULL !=
         (_list_node_ptr = (_list_node_type *) list_pop_front(&_list)))
  {
    _item_ptr = _list_node_ptr->_item_ptr;
    (void) _item_ptr->ReleaseWeak();
    PS_SYSTEM_HEAP_MEM_FREE (_list_node_ptr);
  }

} /* ~ListInternal() */

void* ListInternal::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ps_system_heap_mem_alloc(numBytes);

} /* ListInternal::operator new() */


void ListInternal::operator delete
(
  void* bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_SYSTEM_HEAP_MEM_FREE(bufPtr);

} /* ListInternal::operator delete() */

/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
List::List
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO3 ("obj 0x%p", this, 0, 0);

  pListInternal = (ds::Utils::ListInternal *) new ListInternal();
  if (0 == pListInternal)
  {
    LOG_MSG_ERROR( "No mem for List", 0, 0, 0);
    ASSERT (0);
  }

  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &pCritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }
} /* List() */


List::~List
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO3 ("obj 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF(pCritSect);
  delete (ds::Utils::ListInternal *) pListInternal;

} /* ~List() */

/* Prepares a list node for pushing to front or to back, returns errors.
   Refactors the common part out of PushBack and PushFront.

   Must be called from within critical section.
*/
int32 List::PreparePush
(
  INode *item,
  void **listNodePtr
)
throw()
{
  _list_node_type    * _node_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  *listNodePtr = 0;
  LOG_MSG_INFO3 ("list 0x%p, node 0x%p, size %d", this, item, Count());

  if (0 == item)
  {
    LOG_MSG_ERROR ("Can't push a NULL item", 0, 0, 0);
    return QDS_EFAULT;
  }

  /* Avoid multiple enqueue of the same item */
  for (_node_ptr  = (_list_node_type *) list_peek_front (&pListInternal->_list);
       _node_ptr != 0;
       _node_ptr  = (_list_node_type *)
                    list_peek_next (&pListInternal->_list, &(_node_ptr->_list_link)))
  {
    if (_node_ptr->_item_ptr == item)
    {
      return AEE_SUCCESS;
    }
  }

  /*lint -save -e774 Reason: Mem allocation can fail and NULL check a must*/
  _node_ptr = (_list_node_type*)
    ps_system_heap_mem_alloc (sizeof(_list_node_type));
  if (0 == _node_ptr)
  {
    ASSERT (0);
    return AEE_ENOMEMORY;
  }
  /*lint -restore Restore lint error 774*/

  /* Store INode item, increment weak ref-count of item */
  _node_ptr->_item_ptr = item;
  (void) item->AddRefWeak();

  *listNodePtr = _node_ptr;
  return AEE_SUCCESS;

} /* List::PreparePush() */

int32 List::PushBack
(
  INode* item
)
throw()
{
  _list_node_type    * _node_ptr;
  ds::ErrorType        result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  pCritSect->Enter();

  result = PreparePush(item, (void **)&_node_ptr);
  if (0 != _node_ptr)
  {
    list_push_back (&pListInternal->_list, &(_node_ptr->_list_link));
  }

  pCritSect->Leave();

  return result;

} /* PushBack() */

/**
  @brief      This function inserts an object at the front of the list.

  @param[in]  item - Pointer to the item to be inserted.

  @return     AEE_SUCCESS on success.
  @return     Error code on error.
*/

int32 List::PushFront
(
  INode* item
)
throw()
{
  _list_node_type    * _node_ptr;
  ds::ErrorType        result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  pCritSect->Enter();

  result = PreparePush(item, (void **)&_node_ptr);
  if (0 != _node_ptr)
  {
    list_push_front (&pListInternal->_list, &(_node_ptr->_list_link));
  }

  pCritSect->Leave();

  return result;

} /* PushFront() */

/**
  @brief      This function removes an item from the front of the list.

  @param      None.

  @return     AEE_SUCCESS on success.
  @return     Error code on error.
*/

INode* List::PopFront
(
  void
)
throw()
{
  _list_node_type* _node_ptr;
  INode*           _item_ptr = NULL;
  boolean          _have_strong_ref = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO3 ("list 0x%p, size %d", this, Count(), 0);

  pCritSect->Enter();

  _node_ptr = (_list_node_type *) list_pop_front (&pListInternal->_list);
  while (NULL != _node_ptr)
  {
    _item_ptr = _node_ptr->_item_ptr;
    _have_strong_ref = _item_ptr->GetStrongRef();
    (void) _item_ptr->ReleaseWeak();
    PS_SYSTEM_HEAP_MEM_FREE (_node_ptr);
    if (_have_strong_ref)
    {
      pCritSect->Leave();

      return _item_ptr;
    }
  }

  pCritSect->Leave();

  return NULL;

} /* PopFront() */

void List::RemoveItem
(
  INode*   item
)
throw()
{
  _list_node_type* _node_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO3 ("list 0x%p, node 0x%p, size %d", this, item, Count());

  pCritSect->Enter();

  for (_node_ptr  = (_list_node_type *) list_peek_front (&pListInternal->_list);
       _node_ptr != NULL;
       _node_ptr  = (_list_node_type *) list_peek_next (&pListInternal->_list, &(_node_ptr->_list_link)))
  {
    if (_node_ptr->_item_ptr == item)
    {
      list_pop_item (&pListInternal->_list, &(_node_ptr->_list_link));
      (void) item->ReleaseWeak();
      PS_SYSTEM_HEAP_MEM_FREE (_node_ptr);
      break;
    }
  }

  pCritSect->Leave();

} /* PopFront() */

int32 List::Count
(
  void
)
throw()
{
  int32 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  pCritSect->Enter();  

  result = list_size (&pListInternal->_list);

  pCritSect->Leave();

  return result;

} /* Count() */



INode* List::Get
(
  int32 argIndex
)
throw()
{
  int32 index;
  _list_node_type* _node_ptr;
  INode* result = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 > argIndex || Count() <= argIndex)
  {
    return NULL;
  }

  pCritSect->Enter();

  index = 0;
  for (_node_ptr  = (_list_node_type *) list_peek_front (&pListInternal->_list);
       _node_ptr != NULL;
       _node_ptr  = (_list_node_type *) list_peek_next (&pListInternal->_list, &(_node_ptr->_list_link)))
  {
    if (index == argIndex)
    {
      result = _node_ptr->_item_ptr;
      break;
    }

    index ++;
  }

  pCritSect->Leave();

  return result;

} /* Get() */

boolean List::CloneList
(
  List *  listPtr
)
throw()
{
  _list_node_type *                _node_ptr;
  ds::ErrorType                    dsErrno = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("list 0x%p, size %d", this, Count(), 0);

  if (0 == listPtr)
  {
    LOG_MSG_ERROR( "NULL list", 0, 0, 0);
    ASSERT( 0);
    return FALSE;
  }

  /* Copy items from passed List */

  pCritSect->Enter();

  _node_ptr = (_list_node_type *) list_peek_front (&listPtr->pListInternal->_list);

  while (_node_ptr != NULL && _node_ptr->_item_ptr != NULL)
  {
    dsErrno = PushBack( _node_ptr->_item_ptr );
    if (AEE_SUCCESS != dsErrno)
    {
      LOG_MSG_ERROR( "PushBack failed, list 0x%x err 0x%x",
                     this, dsErrno, 0);
      pCritSect->Leave();
      return FALSE;
    }
    
    /* Increment the iterator */
    _node_ptr = (_list_node_type *) list_peek_next (&listPtr->pListInternal->_list, 
                                                    &(_node_ptr->_list_link));    
  }

  pCritSect->Leave();

  LOG_MSG_INFO3( "Success, list 0x%x", this, 0, 0);
  return TRUE;

} /* CloneList() */

/*---------------------------------------------------------------------------
  Methods from the ITraverser interface.
---------------------------------------------------------------------------*/
boolean List::Traverse
(
  void* userDataPtr
)
throw()
{
  boolean                          result;
  INode*                           currentItem;
  _list_node_type *                _node_ptr;
  List *                           newListPtr      = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("list 0x%p, size %d", this, Count(), 0);

  result = TRUE;

  /* Create a Cloned List */
  newListPtr = new List();
  if (0 == newListPtr)
  {
    LOG_MSG_ERROR( "No mem for list", 0, 0, 0);
    ASSERT( 0);
    return FALSE;
  }

  /* Clone the List */
  pCritSect->Enter();
  result = newListPtr->CloneList( this);
  pCritSect->Leave();
  if (TRUE != result)
  {
    LOG_MSG_ERROR( "CloneList failed, list 0x%x ", this, 0, 0);
    goto bail;
  }

  /* Traverse on cloned List */
  _node_ptr = (_list_node_type *) list_peek_front (&newListPtr->pListInternal->_list);
  while (_node_ptr != NULL && _node_ptr->_item_ptr != NULL)
  {
    /* Get the current item */
    currentItem = _node_ptr->_item_ptr;

    /* Increment the iterator first, the current item can get deleted */
    _node_ptr = (_list_node_type *) list_peek_next (&newListPtr->pListInternal->_list, 
                                                    &(_node_ptr->_list_link));

    /* Try to obtain a strong reference to the item, only if successful
       Perform Process() on current item */
    if (TRUE == currentItem->GetStrongRef())
    {
      result = currentItem->Process (userDataPtr);

      currentItem->Release();

      if (FALSE == result)
      {
        goto bail;
      }
    }

  } /* while */

  delete newListPtr;
  return result;

bail:
  delete newListPtr;
  return result;

} /* Traverse() */

/*---------------------------------------------------------------------------
  Methods from the IIterator interface.
---------------------------------------------------------------------------*/
void List::First
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("list 0x%p", this, 0, 0);

  pCritSect->Enter();

  pListInternal->_current = 0;

  pCritSect->Leave();

} /* First() */


void List::Next
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  pCritSect->Enter();

  pListInternal->_current++;

  pCritSect->Leave();

  LOG_MSG_INFO3 ("list 0x%p, pos %d", this, pListInternal->_current, 0);

} /* Next() */

boolean List::IsDone
(
  void
)
throw()
{
  boolean result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("list 0x%p, pos %d", this, pListInternal->_current, 0);

  pCritSect->Enter();

  result = (pListInternal->_current >= Count());

  pCritSect->Leave();

  return result;

} /* IsDone() */

INode* List::CurrentItem
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("list 0x%p, pos %d", this, pListInternal->_current, 0);

  pCritSect->Enter();

  INode* item = Get(pListInternal->_current);

  pCritSect->Leave();

  return item;

} /* CurrentItem() */


/*---------------------------------------------------------------------------
  Overloaded new/delete methods.
---------------------------------------------------------------------------*/
void* List::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ps_system_heap_mem_alloc(numBytes);

} /* List::operator new() */


void List::operator delete
(
  void* bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_SYSTEM_HEAP_MEM_FREE(bufPtr);

} /* List::operator delete() */

