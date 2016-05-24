#ifndef ONCRPC_RTR_H
#define ONCRPC_RTR_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ R T R . H

GENERAL DESCRIPTION

  This is the ONCRPC transport for the router.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2007-2010 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_rtr.h#3 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove featurization.
05/02/07     ih    Initial version

===========================================================================*/

#define XPORT_CONTROL_LAST  ( RPC_ROUTER_IOCTL_LAST )

/*===========================================================================
FUNCTION XPRTRTR_INIT

DESCRIPTION
  Initialize the router transport.
  
DEPENDENCIES
  None.

ARGUMENTS
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
extern void xprtrtr_init( void );

/*===========================================================================
FUNCTION XPRTRTR_DEINIT

DESCRIPTION
  Deinitialize the router transport

DEPENDENCIES
  None.

ARGUMENTS
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
extern void xprtrtr_deinit( void );

#endif /* ! ONCRPC_RTR_H */
