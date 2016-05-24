/*===========================================================================

                 P S _ S Y S T E M _ H E A P _ L I N U X . C

DESCRIPTION
  The Data Services Protocol Stack system heap wrapper for Linux.

EXTERNALIZED FUNCTIONS

  ps_system_heap_mem_alloc()
    Allocates requested amount of memory from the system heap.

  ps_system_heap_mem_free()
    Frees the memory allocated through ps_system_heap_mem_alloc().

INTIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header:$
  $DateTime:$

when        who    what, where, why
--------    ---    ----------------------------------------------------------

===========================================================================*/

/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/

#include <stdlib.h>
#include "comdef.h"
#include "customer.h"
#include "ds_util.h"
#include "ps_system_heap.h"


/*===========================================================================

                         EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION  ps_system_heap_mem_alloc

DESCRIPTION
  This function is a wrapper on top of the system's malloc function.

PARAMETERS
  num_bytes - Size (in bytes) of the memory to be allocated.

RETURN VALUE
  Pointer to memory block if successful.
  NULL if memory could not be allocated.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void* ps_system_heap_mem_alloc
(
  unsigned long num_bytes
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ds_malloc( (size_t) num_bytes );
} /* ps_system_heap_mem_alloc() */

/*===========================================================================
FUNCTION  ps_system_heap_mem_free

DESCRIPTION
  Frees the memory allocated by ps_system_heap_mem_alloc()

PARAMTERS
  mem_ptr_ptr   - Memory to free.

RETURN VALUE
  None.

DEPENDENCIES
  The memory chunk passed to be freed must have been allocated by
  ps_system_heap_mem_alloc().

SIDE EFFECTS
  Sets the passed in pointer to memory to NULL.
===========================================================================*/
void ps_system_heap_mem_free
(
  void **mem_ptr_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == mem_ptr_ptr)
  {
    MSG_ERROR("ps_system_heap_mem_free called with NULL pointer",0,0,0);
    return;
  }

  ds_free (*mem_ptr_ptr);

  *mem_ptr_ptr = NULL;
  return;

} /* ps_system_heap_mem_free() */


/*===========================================================================
FUNCTION  ps_system_heap_mem_free

DESCRIPTION
  Frees the memory allocated by ps_system_heap_mem_alloc()

PARAMTERS
  mem_ptr_ptr   - Memory to free.

RETURN VALUE
  None.

DEPENDENCIES
  The memory chunk passed to be freed must have been allocated by
  ps_system_heap_mem_alloc().

SIDE EFFECTS
  Sets the passed in pointer to memory to NULL.
===========================================================================*/
void ps_system_heapi_mem_free
(
  void *mem_ptr_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == mem_ptr_ptr)
  {
    MSG_ERROR("ps_system_heap_mem_free called with NULL pointer",0,0,0);
    return;
  }

  ds_free (mem_ptr_ptr);

  mem_ptr_ptr = NULL;
  return;

} /* ps_system_heap_mem_free() */

/*===========================================================================
FUNCTION  ps_system_heap_init

DESCRIPTION
  Performs system heap initialization

PARAMTERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_system_heap_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* ps_system_heap_mem_init */

/*===========================================================================
FUNCTION  ps_system_heap_deinit

DESCRIPTION
  Performs system heap cleanup

PARAMTERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_system_heap_deinit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* ps_system_heap_deinit */

