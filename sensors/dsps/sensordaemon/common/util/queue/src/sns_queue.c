/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        Q U E U E    S E R V I C E S

GENERAL DESCRIPTION

  A queue is a simple data structure used for logically storing and re-
  trieving data blocks, in a first in - first out (FIFO) order without
  physically copying them. Software tasks and interrupt handlers can use
  queues for cleanly transferring data blocks to one another.

                        +-------+     +-------+     +-------+
                        | DATA  |     | DATA  |     | DATA  |
                        | BLOCK |     | BLOCK |     | BLOCK |
          +-------+     +-------+     +-------+     +-------+
          | QUEUE |---->| LINK  |---->| LINK  |---->| LINK  |---- |
   |----->+-------+     +-------+     +-------+     +-------+     |
   |      |       |     |       |     |       |     |       |     |
   |      +-------+     |       |     |       |     |       |     |
   |                    +-------+     +-------+     +-------+     |
   |                                                              |
   ---------------------------------------------------------------|

  The Queue Services provide a small set of declarations and functions for
  defining and initializing a queue, defining and initializing a links with-
  in a data block, placing a data block at the tail of a queue, removing a
  data block from the head of a queue, and removing a data block from any
  position in a queue.

  Aside from requiring each data block to contain a link, the Queue Services
  impose no restrictions on the size of structure of data blocks used with
  with queues. This allows software to pass virtually any type of data on
  queues. Notice that a data block may contain multiple links allowing it to
  be placed simultaneously on multiple queues.


EXTERNALIZED FUNCTIONS

  sns_q_init
    This function initializes a queue. It should be called on behalf of a
    queue prior to using the queue.

  sns_q_link
    This function initializes a link field. It should be called on behalf
    of a link field prior to using the link filed with the other Queue
    Services.

  sns_q_get
    This function removes the data block at head of a queue and returns a
    pointer to the data block. If the queue is empty, then a NULL pointer
    is returned.

  sns_q_check
    This function returns a pointer to the data block at the head of a queue.
    The data block is not removed from the queue. If the queue is empty, then
    a NULL pointer is returned.

  sns_q_last_get
    This function removes the data block at the tail of a queue and returns a
    pointer to the data block. If the queue is empty, then a NULL pointer
    is returned.

  sns_q_put
    This function places a data block at the tail of a queue.

  sns_q_cnt
    This function returns the number of items currently on a queue.

  sns_q_linear_search
    Performs a linear search of the queue calling a users callback on each
    nodal visit to determine whether or not the search should be terminated.

  sns_q_linear_delete
    Performs a linear traversal of the queue calling a compare callback on
    each node, should the compare callback indicate that the item should be
    deleted the item is removed from the queue and the users action callback
    is called passing to the callback the pointer to the now deleted item

INITIALIZATION AND SEQUENCING REQUIREMENTS

  Prior to use, a queue must be initialized by calling sns_q_init. Similarly,
  a link must be initialized prior to use by calling sns_q_link.

Copyright (c) 1990-2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================
                              Edit History

$PVCSPath: O:/src/asw/COMMON/vcs/queue.c_v   1.4   02 Oct 2001 10:46:56   rajeevg  $
$Header: //source/qcom/qct/core/pkg/dsps/dev/gulansey.gulansey_common_smr/adsp_proc/Sensors/common/util/queue/src/sns_queue.c#1 $ $DateTime: 2013/01/25 10:55:20 $ $Author: coresvc $

when       who    what, where, why
--------   ---    ----------------------------------------------------------
2012-04-09 sc     Removed macro FEATURE_Q_NO_SELF_QPTR and FEATURE_Q_SINGLE_LINK
2010-08-25 br     name changed from q_last_check to sns_q_last_check
2010-07-06 Br     Branch from //source/qcom/qct/core/services/utils/main/latest/src/queue.c#15.
                  Renamed types and functions to sns_q_x style.
                  Deleted lines for synchronization, q_m_put(), q_linear_delete_new(), and q_destroy()

===========================================================================*/

/* ------------------------------------------------------------------------
** Includes
** ------------------------------------------------------------------------ */
#include "sensor1.h"
#include "sns_queue.h"

#define SNS_ASSERT(x)
/*==========================================================================
FUNCTION sns_q_init

DESCRIPTION
  This function initializes a specified queue. It should be called for each
  queue prior to using the queue with the other Queue Services.

DEPENDENCIES
  None.

RETURN VALUE
  A pointer to the initialized queue.

SIDE EFFECTS
  The specified queue is initialized for use with Queue Services.
===========================================================================*/
sns_q_s* sns_q_init(
   sns_q_s  *q_ptr  /* Ptr to queue to be initialized. */
)
{
   SNS_ASSERT( q_ptr != NULL );
   q_ptr->link.next_ptr = (sns_q_link_s *)(&q_ptr->link);  /* Points to q link. */
   q_ptr->link.prev_ptr = (sns_q_link_s *)(&q_ptr->link);  /* Points to q link. */

   q_ptr->link.self_ptr = NULL;
   q_ptr->link.q_ptr    = NULL;

   q_ptr->cnt = 0;

   return q_ptr;
} /* END sns_q_init */



/*===========================================================================
FUNCTION sns_q_link

DESCRIPTION
  This function initializes a specified link. It should be called for each
  link prior to using the link with the other Queue Services.

DEPENDENCIES
  None.

RETURN VALUE
  A pointer to the initialized link.

SIDE EFFECTS
  The specified link is initialized for use with the Queue Services.
===========================================================================*/
sns_q_link_s* sns_q_link(
   void         *item_ptr,  /* Ptr to item or variable containing link. */
   sns_q_link_s *link_ptr   /* Ptr to link field within variable. */
)
{
   SNS_ASSERT( link_ptr != NULL );
   link_ptr->next_ptr = NULL;
   link_ptr->prev_ptr = NULL;
   link_ptr->self_ptr = item_ptr;
   link_ptr->q_ptr    = NULL;

   return link_ptr;
} /* END sns_q_link */

/*===========================================================================
FUNCTION sns_q_put

DESCRIPTION
  This function enqueues an item onto a specified queue using a specified
  link.

DEPENDENCIES
  The specified queue should have been previously initialized via a call
  to sns_q_init. The specified link field of the item should have been prev-
  iously initialized via a call to q_init_link.

RETURN VALUE
  None.

SIDE EFFECTS
  The specified item is placed at the tail of the specified queue.
===========================================================================*/
void sns_q_put(
   sns_q_s       *q_ptr,    /* Ptr to queue. */
   sns_q_link_s  *link_ptr  /* Ptr to item link to use for queueing. */
)
{

   SNS_ASSERT( q_ptr != NULL );
   SNS_ASSERT( link_ptr != NULL );
   link_ptr->next_ptr = (sns_q_link_s *)&q_ptr->link;

   link_ptr->q_ptr    = q_ptr;
   link_ptr->prev_ptr = q_ptr->link.prev_ptr;

   q_ptr->link.prev_ptr->next_ptr = link_ptr;
   q_ptr->link.prev_ptr           = link_ptr;
   q_ptr->cnt++;

   return;
} /* END sns_q_put */



/*===========================================================================
FUNCTION sns_q_get

DESCRIPTION
  This function removes an item from the head of a specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  A pointer to the dequeued item. If the specified queue is empty, then
  NULL is returned.

SIDE EFFECTS
  The head item, if any, is removed from the specified queue.
===========================================================================*/
void* sns_q_get(
  sns_q_s  *q_ptr  /* Ptr to queue. */
)
{
   sns_q_link_s  *link_ptr;

   SNS_ASSERT( q_ptr != NULL );

   /* Get ptr to 1st queue item.
   */
   link_ptr = q_ptr->link.next_ptr;

   /* Can only get an item if the queue is non empty
   */
   if( q_ptr->cnt > 0 )
   {
      q_ptr->link.next_ptr = link_ptr->next_ptr;

      link_ptr->next_ptr->prev_ptr = &q_ptr->link;

      q_ptr->cnt--;

      /* Mark item as no longer in a queue.
      */
      link_ptr->q_ptr = NULL;
   }

   return link_ptr->self_ptr;
} /* END sns_q_get */

/*===========================================================================
FUNCTION sns_q_last_get

DESCRIPTION
  This function returns the item which was most recently enqueued in a queue.

  Note, this is different from sns_q_get() which returns the oldest item in a
  queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void * sns_q_last_get(
   sns_q_s* q_ptr
)
{
   sns_q_link_s  *link_ptr;
   SNS_ASSERT( q_ptr != NULL );

   link_ptr = q_ptr->link.prev_ptr;

   if ( q_ptr->cnt > 0 )
   {
      q_ptr->link.prev_ptr         = link_ptr->prev_ptr;
      link_ptr->prev_ptr->next_ptr = &q_ptr->link;
      q_ptr->cnt--;
      link_ptr->q_ptr = NULL;
   }

   return link_ptr->self_ptr;
}  /* sns_q_last_get */

/*===========================================================================
FUNCTION SNS_Q_NEXT

DESCRIPTION
  This function returns a pointer to the next item on the queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  A pointer to the next item on the queue. If the end of the queue is reached,
  then NULL is returned.

SIDE EFFECTS
  None.
===========================================================================*/
void* sns_q_next(
   sns_q_s       *q_ptr,
   sns_q_link_s  *q_link_ptr
)
{
   void       *q_semp_ptr = NULL;
   SNS_ASSERT( q_link_ptr != NULL );


   if( (void *) q_link_ptr->next_ptr != (void *) q_ptr )
   {
     q_semp_ptr = q_link_ptr->next_ptr;
     return q_semp_ptr;
   }
   else
   {
     return NULL;
   }
} /* END sns_q_next */


/*===========================================================================
FUNCTION SNS_Q_INSERT

DESCRIPTION
  This function inserts an item before a specified item on a queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  None.

SIDE EFFECTS
  Input item is inserted before input item.
===========================================================================*/
void sns_q_insert(
   sns_q_link_s  *q_insert_ptr,   /* Ptr to link of item to insert */
   sns_q_link_s  *q_item_ptr      /* Ptr to link item to insert before */
)
{
   SNS_ASSERT( q_ptr        != NULL );
   SNS_ASSERT( q_insert_ptr != NULL );

   q_insert_ptr->next_ptr = q_item_ptr;
   q_insert_ptr->prev_ptr = q_item_ptr->prev_ptr;
   q_item_ptr->prev_ptr->next_ptr = q_insert_ptr;
   q_item_ptr->prev_ptr = q_insert_ptr;

   q_insert_ptr->q_ptr = q_item_ptr->q_ptr;
   q_item_ptr->q_ptr->cnt++;

   return;
} /* END sns_q_insert */

/*===========================================================================
FUNCTION SNS_Q_DELETE

DESCRIPTION
  This function removes an item from a specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  None.

SIDE EFFECTS
  Input item is delete from the queue.
===========================================================================*/
void sns_q_delete(
   sns_q_link_s  *q_delete_ptr   /* Ptr to link of item to delete */
)
{
   SNS_ASSERT( q_ptr        != NULL );
   SNS_ASSERT( q_delete_ptr != NULL );

   q_delete_ptr->prev_ptr->next_ptr = q_delete_ptr->next_ptr;
   q_delete_ptr->next_ptr->prev_ptr = q_delete_ptr->prev_ptr;
   q_delete_ptr->q_ptr->cnt--;

   /* make null after unlock */
   q_delete_ptr->q_ptr = NULL;
   return;
} /* END sns_q_delete */

/*===========================================================================
FUNCTION SNS_Q_DELETE_EXT

DESCRIPTION
  This function removes an item from a specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  FALSE : if the item is not found in the queue.
  TRUE  : if the item is found and removed from the queue.

SIDE EFFECTS
  Input item is deleted from the queue.
===========================================================================*/
int_fast8_t sns_q_delete_ext(
   sns_q_link_s  *q_delete_ptr   /* Ptr to link of item to delete */
)
{
   int_fast8_t item_in_q = FALSE;

   SNS_ASSERT( q_ptr        != NULL );
   SNS_ASSERT( q_delete_ptr != NULL );

   q_delete_ptr->prev_ptr->next_ptr = q_delete_ptr->next_ptr;
   q_delete_ptr->next_ptr->prev_ptr = q_delete_ptr->prev_ptr;

   q_delete_ptr->q_ptr->cnt--;
   q_delete_ptr->q_ptr = NULL;

   item_in_q = TRUE;
   return item_in_q;
} /* END sns_q_delete_ext */

/*===========================================================================
FUNCTION sns_q_check

DESCRIPTION
  This function returns a pointer to the data block at the head of the queue.
  The data block is not removed from the queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  A pointer to the queue item. If the specified queue is empty, then
  NULL is returned.

SIDE EFFECTS
  None
===========================================================================*/
void* sns_q_check(
  sns_q_s  *q_ptr
)
{
   sns_q_link_s  *link_ptr;
   SNS_ASSERT( q_ptr != NULL );

   link_ptr = q_ptr->link.next_ptr;
   return link_ptr->self_ptr;
} /* END sns_q_check */


/*===========================================================================

FUNCTION Q_LAST_CHECK

DESCRIPTION
  This function returns the item which was most recently enqueued in a queue.

  Note, this is different from sns_q_check() which returns the oldest item in a
  queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void * sns_q_last_check
(
  sns_q_s* q_ptr           /* The queue from which the item will be removed */
)
{
   sns_q_link_s  *link_ptr;                         /* For returning value. */

 /*- - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - -*/

   SNS_ASSERT( q_ptr != NULL );

   link_ptr = q_ptr->link.prev_ptr;

   return ( link_ptr->self_ptr );
}  /* q_last_check */


/*===========================================================================
FUNCTION sns_q_cnt

DESCRIPTION
  This function returns the number of items currently queued on a specified
  queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

RETURN VALUE
  The number of items currently queued on the specified queue.

SIDE EFFECTS
  None.
===========================================================================*/
int sns_q_cnt(
  sns_q_s  *q_ptr
)
{
   SNS_ASSERT( q_ptr != NULL );
   return q_ptr->cnt;
} /* END sns_q_cnt */


/*===========================================================================
FUNCTION sns_q_linear_search

DESCRIPTION
  Given a comparison function, this function traverses the elements in
  a queue, calls the compare function, and returns a pointer to the
  current element being compared if the user passed compare function
  returns non zero.

  The user compare function should return 0 if the current element is
  not the element in which the compare function is interested.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

  The user's queue elements must have sns_q_link_s as the first element
  of the queued structure.

RETURN VALUE
  A pointer to the found element

SIDE EFFECTS
  None.
===========================================================================*/
void* sns_q_linear_search(
   sns_q_s            *q_ptr,
   q_compare_func_t   compare_func,
   void               *compare_val
)
{
   q_generic_item_s *item_ptr = NULL;

   SNS_ASSERT( compare_func != NULL );
   item_ptr = (q_generic_item_s*)sns_q_check( q_ptr );

   while( item_ptr != NULL )
   {
      if( compare_func( item_ptr, compare_val ) != 0 )
      {
         return item_ptr;
      }
      item_ptr = (q_generic_item_s*)sns_q_next( q_ptr, &item_ptr->link );
   } /* END while traversing the queue */

   return NULL;
} /* END sns_q_linear_search */


/*===========================================================================
FUNCTION sns_q_linear_delete

DESCRIPTION
  Given a comparison function, this function traverses the elements in
  a queue, calls the compare function, and returns a pointer to the
  current element being compared if the user passed compare function
  returns non zero.  In addition, the item will be removed from the queue.

  The user compare function should return 0 if the current element is
  not the element in which the compare function is interested.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to sns_q_init.

  The user's queue elements must have sns_q_link_s as the first element
  of the queued structure.

  The user's compare function will be passed NULL for the compare value.

RETURN VALUE
  None

SIDE EFFECTS
  None.
===========================================================================*/
void sns_q_linear_delete(
   sns_q_s            *q_ptr,
   q_compare_func_t   compare_func,
   void               *param,
   q_action_func_t    action_func
)
{
   q_generic_item_s *item_ptr = NULL;
      /* Used in the traversal to point to the current item
      */
   q_generic_item_s *prev_ptr = NULL;
      /* Used in the traversal to point to the item previous to
      ** the current item.  This makes removing the current item
      ** a constant time operation
      */

   /* User must provide a compare function, otherwise, this is
   ** meaningless.
   */
   if( compare_func == NULL )
   {
      return;
   }


   /* item_ptr points to the first item on the list
   */
   item_ptr = (q_generic_item_s*)sns_q_check( q_ptr );
   prev_ptr = NULL;

   while( item_ptr != NULL )
   {
      if( compare_func( item_ptr, NULL ) != 0 )
      {
         /* Remove the item
         */
         if( prev_ptr != NULL )
         {
            /* Remove from the middle or tail
            */
            prev_ptr->link.next_ptr = item_ptr->link.next_ptr;
            item_ptr->link.next_ptr = NULL;
            q_ptr->cnt--;

            if( q_ptr->cnt == 1)
            {
              q_ptr->link.prev_ptr = q_ptr->link.next_ptr;
            }
         }
         else
         {
            /* Remove from the head
            */
            sns_q_get( q_ptr );
         }

         /* Call the action function if there is one
         */
         if( action_func )
         {
            action_func( item_ptr, param );
         }
         break;
      }

      /* Move on to the next item
      */
      prev_ptr = item_ptr;
      item_ptr = (q_generic_item_s*)sns_q_next( q_ptr, &item_ptr->link );
   } /* END while traversing the queue */

   return;

} /* END sns_q_linear_delete */
