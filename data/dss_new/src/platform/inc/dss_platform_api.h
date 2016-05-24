#ifndef DSS_PLATFORM_API_H
#define DSS_PLATFORM_API_H

/*==========================================================================*/
/*!
  @file 
  dss_platform_api.h

  @brief
  This file provides DSS platform API that can be implemented by
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
/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/


/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSS_API_INIT

DESCRIPTION
  This function initializes the DSS API for client usage.  Invokes the
  various initialization routines for modules within DSS.  Starts
  command thread execution.
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  Command threads are spawnwed
===========================================================================*/
void ds_api_init( void );


/*===========================================================================
FUNCTION DSS_API_INIT

DESCRIPTION
  This function releases the DSS API from client.
  TBD
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  TBD
===========================================================================*/
void ds_api_release( void );

#endif /* DSS_PLATFORM_API_H */

