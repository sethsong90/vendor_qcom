#ifndef DSM_QUEUE_H
#define DSM_QUEUE_H
/*===========================================================================

                                  D S M _ Q U E U E . H

DESCRIPTION
  This file contains types and declarations associated with the DMSS Data
  Service Memory pool and services.

-----------------------------------------------------------------------------
Copyright (c) 2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "queue.h"
#include "dsm_lock.h"


/*===========================================================================
                        DATA DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
  Watermark Type definition. This structure provides the means for 2 software
  entities to convey data and control its flow. Note that the function
  pointer fields must 1st be checked to non-NULL prior to invocation.
---------------------------------------------------------------------------*/

#ifdef FEATURE_DSM_WM_CB
struct dsm_watermark_type_s; 
typedef void(*wm_cb_type)(struct dsm_watermark_type_s *, void *);
#else /* FEATURE_DSM_WM_CB */
typedef void(*wm_cb_type)(void);
#endif /* FEATURE_DSM_WM_CB */

typedef struct dsm_watermark_type_s
{
  uint32  lo_watermark;		/* Low watermark for total bytes */
  uint32  hi_watermark;		/* Hi watermark for total bytes */
  uint32  dont_exceed_cnt;	/* Discard count for RX bytes */
  uint32  current_cnt;		/* Current total byte count */
  uint32  highest_cnt;		/* largest count since powerupp */
  uint32  total_rcvd_cnt;	/* Total RX bytes thru watermark */

  wm_cb_type each_enqueue_func_ptr; /* Called when any item enqueued  */ 
  void * each_enqueue_func_data;

  wm_cb_type lowater_func_ptr;	/* Lo Watermark function pointer, Invoke
                                   function when queue hits the Lo watermrk*/
  void * lowater_func_data;

  wm_cb_type gone_empty_func_ptr; /* Gone empty function pointer, invoke when
                                     queue goes empty */
  void * gone_empty_func_data;

  wm_cb_type non_empty_func_ptr; /* Non-empty function pointer, Invoke when
                                    queue goes nonempty */
  void * non_empty_func_data;

  wm_cb_type hiwater_func_ptr;   /* Hi Watermark function pointer, Invoke
                                    function when queue hits the Hi wm     */
  void * hiwater_func_data;

  q_type *q_ptr;		 /* Pointer to associated queue */

  dsm_lock_type lock;            /* Lock for the queue */

} dsm_watermark_type;


/*---------------------------------------------------------------------------
  Priority Field Values
---------------------------------------------------------------------------*/
#ifdef DSM_PRIORITY
  #define DSM_NORMAL          0
  #define DSM_HIGHEST         0x7f
#endif
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/


/*================ QUEUE MANAGMENT FUNCTIONS ==============================*/
/*===========================================================================
FUNCTION DSM_QUEUE_INIT()

DESCRIPTION
   This function initalizes a watermark queue.  Setting all the callbacks and 
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
extern void dsm_queue_init
(
  dsm_watermark_type *wm_ptr,
  int dne,
  q_type * queue
);
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
  wm_ptr - Pinter to Watermark item to put to
  pkt_head_ptr - Pointer to pointer to item to add to queue

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
#define dsm_enqueue(wm_ptr,pkt_head_ptr) \
  dsmi_enqueue(wm_ptr,pkt_head_ptr,__FILE__,__LINE__)

extern void dsmi_enqueue
(
  dsm_watermark_type *wm_ptr,
  dsm_item_type **pkt_head_ptr,
  const char * file,
  uint32 line
);


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
#define dsm_simple_enqueue_isr(wm_ptr,pkt_head_ptr) \
  dsmi_simple_enqueue_isr(wm_ptr,pkt_head_ptr,__FILE__,__LINE__)

extern void dsmi_simple_enqueue_isr
(
  dsm_watermark_type *wm_ptr,
  dsm_item_type **pkt_head_ptr,
  const char * file,
  uint32 line
);


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
#define dsm_dequeue(wm_ptr) \
  dsmi_dequeue(wm_ptr,__FILE__,__LINE__)

extern dsm_item_type *dsmi_dequeue
(
  dsm_watermark_type *wm_ptr,
  const char * file, 
  uint32 line
);



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
#define dsm_empty_queue(wm_ptr) \
  dsmi_empty_queue(wm_ptr,__FILE__,__LINE__)

extern void dsmi_empty_queue
( 
  dsm_watermark_type *wm_ptr,
  const char * file,
  uint32 line
);

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
);

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
 TRUE if watermark has no data queued in it, FALSE if it does

SIDE EFFECTS
 None
===========================================================================*/
void dsm_set_low_wm
(
  dsm_watermark_type *wm_ptr,
  uint32             val
);

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
);

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
);

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
);

#endif /* DSM_QUEUE_H */
