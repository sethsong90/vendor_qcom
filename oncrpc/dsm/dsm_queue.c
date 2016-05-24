/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                                  D S M _ Q U E U E . C

GENERAL DESCRIPTION
  DMSS Data Services memory pool module.

EXTERNALIZED FUNCTIONS

  dsm_dequeue()
    Dequeue the next buffer item from the passed Watermark structure. Perform
    relevent 'get' event functions if appropriate.

  dsm_enqueue()
    Enqueue the passed buffer item onto the passed Watermark structure. 
    Perform any relevent 'put' event functions as appropriate.

  dsm_empty_queue()
    completely empty the dsm watermark queue.

  dsm_simple_enqueue_isr()
    This function will put the passed DSM item to the passed shared queue 
    then check for and perform any 'put' events.  This function does not 
    check for priority.  It simply enqueues to the tail of the queue.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  dsm_init() must be called prior to any other DSM function.
  dsm_queue_init() must be called prior to using a DSM watermark queue.

-----------------------------------------------------------------------------
Copyright (c) 2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_queue.c#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/26/07    mjb    Added queue-specific locking mechanism.
06/29/06    mjb    Added file/line tracing,promoted internal uint16 to uint32
01/18/05    pjb    Added dsm_is_wm_empty
01/01/05    pjb    Created file
===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
/* Target-independent Include files */
#include "comdef.h"
#include "customer.h"
#include "queue.h"

#define FEATURE_DSM_WM_CB
#include "dsm_item.h"
#include "dsmi.h"
#include "dsm_lock.h"
#include "msg.h"
#include "err.h"
#include "memory.h"
#include "assert.h"
#include "dsm_queue.h"



/*===========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/
/*===========================================================================

                MACROS AND DEFINES

===========================================================================*/
#define DSM_WM_CB(CB_FPTR,WM_PTR,CB_DATA) do {\
  if(CB_FPTR != NULL) (CB_FPTR)(WM_PTR,CB_DATA);}while(0)

#define DSMI_QUEUE_LOCK_WM(wm_ptr) \
  DSM_LOCK( &((wm_ptr)->lock) )

#define DSMI_QUEUE_UNLOCK_WM(wm_ptr) \
  DSM_UNLOCK( &((wm_ptr)->lock) )

#define DSMI_QUEUE_LOCK_CREATE(wm_ptr) \
  DSM_LOCK_CREATE( &((wm_ptr)->lock) )


/*===========================================================================
FUNCTION DSMI_WM_CHECK_LEVELS()

DESCRIPTION
   Call Watermark item call back functions if water mark level change 
   appropriately.

DEPENDENCIES
  None

PARAMETERS
   wm_ptr - Pointer to the watermark item header.  This has the pointers
            to the callbacks and the callback data associated with this 
            watermark.
   initial_count - This is the size of this water mark queue before the 
                   "operation".  This function is called after the "operation"
                   has changed the count for the watermark.  
   enqueue - This boolean indicates that the each enqueue function should 
             be called.

RETURN VALUE
  None

SIDE EFFECTS
  One or more callbacks may get invoked.  Highest count may get updated 
  (If debugging is enabled, and the high watermark has gone up.)
===========================================================================*/
void
dsmi_wm_check_levels(dsm_watermark_type * wm_ptr, 
                     uint32 initial_count, 
                     boolean enqueue)
{
  if(initial_count == 0 &&
     wm_ptr->current_cnt > 0)
  {
    DSM_WM_CB(wm_ptr->non_empty_func_ptr,wm_ptr,wm_ptr->non_empty_func_data);
  }
  
  if(initial_count <= wm_ptr->hi_watermark &&
     wm_ptr->current_cnt > wm_ptr->hi_watermark)
  {
    DSM_WM_CB(wm_ptr->hiwater_func_ptr,wm_ptr,wm_ptr->hiwater_func_data);
  }
  
  if(enqueue) 
  {
    DSM_WM_CB(wm_ptr->each_enqueue_func_ptr,
              wm_ptr,wm_ptr->each_enqueue_func_data);
  }
  
  if(initial_count >= wm_ptr->lo_watermark && 
     wm_ptr->current_cnt < wm_ptr->lo_watermark)
  {
    DSM_WM_CB(wm_ptr->lowater_func_ptr,wm_ptr,
              wm_ptr->lowater_func_data);
  }
  
  if(initial_count != 0 && 
     wm_ptr->current_cnt == 0)
  {
    DSM_WM_CB(wm_ptr->gone_empty_func_ptr,wm_ptr,
              wm_ptr->gone_empty_func_data);
  }
  
 #ifdef FEATURE_DSM_MEM_CHK
  if(wm_ptr->current_cnt > wm_ptr->highest_cnt)
  {
    wm_ptr->highest_cnt = wm_ptr->current_cnt;
  }
 #endif /* FEATURE_DSM_MEM_CHK */  

} /* dsmi_wm_check_levels */

#ifdef FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH
/*===========================================================================
FUNCTION DSMI_WM_GET_COUNT()

DESCRIPTION
   Traverse the watermark queue and get the actual count of bytes on it.
   This is EXPENSIVE and is only used internally for debug.

DEPENDENCIES
  None

PARAMETERS
   wm_ptr - Pointer to the watermark item header.  This has the pointers
            to the callbacks and the callback data associated with this 
            watermark.

RETURN VALUE
  Count of bytes on watermark queue.

SIDE EFFECTS
  None.
===========================================================================*/
uint32
dsmi_wm_get_count(dsm_watermark_type * wm_ptr)
{
  uint32 count = 0;
  dsm_item_type * dsm_ptr;

  if( wm_ptr->q_ptr != NULL )
  {
    dsm_ptr = (dsm_item_type*)q_check( wm_ptr->q_ptr );
    while( dsm_ptr != NULL )
    {
      count += dsm_ptr->used;
      dsm_ptr = (dsm_item_type*)q_next(wm_ptr->q_ptr,&(dsm_ptr->link));
    }
  }
  return count;
} /* dsmi_wm_get_count */
#endif /* FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           EXTERNALIZED FUNTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION DSM_QUEUE_INIT()

DESCRIPTION
   This function initializes a watermark queue.  Setting all the callbacks and 
   callback data to NULL, watermark levels to 0, and initializing the queue 
   that this will use.  

DEPENDENCIES
   None

PARAMETERS
   wm_ptr - Pointer to the watermark to initialize
   dne - Do not exceed level for this watermark
   queue - Pointer to the queue header that this water mark should use

RETURN VALUE
   None

SIDE EFFECTS
   Queue is initialized
===========================================================================*/
void dsm_queue_init
(
  dsm_watermark_type *wm_ptr,
  int dne,
  q_type * queue
)
{
  ASSERT(wm_ptr != NULL);
  ASSERT(queue != NULL);        /* You must have a queue */

  memset(wm_ptr, 0, sizeof(dsm_watermark_type));
  wm_ptr->dont_exceed_cnt = dne;

  q_init(queue); 
  wm_ptr->q_ptr = queue;

  DSMI_QUEUE_LOCK_CREATE(wm_ptr);

}

/*===========================================================================
FUNCTION DSM_ENQUEUE()

DESCRIPTION
  This function will put the passed DSM item to the passed shared queue then
  check for and perform any 'put' events.

DEPENDENCIES
  1) Both parameters must NOT be NULL.
  2) The prioritized queuing will always put a DSM_HIGHEST priority item to
     the head of the queue.

PARAMETERS
  wm_ptr - Pointer to Watermark item to put to
  pkt_head_ptr - Pointer to pointer to item to add to queue

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsmi_enqueue
(
  dsm_watermark_type *wm_ptr,
  dsm_item_type **pkt_head_ptr,
  const char * file,
  uint32 line
)
{
  uint32 initial_count;          /* initial count of bytes in WM           */
  uint32 item_length;            /* length of new item to add              */

#ifdef DSM_PRIORITY
  dsm_item_type *insert_ptr;     /* pointer to 1st item in queue           */
#endif /* DSM_PRIORITY */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT(wm_ptr != NULL);
  ASSERT(wm_ptr->q_ptr != NULL);

  if((pkt_head_ptr == NULL) || (*pkt_head_ptr == NULL))
  {
#ifdef FEATURE_DSM_MEM_CHK
    ERR_FATAL("dsm_enqueue: Invalid Parameter", 0, 0, 0);
#else
    return;
#endif /*FEATURE_DSM_MEM_CHK */
  }

#ifdef FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_PACKET_ENQUEUE
  dsmi_verify_packet(*pkt_head_ptr);
#endif /* FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_PACKET_ENQUEUE */

#ifdef FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH
  DSMI_QUEUE_LOCK_WM(wm_ptr);
  ASSERT( dsmi_wm_get_count(wm_ptr) == wm_ptr->current_cnt );
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
#endif /* FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH */


#ifdef FEATURE_DSM_MEM_CHK
  dsmi_verify_pool_id((*pkt_head_ptr)->pool_id);

  if(wm_ptr->dont_exceed_cnt < 1)
  {
    ERR_FATAL("dsm_enqueue: Impossibly small dont exceed level",0,0,0);
  }
#endif

  /*-------------------------------------------------------------------------
    If the Watermark Item does not now have too many bytes in it then 
    proceed to stuff the passed item into it. Check 'priority'field to 
    determine whether or not the item should be put to front of queue.
  -------------------------------------------------------------------------*/
  item_length = dsm_length_packet(*pkt_head_ptr);
  if((wm_ptr->current_cnt + item_length) <= wm_ptr->dont_exceed_cnt)
  {
    DSMI_QUEUE_LOCK_WM(wm_ptr);
    initial_count = wm_ptr->current_cnt;
    wm_ptr->total_rcvd_cnt += item_length;

#ifdef FEATURE_DSM_MEM_CHK
    dsmi_touch_item(*pkt_head_ptr,file,line);
#endif /* FEATURE_DSM_MEM_CHK */

#ifdef FEATURE_DSM_MEM_CHK_QUEUE_CACHE_LENGTH
    (*pkt_head_ptr)->enqueued_packet_length = item_length;
#endif

#ifdef DSM_PRIORITY
    if((*pkt_head_ptr)->priority == DSM_HIGHEST && q_cnt(wm_ptr->q_ptr) != 0)
    {
      insert_ptr = (dsm_item_type *)q_check( wm_ptr->q_ptr);
    /*-----------------------------------------------------------------------
      The following functionality ensures that a message with dsm_highest
      is not added to a queue before another message with dsm_highest 
      priority. (ds_to_ps_q was in mind when adding this functionality).
    -----------------------------------------------------------------------*/
      while((insert_ptr != NULL) && (insert_ptr->priority == DSM_HIGHEST))
      {
        /*------------------------------------------------------------------- 
          q_next returns next item in q or null if end of queue reached 
        -------------------------------------------------------------------*/
        insert_ptr = (dsm_item_type *)q_next(wm_ptr->q_ptr, 
                                             &insert_ptr->link);
      }

      if(insert_ptr == NULL)
      {
        /*-------------------------------------------------------------------
          Puts item at the tail of the queue if all the items in the queue
          are dsm_highest.
        -------------------------------------------------------------------*/
        q_put(wm_ptr->q_ptr, &(*pkt_head_ptr)->link);
      }
      else
      {
        /*-------------------------------------------------------------------
          Inserts item before the last non-highest item on the queue.
        -------------------------------------------------------------------*/
#ifdef FEATURE_Q_NO_SELF_QPTR
         q_insert(wm_ptr->q_ptr, 
                  &(*pkt_head_ptr)->link,
                  &insert_ptr->link);
#else
         q_insert(&(*pkt_head_ptr)->link, 
                  &insert_ptr->link);
#endif

      }

    }  /* if (*pkt_head_ptr.........)*/
    else 
#endif /* DSM_PRIORITY */
    {
      /*--------------------------------------------------------------------- 
        Put to queue at end of list.
      ---------------------------------------------------------------------*/
      q_put(wm_ptr->q_ptr, &(*pkt_head_ptr)->link);
    }
    wm_ptr->current_cnt += item_length;

    /*-----------------------------------------------------------------------
      Now check for the Non-empty and Hi-watermark events.
    -----------------------------------------------------------------------*/
    dsmi_wm_check_levels(wm_ptr, initial_count, TRUE);

    *pkt_head_ptr = NULL;

    DSMI_QUEUE_UNLOCK_WM(wm_ptr);
  }
  else 
  {
    /*----------------------------------------------------------------------- 
      Display message, put item to free queue. 
    -----------------------------------------------------------------------*/
    ERR("WM full,freeing packet 0x%x:Watermark 0x%x:Tried %d",
        ((int) *pkt_head_ptr),
        ((int) wm_ptr),
        ((int) item_length)
        );

    (void)dsmi_free_packet(pkt_head_ptr,file,line);
  }
} /* dsmi_enqueue() */

/*===========================================================================
FUNCTION DSM_SIMPLE_ENQUEUE_ISR()

DESCRIPTION
  This function will put the passed DSM item to the passed shared queue then
  check for and perform any 'put' events.  This function does not check
  for priority.  It simply enqueues to the tail of the queue.

DEPENDENCIES
  1) Both parameters must NOT be NULL.
  2) Does not support packet chaining.
  3) Should only be called from ISR or from within critical section in which
     interrupts are disabled.

PARAMETERS
  wm_ptr - Pointer to watermark to put to
  pkt_head_ptr - Pointer to pointer to item to put.

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsmi_simple_enqueue_isr
(
  dsm_watermark_type *wm_ptr,
  dsm_item_type **pkt_head_ptr,
  const char * file,
  uint32 line
)
{
  dsm_item_type *temp_ptr;      /* pointer to 1st item in queue */
  uint32 initial_count;		/* initial count of bytes in WM */
  uint32 item_length;		/* length of new item to add */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/  
  ASSERT(pkt_head_ptr != NULL && *pkt_head_ptr != NULL);
  ASSERT(wm_ptr != NULL);
  ASSERT(wm_ptr->q_ptr != NULL);

#ifdef FEATURE_DSM_MEM_CHK
  dsmi_verify_pool_id((*pkt_head_ptr)->pool_id);
#endif

  temp_ptr = *pkt_head_ptr;

  ASSERT(temp_ptr->pkt_ptr == NULL);

  /*-------------------------------------------------------------------------
    If the Watermark Item does not now have too many bytes in it then 
    proceed to stuff the passed item into it. Check 'priority'field to 
    determine whether or not the item should be put to front of queue.
  -------------------------------------------------------------------------*/
  /* promote from 16bit to 32bit */
  item_length = temp_ptr->used;

  DSMI_QUEUE_LOCK_WM(wm_ptr);
  if((wm_ptr->current_cnt + item_length) <= wm_ptr->dont_exceed_cnt)
  {
    initial_count = wm_ptr->current_cnt;
    wm_ptr->total_rcvd_cnt += item_length;

#ifdef FEATURE_DSM_MEM_CHK
    dsmi_touch_item(*pkt_head_ptr,file,line);
#endif /* FEATURE_DSM_MEM_CHK */

#ifdef FEATURE_DSM_MEM_CHK_QUEUE_CACHE_LENGTH
    (*pkt_head_ptr)->enqueued_packet_length = item_length;
#endif

    q_put(wm_ptr->q_ptr, &(temp_ptr->link));
    wm_ptr->current_cnt += item_length;

    /*-----------------------------------------------------------------------
      Now check for the Non-empty and Hi-watermark events.
    -----------------------------------------------------------------------*/
    dsmi_wm_check_levels(wm_ptr, initial_count, TRUE); 
  }
  else 
  {
    /*----------------------------------------------------------------------- 
      Display message, put item to free queue 
    -----------------------------------------------------------------------*/
    ERR("WM full,freeing packet 0x%x:Watermark 0x%x:Tried %d",
        ((int) *pkt_head_ptr),
        ((int) wm_ptr),
        ((int) item_length)
        );
    (void)dsmi_free_buffer(*pkt_head_ptr,file,line);
  }
  *pkt_head_ptr = NULL;
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);

} /* dsm_simple_enqueue_isr() */

/*===========================================================================
FUNCTION DSM_DEQUEUE()

DESCRIPTION
  This function will return a pointer to the next item on the shared queue
  associated with the passed Watermark item. This function will also update
  the 'current_cnt' field in the passed Watermark item and check for and
  perform any relevent 'get' events.

DEPENDENCIES
  The parameter must NOT be NULL.

PARAMETERS
  wm_ptr - Pointer to watermark item to get item from 

RETURN VALUE
  A pointer to a 'dsm_item_type' or NULL if no item_array available.

SIDE EFFECTS
  None
===========================================================================*/
dsm_item_type *dsmi_dequeue
(
 dsm_watermark_type *wm_ptr,
 const char * file, 
 uint32 line
)
{
  uint32 initial_count;		/* initial number of bytes in watermark */
  dsm_item_type *item_ptr;      /* pointer to item to retrieve */
  uint32 item_length;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ASSERT(wm_ptr != NULL);
  ASSERT(wm_ptr->q_ptr != NULL);

  DSMI_QUEUE_LOCK_WM(wm_ptr);

#ifdef FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH
  ASSERT( dsmi_wm_get_count(wm_ptr) == wm_ptr->current_cnt );
#endif /* FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH */

  if((item_ptr = (dsm_item_type *)q_get(wm_ptr->q_ptr)) != NULL)
  {
    /*-----------------------------------------------------------------------
      First get current count then get the item and update current count.
      Check for and perform Lo water and Gone Empty events if appropriate.
    -----------------------------------------------------------------------*/
#ifdef FEATURE_DSM_MEM_CHK
    dsmi_touch_item(item_ptr,file,line);
#endif /* FEATURE_DSM_MEM_CHK */
    
    initial_count = wm_ptr->current_cnt;
    item_length = dsm_length_packet(item_ptr);
    wm_ptr->current_cnt -= item_length;

#ifdef FEATURE_DSM_MEM_CHK_QUEUE_CACHE_LENGTH
    ASSERT( item_ptr->enqueued_packet_length == item_length );
    item_ptr->enqueued_packet_length = 0;
#endif

    dsmi_wm_check_levels(wm_ptr, initial_count, FALSE);
  }
  else
  {
    ASSERT(wm_ptr->current_cnt == 0);
  }
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
  return item_ptr;

} /* dsm_dequeue() */

/*===========================================================================
FUNCTION DSM_EMPTY_QUEUE()

DESCRIPTION
  This routine completely empties a queue.
  
DEPENDENCIES
  None

PARAMETERS
  wm_ptr - Pointer to watermark queue to empty

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsmi_empty_queue
( 
  dsm_watermark_type *wm_ptr,
  const char * file,
  uint32 line
)
{
  dsm_item_type *item_ptr;	

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ASSERT(wm_ptr != NULL);
  ASSERT(wm_ptr->q_ptr != NULL);

  while((item_ptr = dsmi_dequeue(wm_ptr,file,line)) != NULL)
  {
    (void)dsmi_free_packet(&item_ptr,file,line);
  } 
} /* dsm_empty_queue() */


/*===========================================================================
FUNCTION DSM_IS_WM_EMPTY()

DESCRIPTION
 This routine determines whether the input watermark has data queued in
 it or not.

DEPENDENCIES
 None

PARAMETERS
 wm_ptr - Pointer to a watermark

RETURN VALUE
 TRUE if watermark has no data queued in it, FALSE if it does

SIDE EFFECTS
 None
===========================================================================*/
boolean dsm_is_wm_empty
(
 dsm_watermark_type *wm_ptr
)
{
 ASSERT(wm_ptr != NULL);
 ASSERT(wm_ptr->q_ptr != NULL);

 return (boolean) (q_cnt (wm_ptr->q_ptr) == 0);
} /* dsm_is_wm_empty */


/*===========================================================================
FUNCTION DSM_SET_LOW_WM()

DESCRIPTION
 This routine resets the low watermark value. This change may trigger
 watermark callbacks.

DEPENDENCIES
 None

PARAMETERS
 wm_ptr - Pointer to a watermark
 val    - New value for low watermark.

RETURN VALUE
 None.

SIDE EFFECTS
 None
===========================================================================*/
void dsm_set_low_wm
(
  dsm_watermark_type *wm_ptr,
  uint32             val
)
{
  DSMI_QUEUE_LOCK_WM(wm_ptr);
  if(wm_ptr->current_cnt >= wm_ptr->lo_watermark &&
     wm_ptr->current_cnt < val )
  {
    DSM_WM_CB(wm_ptr->lowater_func_ptr,wm_ptr,wm_ptr->lowater_func_data);
  }
  wm_ptr->lo_watermark = val;
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
} /* dsm_set_low_wm */

/*===========================================================================
FUNCTION DSM_SET_HI_WM()

DESCRIPTION
 This routine resets the high watermark value. This change may trigger
 watermark callbacks.

DEPENDENCIES
 None

PARAMETERS
 wm_ptr - Pointer to a watermark
 val    - New value for hi watermark.

RETURN VALUE
 None.

SIDE EFFECTS
 WB callback triggered. Function locks the context for the extent of the
 callback.
===========================================================================*/
void dsm_set_hi_wm
(
  dsm_watermark_type *wm_ptr,
  uint32             val
)
{
  DSMI_QUEUE_LOCK_WM(wm_ptr);  
  if(wm_ptr->current_cnt <= wm_ptr->hi_watermark &&
     wm_ptr->current_cnt > val )
  {
    DSM_WM_CB(wm_ptr->hiwater_func_ptr,wm_ptr,wm_ptr->hiwater_func_data);
  }
  wm_ptr->hi_watermark = val;
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
} /* dsm_set_hi_wm */

/*===========================================================================
FUNCTION DSM_SET_DNE()

DESCRIPTION
 This routine resets the DNE (do not exceed) value.
 
DEPENDENCIES
 None

PARAMETERS
 wm_ptr - Pointer to a watermark
 val    - New value for hi watermark.

RETURN VALUE
 None.

SIDE EFFECTS
 WB callback triggered. Function locks the context for the extent of the
 callback.
===========================================================================*/
void dsm_set_dne
(
  dsm_watermark_type *wm_ptr,
  uint32             val
)
{
  DSMI_QUEUE_LOCK_WM(wm_ptr);  
  wm_ptr->dont_exceed_cnt = val;
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
}

/*===========================================================================
FUNCTION DSM_QUEUE_CNT()

DESCRIPTION
 Returns the number of bytes on the watermark queue.
 
DEPENDENCIES
 None

PARAMETERS
 wm_ptr - Pointer to a watermark

RETURN VALUE
 Number of bytes recorded on queue.

SIDE EFFECTS
 None.
===========================================================================*/
uint32 dsm_queue_cnt
(
  dsm_watermark_type *wm_ptr
)
{
  uint32 ret;
  DSMI_QUEUE_LOCK_WM(wm_ptr);
  ret = wm_ptr->current_cnt;
  DSMI_QUEUE_UNLOCK_WM(wm_ptr);
  return ret;
}
