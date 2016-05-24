#ifndef DSMI_H
#define DSMI_H
/*===========================================================================

                                  D S M I . H

DESCRIPTION
  This file contains types and declarations associated with the DMSS Data
  Service Memory pool and services.

-----------------------------------------------------------------------------
Copyright (c) 2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
===========================================================================*/


/*===========================================================================
                            EDIT HISTORY FOR FILE
                                      
  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsmi.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/01/05    pjb    Created
===========================================================================*/

#include "comdef.h"
#include "customer.h"


/*===========================================================================
                        DATA DECLARATIONS
===========================================================================*/

/* ------------- Runtime Debug Features ------------- */

#ifdef FEATURE_DSM_MEM_CHK
/* Only fill freed items with AA if this is non-zero. */
extern boolean DSM_Fill_AA;
#endif /* FEATURE_DSM_MEM_CHK */

/* --------- Cookies --------- */
/* Cookie must have this value */
#define DSM_COOKIE                             (0x4EAD4EAD)
#define DSM_TAIL_COOKIE                        (0xF007F007)
#define DSM_POOL_MGMT_TABLE_COOKIE             (0x7AB1E5E7)

#ifdef FEATURE_DSM_MEM_CHK
  #define DSM_TAIL_COOKIE_LEN                  (4) 
  #define DSM_HEAD_COOKIE_LEN                  (4)
#else
  #define DSM_TAIL_COOKIE_LEN                  (0) 
  #define DSM_HEAD_COOKIE_LEN                  (0)
#endif /* FEATURE_DSM_MEM_CHK */

#ifdef FEATURE_DSM_MEM_CHK_EXPENSIVE

/* Most EXPENSIVE memory checks require the basic DSM_MEM_CHK feature
 * be enabled
 */
#ifndef FEATURE_DSM_MEM_CHK
#define FEATURE_DSM_MEM_CHK
#endif

/* Expensive check for double free packet */
#ifndef FEATURE_DSM_MEM_CHK_EXPENSIVE_DOUBLE_FREE
#define FEATURE_DSM_MEM_CHK_EXPENSIVE_DOUBLE_FREE
#endif

/*  Verify all packets for all operations */
#ifndef FEATURE_DSM_MEM_CHK_EXPENSIVE_ALWAYS_VERIFY_PACKET
#define FEATURE_DSM_MEM_CHK_EXPENSIVE_ALWAYS_VERIFY_PACKET
#endif

/*  Verify packets on enqueue */
#ifndef FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_PACKET_ENQUEUE
#define FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_PACKET_ENQUEUE
#endif

/* Verify that IOV packets are not duplicates. This check
 * will fail on some targets
 */
//#define FEATURE_DSM_MEM_CHK_EXPENSIVE_IOV_DUP


/* Verify that wm and actual bytes on queue match up */
//#define FEATURE_DSM_MEM_CHK_EXPENSIVE_VERIFY_QUEUE_LENGTH

/* Verify that wm and actual bytes on queue match up */
//#define FEATURE_DSM_MEM_CHK_QUEUE_CACHE_LENGTH


#endif /* FEATURE_DSM_MEM_CHK_EXPENSIVE */

/* Pointer to list of memory pools */
extern dsm_pool_mgmt_table_type * dsm_pool_head_ptr;


/*===========================================================================
                      MACRO DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
MACRO DSMI_POOL_OBJECT_SIZE(item_size,pad_size)

DESCRIPTION
  This takes the size of an item and calculates the size of the whole 
  item with overhead.

  In DSM 3 this should be a multiple of 32.  Item size should already be a 
  multiple of 32. 
---------------------------------------------------------------------------*/
#define DSMI_POOL_OBJECT_SIZE(item_size, pad_size)\
 ( sizeof(dsm_item_type) +  \
   DSM_HEAD_COOKIE_LEN   +  \
   item_size             +  \
   DSM_TAIL_COOKIE_LEN   +  \
   pad_size )




/*---------------------------------------------------------------------------
MACRO DSMI_ITEM_HEAD(item_ptr)

DESCRIPTION
  Return the address of the first bit of data for this item
---------------------------------------------------------------------------*/
#define DSMI_ITEM_HEAD(item_ptr)                                            \
  ((uint8*)(((uint8 *)((item_ptr) + 1)) + DSM_HEAD_COOKIE_LEN))



/*---------------------------------------------------------------------------
MACRO DSMI_ITEM_TAIL(item_ptr)

DESCRIPTION
 Return the address just past the last bit of data for this item. 
---------------------------------------------------------------------------*/
#define DSMI_ITEM_TAIL(item_ptr)                                            \
  (DSMI_ITEM_HEAD(item_ptr) +                                               \
   DSM_POOL_ITEM_SIZE(DSM_ITEM_POOL(item_ptr)))



/*---------------------------------------------------------------------------
MACRO DSMI_DUP(item_ptr)

DESCRIPTION
 Return true if this item is from a DUP pool.
---------------------------------------------------------------------------*/
#define DSMI_DUP(item_ptr)                             \
  ( (item_ptr)->dup_ptr != NULL ||                     \
    DSM_POOL_ITEM_SIZE(DSM_ITEM_POOL(item_ptr)) == 0 )



/*---------------------------------------------------------------------------
MACRO DSMI_HEAD_SIZE(item_ptr)

DESCRIPTION
  Returns the size between the head pointer and the data pointer
---------------------------------------------------------------------------*/
#define DSMI_HEAD_SIZE(item_ptr) \
  ( (item_ptr)->data_ptr - DSMI_ITEM_HEAD(item_ptr) )



/*---------------------------------------------------------------------------
MACRO DSMI_TAIL_SIZE(item_ptr)

DESCRIPTION
  Returns the size of unused space between the current data and the end
  of the packet.
---------------------------------------------------------------------------*/
#define DSMI_TAIL_SIZE(item_ptr) \
  ( (DSMI_ITEM_TAIL(item_ptr) - (item_ptr)->data_ptr) - (item_ptr)->used )



/*---------------------------------------------------------------------------
MACRO DSMI_SIZE(item_ptr)

DESCRIPTION
  Returns the value in the local size field of the item, or what it should
  be if the size field is not defined for the dsm_item_type.
---------------------------------------------------------------------------*/
#define DSMI_SIZE(item_ptr) \
  ( DSMI_DUP(item_ptr) ? (item_ptr)->used : \
    ( DSMI_ITEM_TAIL(item_ptr) - (item_ptr)->data_ptr ) )



/*---------------------------------------------------------------------------
                            FORWARD DECLARATIONS
---------------------------------------------------------------------------*/
uint8 dsmi_ref_inc(dsm_item_type * item_ptr);
uint8 dsmi_ref_dec(dsm_item_type * item_ptr);
void dsmi_verify_packet(dsm_item_type * item_ptr);
void dsmi_verify_buffer(dsm_item_type * item_ptr);
void dsmi_verify_pool_id(dsm_mempool_id_type pool_id);
void dsmi_verify_mem_level(dsm_mem_level_enum_type mem_level);
void dsmi_verify_mem_op(dsm_mem_op_enum_type mem_op);

#endif /* DSMI_H */
