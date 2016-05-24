#ifndef ONCRPC_PLUGGER_INIT_H
#define ONCRPC_PLUGGER_INIT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                  O N C R P C _ P L U G G E R _ I N I T . H

GENERAL DESCRIPTION
  The plugger keeps track of the svc_register registeration database.
  This file exports the find/dispatch function that is called from the
  top of the server loop. 

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

  None.

 Copyright (c) 2005 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_plugger_init.h#3 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/03/06    hn     Added support for locking/unlocking RPC services.
05/20/05    clp    Added comment block
===========================================================================*/

typedef enum
{
  ONCRPC_PLUGGER_FIND_DISPATCHED = 0,
  ONCRPC_PLUGGER_FIND_NOT_FOUND  = 1,
  ONCRPC_PLUGGER_FIND_FOUND      = 2,
  ONCRPC_PLUGGER_FIND_LOCKED     = 3
} oncrpc_plugger_find_status;

oncrpc_plugger_find_status
svc_find_dispatch
(
 xdr_s_type     * xdr,
 struct svc_req * call,
 rpcvers_t      * max,
 rpcvers_t      * min
);

#endif /* ONCRPC_PLUGGER_INIT_H */
