/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                      D S   S O C K   L I B   I N I T

GENERAL DESCRIPTION
  This is the platform specific source file for initializing DSSock Library. 

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
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

#include "msg.h"
#include "dsm.h"
#include "dss_init.h"
#include "dsnet_lib.h"
#include "DS_Sock_Init.h"

extern void PlatformSockPowerUp( void );
extern void PlatformSockPowerDown( void );
extern void PlatformSockInit( void );
extern void PlatformSockDeInit( void );

extern int DSSockSocketFactoryCreateInstance
(
  void *    envPtr,
  AEECLSID  clsID,
  void *    privSetPtr,
  void **   newObjPtrPtr
);

extern int DSSockSocketFactoryPrivCreateInstance
(
  void *    envPtr,
  AEECLSID  clsID,
  void *    privSetPtr,
  void **   newObjPtrPtr
);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                        LOCAL DECLARATIONS FOR MODULE

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

LOCAL boolean dssock_initialized = FALSE;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         LOCAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         GLOBAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/


/*===========================================================================
FUNCTION DSSOCK_INIT

DESCRIPTION
  This function initializes the DSSOCK API for client usage.  Invokes the
  various initialization routines for modules within DSSOCK and DSS.

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dssock_init( void )
{
  dsnet_create_instance_cb_t cb_tbl;
  
  if( dssock_initialized ) {
    MSG_ERROR( "DSSOCK API already initialized, ignoring!",0,0,0 );
    return;
  }

  /* Register Socket create instance callbacks with DSNet library */
  cb_tbl.socketFactory_cb     = DSSockSocketFactoryCreateInstance;
  cb_tbl.socketFactoryPriv_cb = DSSockSocketFactoryPrivCreateInstance;
  dsnet_register_socket_callbacks( &cb_tbl );

  /* Initialize DSM pools */
  dsm_init();
  
  /* Initialize DSSock layer modules */
  DSSockPowerup();
  DSSockInit();
  PlatformSockPowerUp();
  PlatformSockInit();

  dss_powerup();
  dss_init();
  
  MSG_HIGH( "DSSOCK initialized!",0,0,0 );
  dssock_initialized = TRUE;
  return;
}


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
void dssock_release( void )
{
  if( !dssock_initialized ) {
    MSG_ERROR( "DSSOCK API already released, ignoring!",0,0,0 );
    return;
  }
  
  /* Release DSSock layer modules */
  PlatformSockDeInit();
  PlatformSockPowerDown();

  MSG_HIGH( "DSSOCK released!",0,0,0 );
  dssock_initialized = FALSE;
  return;
}

