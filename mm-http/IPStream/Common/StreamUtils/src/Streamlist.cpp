/* =========================================================================

DESCRIPTION
   Linked list and ordered link list routines and definitions

COPYRIGHT 2011-2013 Qualcomm Technologies, Inc.
All rights reserved. Qualcomm Technologies proprietary and confidential.
============================================================================ */

/* =========================================================================

                             Edit History

$PVCSPath: O:/src/asw/COMMON/vcs/list.c_v   1.0   24 May 2001 09:34:22   karthick  $
$Header: //source/qcom/qct/multimedia2/Video/Source/IPStream/Common/StreamUtils/main/latest/src/Streamlist.cpp#11 $ $DateTime: 2013/08/02 06:28:55 $ $Author: sujitd $

when       who     what, where, why
--------   ---     ---------------------------------------------------------
09/01/00    gr     Added merge and split functions for lists and ordered
                   lists.
08/23/00    gr     Improved implementation of ordered_StreamList_push.
08/22/00    gr     Implemented ordered list API.
08/09/00   jct     Created.
============================================================================ */

/* ------------------------------------------------------------------------
** Includes
** ------------------------------------------------------------------------ */
#include "Streamlist.h"
#include "IPStreamSourceUtils.h"
#include "qtv_msg.h"

/* ------------------------------------------------------------------------
**
** Unordered Lists
**
** ------------------------------------------------------------------------ */
#if 0

/* ==================================================================
FUNCTION LIST_INIT
DESCRIPTION
   Initializes a list.
===================================================================== */
void StreamList_init(
   StreamList_type *StreamList_ptr
)
{
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_ptr != NULL );
      StreamList_ptr->back_ptr  = NULL;
      StreamList_ptr->front_ptr = NULL;
      StreamList_ptr->size      = 0;
   STREAMLIST_FREE();
   return;
} /* END StreamList_init */


/* ==================================================================
FUNCTION LIST_PUSH_FRONT
DESCRIPTION
   Inserts the given item_link_ptr at the front of the list pointed
   to by StreamList_ptr.
===================================================================== */
void StreamList_push_front(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_link_ptr
)
{
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_link_ptr != NULL );
      STREAMLIST_ASSERT( !item_is_in_streamlist( StreamList_ptr, item_link_ptr ) );
      item_link_ptr->next_ptr = StreamList_ptr->front_ptr;
      StreamList_ptr->front_ptr     = item_link_ptr;
      if( StreamList_ptr->back_ptr == NULL )
      {
         StreamList_ptr->back_ptr = item_link_ptr;
      }
      StreamList_ptr->size++;
   STREAMLIST_FREE();
   return;
} /* END StreamList_push_front */


/* ==================================================================
FUNCTION LIST_POP_FRONT
DESCRIPTION
   Removes the item at the front of the list.
===================================================================== */
void*
StreamList_pop_front(
   StreamList_type *StreamList_ptr
)
{
   StreamList_link_type *ret_ptr = NULL;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      if( StreamList_ptr->size > 0 )
      {
         ret_ptr = StreamList_ptr->front_ptr;
         StreamList_ptr->front_ptr = ret_ptr->next_ptr;
         if( StreamList_ptr->front_ptr == NULL )
         {
            StreamList_ptr->back_ptr = NULL;
         }
         StreamList_ptr->size--;
      }
   STREAMLIST_FREE();
   return ret_ptr;
} /* END StreamList_pop_front */


/* ==================================================================
FUNCTION LIST_PUSH_BACK
DESCRIPTION
   Inserts the given item_link_ptr at the back of the list pointed
   to by StreamList_ptr
===================================================================== */
void StreamList_push_back(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_link_ptr
)
{
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_link_ptr != NULL );
      STREAMLIST_ASSERT( !item_is_in_streamlist( StreamList_ptr, item_link_ptr ) );
      item_link_ptr->next_ptr = NULL;
      if ( StreamList_ptr->size == 0 )
      {
         StreamList_ptr->front_ptr = item_link_ptr;
      }
      else
      {
         StreamList_ptr->back_ptr->next_ptr = item_link_ptr;
      }
      StreamList_ptr->back_ptr = item_link_ptr;

      StreamList_ptr->size++;
   STREAMLIST_FREE();
   return;
} /* END StreamList_push_back */


/* ==================================================================
FUNCTION LIST_POP_BACK
DESCRIPTION
   Removes the item at the back of the list.
===================================================================== */
void*
StreamList_pop_back(
   StreamList_type *StreamList_ptr
)
{
   StreamList_link_type *ntl_ptr = NULL;
   StreamList_link_type *ret_ptr = NULL;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      if( StreamList_ptr->size > 0 )
      {
         ret_ptr = StreamList_ptr->back_ptr;

         ntl_ptr = StreamList_ptr->front_ptr;
         if ( ntl_ptr == ret_ptr )
         {
            StreamList_ptr->back_ptr = StreamList_ptr->front_ptr = NULL;
         }
         else
         {
            while ( ntl_ptr->next_ptr != ret_ptr )
            {
               ntl_ptr = ntl_ptr->next_ptr;
            }
            StreamList_ptr->back_ptr = ntl_ptr;
            ntl_ptr->next_ptr  = NULL;
         }

         StreamList_ptr->size--;
      }
   STREAMLIST_FREE();
   return ret_ptr;
} /* END StreamList_pop_back */


/* ==================================================================
FUNCTION LIST_SIZE
DESCRIPTION
   Returns the number of elements in the list.
===================================================================== */
StreamList_size_type
StreamList_size(
   StreamList_type *StreamList_ptr
)
{
   StreamList_size_type size;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      size = StreamList_ptr->size;
   STREAMLIST_FREE();
   return size;
} /* END StreamList_size */


/* ==================================================================
FUNCTION LIST_PEEK_FRONT
DESCRIPTION
   Returns a pointer to the element at the front of the list.
===================================================================== */
void*
StreamList_peek_front(
   StreamList_type *StreamList_ptr
)
{
   void *item_ptr;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      item_ptr = StreamList_ptr->front_ptr;
   STREAMLIST_FREE();
   return item_ptr;
} /* END StreamList_peek_front */


/* ==================================================================
FUNCTION LIST_PEEK_BACK
DESCRIPTION
   Returns a pointer to the element at the back of the list.
===================================================================== */
void*
StreamList_peek_back(
   StreamList_type *StreamList_ptr
)
{
   void *item_ptr;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      item_ptr = StreamList_ptr->back_ptr;
   STREAMLIST_FREE();
   return item_ptr;
} /* END StreamList_peek_back */


/* ==================================================================
FUNCTION LIST_PEEK_NEXT
DESCRIPTION
   Returns a pointer to the element that follows the element
   pointed to by item_after_which_to_peek
===================================================================== */
void*
StreamList_peek_next(
#ifdef LIST_DEBUG
   StreamList_type      *StreamList_ptr,
#endif
   StreamList_link_type *item_after_which_to_peek
)
{
   void *item_ptr;
   STREAMLIST_LOCK();

 #ifdef LIST_DEBUG
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist( StreamList_ptr, item_after_which_to_peek ) );
 #endif

      item_ptr = item_after_which_to_peek->next_ptr;
   STREAMLIST_FREE();
   return item_ptr;
} /* END StreamList_peek_next */


/* ==================================================================
FUNCTION LIST_PEEK_PREV
DESCRIPTION
   Returns a pointer to the element that precedes the element
   pointed to by item_before_which_to_peek
===================================================================== */
void*
StreamList_peek_prev(
   StreamList_type      *StreamList_ptr,
      /*lint -esym(715,StreamList_ptr)
      ** Have lint not complain about the ignored parameter 'StreamList_ptr'.
      */
   StreamList_link_type *item_before_which_to_peek
)
{
   void           *item_ptr;
   StreamList_link_type *temp_ptr;
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist(StreamList_ptr,  item_before_which_to_peek ) );
      if ( item_before_which_to_peek == StreamList_ptr->front_ptr )
      {
         return NULL;
      }
      temp_ptr = StreamList_ptr->front_ptr;
      while ( temp_ptr != NULL &&
              temp_ptr->next_ptr != item_before_which_to_peek )
      {
         temp_ptr = temp_ptr->next_ptr;
      }
      item_ptr = temp_ptr;
   STREAMLIST_FREE();
   return item_ptr;
} /* END StreamList_peek_prev */


/* ==================================================================
FUNCTION LIST_PUSH_BEFORE
DESCRIPTION
   Pushes an item before another specified item.
===================================================================== */
void StreamList_push_before(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_to_push_ptr,
   StreamList_link_type *item_to_push_before_ptr
)
{
   StreamList_link_type *temp_ptr;

   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_to_push_ptr != NULL );
      STREAMLIST_ASSERT( !item_is_in_streamlist( StreamList_ptr, item_to_push_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist(StreamList_ptr,  item_to_push_before_ptr ) );
      item_to_push_ptr->next_ptr = item_to_push_before_ptr;
      temp_ptr                   = StreamList_ptr->front_ptr;
      if ( temp_ptr == item_to_push_before_ptr )
      {
         StreamList_ptr->front_ptr = item_to_push_ptr;
      }
      else
      {
         while ( temp_ptr->next_ptr != item_to_push_before_ptr )
         {
            temp_ptr = temp_ptr->next_ptr;
         }
         temp_ptr->next_ptr = item_to_push_ptr;
      }

      StreamList_ptr->size++;
   STREAMLIST_FREE();

   return;
} /* END */

/* ==================================================================
FUNCTION LIST_PUSH_AFTER
DESCRIPTION
   Pushes an item after another specified item.
===================================================================== */
void StreamList_push_after(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_to_push_ptr,
   StreamList_link_type *item_to_push_after_ptr
)
{
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_to_push_ptr != NULL );
      STREAMLIST_ASSERT( !item_is_in_streamlist( StreamList_ptr, item_to_push_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist( StreamList_ptr,  item_to_push_after_ptr ) );
      item_to_push_ptr->next_ptr       = item_to_push_after_ptr->next_ptr;
      item_to_push_after_ptr->next_ptr = item_to_push_ptr;
      if ( StreamList_ptr->back_ptr == item_to_push_after_ptr )
      {
         StreamList_ptr->back_ptr = item_to_push_ptr;
      }

      StreamList_ptr->size++;
   STREAMLIST_FREE();

   return;
} /* END */

/* ==================================================================
FUNCTION LIST_POP_ITEM
DESCRIPTION
   Pops a specified item off a list.
===================================================================== */
void StreamList_pop_item(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_to_pop_ptr
)
{
   StreamList_link_type *temp_ptr;

   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist(StreamList_ptr,  item_to_pop_ptr ) );
      temp_ptr = StreamList_ptr->front_ptr;
      if ( temp_ptr == item_to_pop_ptr )
      {
         StreamList_ptr->front_ptr = item_to_pop_ptr->next_ptr;
         if ( StreamList_ptr->back_ptr == item_to_pop_ptr )
         {
            StreamList_ptr->back_ptr = NULL;
         }
      }
      else
      {
         while ( temp_ptr->next_ptr != item_to_pop_ptr )
         {
            temp_ptr = temp_ptr->next_ptr;
         }
         temp_ptr->next_ptr = item_to_pop_ptr->next_ptr;
         if ( StreamList_ptr->back_ptr == item_to_pop_ptr )
         {
            StreamList_ptr->back_ptr = temp_ptr;
         }
      }

      StreamList_ptr->size--;
   STREAMLIST_FREE();

   return;
} /* END */

/* ==================================================================
FUNCTION LIST_LINEAR_SEARCH
DESCRIPTION
   Searches a list for a matching item. The match is determined by
   means of a user-provided compare function. If there is no matching
   item in the list, NULL is returned. If there is more than one
   matching item, only the first match is returned.
===================================================================== */
void*
StreamList_linear_search(
  StreamList_type             *StreamList_ptr,
  StreamList_compare_func_type compare_func,
  void                  *compare_val
)
{
   StreamList_link_type *item_ptr = NULL;

   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( compare_func != NULL );
      item_ptr = StreamList_ptr->front_ptr;
      while( item_ptr != NULL )
      {
         if( compare_func( item_ptr, compare_val ) != 0 )
         {
            break;
         }
         item_ptr = item_ptr->next_ptr;
      }
   STREAMLIST_FREE();

   return item_ptr;
} /* END */


#if 0
/* ==================================================================
FUNCTION LIST_LINEAR_DELETE
DESCRIPTION
   Searches a list for a matching item and deletes the item. The match
   is determined by means of a user-provided compare function.
===================================================================== */
void
StreamList_linear_delete(
  StreamList_type             *StreamList_ptr,
  StreamList_compare_func_type compare_func,
  void                  *compare_val
)
{
   return;
} /* END */
#endif


/* ==================================================================
FUNCTION LIST_MERGE
DESCRIPTION
   Merges two lists by appending the second to the first. After the
   merge, the first list will contain all the elements in both lists.
   The second list will be empty.
===================================================================== */
void StreamList_merge(
   StreamList_type *list1_ptr,
   StreamList_type *list2_ptr
)
{
   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( list1_ptr ) );
      STREAMLIST_ASSERT( StreamList_is_valid( list2_ptr ) );
      if ( list1_ptr->size > 0 )
      {
         list1_ptr->back_ptr->next_ptr = list2_ptr->front_ptr;
      }
      else
      {
         list1_ptr->front_ptr = list2_ptr->front_ptr;
      }
      list1_ptr->back_ptr = list2_ptr->back_ptr;
      list1_ptr->size += list2_ptr->size;

      list2_ptr->front_ptr = list2_ptr->back_ptr = NULL;
      list2_ptr->size  = 0;
   STREAMLIST_FREE();

   return;
} /* END StreamList_merge */


/* ==================================================================
FUNCTION LIST_SPLIT
DESCRIPTION
   Splits a list at a specified item. Items before the specified item
   are retained in the original list. The specified item and items
   following it are placed into a new list.
===================================================================== */
void StreamList_split(
   StreamList_type      *list1_ptr,
   StreamList_link_type *item_at_which_to_split_ptr,
   StreamList_type      *list2_ptr
)
{
   StreamList_link_type *temp_ptr = NULL;
   unsigned long   count    = 0;
   unsigned long   total_size;

   STREAMLIST_LOCK();
      STREAMLIST_ASSERT( StreamList_is_valid( list1_ptr ) );
      STREAMLIST_ASSERT( StreamList_is_valid( list2_ptr ) );
      STREAMLIST_ASSERT( item_is_in_streamlist( list1_ptr, item_at_which_to_split_ptr ) );

      total_size = list1_ptr->size;
      /* Figure out the number of elements in the first list after the
      ** split.
      */
      if ( list1_ptr->front_ptr != item_at_which_to_split_ptr )
      {
         count++;
         temp_ptr = list1_ptr->front_ptr;
         while ( temp_ptr->next_ptr != item_at_which_to_split_ptr )
         {
            temp_ptr = temp_ptr->next_ptr;
            count++;
         }
         temp_ptr->next_ptr = NULL;
      }
      else
      {
         list1_ptr->front_ptr = NULL;
      }

      list2_ptr->front_ptr = item_at_which_to_split_ptr;
      list2_ptr->back_ptr  = list1_ptr->back_ptr;
      list2_ptr->size      = total_size - count;
      list1_ptr->back_ptr  = temp_ptr;
      list1_ptr->size      = count;
   STREAMLIST_FREE( );

   return;
} /* END StreamList_split */

#endif

/* ------------------------------------------------------------------------
**
** Ordered Lists
** - These support all list operations except for push_before and
** - push_after, which would disrupt the order of a list.
** ------------------------------------------------------------------------ */

/* ==================================================================
FUNCTION ORDERED_LIST_INIT
DESCRIPTION
   Initializes an ordered list.
===================================================================== */
ordered_StreamList_type       * ordered_StreamList_init(
   ordered_StreamList_type       *StreamList_ptr,
   ordered_StreamList_config_type sort_order,
   ordered_StreamList_config_type compare_type,
   MM_HANDLE critSect /* Ptr to critical section. */
)
{
  StreamList_ptr->link.back_ptr  = NULL;
  StreamList_ptr->link.front_ptr = NULL;
  StreamList_ptr->link.size      = 0;
  StreamList_ptr->link.type      = sort_order | compare_type;

  if (StreamList_ptr->lock)
  {
    MM_CriticalSection_Release(StreamList_ptr->lock);
  }

  if (critSect)
  {
    // user supplied critical section cache it
    StreamList_ptr->lock = critSect;
  }
  else if(MM_CriticalSection_Create(&StreamList_ptr->lock) != 0)
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
           "ordered_StreamList_init: Unable to create a critical section: %p",
           (void *)StreamList_ptr->lock);
  }
  return StreamList_ptr ;
} /* END */


/* ==================================================================
FUNCTION ORDERED_LIST_PUSH_AFTER
DESCRIPTION
   Pushes an item after another specified item in an ordered list.
   The caller must check to ensure that this operation does not
   disrupt the order of the list.
   Note: This function is not exported.
===================================================================== */
void ordered_StreamList_push_after(
   ordered_StreamList_type      *StreamList_ptr,
   ordered_StreamList_link_type *item_to_push_ptr,
   ordered_StreamList_link_type *item_to_push_after_ptr
)
{
 ORDERED_STREAMLIST_LOCK(StreamList_ptr);
   item_to_push_ptr->next_ptr = item_to_push_after_ptr->next_ptr;
   item_to_push_ptr->prev_ptr = item_to_push_after_ptr;
   item_to_push_after_ptr->next_ptr = item_to_push_ptr;
   if ( StreamList_ptr->link.back_ptr == item_to_push_after_ptr )
   {
     StreamList_ptr->link.back_ptr = item_to_push_ptr;
   }
   else
   {
      item_to_push_ptr->next_ptr->prev_ptr = item_to_push_ptr;
   }
   StreamList_ptr->link.size++;
 ORDERED_STREAMLIST_FREE(StreamList_ptr);

   return;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PUSH_BEFORE
DESCRIPTION
   Pushes an item before another specified item in an ordered list.
   The caller must check to ensure that this operation does not
   disrupt the order of the list.
   Note: This function is not exported.
===================================================================== */
void ordered_StreamList_push_before(
   ordered_StreamList_type      *StreamList_ptr,
   ordered_StreamList_link_type *item_to_push_ptr,
   ordered_StreamList_link_type *item_to_push_before_ptr
)
{
 ORDERED_STREAMLIST_LOCK(StreamList_ptr);
   item_to_push_ptr->next_ptr = item_to_push_before_ptr;
   item_to_push_ptr->prev_ptr = item_to_push_before_ptr->prev_ptr;
   item_to_push_before_ptr->prev_ptr = item_to_push_ptr;
   if ( StreamList_ptr->link.front_ptr == item_to_push_before_ptr )
   {
     StreamList_ptr->link.front_ptr = item_to_push_ptr;
   }
   else
   {
      item_to_push_ptr->prev_ptr->next_ptr = item_to_push_ptr;
   }
   StreamList_ptr->link.size++;
 ORDERED_STREAMLIST_FREE(StreamList_ptr);

   return;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PUSH
DESCRIPTION
   Pushes an item on an ordered list. The point of insertion depends
   on the weight of the item, and on the type of list (ascending,
   descending, etc.)
===================================================================== */
void ordered_StreamList_push(
   ordered_StreamList_type        *StreamList_ptr,
   ordered_StreamList_link_type   *item_link_ptr,
   ordered_StreamList_weight_type  weight
)
{
   ordered_StreamList_link_type *temp_ptr;
   ordered_StreamList_link_type *back_ptr;

   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_link_ptr != NULL );
      STREAMLIST_ASSERT( !item_is_in_ordered_streamlist( StreamList_ptr, item_link_ptr ) );

      item_link_ptr->weight = weight;
      item_link_ptr->StreamList_ptr = StreamList_ptr;

      if ( StreamList_ptr->link.size == 0 )
      {
         item_link_ptr->next_ptr = item_link_ptr->prev_ptr = NULL;
        StreamList_ptr->link.front_ptr = StreamList_ptr->link.back_ptr = item_link_ptr;
        StreamList_ptr->link.size++;
      }
      else
      {
        temp_ptr = StreamList_ptr->link.front_ptr;
        back_ptr = StreamList_ptr->link.back_ptr;
        switch( StreamList_ptr->link.type )
         {
            case ORDERED_STREAMLIST_ASCENDING_PUSH_SLT:
               if ( back_ptr->weight < weight )
               {
                  ordered_StreamList_push_after( StreamList_ptr, item_link_ptr, back_ptr );
               }
               else
               {
                  while ( temp_ptr->weight < weight )
                  {
                     temp_ptr = temp_ptr->next_ptr;
                  }
                  ordered_StreamList_push_before( StreamList_ptr, item_link_ptr, temp_ptr );
               }
               break;

            case ORDERED_STREAMLIST_ASCENDING_PUSH_LTE:
               if ( back_ptr->weight <= weight )
               {
                  ordered_StreamList_push_after( StreamList_ptr, item_link_ptr, back_ptr );
               }
               else
               {
                  while ( temp_ptr->weight <= weight )
                  {
                     temp_ptr = temp_ptr->next_ptr;
                  }
                  ordered_StreamList_push_before( StreamList_ptr, item_link_ptr, temp_ptr );
               }
               break;

            case ORDERED_STREAMLIST_DESCENDING_PUSH_SLT:
               if ( back_ptr->weight > weight )
               {
                  ordered_StreamList_push_after( StreamList_ptr, item_link_ptr, back_ptr );
               }
               else
               {
                  while ( temp_ptr->weight > weight )
                  {
                     temp_ptr = temp_ptr->next_ptr;
                  }
                  ordered_StreamList_push_before( StreamList_ptr, item_link_ptr, temp_ptr );
               }
               break;

            case ORDERED_STREAMLIST_DESCENDING_PUSH_LTE:
               if ( back_ptr->weight >= weight )
               {
                  ordered_StreamList_push_after( StreamList_ptr, item_link_ptr, back_ptr );
               }
               else
               {
                  while ( temp_ptr->weight >= weight )
                  {
                     temp_ptr = temp_ptr->next_ptr;
                  }
                  ordered_StreamList_push_before( StreamList_ptr, item_link_ptr, temp_ptr );
               }
               break;

            default:
               /* Unknown list type.
               ** Wailing, gnashing of teeth, etc.
               */
               break;
         }
      }
   ORDERED_STREAMLIST_FREE(StreamList_ptr);

   return;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_POP_FRONT
DESCRIPTION
   Removes an item from the front of an ordered list.
===================================================================== */
void*
ordered_StreamList_pop_front(
   ordered_StreamList_type *StreamList_ptr
)
{
   ordered_StreamList_link_type *ret_ptr = NULL;
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );

      if( StreamList_ptr->link.size > 0 )
      {
         ret_ptr = StreamList_ptr->link.front_ptr;
         StreamList_ptr->link.front_ptr = ret_ptr->next_ptr;
         if( StreamList_ptr->link.front_ptr == NULL )
         {
            StreamList_ptr->link.back_ptr = NULL;
         }
         else
         {
            StreamList_ptr->link.front_ptr->prev_ptr = NULL;
         }
         StreamList_ptr->link.size--;
      }
   ORDERED_STREAMLIST_FREE(StreamList_ptr);
   return ret_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_POP_BACK
DESCRIPTION
   Removes an item from the back of an ordered list.
===================================================================== */
void*
ordered_StreamList_pop_back(
   ordered_StreamList_type *StreamList_ptr
)
{
   ordered_StreamList_link_type *ret_ptr = NULL;
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      if( StreamList_ptr->link.size > 0 )
      {
         ret_ptr = StreamList_ptr->link.back_ptr;
         StreamList_ptr->link.back_ptr = ret_ptr->prev_ptr;
         if ( StreamList_ptr->link.back_ptr == NULL )
         {
            StreamList_ptr->link.front_ptr = NULL;
         }
         else
         {
            StreamList_ptr->link.back_ptr->next_ptr = NULL;
         }
         StreamList_ptr->link.size--;
      }
   ORDERED_STREAMLIST_FREE(StreamList_ptr);
   return ret_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_SIZE
DESCRIPTION
   Returns the number of elements in an ordered list.
===================================================================== */
StreamList_size_type
ordered_StreamList_size(
   ordered_StreamList_type *StreamList_ptr
)
{
   StreamList_size_type size;
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      size = StreamList_ptr->link.size;
   ORDERED_STREAMLIST_FREE(StreamList_ptr);
   return size;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PEEK_FRONT
DESCRIPTION
   Returns a pointer to the first item in an ordered list, or NULL
   if the list is empty.
===================================================================== */
void*
ordered_StreamList_peek_front(
   ordered_StreamList_type *StreamList_ptr
)
{
   void *item_ptr;
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      item_ptr = StreamList_ptr->link.front_ptr;
   ORDERED_STREAMLIST_FREE(StreamList_ptr);
   return item_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PEEK_BACK
DESCRIPTION
   Returns a pointer to the last item in an ordered list, or NULL
   if the list is empty.
===================================================================== */
void*
ordered_StreamList_peek_back(
   ordered_StreamList_type *StreamList_ptr
)
{
   void *item_ptr;
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      item_ptr = StreamList_ptr->link.back_ptr;
   ORDERED_STREAMLIST_FREE(StreamList_ptr);
   return item_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_POP_ITEM
DESCRIPTION
   Removes a specified item from an ordered list.
===================================================================== */
void ordered_StreamList_pop_item(
   ordered_StreamList_type      *StreamList_ptr,
   ordered_StreamList_link_type *item_to_pop_ptr
)
{
   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_ordered_streamlist( StreamList_ptr, item_to_pop_ptr ) );
      if ( item_to_pop_ptr == StreamList_ptr->link.front_ptr )
      {
         StreamList_ptr->link.front_ptr = item_to_pop_ptr->next_ptr;
      }
      else
      {
         item_to_pop_ptr->prev_ptr->next_ptr = item_to_pop_ptr->next_ptr;
      }
      if ( item_to_pop_ptr == StreamList_ptr->link.back_ptr )
      {
         StreamList_ptr->link.back_ptr = item_to_pop_ptr->prev_ptr;
      }
      else
      {
         item_to_pop_ptr->next_ptr->prev_ptr = item_to_pop_ptr->prev_ptr;
      }
      StreamList_ptr->link.size--;
   ORDERED_STREAMLIST_FREE(StreamList_ptr);

   return;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PEEK_NEXT
DESCRIPTION
   Returns a pointer to the item following a specified item in an
   ordered list, or NULL if the input item is the last item in the
   list.
===================================================================== */
void*
ordered_StreamList_peek_next(
#ifdef LIST_DEBUG
   ordered_StreamList_type      *StreamList_ptr,
#endif
      /*lint -esym(715,StreamList_ptr)
      ** Have lint not complain about the ignored parameter 'StreamList_ptr'.
      */
   ordered_StreamList_link_type *item_after_which_to_peek
)
{
   void *item_ptr;
   ORDERED_STREAMLIST_LOCK(item_after_which_to_peek->StreamList_ptr);

 #ifdef LIST_DEBUG
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_ordered_streamlist( StreamList_ptr, item_after_which_to_peek ) );
#endif

      item_ptr = item_after_which_to_peek->next_ptr;
  ORDERED_STREAMLIST_FREE(item_after_which_to_peek->StreamList_ptr);
   return item_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_PEEK_PREV
DESCRIPTION
   Returns a pointer to the item preceding a specified item in an
   ordered list, or NULL if the input item is the last item in the
   list.
===================================================================== */
void*
ordered_StreamList_peek_prev(
#ifdef LIST_DEBUG
   ordered_StreamList_type      *StreamList_ptr,
#endif
   ordered_StreamList_link_type *item_before_which_to_peek
)
{
   void *item_ptr;
   ORDERED_STREAMLIST_LOCK(item_before_which_to_peek->StreamList_ptr);

#ifdef LIST_DEBUG
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( item_is_in_ordered_streamlist( StreamList_ptr, item_before_which_to_peek ) );
#endif

      item_ptr = item_before_which_to_peek->prev_ptr;
   ORDERED_STREAMLIST_FREE(item_before_which_to_peek->StreamList_ptr);
   return item_ptr;
} /* END */

/* ==================================================================
FUNCTION ORDERED_LIST_LINEAR_SEARCH

DESCRIPTION
   Searches an ordered list for a matching item. The match is
   determined by means of a user-provided compare function. If there
   is no matching item in the list, NULL is returned. If there is
   more than one matching item, only the first match is returned.

===================================================================== */
void*
ordered_StreamList_linear_search(
   ordered_StreamList_type      *StreamList_ptr,
   StreamList_compare_func_type  compare_func,
   void                   *compare_val
)
{
   ordered_StreamList_link_type *item_ptr = NULL;

   ORDERED_STREAMLIST_LOCK(StreamList_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( StreamList_ptr ) );
      STREAMLIST_ASSERT( compare_func != NULL );
      item_ptr = StreamList_ptr->link.front_ptr;
      while( item_ptr != NULL )
      {
         if( compare_func( item_ptr, compare_val ) != 0 )
         {
            break;
         }
         item_ptr = item_ptr->next_ptr;
      }
   ORDERED_STREAMLIST_FREE(StreamList_ptr);

   return item_ptr;
} /* END */

#if 0
/* ==================================================================
FUNCTION ORDERED_LIST_LINEAR_DELETE
DESCRIPTION
   Searches an ordered list for a matching item and deletes the item.
   The match is determined by means of a user-provided compare
   function.
===================================================================== */
void
ordered_StreamList_linear_delete(
  StreamList_type             *StreamList_ptr,
  StreamList_compare_func_type compare_func,
  void                  *compare_val
)
{
   return;
} /* END */
#endif


/* ==================================================================
FUNCTION ORDERED_LIST_APPEND
DESCRIPTION
   Appends an ordered list to another. The caller has to ensure that
   this operation does not disrupt the order of the list.
   NOTE: This function is not exported.
===================================================================== */
void ordered_StreamList_append(
   ordered_StreamList_type *list1_ptr,
   ordered_StreamList_type *list2_ptr
)
{
   ORDERED_STREAMLIST_LOCK(list1_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list1_ptr ) );
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list2_ptr ) );
      if ( list1_ptr->link.size > 0 )
      {
         list1_ptr->link.back_ptr->next_ptr = list2_ptr->link.front_ptr;
      }
      else
      {
         list1_ptr->link.front_ptr = list2_ptr->link.front_ptr;
      }
      if ( list2_ptr->link.size > 0 )
      {
         list2_ptr->link.front_ptr->prev_ptr = list1_ptr->link.back_ptr;
      }
      list1_ptr->link.back_ptr = list2_ptr->link.back_ptr;
      list1_ptr->link.size += list2_ptr->link.size;

      list2_ptr->link.front_ptr = list2_ptr->link.back_ptr = NULL;
      list2_ptr->link.size  = 0;
   ORDERED_STREAMLIST_FREE(list1_ptr);

   return;
} /* END ordered_StreamList_append */


/* ==================================================================
FUNCTION ORDERED_LIST_SWAP
DESCRIPTION
   Swaps two ordered lists by swapping their head nodes.
   For internal use only.
===================================================================== */
void ordered_StreamList_swap(
   ordered_StreamList_type *list1_ptr,
   ordered_StreamList_type *list2_ptr
)
{
   ordered_StreamList_type temp_StreamList_hdr;

   temp_StreamList_hdr = *list1_ptr;
   *list1_ptr    = *list2_ptr;
   *list2_ptr    = temp_StreamList_hdr;

   return;
} /* END ordered_StreamList_swap */

#if 0
/* ==================================================================
FUNCTION ORDERED_LIST_MERGE
DESCRIPTION
   Merges two ordered lists. The two lists must be of the same type;
   that is, both should be sorted the same way and the insertion rule
   (SLT or LTE) should be the same as well. After the merge, the first
   list will contain all the elements in both lists. The second list
   will be empty.
===================================================================== */
void ordered_StreamList_merge(
   ordered_StreamList_type *list1_ptr,
   ordered_StreamList_type *list2_ptr
)
{
   ordered_StreamList_link_type *temp1_ptr;
   ordered_StreamList_link_type *temp2_ptr;
   ordered_StreamList_link_type *temp3_ptr;

   ORDERED_STREAMLIST_LOCK(list1_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list1_ptr ) );
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list2_ptr ) );
      if ( list1_ptr->link.size == 0 )
      {
          list1_ptr->link.size      = list2_ptr->link.size;
          list1_ptr->link.front_ptr = list2_ptr->link.front_ptr;
          list1_ptr->link.back_ptr  = list2_ptr->link.back_ptr;
          list2_ptr->link.size      = 0;
          list2_ptr->link.front_ptr = NULL;
          list2_ptr->link.back_ptr  = NULL;
          ORDERED_STREAMLIST_FREE(list1_ptr);
          return;
      }

      if ( list2_ptr->link.size == 0 )
      {
         ORDERED_STREAMLIST_FREE(list1_ptr);
         return;
      }

      if ( list1_ptr->link.type != list2_ptr->link.type )
      {
         ORDERED_STREAMLIST_FREE(list1_ptr);
         return;
      }

      /* If the second list can be appended to the first without destroying
      ** order, do so. This check is done because in some use cases, the
      ** lists merged satisfy this property.
      */
      switch( list2_ptr->link.type )
      {
         case ORDERED_STREAMLIST_ASCENDING_PUSH_SLT:
            if ( list1_ptr->link.back_ptr->weight <= list2_ptr->link.front_ptr->weight )
            {
               /* Case 1a: All items in list1 are smaller than or equal to
               ** the smallest item in list2. Append list2 to list1.
               */
               ordered_StreamList_append( list1_ptr, list2_ptr );
               ORDERED_STREAMLIST_FREE(list1_ptr);
               return;
            }

            if ( list2_ptr->link.back_ptr->weight < list1_ptr->link.front_ptr->weight )
            {
               /* Case 1b: All items in list2 are smaller than the
               ** smallest item in list1. Append list1 to list2.
               */
               ordered_StreamList_swap( list1_ptr, list2_ptr );
               ordered_StreamList_append( list1_ptr, list2_ptr );
               ORDERED_STREAMLIST_FREE(list1_ptr);
               return;
            }

            /*
            ** Case 2: A simple append is not possible, so do a merge. Scan
            ** both lists, moving items from the second list to the first
            ** where appropriate.
            */
            temp1_ptr = list1_ptr->link.front_ptr;
            while ( list2_ptr->link.front_ptr != NULL )
            {
               temp2_ptr = temp3_ptr = list2_ptr->link.front_ptr;

               if ( list1_ptr->link.back_ptr->weight <= temp2_ptr->weight )
               {
                  /* Append list2 to list1.
                  */
                  list1_ptr->link.back_ptr->next_ptr  = list2_ptr->link.front_ptr;
                  list2_ptr->link.front_ptr->prev_ptr = list1_ptr->link.back_ptr;
                  list1_ptr->link.back_ptr            = list2_ptr->link.back_ptr;
                  list2_ptr->link.front_ptr           = NULL;
               }
               else
               {
                  while ( temp1_ptr->weight <= temp2_ptr->weight )
                  {
                     temp1_ptr = temp1_ptr->next_ptr;
                  }
                  if ( temp1_ptr->weight > list2_ptr->link.back_ptr->weight )
                  {
                     temp3_ptr = list2_ptr->link.back_ptr;
                  }
                  else
                  {
                     while ( temp1_ptr->weight > temp3_ptr->next_ptr->weight )
                     {
                        temp3_ptr = temp3_ptr->next_ptr;
                     }
                  }
                  list2_ptr->link.front_ptr           = temp3_ptr->next_ptr;
                  if ( list2_ptr->link.front_ptr != NULL )
                  {
                     list2_ptr->link.front_ptr->prev_ptr = NULL;
                  }
                  temp2_ptr->prev_ptr            = temp1_ptr->prev_ptr;
                  if ( temp1_ptr->prev_ptr != NULL )
                  {
                     temp1_ptr->prev_ptr->next_ptr = temp2_ptr;
                  }
                  else
                  {
                     list1_ptr->link.front_ptr = temp2_ptr;
                  }
                  temp3_ptr->next_ptr = temp1_ptr;
                  temp1_ptr->prev_ptr = temp3_ptr;
               }
            }
            break;

         case ORDERED_STREAMLIST_ASCENDING_PUSH_LTE:
            if ( list1_ptr->link.back_ptr->weight < list2_ptr->link.front_ptr->weight )
            {
               /* Case 1a: All items in list1 are smaller than the
               ** smallest item in list2. Append list2 to list1.
               */
                ordered_StreamList_append( list1_ptr, list2_ptr );
                ORDERED_STREAMLIST_FREE(list1_ptr);
                return;
            }

            if ( list2_ptr->link.back_ptr->weight <= list1_ptr->link.front_ptr->weight )
            {
               /* Case 1b: All items in list2 are smaller than or equal to
               ** the smallest item in list1. Append list1 to list2.
               */
               ordered_StreamList_swap( list1_ptr, list2_ptr );
               ordered_StreamList_append( list1_ptr, list2_ptr );
               ORDERED_STREAMLIST_FREE(list1_ptr);
               return;
            }

            /*
            ** Case 2: A simple append is not possible, so do a merge. Scan
            ** both lists, moving items from the second list to the first
            ** where appropriate.
            */
            temp1_ptr = list1_ptr->link.front_ptr;
            while ( list2_ptr->link.front_ptr != NULL )
            {
               temp2_ptr = temp3_ptr = list2_ptr->link.front_ptr;

               if ( list1_ptr->link.back_ptr->weight < temp2_ptr->weight )
               {
                  /* Append list2 to list1.
                  */
                  list1_ptr->link.back_ptr->next_ptr  = list2_ptr->link.front_ptr;
                  list2_ptr->link.front_ptr->prev_ptr = list1_ptr->link.back_ptr;
                  list1_ptr->link.back_ptr            = list2_ptr->link.back_ptr;
                  list2_ptr->link.front_ptr           = NULL;
               }
               else
               {
                  while ( temp1_ptr->weight < temp2_ptr->weight )
                  {
                     temp1_ptr = temp1_ptr->next_ptr;
                  }
                  if ( temp1_ptr->weight >= list2_ptr->link.back_ptr->weight )
                  {
                     temp3_ptr = list2_ptr->link.back_ptr;
                  }
                  else
                  {
                     while ( temp1_ptr->weight >= temp3_ptr->next_ptr->weight )
                     {
                        temp3_ptr = temp3_ptr->next_ptr;
                     }
                  }
                  list2_ptr->link.front_ptr           = temp3_ptr->next_ptr;
                  if ( list2_ptr->link.front_ptr != NULL )
                  {
                     list2_ptr->link.front_ptr->prev_ptr = NULL;
                  }
                  temp2_ptr->prev_ptr            = temp1_ptr->prev_ptr;
                  if ( temp1_ptr->prev_ptr != NULL )
                  {
                     temp1_ptr->prev_ptr->next_ptr = temp2_ptr;
                  }
                  else
                  {
                     list1_ptr->link.front_ptr = temp2_ptr;
                  }
                  temp3_ptr->next_ptr = temp1_ptr;
                  temp1_ptr->prev_ptr = temp3_ptr;
               }
            }
            break;

         case ORDERED_STREAMLIST_DESCENDING_PUSH_SLT:
            if ( list1_ptr->link.back_ptr->weight >= list2_ptr->link.front_ptr->weight )
            {
               /* Case 1a: All items in list1 are greater than or equal to
               ** the biggest item in list2. Append list2 to list1.
               */
                ordered_StreamList_append( list1_ptr, list2_ptr );
                ORDERED_STREAMLIST_FREE(list1_ptr);
                return;
            }

            if ( list2_ptr->link.back_ptr->weight > list1_ptr->link.front_ptr->weight )
            {
               /* Case 1b: All items in list2 are greater than the
               ** biggest item in list1. Append list1 to list2.
               */
               ordered_StreamList_swap( list1_ptr, list2_ptr );
               ordered_StreamList_append( list1_ptr, list2_ptr );
               ORDERED_STREAMLIST_FREE(list1_ptr);
               return;
            }

            /*
            ** Case 2: A simple append is not possible, so do a merge. Scan
            ** both lists, moving items from the second list to the first
            ** where appropriate.
            */
            temp1_ptr = list1_ptr->link.front_ptr;
            while ( list2_ptr->link.front_ptr != NULL )
            {
               temp2_ptr = temp3_ptr = list2_ptr->link.front_ptr;

               if ( list1_ptr->link.back_ptr->weight >= temp2_ptr->weight )
               {
                  /* Append list2 to list1.
                  */
                  list1_ptr->link.back_ptr->next_ptr  = list2_ptr->link.front_ptr;
                  list2_ptr->link.front_ptr->prev_ptr = list1_ptr->link.back_ptr;
                  list1_ptr->link.back_ptr            = list2_ptr->link.back_ptr;
                  list2_ptr->link.front_ptr           = NULL;
               }
               else
               {
                  while ( temp1_ptr->weight >= temp2_ptr->weight )
                  {
                     temp1_ptr = temp1_ptr->next_ptr;
                  }
                  if ( temp1_ptr->weight < list2_ptr->link.back_ptr->weight )
                  {
                     temp3_ptr = list2_ptr->link.back_ptr;
                  }
                  else
                  {
                     while ( temp1_ptr->weight < temp3_ptr->next_ptr->weight )
                     {
                        temp3_ptr = temp3_ptr->next_ptr;
                     }
                  }
                  list2_ptr->link.front_ptr           = temp3_ptr->next_ptr;
                  if ( list2_ptr->link.front_ptr != NULL )
                  {
                     list2_ptr->link.front_ptr->prev_ptr = NULL;
                  }
                  temp2_ptr->prev_ptr            = temp1_ptr->prev_ptr;
                  if ( temp1_ptr->prev_ptr != NULL )
                  {
                     temp1_ptr->prev_ptr->next_ptr = temp2_ptr;
                  }
                  else
                  {
                     list1_ptr->link.front_ptr = temp2_ptr;
                  }
                  temp3_ptr->next_ptr = temp1_ptr;
                  temp1_ptr->prev_ptr = temp3_ptr;
               }
            }
            break;

         case ORDERED_STREAMLIST_DESCENDING_PUSH_LTE:
            if ( list1_ptr->link.back_ptr->weight > list2_ptr->link.front_ptr->weight )
            {
               /* Case 1a: All items in list1 are greater than the
               ** biggest item in list2. Append list2 to list1.
               */
                ordered_StreamList_append( list1_ptr, list2_ptr );
                ORDERED_STREAMLIST_FREE(list1_ptr);
                return;
            }

            if ( list2_ptr->link.back_ptr->weight >= list1_ptr->link.front_ptr->weight )
            {
               /* Case 1b: All items in list2 are greater than or equal to
               ** the biggest item in list1. Append list1 to list2.
               */
               ordered_StreamList_swap( list1_ptr, list2_ptr );
               ordered_StreamList_append( list1_ptr, list2_ptr );
               ORDERED_STREAMLIST_FREE(list1_ptr);
               return;
            }

            /*
            ** Case 2: A simple append is not possible, so do a merge. Scan
            ** both lists, moving items from the second list to the first
            ** where appropriate.
            */
            temp1_ptr = list1_ptr->link.front_ptr;
            while ( list2_ptr->link.front_ptr != NULL )
            {
               temp2_ptr = temp3_ptr = list2_ptr->link.front_ptr;

               if ( list1_ptr->link.back_ptr->weight > temp2_ptr->weight )
               {
                  /* Append list2 to list1.
                  */
                  list1_ptr->link.back_ptr->next_ptr  = list2_ptr->link.front_ptr;
                  list2_ptr->link.front_ptr->prev_ptr = list1_ptr->link.back_ptr;
                  list1_ptr->link.back_ptr            = list2_ptr->link.back_ptr;
                  list2_ptr->link.front_ptr           = NULL;
               }
               else
               {
                  while ( temp1_ptr->weight > temp2_ptr->weight )
                  {
                     temp1_ptr = temp1_ptr->next_ptr;
                  }
                  if ( temp1_ptr->weight <= list2_ptr->link.back_ptr->weight )
                  {
                     temp3_ptr = list2_ptr->link.back_ptr;
                  }
                  else
                  {
                     while ( temp1_ptr->weight <= temp3_ptr->next_ptr->weight )
                     {
                        temp3_ptr = temp3_ptr->next_ptr;
                     }
                  }
                  list2_ptr->link.front_ptr           = temp3_ptr->next_ptr;
                  if ( list2_ptr->link.front_ptr != NULL )
                  {
                     list2_ptr->link.front_ptr->prev_ptr = NULL;
                  }
                  temp2_ptr->prev_ptr            = temp1_ptr->prev_ptr;
                  if ( temp1_ptr->prev_ptr != NULL )
                  {
                     temp1_ptr->prev_ptr->next_ptr = temp2_ptr;
                  }
                  else
                  {
                     list1_ptr->link.front_ptr = temp2_ptr;
                  }
                  temp3_ptr->next_ptr = temp1_ptr;
                  temp1_ptr->prev_ptr = temp3_ptr;
               }
            }
            break;

          default:
            ORDERED_STREAMLIST_FREE(list1_ptr);
            return;
      }

   list1_ptr->link.size    += list2_ptr->link.size;
   list2_ptr->link.size     = 0;
   list1_ptr->link.back_ptr = list2_ptr->link.back_ptr;
   list2_ptr->link.back_ptr = NULL;

   ORDERED_STREAMLIST_FREE(list1_ptr);

   return;
} /* END ordered_StreamList_merge */

#endif

/* ==================================================================
FUNCTION ORDERED_LIST_SPLIT
DESCRIPTION
   Splits an ordered list at a specified item. Items before the
   specified item are retained in the original list. The specified
   item and items following it are placed into a new list.
===================================================================== */
void ordered_StreamList_split(
   ordered_StreamList_type      *list1_ptr,
   ordered_StreamList_link_type *item_at_which_to_split_ptr,
   ordered_StreamList_type      *list2_ptr
)
{
   ordered_StreamList_link_type *temp_ptr   = NULL;
   unsigned long           count      = 0;
   unsigned long           total_size = list1_ptr->link.size;

   ORDERED_STREAMLIST_LOCK(list1_ptr);
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list1_ptr ) );
      STREAMLIST_ASSERT( ordered_StreamList_is_valid( list2_ptr ) );
      STREAMLIST_ASSERT( item_is_in_ordered_streamlist(
                                           list1_ptr,
                                           item_at_which_to_split_ptr
                   ) );

      /* Figure out the number of elements in the first list after the
      ** split.
      */
      if ( list1_ptr->link.front_ptr != item_at_which_to_split_ptr )
      {
         count++;
         temp_ptr = list1_ptr->link.front_ptr;
         while ( temp_ptr->next_ptr != item_at_which_to_split_ptr )
         {
            temp_ptr = temp_ptr->next_ptr;
            count++;
         }
         temp_ptr->next_ptr = NULL;
      }
      else
      {
         list1_ptr->link.front_ptr = NULL;
      }

      item_at_which_to_split_ptr->prev_ptr = NULL;
      list2_ptr->link.front_ptr = item_at_which_to_split_ptr;
      list2_ptr->link.back_ptr  = list1_ptr->link.back_ptr;
      list2_ptr->link.size      = total_size - count;
      list1_ptr->link.back_ptr  = temp_ptr;
      list1_ptr->link.size      = count;
   ORDERED_STREAMLIST_FREE(list1_ptr);

   return;
} /* END ordered_StreamList_split */


#ifdef LIST_DEBUG

/* ------------------------------------------------------------------------
**
** Sanity checks that are invoked in debug mode
**
** ------------------------------------------------------------------------ */

void StreamList_croak( )
{
   STREAMLIST_ASSERT( 0 );
} /* END StreamList_croak */


unsigned long
StreamList_is_valid(
   StreamList_type *StreamList_ptr
)
{
   StreamList_link_type *item_ptr;
   unsigned long   cur_size = 0;

   STREAMLIST_ASSERT( StreamList_ptr != NULL );

   item_ptr = StreamList_ptr->front_ptr;
   while( item_ptr != NULL )
   {
      cur_size++;
      if ( cur_size > StreamList_ptr->size )
      {
         return 0;
      }
      item_ptr = item_ptr->next_ptr;
   }

   if ( cur_size != StreamList_ptr->size )
   {
      return 0;
   }
   else
   {
      return 1;
   }
} /* END StreamList_is_valid */


unsigned long
item_is_in_streamlist(
   StreamList_type      *StreamList_ptr,
   StreamList_link_type *item_ptr
)
{
   StreamList_link_type *cur_item_ptr;

   STREAMLIST_ASSERT( StreamList_ptr != NULL );
   STREAMLIST_ASSERT( item_ptr != NULL );

   cur_item_ptr = StreamList_ptr->front_ptr;
   while( cur_item_ptr != NULL && cur_item_ptr != item_ptr )
   {
      cur_item_ptr = cur_item_ptr->next_ptr;
   }

   if ( cur_item_ptr == NULL )
   {
      return 0;
   }
   else
   {
      return 1;
   }
} /* END item_is_in_streamlist */


unsigned long
ordered_StreamList_is_valid( ordered_StreamList_type *StreamList_ptr )
{
   ordered_StreamList_link_type *item_ptr;
   unsigned long   cur_size = 0;

   STREAMLIST_ASSERT( StreamList_ptr != NULL );

   item_ptr = StreamList_ptr->front_ptr;
   while( item_ptr != NULL )
   {
      cur_size++;
      if ( cur_size > StreamList_ptr->size )
      {
         return 0;
      }
      item_ptr = item_ptr->next_ptr;
   }

   if ( cur_size != StreamList_ptr->size )
   {
      return 0;
   }
   else
   {
      return 1;
   }
} /* END ordered_StreamList_is_valid */


unsigned long
item_is_in_ordered_streamlist(
   ordered_StreamList_type      *StreamList_ptr,
   ordered_StreamList_link_type *item_ptr
)
{
   ordered_StreamList_link_type *cur_item_ptr;

   STREAMLIST_ASSERT( StreamList_ptr != NULL );
   STREAMLIST_ASSERT( item_ptr != NULL );

   cur_item_ptr = StreamList_ptr->front_ptr;
   while( cur_item_ptr != NULL && cur_item_ptr != item_ptr )
   {
      cur_item_ptr = cur_item_ptr->next_ptr;
   }

   if ( cur_item_ptr == NULL )
   {
      return 0;
   }
   else
   {
      return 1;
   }
} /* END item_is_in_ordered_streamlist */


#endif
