#ifndef PS_SCRATCHPAD_H
#define PS_SCRATCHPAD_H

/*===========================================================================

                         P S _ S C R A T C H P A D . H

DESCRIPTION
  A wrapper to use malloc() when ps_scratchpad_* API is called
  on Linux

EXTERNALIZED FUNCTIONS
  ps_scratchpad_init()
    No OP

  ps_scratchpad_mem_alloc()
    malloc()

  ps_scratchpad_mem_free()
    free()

INTIALIZATION AND SEQUENCING REQUIREMENTS
  ps_scratchpad_mem_alloc() or ps_scratchpad_mem_free()

Copyright (c) 2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/
/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ds_util.h"

/*===========================================================================

                         EXTERNAL DATA DEFINITIONS

===========================================================================*/
typedef enum
{
  PS_SCRATCHPAD_CLIENT_MIN          = 0,
  PS_SCRATCHPAD_CLIENT_DSS          = PS_SCRATCHPAD_CLIENT_MIN,
  PS_SCRATCHPAD_CLIENT_DSNET        = 1,
  PS_SCRATCHPAD_CLIENT_DSSOCK       = 2,
  PS_SCRATCHPAD_CLIENT_QMIMH        = 3,
  PS_SCRATCHPAD_CLIENT_MAX          = 4,
  PS_SCRATCHPAD_CLIENT_FORCE_32_BIT = 0x7FFFFFFF

} ps_scratchpad_client_enum_type;


/*===========================================================================

                         EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION  ps_scratchpad_init()

DESCRIPTION
  Does nothing

PARAMETERS
  client      - Client ID.
  buffer      - Buffer to be initialized as scratchpad.
  num_bytes   - Number of bytes allocated for the scratchpad.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
#define ps_scratchpad_init(x,y,z)

/*===========================================================================
FUNCTION  ps_scratchpad_mem_alloc

DESCRIPTION
  ignores the first argument. Calls malloc()

PARAMETERS
  size    - Size (in bytes) of the memory to be allocated.

RETURN VALUE
  Pointer to memory block if successful.
  NULL if could not get memory.

DEPENDENCIES
  ps_scratchpad_init() must have been called previously.

SIDE EFFECTS
  None.
===========================================================================*/
#define ps_scratchpad_mem_alloc(x,y) ds_malloc(y)

/*===========================================================================
FUNCTION  ps_scratchpad_mem_free

DESCRIPTION
  ignores first argument. frees the memory pointer using free()

PARAMTERS
  mem_ptr_ptr   - Memory to free.

RETURN VALUE
  None.

DEPENDENCIES
  The memory chunk passed to be freed must have been allocated by
  ps_scratchpad_mem_alloc().

SIDE EFFECTS
  Sets the passed in pointer to memory to NULL.
===========================================================================*/
#define ps_scratchpad_mem_free(x,y)  ds_free (*y)

#ifdef __cplusplus
}
#endif

#endif /* PS_SCRATCHPAD_H */

