/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ I N I T . C 

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) init code.
  This file deals with initializing various services that use RPC. 

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_init should be called from oncrpctask once it is running.

 Copyright (c) 2003-2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/COMMON/vcs/oncrpcinit.c_v   1.23   29 Oct 2003 18:39:06   pbostley  $
  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_init.c#4 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/29/08    ih     Added WM7 support
03/05/08    ih     Added daemon and ping server support
09/05/07    ptm    Add Router Message server.
04/01/05    clp    Include header cleanup changes.
03/14/05    clp    Cleanup white space.
02/13/02    clp    Created this file.
10/09/02    mjb    fixed rpccm_app_server_init line
===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "oncrpc.h"             // FIXME - delete when taski cleaned up
#include "oncrpc_taski.h"

#ifdef FEATURE_ONCRPC_PM
//#include "oncrpc_pm.h"
#endif /* FEATURE_ONCRPC_PM */

#ifdef FEATURE_ONCRPC_DAEMON_API
#include "oncrpc_daemon_api.h"
#include "oncrpc_daemon_api_rpc.h"
#endif

#ifdef FEATURE_EXPORT_ONCRPC_DAEMON_MSG
#include "oncrpc_daemon_msg_rpc.h"
#endif /* FEATURE_EXPORT_ONCRPC_DAEMON_MSG */

#ifdef FEATURE_ONCRPC_PLUGGER
#include "oncrpc_plugger.h"
#endif /* FEATURE_ONCRPC_PLUGGER */

#ifdef FEATURE_ONCRPC_PING_SERVER
#include "ping_rpc_rpc.h"
#endif

/*===========================================================================
FUNCTION ONCRPC_APP_INIT

DESCRIPTION
  This function initializes the RPC applications. 

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  ONCRPC should be up and running.

SIDE EFFECTS
  None
===========================================================================*/
void 
oncrpc_app_init( void )
{

#ifdef FEATURE_ONCRPC_PING_SERVER
  ping_rpc_app_init();
#endif

#ifdef FEATURE_ONCRPC_PM
  /* ONCRPC program number 100000 */
  /* This must be initialized before anyone else!!!!! */ 
  oncrpc_pm_app_server_init();
#endif /* FEATURE_ONCRPC_PM */

#ifdef FEATURE_ONCRPC_DAEMON_API
  /* ONCRPC program number 0x3000FExx where xx = processor number */
  oncrpc_daemon_api_init();
  oncrpc_daemon_api_app_init();
#endif

#ifdef FEATURE_EXPORT_ONCRPC_DAEMON_MSG
  /* If the daemon and the stack run in the same process, no need to export
   * the APIs
   */
  /* ONCRPC program number 0x3000FFFF */
  oncrpc_daemon_msg_app_init();
#endif /* FEATURE_ONCRPC_RTR_MSG */

#ifdef FEATURE_ONCRPC_PLUGGER
  /* ONCRPC program number 0x2FFFFFFF */
  oncrpc_plugger_app_init();
#endif /* FEATURE_ONCRPC_PLUGGER */

#ifdef FEATURE_ONCRPC_NFS
  /* ONCRPC program number 100003 */
  /* ONCRPC program number 100005 */
  oncrpc_nfs_app_server_init();
#endif /* FEATURE_ONCRPC_NFS */
}
