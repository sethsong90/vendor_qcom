/*==========================================================================
                            DYNAMIC MEMORY

Description:

  Backwards compatibility for oncprc memory allocator. 
  
Initialization and sequencing requirements:

  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 Copyright (c) 2000-2007 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 
===========================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_mem.c#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/19/08     rr    Changed to use system allocator
===========================================================================*/

/*==========================================================================
                       INCLUDE FILES
===========================================================================*/


/*==========================================================================
                 DEFINITIONS AND DECLARATIONS
===========================================================================*/
/*========================================================================
  Function: oncrpc_mem_init_all_pools()

  Description:
    This function initializes the memory pools and free memory lists
    used in dynamic memory allocation
 
  Dependencies:
    Nothin'
 
  Return Value:
    There is no return value.

  Side Effects:
    Initializes Mem_pool_table array
========================================================================*/
void oncrpc_mem_init_all_pools(void)
{

}

