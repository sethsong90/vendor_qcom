#ifndef _ONCRPC_MEM_H
#define _ONCRPC_MEM_H
/*===================================================================
                     DYNAMIC MEMORY ROUTINES

DESCRIPTION
  This file contains routines for dynamic memory allocation/deallocation.
  The dynamic memory scheme employs the "buddy" system and is written
  to work in 32 bit flat model

 
 Copyright (c) 1999, 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

===================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_mem.h#1 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/19/08    rr     Changed to use system allocator
01/08/08    al,pc  Fixed and enhanced FEATURE_ONCRPC_MEM_CHECK_LEAK.
07/10/07    ptm    Remove featurization.
03/16/05    clp    Added header to source file.

===========================================================================*/

/*========================================================================
  Function: oncrpc_mem_init_all_pools()

  Description:
    This function initializes all of the pools of dynamic memory.
 
  Dependencies:
    None
 
  Return Value:
    There is no return value.

  Side Effects:
    Initialzes data structures internal to the dynamic memory module
========================================================================*/
extern
void
oncrpc_mem_init_all_pools
(
   void
);

/*========================================================================
  Macro: oncrpc_mem_alloc()

  Description:
    Wrapper around malloc for backwards compatibility to existing oncrpc
    code.

  Dependencies:
    system allocator and heap memory.
 
  Return Value:
    Pointer to memory block or NULL if not enough memory is
    available
    
  Side Effects:
    Memory usage.
========================================================================*/
#define oncrpc_mem_alloc(size)\
  malloc(size)


#define oncrpc_mem_free(n)\
  free(n)



#endif  /* _ONCRPC_MEM_H */
