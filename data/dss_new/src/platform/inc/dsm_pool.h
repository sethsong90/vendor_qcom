#ifndef DSM_POOL_H
#define DSM_POOL_H
/*===========================================================================

                                  D S M _ P O O L . H

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
                                      
  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_pool.h#3 $

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "dsm_lock.h"


/*===========================================================================
                        MACRO DECLARATIONS
===========================================================================*/

#ifdef __GNUC__
#define DSM_ALIGNED_ALLOC(name,size) \
  static uint8 name[size] __attribute__ ((aligned(32)))
#elif defined (__arm)
#define DSM_ALIGNED_ALLOC(name,size) \
  static uint8 __align((32)) name[size]
#elif defined FEATURE_WINCE
#define DSM_ALIGNED_ALLOC(name,size) \
  __declspec(align(32)) static uint8 name[size]
#else
#define DSM_ALIGNED_ALLOC(name,size) \
  static uint32 name[(size + 31) /  sizeof(uint32)]
#endif

#define DSMI_POOL_LOCK( pool ) \
  DSM_LOCK( &(((dsm_pool_mgmt_table_type*)(pool))->lock) )

#define DSMI_POOL_UNLOCK( pool ) \
  DSM_UNLOCK( &(((dsm_pool_mgmt_table_type*)(pool))->lock) )


/*===========================================================================
                        DATA DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
  Predeclaration of dsm_item_type.
---------------------------------------------------------------------------*/

struct dsm_item_s; 

typedef struct dsm_item_s dsm_item_type;
typedef int dsm_mempool_id_type;

/*  This is not an ENUM. It is called this for historical reasons. */
typedef int dsm_mempool_id_enum_type;


/*---------------------------------------------------------------------------
  Types of memory operations, used to associate a callback with a particular
  operation.
---------------------------------------------------------------------------*/

typedef enum
{
  DSM_MEM_OP_MIN     = -1,
  DSM_MEM_OP_NEW     =  0,
  DSM_MEM_OP_FREE    =  1,
  DSM_MEM_OP_MAX     =  2
} dsm_mem_op_enum_type;

/*---------------------------------------------------------------------------
  Callbacks can be registered for the following levels. One can think of a
  level as an index which lets DSM know where to insert the callback
  entry. The free_count associated with these levels should be in 
  non-descending order, i.e., free_count for FEW level >= free_count for DNE 
  level and so on. 
---------------------------------------------------------------------------*/
typedef enum
{
  DSM_MEM_LEVEL_INVALID         = -2, 
  DSM_MEM_LEVEL_MIN             = -1,
  DSM_MEM_LEVEL_DNE             = 0,
  DSM_MEM_LEVEL_LINK_LAYER_DNE  = 1,
  DSM_MEM_LEVEL_TRANSPORT_LAYER_FEW = 2,
  DSM_MEM_LEVEL_LINK_LAYER_FEW  = 3,
  DSM_MEM_LEVEL_HDR_FEW         = 4, 
  DSM_MEM_LEVEL_RLC_FEW         = 5,
  DSM_MEM_LEVEL_FEW             = 6,
  DSM_MEM_LEVEL_SIO_FEW         = 7,
  DSM_MEM_LEVEL_HDR_MANY        = 8,
  DSM_MEM_LEVEL_RLC_MANY        = 9,
  DSM_MEM_LEVEL_MANY            = 10,
  DSM_MEM_LEVEL_SIO_MANY        = 11,
  DSM_MEM_LEVEL_MAX 
} dsm_mem_level_enum_type;

/*---------------------------------------------------------------------------
  DSM uses the following struct to store information about memory event
  callbacks. A maximum of two callbacks can be associated at a particular
  level, one for free operation and the other for new. If more callbacks
  need to be registered for the same free count, one needs to add another
  level. 
---------------------------------------------------------------------------*/
typedef struct
{
  dsm_mem_level_enum_type level;
  uint16 avail_item_count;        /* cb is called when free_count equals   */
                                  /* this count                            */
  void   (*mem_free_event_cb) (dsm_mempool_id_type, 
                               dsm_mem_level_enum_type,
                               dsm_mem_op_enum_type);
                                  /* the cb associated with free operation */
  void   (*mem_new_event_cb)  (dsm_mempool_id_type, 
                               dsm_mem_level_enum_type,
                               dsm_mem_op_enum_type);
                                  /* the cb associated with new operation  */
} dsm_mem_event_cb_entry;



/*---------------------------------------------------------------------------
  Type for the Table definition for managing the memory free pools.

  free_count: The current number of available memory pool items.

  next_level: The next level for which the next callback is to be called.
  We follow the convention that next_level points to the level in the
  direction of lower memory. For example, if level 1 corresponds to 
  free count of 5, level 2 to 10 and current free_count is 8 then next_level
  would be 1. In other words, we call the cb at next_level for new operations
  and at next_level+1 for free operations.

  mem_event_cb_list: The list of callbacks to be invoked when a particular
  memory event occurs, i.e., free_count drops to or reaches a particular 
  level.
---------------------------------------------------------------------------*/
typedef struct dsm_pool_mgmt_table_s
{
  int cookie;                   /* Magic Cookie                            */ 
  uint8 *item_array;		/* Pointer to Memory Array                 */
  dsm_item_type **free_stack;   /* Pointer to the free stack               */
  uint16  free_count;		/* The current number of items in Free     */
                                /* Queue                                   */
  uint16  item_count;		/* The max number of items in the pool     */
  uint16  pool_item_size;	/* Size of items for the pool              */
  uint16  pad_size;             /* Size of pad,also size of metadata       */
  
  uint16  stats_min_free_count;	/* Minimum count the free pool dropped to  */

  dsm_mem_level_enum_type  next_level; 
  dsm_mem_event_cb_entry mem_event_cb_list[DSM_MEM_LEVEL_MAX];

  /* free callback */
  void (*free_cb) (void * user_data,dsm_item_type * item);
  void * free_cb_user_data;

  struct dsm_pool_mgmt_table_s * next;   /* pointer to next pool */
  char * desc;

  const char * file;
  uint32 line;

  dsm_lock_type  lock;   /* lock for pool */
  
} dsm_pool_mgmt_table_type;



/*---------------------------------------------------------------------------
  Memory level macros.... 
---------------------------------------------------------------------------*/
#define DSM_MORE_THAN_MANY_FREE_ITEMS(pool_id)                              \
 (DSM_POOL_FREE_CNT(pool_id) >=                                             \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_MANY))

#define DSM_LESS_THAN_MANY_FREE_ITEMS(pool_id)                              \
 (DSM_POOL_FREE_CNT(pool_id) <                                              \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_MANY))

#define DSM_LESS_THAN_FEW_FREE_ITEMS(pool_id)                               \
 (DSM_POOL_FREE_CNT(pool_id) <                                              \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_FEW))

#define DSM_LESS_THAN_LINK_LAYER_FEW_FREE_ITEMS(pool_id)                    \
 (DSM_POOL_FREE_CNT(pool_id) <                                              \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_LINK_LAYER_FEW))

#define DSM_LESS_THAN_TRANSPORT_LAYER_FEW_FREE_ITEMS(pool_id)               \
 (DSM_POOL_FREE_CNT(pool_id) <                                              \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_TRANSPORT_LAYER_FEW))

#define DSM_DONT_EXCEED_ITEMS(pool_id)                                      \
 (DSM_POOL_FREE_CNT(pool_id) <=                                             \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_DNE))

#define DSM_LINK_LAYER_DONT_EXCEED_ITEMS(pool_id)                           \
 (DSM_POOL_FREE_CNT(pool_id) <                                              \
    DSM_POOL_MEM_EVENT_LEVEL(pool_id,DSM_MEM_LEVEL_LINK_LAYER_DNE))

/*---------------------------------------------------------------------------
  Macro to return the size of items in a given dsm pool.
---------------------------------------------------------------------------*/
#define DSM_POOL_ITEM_SIZE(pool_id)                                         \
  (((dsm_pool_mgmt_table_type*)pool_id)->pool_item_size)

/*---------------------------------------------------------------------------
  This MACRO will return free count for the givin pool.
---------------------------------------------------------------------------*/
#define DSM_POOL_FREE_CNT(pool_id)                                          \
  (((dsm_pool_mgmt_table_type*)pool_id)->free_count)

/*---------------------------------------------------------------------------
  This MACRO will return item count for the given pool.
---------------------------------------------------------------------------*/
#define DSM_POOL_ITEM_CNT(pool_id)                                          \
  (((dsm_pool_mgmt_table_type*)pool_id)->item_count)


/*---------------------------------------------------------------------------
  This Macro will return the level for a given memory pool and memory level. 
  This level is the absolute count of number of dsm items.
---------------------------------------------------------------------------*/
#define DSM_POOL_MEM_EVENT_LEVEL(pool_id,lvl)                               \
  dsm_pool_mem_event_level(pool_id,lvl)

/*---------------------------------------------------------------------------
  This MACRO will return the pad size for the given pool.
  This works for ANY pool, ANY pool can have padding for alignment, but
  only DUP pools allow meta data.
---------------------------------------------------------------------------*/
#define DSM_POOL_PAD_SIZE(pool_id)                                          \
  (((dsm_pool_mgmt_table_type*)pool_id)->pad_size)
    
/* External Data Declarations */
extern dsm_pool_mgmt_table_type * dsm_pool_head_ptr;
				     
/*===========================================================================
MACRO DSM_UNREG_MEM_EVENT_CB()

DESCRIPTION
  This macro will call dsm_reg_mem_event_cb() to unregister the callback 
  registered at the specified level for the given memory operation in the 
  specified memory pool.

  Example: One can unregister the new callback for the FEW level for small
  item pool as follows.

  dsm_unreg_mem_event_cb( DSM_DS_SMALL_ITEM_POOL, 
                          DSM_MEM_LEVEL_FEW,
                          DSM_MEM_OP_NEW );

DEPENDENCIES
  None

PARAMETERS
  pool_id - Which memory pool to unregister from.
  level - The level for which the the callback is to be unregistered.
  mem_op - The memory operation (free/new) for which the callback is to 
  be unregistered.

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
#define dsm_unreg_mem_event_cb(pool_id, level, mem_op)                      \
  dsm_reg_mem_event_cb(pool_id, level, mem_op, NULL);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*===========================================================================
                      FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION DSM_INIT_POOL()

DESCRIPTION
  Initialize a single data pool.  

DEPENDENCIES
  None

PARAMETERS
  pool_id - The identifier for the pool that should be initialized. 
  item_array - A pointer to a block of memory to be used for this pool.
  item_array_size - The size of the block of memory passed in item_array
  item_size - The size of the items that this pool should provide

RETURN VALUE
  An integer indicating the NUMBER of items that are available in this pool 
  after the "item_array" block of memory is split up inte "item_size" chunks
  plus overhead. 

SIDE EFFECTS
  None
===========================================================================*/
#define dsm_init_pool(pool_id,item_array,item_array_size,item_size)   \
  dsmi_init_pool_desc(pool_id,item_array,item_array_size,item_size,   \
                      0,NULL,__FILE__,__LINE__)

#define dsm_init_pool_desc(pool_id,item_array,item_array_size,        \
                           item_size,desc)                            \
  dsmi_init_pool_desc(pool_id,item_array,item_array_size,item_size,   \
                      0,desc,__FILE__,__LINE__)

#define dsm_init_pool_meta(pool_id,item_array,item_array_size,          \
                           meta_size,desc)                              \
  dsmi_init_pool_desc(pool_id,item_array,item_array_size,               \
                      0,meta_size,desc,__FILE__,__LINE__)


int dsmi_init_pool_desc
(
 dsm_mempool_id_type pool_id,
 uint8 * item_array,
 uint32 item_array_size,
 uint16 item_size,
 uint16 meta_size,
 char * desc,
 const char * file,
 uint32 line
);


/*===========================================================================
FUNCTION DSM_REG_MEM_EVENT_CB()

DESCRIPTION
  This function will register the passed function pointer to be later 
  invoked if the specified event occurs for the specified memory pool.
  
  Example: One can register dsi_mem_event_ctrl() function of dsmgr as
  a callback to be invoked when the memory level drops to DSM_MEM_LEVEL_FEW
  during memory allocations from small item pool as follows. The free count
  associated with DSM_MEM_LEVEL_FEW is assigned as part of initialization 
  and it cannot be changed dynamically. 

  dsm_reg_mem_event_cb
  (
    DSM_DS_SMALL_ITEM_POOL,
    DSM_MEM_LEVEL_FEW,
    DSM_MEM_OP_NEW,
    dsi_mem_event_ctrl    
  );

DEPENDENCIES
  None

PARAMETERS
  pool_id - Which memory pool to register with.
  level - The level associated with this callback. 
  mem_op - The memory operation (new/free) the callback is associated with.
  mem_event_cb - Pointer to function to be registered as a callback.

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
extern void dsm_reg_mem_event_cb
(
  dsm_mempool_id_type pool_id,
  dsm_mem_level_enum_type  level,
  dsm_mem_op_enum_type     mem_op,
  void                     (*mem_event_cb) (dsm_mempool_id_type,
                                            dsm_mem_level_enum_type,
                                            dsm_mem_op_enum_type) 
);

/*===========================================================================
FUNCTION DSM_REG_MEM_EVENT_LEVEL()

DESCRIPTION
  This function will set the avail_item_count for a given memory level.  

DEPENDENCIES
  None

PARAMETERS
  pool_id - Which memory pool to register with.
  level - The level we want to register.. 
  avail_item_count - The count for this level

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/

extern void dsm_reg_mem_event_level
(
  dsm_mempool_id_type pool_id,
  dsm_mem_level_enum_type level,
  uint32 avail_item_count
);

/*===========================================================================
FUNCTION DSM_POOL_MEM_EVENT_LEVEL()

DESCRIPTION
  This function returns the memory level in bytes for an enum level for
  a memory pool.

DEPENDENCIES
  None

PARAMETERS
  pool_id - Which memory pool to search through.
  level   - The level we want to index

RETURN VALUE
  Memory level in bytes for the given level_type

SIDE EFFECTS
  None
===========================================================================*/
extern uint32 dsm_pool_mem_event_level
(
 dsm_mempool_id_type pool_id,
 dsm_mem_level_enum_type level
);

/*===========================================================================
FUNCTION DSM_REMOVE_POOL()

DESCRIPTION
  Removes a DSM pool from the linked list and resets cookie and rest of
  pool management table content. It does NOT free any DSM memory itself.
  It's up to the user to free up the buffer memory.

DEPENDENCIES
  None

PARAMETERS
  pool_id - Which memory pool to remove.

RETURN VALUE
  None.

SIDE EFFECTS
  Memory pool is removed from linked list.
===========================================================================*/

void dsm_remove_pool
(
 dsm_mempool_id_type pool_id
);

/*===========================================================================
FUNCTION DSM_POOL_REG_MEM_FREE_EVENT_CB

DESCRIPTION
  Registers a callback that is called just before a dsm item is freed
  and returned to the stack.

DEPENDENCIES
  None

PARAMETERS
  pool_id               - Which pool to assign callback.
  free_event_user_data  - User defined handle.
  free_event_cb         - Callback to call.

RETURN VALUE
  None.

SIDE EFFECTS
  Callback is registered in pool management table.
===========================================================================*/

void dsm_pool_reg_mem_free_event_cb
(
  dsm_mempool_id_type pool_id,
  void (*free_event_cb)
  (
    void * user_data,
    dsm_item_type * item
    ),
  void * free_event_user_data
);  

#endif /* DSM_POOL_H */
