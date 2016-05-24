#ifndef PS_LL_ADDR_MGR_H
#define PS_LL_ADDR_MGR_H
/*===========================================================================

                  P S _ L L _ A D D R _ M G R . H


DESCRIPTION
This is the header file for the PS Link Local Address manager.

This entity is responsible for dynamic configuration of IPv4 link local
addresses per RFC 3927

This is particularly useful on a ilnk where connectivity is restricted to
`nodes' on the same link. e.g. in Ad Hoc networks

It is important to note that the 169.254/16 addresses registered with IANA
for this purpose are not routable addresses.

NOTE: Use of this module for IP address configuration may cause some
      applications to fail!!


EXTERNALIZED FUNCTIONS

  ll_addr_mgr_start()
    Called to start the link local address manager

  ll_addr_mgr_stop()
    Called to stop the link local address manager and thereby release the
    address acquired

  ll_addr_mgr_claim()
    Called to acquire a link local address


Copyright (c) 2006-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ll_addr_mgr.h#1 $
  $Author: maldinge $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/17/08    pp     CMI: Public/Private API split.
06/23/06    lyr     Created module
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_iface.h"

/*===========================================================================

                          FORWARD DECLARATIONS

===========================================================================*/


/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
TYPEDEF LL_ADDR_MGR_IND_ENUM_TYPE

DESCRIPTION
  List of status indications clients can receive from LL ADDR MGR
---------------------------------------------------------------------------*/
typedef enum
{
  LL_ADDR_MGR_MIN_IND               = -1,

  LL_ADDR_MGR_IP_CLAIMED_IND        =  0,  /* when IP (re)established      */
  LL_ADDR_MGR_IP_UNAVAIL_IND        =  1,  /* when IP address unavailable  */
  LL_ADDR_MGR_IP_CHANGED_IND        =  2,  /* when IP address changed, because
                                              of conflict (defending)
                                              or starting over */

  LL_ADDR_MGR_MAX_IND

} ll_addr_mgr_ind_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF LL_ADDR_MGR_IND_CBACK_TYPE

DESCRIPTION
  This is the type for callbacks to be registered with LL ADDR MGR for
  indications

RETURN VALUE
  None.
---------------------------------------------------------------------------*/

typedef void (* ll_addr_mgr_ind_cback_type)
(
  /* Indication being posted                                               */
  ll_addr_mgr_ind_enum_type     ind,

  /* Any user data given at registration                                   */
  void                         *user_data_ptr
);


/*---------------------------------------------------------------------------
TYPEDEF LL_ADDR_MGR_PARAMS_TYPE

DESCRIPTION
  This is the set of parameters required to start the LL address mgr

RETURN VALUE
  None.
---------------------------------------------------------------------------*/

typedef struct
{
  uint32   acquisition_retries;
          /* number of times to try to acquire an IP address before giving
           * up and reporting to client; this is not specified in RFC but
           * would avoid long waits to get an IP address
           * RFC puts forth a 99.96% chance of succeeding within 2 retries
           * Also: each retry ~ 7 seconds:
           * PROBE_WAIT[1sec] + ( (PROBE_NUM[3] - 1) * PROBE_MAX[2sec] ) +
           * ANNOUNCE_WAIT[2sec]
           * Suggest 2
           */
  uint64   mac_addr; /* will use it as a seed for generating random number */
} ll_addr_mgr_params_type;

/* Data structure for passing start data to PS context */
typedef struct
{
  ps_iface_type                 *iface_ptr;     /* iface pointer of client    */
  ll_addr_mgr_params_type        params;        /* related parameters      */
  ll_addr_mgr_ind_cback_type     ind_cback;     /* cback for notification  */
  void                          *user_data_ptr; /* user data for caller    */
  uint32                         ll_addr_mgr_sm_cb_instance;
                                                /* ptr to the ll mgr cb */
}ll_addr_mgr_start_param_type;


/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION LL_ADDR_MGR_START()

DESCRIPTION
  This is called to start the LL address mgr.

  start() can only be followed by a stop() or any number of calls to claim().

DEPENDENCIES
  This MUST be called in mode controller context

RETURN VALUE
  handle for the link local address manager (to be used upon subsequent
  calls into the LL addr mgr)

SIDE EFFECTS
  None.
===========================================================================*/

extern void * ll_addr_mgr_start
(
  ps_iface_type                 *iface_ptr,     /* iface pointer of client */
  ll_addr_mgr_params_type       *params_ptr,    /* related parameters ptr  */
  ll_addr_mgr_ind_cback_type     ind_cback,     /* cback for notification  */
  void                          *user_data_ptr  /* user data for caller    */
);


/*===========================================================================
FUNCTION LL_ADDR_MGR_STOP()

DESCRIPTION
  This is called to stop the link local address manager: command will be
  serialized through PS task

DEPENDENCIES
  None.

RETURN VALUE
   0 : stop was taken into account
  -1 : serious error -> stop could not even be taken into account

SIDE EFFECTS
  None.
===========================================================================*/

extern int ll_addr_mgr_stop
(
  void *ll_addr_mgr_handle
);


/*===========================================================================
FUNCTION LL_ADDR_MGR_CLAIM()

DESCRIPTION
  This is called to claim an IP address on the network

  It triggers the behavior per RFC 3927 of detecting conflicts and obtaining
  a unique link local address

DEPENDENCIES
  This MUST be called in mode controller context

RETURN VALUE
   0 : claim was taken into account
  -1 : serious error -> claim could not even be taken into account

SIDE EFFECTS
  None.
===========================================================================*/

extern int ll_addr_mgr_claim
(
  void *ll_addr_mgr_handle
);

#endif /* PS_LL_ADDR_MGR_H */

