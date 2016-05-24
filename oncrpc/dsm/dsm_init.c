/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                                  D S M _ I N I T . C

GENERAL DESCRIPTION
  DMSS Data Services memory pool module.

EXTERNALIZED FUNCTIONS

  dsm_init()
    Initialize the Data Services Memory pool unit.
 
INITIALIZATION AND SEQUENCING REQUIREMENTS

  dsm_init() must be called prior to any other DSM function.

-----------------------------------------------------------------------------
Copyright (c) 2005-2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_init.c#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/24/07    hn     Made WinMobile changes to this file in same manner that was
                   done by "11/15/06    rsb" on older 7200.
12/22/06    hrk    Added HDR specific initializations.
11/15/06    rsb    Added dsm_init_pool_wince wrapper function.
08/23/06    ptm    Remove oncrpc item pool.
07/24/06    hal    Added HDR message item pool
03/03/06    ptm    Added oncrpc dsm item pool.
10/10/94    jjw    Created file
===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
/* Target-independent Include files */
#include "comdef.h"
#include "customer.h"
#include "queue.h"
#include "rex.h"
#include "dsm.h"
#include "msg.h"
#include "err.h"
#include "memory.h"
#include "assert.h"
#include "dsm_init.h"

/*===========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

/*---------------------------------------------------------------------------
                         TARGET SPECIFIC ITEM SIZING
  This whole section is a complete mess..
---------------------------------------------------------------------------*/

#define DSMI_DS_SMALL_ITEM_BASE          128 
#define DSMI_DS_SMALL_ITEM_SIZ           (DSMI_DS_SMALL_ITEM_BASE)

  /*-------------------------------------------------------------------------
    Counts for various memory levels/marks for small items.

    DONT_EXCEED:    PPP starts dumping frames and the resequencing queue of
                    all the tcp connections is flushed
    LINK_LAYER_DNE: RLP resets
    TRANSPORT_FEW:  TCP stops adding out of order segments to resequencing
                    queue. Note that this is a count and not a memory level,
                    i.e., no callback is registered for this count.
    LINK_LAYER_FEW: RLP stops processing new frames. It entertains only
                    retransmissions.
    FEW:            The application is flow controlled while attempting 
                    socket writes, i.e., it gets EWOULDBLOCK.
    MANY:           The application is informed that it is no longer flow
                    controlled and socket writes succeed.
 
    Note: The counts corresponding to different memory levels should be in
          order: DONT_EXCEED <= LINK_LAYER_DNE <= LINK_LAYER_FEW <=
          FEW <= MANY. If one needs to change this order, the order of 
          levels should also be changed for dsm_mem_level_enum_type.
  -------------------------------------------------------------------------*/

#define DSMI_DS_SMALL_ITEM_DONT_EXCEED       3
#define DSMI_DS_SMALL_ITEM_LINK_LAYER_DNE    6
#define DSMI_DS_SMALL_ITEM_TRANSPORT_LAYER_FEW 25
#define DSMI_DS_SMALL_ITEM_LINK_LAYER_FEW    50

/*---------------------------------------------------------------------------
  These settings track 6280 but are smaller due to memory constraints.

---------------------------------------------------------------------------*/
#if defined(FEATURE_WINMOB_SIM) || defined(FEATURE_BOOT_IMAGE_WINMOB)
  #define DSMI_DS_SMALL_ITEM_CNT             8000 //Vijay: May need review for Data on Modem
  #define DSMI_DS_SMALL_ITEM_FEW_MARK        105
  #define DSMI_DS_SMALL_ITEM_SIO_FEW_MARK    110
  #define DSMI_DS_SMALL_ITEM_MANY_MARK       700
  #define DSMI_DS_SMALL_ITEM_SIO_MANY_MARK   705
  #define DSMI_DS_SMALL_ITEM_RLC_FEW_MARK    1500
  #define DSMI_DS_SMALL_ITEM_RLC_MANY_MARK   2200
#else
#if defined (IMAGE_MODEM_PROC)
#ifdef FEATURE_THIN_UI
  #define DSMI_DS_SMALL_ITEM_CNT             6500
#else
  #define DSMI_DS_SMALL_ITEM_CNT             11000
#endif
  #define DSMI_DS_SMALL_ITEM_FEW_MARK        105
  #define DSMI_DS_SMALL_ITEM_SIO_FEW_MARK    110
  #define DSMI_DS_SMALL_ITEM_MANY_MARK       705
  #define DSMI_DS_SMALL_ITEM_SIO_MANY_MARK   700
  #define DSMI_DS_SMALL_ITEM_RLC_FEW_MARK    1600
  #define DSMI_DS_SMALL_ITEM_RLC_MANY_MARK   3000

#elif defined (IMAGE_APPS_PROC)
#ifdef FEATURE_THIN_UI
  #define DSMI_DS_SMALL_ITEM_CNT             4000
#else
  #define DSMI_DS_SMALL_ITEM_CNT             6000
#endif
  #define DSMI_DS_SMALL_ITEM_FEW_MARK        105
  #define DSMI_DS_SMALL_ITEM_SIO_FEW_MARK    110
  #define DSMI_DS_SMALL_ITEM_MANY_MARK       700
  #define DSMI_DS_SMALL_ITEM_SIO_MANY_MARK   705
 #endif
#endif  

#define DSMI_DS_SMALL_ITEM_HDR_FEW_MARK    85
#define DSMI_DS_SMALL_ITEM_HDR_MANY_MARK   110

#ifdef FEATURE_DSM_LARGE_ITEMS
/*---------------------------------------------------------------------------
  Size, Count and count for different memory marks/levels for LARGE items. 
  The significants of the counts DONT_EXCEED, TRANSPORT_FEW, FEW and MANY
  is same as described above for small items.

  Sizing for Large Memory Pool items. Header size defines enough space for
  worst case TCP/IP/PPP header.  This should be:

    Default Max MSS + TCP Header Size + MAX TCP Options Size + IP Header Size
    + MAX IP Options Size + MAX PPP Header Size + PPP Checksum Size.

  The large item count needs to be increased if maximum-size SSL records
  are processed.
---------------------------------------------------------------------------*/
  #define DSMI_DS_LARGE_ITEM_SIZ               768


  #define DSMI_DS_LARGE_ITEM_CNT               70

  #define DSMI_DS_LARGE_ITEM_DONT_EXCEED       1  
  #define DSMI_DS_LARGE_ITEM_TRANSPORT_FEW     3
  #define DSMI_DS_LARGE_ITEM_LINK_LAYER_FEW    7
  #define DSMI_DS_LARGE_ITEM_FEW_MARK          7
  #define DSMI_DS_LARGE_ITEM_SIO_FEW_MARK      7
  #define DSMI_DS_LARGE_ITEM_MANY_MARK         11
  #define DSMI_DS_LARGE_ITEM_SIO_MANY_MARK     11
#endif /* FEATURE_DSM_LARGE_ITEMS */

#ifdef FEATURE_DSM_DUP_ITEMS
/*---------------------------------------------------------------------------
  Size, Count, Few, many and do not exceed counts for DUP items 
---------------------------------------------------------------------------*/
  #define DSMI_DUP_ITEM_CNT                   2400

  #define DSMI_DUP_ITEM_DONT_EXCEED           5
  #define DSMI_DUP_ITEM_FEW_MARK              550
  #define DSMI_DUP_ITEM_MANY_MARK             600
#endif /* FEATURE_DSMI_DUP_ITEMS */

/*---------------------------------------------------------------------------
  Size, Count, Few, many and do not exceed coutns for HDR items
---------------------------------------------------------------------------*/
#ifdef FEATURE_DSM_HDR_MSG_ITEMS
  #define DSMI_HDR_MSG_ITEM_SIZ                 128

  #define DSMI_HDR_MSG_ITEM_CNT                  20
  #define DSMI_HDR_MSG_ITEM_DONT_EXCEED           5
  #define DSMI_HDR_MSG_ITEM_FEW_MARK              8
  #define DSMI_HDR_MSG_ITEM_MANY_MARK            12
  
#endif /* FEATURE_DSM_HDR_MSG_ITEMS*/

/*--------------------------------------------------------------------------
  Defining the static array that stores the small items.
--------------------------------------------------------------------------*/
#define DSMI_DS_SMALL_ITEM_ARRAY_SIZ (DSMI_DS_SMALL_ITEM_CNT * \
   (DSMI_DS_SMALL_ITEM_SIZ + DSM_ITEM_HEADER_SIZE) + 31)
#if !defined(FEATURE_WINCE)
static uint32  dsm_ds_small_item_array[DSMI_DS_SMALL_ITEM_ARRAY_SIZ/4];
#endif /* FEATURE_WINCE */
dsm_pool_mgmt_table_type dsm_ds_small_item_pool;

/*--------------------------------------------------------------------------
  Defining the static array that stores the large items.
--------------------------------------------------------------------------*/
#ifdef FEATURE_DSM_LARGE_ITEMS

#define DSMI_DS_LARGE_ITEM_ARRAY_SIZ (DSMI_DS_LARGE_ITEM_CNT * \
   (DSMI_DS_LARGE_ITEM_SIZ + DSM_ITEM_HEADER_SIZE) + 31)
#if !defined(FEATURE_WINCE)
static uint32  dsm_ds_large_item_array[DSMI_DS_LARGE_ITEM_ARRAY_SIZ/4];
#endif /* FEATURE_WINCE */
dsm_pool_mgmt_table_type dsm_ds_large_item_pool;

#endif /* FEATURE_DSM_LARGE_ITEMS */

/*--------------------------------------------------------------------------
  Defining the static array that stores the DUP items.
--------------------------------------------------------------------------*/
#ifdef FEATURE_DSM_DUP_ITEMS

#define DSMI_DUP_ITEM_ARRAY_SIZ (DSMI_DUP_ITEM_CNT * \
   (0 + DSM_ITEM_HEADER_SIZE) + 31)
#if !defined(FEATURE_WINCE)
static uint32 dsm_dup_item_array[DSMI_DUP_ITEM_ARRAY_SIZ/4];
#endif /* FEATURE_WINCE */
dsm_pool_mgmt_table_type dsm_dup_item_pool;

#endif /* FEATURE_DSM_DUP_ITEMS */

/*---------------------------------------------------------------------------
  Definitions for the HDR Message Items Pool 
---------------------------------------------------------------------------*/
#ifdef FEATURE_DSM_HDR_MSG_ITEMS

#define DSMI_HDR_MSG_ITEM_ARRAY_SIZ ((DSMI_HDR_MSG_ITEM_CNT * \
   (DSMI_HDR_MSG_ITEM_SIZ + DSM_ITEM_HEADER_SIZE)) + 31)
   
#if !defined(FEATURE_WINCE)
static uint32 dsm_hdr_msg_item_array[DSMI_HDR_MSG_ITEM_ARRAY_SIZ/4];
#endif /* FEATURE_WINCE */
dsm_pool_mgmt_table_type dsm_hdr_msg_item_pool;

#endif /* FEATURE_DSM_HDR_MSG_ITEMS */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           EXTERNALIZED FUNTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#ifndef MSM5000_IRAM_FWD
/*===========================================================================
FUNCTION DSM_INIT()

DESCRIPTION
  This function will initialize the Data Service Memory Pool. It should be
  called once upon system startup. All the memory items are initialized and
  put onto their respective free queues.

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsm_init(void)
{
#if defined(FEATURE_WINCE)
  dsm_init_pool_wince(DSM_DS_SMALL_ITEM_POOL, 
                      DSMI_DS_SMALL_ITEM_ARRAY_SIZ,
                      DSMI_DS_SMALL_ITEM_SIZ);
#else 
  dsm_init_pool(DSM_DS_SMALL_ITEM_POOL, 
		(uint8*)dsm_ds_small_item_array,
		DSMI_DS_SMALL_ITEM_ARRAY_SIZ,
		DSMI_DS_SMALL_ITEM_SIZ);
#endif /* FEATURE_WINCE */

  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_DNE,
                          DSMI_DS_SMALL_ITEM_DONT_EXCEED);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_LINK_LAYER_DNE,
                          DSMI_DS_SMALL_ITEM_LINK_LAYER_DNE);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_TRANSPORT_LAYER_FEW,
                          DSMI_DS_SMALL_ITEM_TRANSPORT_LAYER_FEW);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_LINK_LAYER_FEW,
                          DSMI_DS_SMALL_ITEM_LINK_LAYER_FEW);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_HDR_FEW,
                          DSMI_DS_SMALL_ITEM_HDR_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_FEW,
                          DSMI_DS_SMALL_ITEM_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_SIO_FEW,
                          DSMI_DS_SMALL_ITEM_SIO_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_HDR_MANY,
                          DSMI_DS_SMALL_ITEM_HDR_MANY_MARK);
#if !defined(IMAGE_APPS_PROC)
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_RLC_FEW,
                          DSMI_DS_SMALL_ITEM_RLC_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_RLC_MANY,
                          DSMI_DS_SMALL_ITEM_RLC_MANY_MARK);
#endif
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_MANY,
                          DSMI_DS_SMALL_ITEM_MANY_MARK);
  dsm_reg_mem_event_level(DSM_DS_SMALL_ITEM_POOL,
                          DSM_MEM_LEVEL_SIO_MANY,
                          DSMI_DS_SMALL_ITEM_SIO_MANY_MARK);


#ifdef FEATURE_DSM_LARGE_ITEMS
#if defined(FEATURE_WINCE)  
  dsm_init_pool_wince(DSM_DS_LARGE_ITEM_POOL, 
                      DSMI_DS_LARGE_ITEM_ARRAY_SIZ,
                      DSMI_DS_LARGE_ITEM_SIZ);
#else  
  dsm_init_pool(DSM_DS_LARGE_ITEM_POOL, 
		(uint8*)dsm_ds_large_item_array,
		DSMI_DS_LARGE_ITEM_ARRAY_SIZ,
		DSMI_DS_LARGE_ITEM_SIZ);
#endif /* FEATURE_WINCE */  

//     { DSMI_DS_LARGE_ITEM_DONT_EXCEED,    NULL, NULL }, /* LINK_LAYER_DNE   */
//     { DSMI_DS_LARGE_ITEM_FEW_MARK,       NULL, NULL }, /* LINK_LAYER_FEW   */
//     { DSMI_DS_LARGE_ITEM_FEW_MARK,       NULL, NULL }, /* RLC FEW          */
//     { DSMI_DS_LARGE_ITEM_MANY_MARK,      NULL, NULL }, /* RLC MANY         */

  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_DNE,
                          DSMI_DS_LARGE_ITEM_DONT_EXCEED);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_TRANSPORT_LAYER_FEW,
                          DSMI_DS_LARGE_ITEM_TRANSPORT_FEW);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_LINK_LAYER_FEW,
                          DSMI_DS_LARGE_ITEM_LINK_LAYER_FEW);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_FEW,
                          DSMI_DS_LARGE_ITEM_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_SIO_FEW,
                          DSMI_DS_LARGE_ITEM_SIO_FEW_MARK);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_MANY,
                          DSMI_DS_LARGE_ITEM_MANY_MARK);
  dsm_reg_mem_event_level(DSM_DS_LARGE_ITEM_POOL,
                          DSM_MEM_LEVEL_SIO_MANY,
                          DSMI_DS_LARGE_ITEM_SIO_MANY_MARK);

#endif /* FEATURE_DSM_LARGE_ITEMS */
#ifdef FEATURE_DSM_DUP_ITEMS
#if defined(FEATURE_WINCE)
  dsm_init_pool_wince(DSM_DUP_ITEM_POOL,
                      DSMI_DUP_ITEM_ARRAY_SIZ,
                      0);
#else  
  dsm_init_pool(DSM_DUP_ITEM_POOL,
		(uint8*)dsm_dup_item_array,
		DSMI_DUP_ITEM_ARRAY_SIZ,
		0);
#endif /* FEATURE_WINCE */

  dsm_reg_mem_event_level(DSM_DUP_ITEM_POOL,
                          DSM_MEM_LEVEL_FEW,
                          DSMI_DUP_ITEM_FEW_MARK);


  dsm_reg_mem_event_level(DSM_DUP_ITEM_POOL,
                          DSM_MEM_LEVEL_MANY,
                          DSMI_DUP_ITEM_MANY_MARK);

//      { DSMI_DUP_ITEM_DONT_EXCEED,         NULL, NULL }, /* DNE              */
//      { DSMI_DUP_ITEM_DONT_EXCEED,         NULL, NULL }, /* LINK_LAYER_DNE   */
//      { DSMI_DUP_ITEM_FEW_MARK,            NULL, NULL }, /* TR..RT_LAYER_FEW */
//      { DSMI_DUP_ITEM_FEW_MARK,            NULL, NULL }, /* LINK_LAYER_FEW   */
//      { DSMI_DUP_ITEM_FEW_MARK,            NULL, NULL }, /* RLC FEW          */
//      { DSMI_DUP_ITEM_FEW_MARK,            NULL, NULL }, /* FEW              */
//      { DSMI_DUP_ITEM_FEW_MARK,            NULL, NULL }, /* SIO FEW          */
//      { DSMI_DUP_ITEM_MANY_MARK,           NULL, NULL }, /* RLC MANY         */
//      { DSMI_DUP_ITEM_MANY_MARK,           NULL, NULL }, /* MANY             */
//      { DSMI_DUP_ITEM_MANY_MARK,           NULL, NULL }  /* SIO MANY         */

#endif /* FEATURE_DSM_DUP_ITEMS */
#ifdef FEATURE_DSM_HDR_MSG_ITEMS
#if defined(FEATURE_WINCE)
  dsm_init_pool_wince(DSM_HDR_MSG_ITEM_POOL,
                      DSMI_HDR_MSG_ITEM_ARRAY_SIZ,
                      DSMI_HDR_MSG_ITEM_SIZ);
#else  
  dsm_init_pool(DSM_HDR_MSG_ITEM_POOL,
        (uint8*)dsm_hdr_msg_item_array,
        DSMI_HDR_MSG_ITEM_ARRAY_SIZ,
        DSMI_HDR_MSG_ITEM_SIZ);
#endif /* FEATURE_WINCE */

#endif /* FEATURE_DSM_HDR_MSG_ITEMS */

} /* dsm_init() */

#if defined(FEATURE_WINCE)
/*===========================================================================
FUNCTION DSM_INIT_POOL_WINCE()

DESCRIPTION
  WINCE wrapper to allocate the virtual memory and initialize the data pool.  

DEPENDENCIES
  None

PARAMETERS
  pool_id - The indentifier for the pool that should be initialized. 
  item_array_size - The size of the block of memory passed in item_array
  item_size - The size of the items that this pool should provide

RETURN VALUE
  None
===========================================================================*/
#define ALLOC_SHARED_MEMORY_SIZ  0x200000
void dsm_init_pool_wince
(
  dsm_mempool_id_type  pool_id, 
  uint32               item_array_size,
  uint16               item_size
)
{
  static uint8  *curr_ptr       = NULL;
  static uint32  avail_size     = 0;
  uint8         *item_array_ptr = NULL;
  uint32         alloc_size     = 0;
  void          *virt_mem_ptr   = NULL;

  /* VirtualAlloc allocates in page size chunks. Therefore, round
     item_array_size up to include at least one byte in every page
     allocated. */
  item_array_size = ROUND_TO_PAGES(item_array_size);

  /* if there is enough allocated memory for this */
  if (avail_size >= item_array_size)
  {
      /* Commit only the required amount of physical memory. */
      virt_mem_ptr = curr_ptr;
      virt_mem_ptr = VirtualAlloc( virt_mem_ptr,
                                   item_array_size,
                                   MEM_COMMIT,
                                   PAGE_READWRITE );
      
      if (virt_mem_ptr == NULL)
      {
          ERR_FATAL("ERR: failed VirtualAlloc commit", 0, 0, 0);
      }

      item_array_ptr   = (uint8 *)virt_mem_ptr;
      curr_ptr        += item_array_size;
      avail_size      -= item_array_size;
  }
  /* there is not enought memory left */
  else
  {
      /* the size has to be more than ALLOC_SHARED_MEMORY_SIZ in order
         to reserve into the virutal memory space */ 
      alloc_size = ( item_array_size > ALLOC_SHARED_MEMORY_SIZ ) ?
          item_array_size : ALLOC_SHARED_MEMORY_SIZ;
      
      /* Reserve a block of memory. The virtual address will be
         assigned to the shared memory slot rather than in the current
         process (Slot 0) region to save space. */
      virt_mem_ptr  = VirtualAlloc( 0,
                                    alloc_size,
                                    MEM_RESERVE,
                                    PAGE_NOACCESS );
      if ( virt_mem_ptr == NULL )
      {
          ERR_FATAL("ERR: failed VirtualAlloc reserve", 0, 0, 0);
      }
      
      /* Commit only the required amount of physical memory. */
      virt_mem_ptr = VirtualAlloc( virt_mem_ptr,
                                   item_array_size,
                                   MEM_COMMIT,
                                   PAGE_READWRITE );
      
      if (virt_mem_ptr == NULL)
      {
          ERR_FATAL("ERR: failed VirtualAlloc commit", 0, 0, 0);
      }
      
      item_array_ptr = (uint8 *)virt_mem_ptr;
      
      /* adjust the curr_ptr and avail_size.
         if the requested size is larger than ALLOC_SHARED_MEMORY_SIZ,
         the allocated is exactly the same as the requested.
         
         if the requested size is less than ALLOC_SHARED_MEMORY_SIZ,
         there will be unused memory available */
      avail_size = alloc_size - item_array_size;
      if (avail_size == 0) 
      {
          curr_ptr = NULL;
      }
      else
      {
          curr_ptr = item_array_ptr + item_array_size; 
      }
  }
    
  /* initialize the DSM item pool */
  dsm_init_pool(pool_id, 
                item_array_ptr,
                item_array_size,
                item_size);

  return;
}/* dsm_init_pool_wince */

#endif /* FEATURE_WINCE */

#endif /* !MSM5000_IRAM_FWD */
