#ifndef ONCRPC_PMAP_CLNT_H
#define ONCRPC_PMAP_CLNT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   O N C R P C _ P M _ C L N T . H 

GENERAL DESCRIPTION
  Function declarations for registering and un-registering
  programs with the portmapper.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  ONCRPC calls these functions.  See ONCRPC.

 Copyright (c) 2003 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_pmap_clnt.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/15/02    SV    Created the file.


===========================================================================*/



/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

/*===========================================================================

FUNCTION PMAP_SET

DESCRIPTION
  This function registers a mapping from program,version to  port, protocol 
  with the portmapper.
  
DEPENDENCIES
  None.

RETURN VALUE
  Returns true if the port mapper is able to register the program to the  
  corressponding port else returns false.

SIDE EFFECTS
  None.
===========================================================================*/

bool_t 
pmap_set(u_int prog, u_int vers, u_int prot, u_int port);

/*===========================================================================

FUNCTION PMAP_UNSET

DESCRIPTION
  This function unregisters all the mappings from program ,version to port, 
  protocol with the portmapper

DEPENDENCIES
  None.

RETURN VALUE
  None
  
SIDE EFFECTS
  Removes all the mappings from program ,version to port protocol from 
  the internal mapping table of the portmapper.

===========================================================================*/

void 
pmap_unset(u_int prog, u_int vers);

#endif /* ONCRPC_PMAP_CLNT_H */
