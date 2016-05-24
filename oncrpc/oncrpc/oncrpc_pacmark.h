#ifndef XPORT_PACMARK_H
#define XPORT_PACMARK_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ P A C M A R K . H

GENERAL DESCRIPTION

  Pacmark is a method for enacpulating ONCRPC message on a stream
  interface, similar to record marking.  This is used for the SM
  transport.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2010 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_pacmark.h#3 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove featurization.
05/15/07    RJS    Split OS specific parts into seperate files. 
                   Remove Int and task locks
05/04/07    rr     Support for Pacmark 2 (rpc_router support)
06/07/05    ptm    Add block comments.
04/01/05    clp    Include header cleanup changes.
03/16/05    clp    Added header to source file.
===========================================================================*/

#define XPORT_PACMARK_NUM_MIDS        256
#if defined ONCRPC_64K_RPC
#define PACMARK_MAX_MSG_SIZE          (1024*65)
#else
#define PACMARK_MAX_MSG_SIZE          (1024*17)
#endif

#define PACMARK_CONTROL_SET_DEST (XPORT_CONTROL_LAST +1)
#define PACMARK_CONTROL_LAST     (PACMARK_CONTROL_SET_DEST)
/*===========================================================================
FUNCTION PACMARK_MID_INIT

DESCRIPTION
  Initialises packet marking processing.

DEPENDENCIES
  None

ARGUMENTS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_pacmark_mid_init(void);


/*===========================================================================
FUNCTION XPORTCLNT_PACMARK_CREATE

DESCRIPTION
  Create a client packet marking xport. This xport can only be used to send
  messages and handle replies.
  
DEPENDENCIES
  None

ARGUMENTS
  child - pointer to xport below this one
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  pointer to packet marking xport, or NULL on error

SIDE EFFECTS
  None.
===========================================================================*/
xport_s_type *
xportclnt_pacmark_create
(
  xport_s_type *child,
  uint32 rec_size,
  dsm_mempool_id_enum_type pool
);

/*===========================================================================
FUNCTION XPORTSVR_PACMARK_CREATE

DESCRIPTION
  Create a server packet marking xport. This xport can be use both for sending
  and receiving messages.
  
DEPENDENCIES
  None

ARGUMENTS
  child - pointer to xport below this one
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  pointer to the packet marking xport, or NULL on error

SIDE EFFECTS
  None.
===========================================================================*/
xport_s_type *
xportsvr_pacmark_create
(
  xport_s_type *child,
  uint32 rec_size,
  dsm_mempool_id_enum_type pool
);

/*===========================================================================
FUNCTION XPORT_PACMARK_INIT

DESCRIPTION
  Common routine for initializing a packet marking xport.
  
DEPENDENCIES
  None

ARGUMENTS
  xport - pointer to xport to initialize
  child - pointer to xport below this one
  incoming_msgs - pointer to array of dsm items
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  TRUE if initialization sucessful, FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
boolean
xport_pacmark_init
(
  xport_s_type *xport,
  xport_s_type *child,  
  uint32 rec_size,
  dsm_mempool_id_enum_type dsm_pool
);

/*===========================================================================
FUNCTION      PACMARK_CLIENT_DIED

DESCRIPTION   Clean up any pacmark structures related to the dead client.
              Currently the only action to take is to remove any partial
              incoming messages from that client.

ARGUMENTS     xport, pointer to xport
              client_addr, Address of dead client.

RETURN VALUE  N/A

SIDE EFFECTS  
===========================================================================*/
void pacmark_client_died
(
  xport_s_type *xport,
  oncrpc_addr_type client_addr
);

#endif /* XPORT_PACMARK_H */
