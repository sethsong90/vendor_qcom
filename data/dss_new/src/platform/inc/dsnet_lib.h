#ifndef DSNET_LIB_H
#define DSNET_LIB_H
/*==========================================================================*/
/*!
  @file 
  dsnet_lib.h

  @brief
  This file provides DSNet API that can be implemented by
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

#include "AEEStdDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------
  Types to support decoupling DSNet and DSSock libraries
-----------------------------------------------------------------*/
typedef int (*create_instance_cb) ( void* env,
                                    AEECLSID clsid,
                                    void* privset,
                                    void** newObj );

typedef struct dsnet_create_instance_cb_s {
  create_instance_cb socketFactory_cb;
  create_instance_cb socketFactoryPriv_cb;
} dsnet_create_instance_cb_t;

extern dsnet_create_instance_cb_t dsnet_create_instance_table;


/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

extern inline create_instance_cb dsnet_get_socketFactory_ci(void)
{
  return dsnet_create_instance_table.socketFactory_cb;
}

extern inline create_instance_cb dsnet_get_socketFactoryPriv_ci(void)
{
  return dsnet_create_instance_table.socketFactoryPriv_cb;
}
  
/*===========================================================================
FUNCTION DSNET_REGISTER_SOCKET_CALLBACKS

DESCRIPTION
  This function initializes the DSNET callback table for DSSocket
  create instance methods.  Callback are used to decouple the libraries
  for DSNet can be used only when control-plane functionality is required.
  
DEPENDENCIES
  None

PARAMETERS
  cb_tbl_ptr  - Pointer to callback table structure

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsnet_register_socket_callbacks
(
  dsnet_create_instance_cb_t * cb_tbl_ptr
);

/*===========================================================================
FUNCTION DSNET_INIT

DESCRIPTION
  This function initializes the DSNET API for client usage.  Invokes the
  various initialization routines for modules within DNET and PS.  Starts
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
void dsnet_init( void );

/*===========================================================================
FUNCTION DSNET_RELEASE

DESCRIPTION
  This function releases DSNET API from client.
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsnet_release( void );

#ifdef __cplusplus
}
#endif

#endif /* DSNET_LIB_H */

