#ifndef DSSOCK_LIB_H
#define DSSOCK_LIB_H
/*==========================================================================*/
/*!
  @file 
  dssock_lib.h

  @brief
  This file provides DSSock API that can be implemented by
  different platforms (Rex/WM/Linux etc) 

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/03/2010 ar  Created module.

===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSSOCK_INIT

DESCRIPTION
  This function initializes the DSSOCK API for client usage.  Invokes the
  various initialization routines for modules within DSSock and DSS.
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dssock_init( void );

/*===========================================================================
FUNCTION DSSOCK_RELEASE

DESCRIPTION
  This function releases DSSOCK API from client.
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dssock_release( void );

#ifdef __cplusplus
}
#endif
  
#endif /* DSSOCK_LIB_H */

