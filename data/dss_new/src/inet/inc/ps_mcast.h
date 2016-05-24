#ifndef PS_MCAST_H
#define PS_MCAST_H
/*===========================================================================
           Internet Group Management Protocol (IGMP)
                            And
                Multicast Listener Discovery (MLD)

                   A P I   H E A D E R   F I L E

DESCRIPTION
 The IGMP and MLD API header file.


Copyright (c) 2006-2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


$Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_mcast.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $


when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/28/07    rs     Added ps_mcast_join() and ps_mcast_leave() prototypes
11/15/06    rt     Created module.

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"
#ifdef FEATURE_DATA_PS
#include "dsm.h"
#include "dssocket.h"
#include "ps_in.h"
#include "ps_iface_defs.h"

/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/


/*---------------------------------------------------------------------------
  IGMP/MLD structure to send JOIN/LEAVE message.
---------------------------------------------------------------------------*/

typedef struct {
  ps_iface_type        * iface_ptr;
  ps_ip_addr_type        mcast_grp;
} ps_mcast_send_msg_type;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION PS_MCAST_SEND_REPORT_MSG

DESCRIPTION
  This function is used to send a IGMP/MLD JOIN message. This function sends
  a command to the PS task so that the actual message is sent in the PS task
  context.

DEPENDENCIES
  None.

RETURN VALUE
  Returns AEE_SUCCESS if it is successful else return ERROR.

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_mcast_send_report_msg
(
  ps_iface_type       * iface_ptr,
  ps_ip_addr_type     * mcast_grp,
  int16               * ps_errno
);


/*===========================================================================

FUNCTION PS_MCAST_SEND_LEAVE_MSG

DESCRIPTION
  This function is used to send a IGMP/MLD LEAVE message. This function sends
  a command to the PS task so that the actual message is sent in the PS task
  context.

DEPENDENCIES
  None.

RETURN VALUE
  Returns AEE_SUCCESS if it is successful else return ERROR.

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_mcast_send_leave_msg
(
  ps_iface_type       * iface_ptr,
  ps_ip_addr_type     * mcast_grp,
  int16               * ps_errno
);

/*===========================================================================

FUNCTION PS_MCAST_JOIN

DESCRIPTION
  This function initiates sending of JOIN request for a multicast group.

DEPENDENCIES
  None

RETURN VALUE
  Returns AEE_SUCCESS if it is successful else returns appropriate error message

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_mcast_join
(
  void             * m_info,
  int16            * ps_errno
);

/*===========================================================================

FUNCTION PS_MCAST_LEAVE

DESCRIPTION
  This function initiates sending of LEAVE request for a multicast group.

DEPENDENCIES
  None

RETURN VALUE
  Returns AEE_SUCCESS if it is successful else returns appropriate error message

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_mcast_leave
(
  void             * m_info,
  int16            * ps_errno
);

#endif /* FEATURE_DATA_PS */
#endif  /* PS_MCAST_H */
