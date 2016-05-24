#ifndef PS_ALG_MGR_H
#define PS_ALG_MGR_H
/*===========================================================================

                P S _ A L G _ M G R . H

DESCRIPTION
   API Definitions used by ALG clients for ALG operations

Copyright (c) 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_alg_mgr.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS_SOFTAP
#ifdef __cplusplus
extern "C"
{
#endif

#include "dsm.h"
#include "ps_alg_defs.h"
#include "dssocket_defs.h"

/*===========================================================================

                           EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_ALG_MGR_REGISTER_RULE()

DESCRIPTION
  This API is used to register a new rule with ALG manager

PARAMETERS
  (in)config_ptr - Pointer to Rule specification
  (out)rule_handle - upon successful registration a handle is returned to
                     ALG client
  (out)errno - if operation failed, errno value is set appropriately

RETURNS
  0 on SUCCESS, -1 on FAILURE
  .
DEPENDENCIES
  Rule to be registered after ALG client gets sys start callback from manager

===========================================================================*/
int ps_alg_mgr_register_rule
(
  alg_rule_config_s    *config_ptr,
  alg_handle_type      *rule_handle,
  int16                *errno_val
);

/*===========================================================================
FUNCTION PS_ALG_MGR_DELETE_RULE()

DESCRIPTION
  This API is used to delete a registered rule with ALG manager

PARAMETERS
  (out)del_handle - Rule to be deleted
  (out)errno - if operation failed, errno value is set appropriately

RETURNS
  0 on SUCCESS, -1 on FAILURE
  .
DEPENDENCIES
  Rule should have been registered previously with ALG manager
===========================================================================*/
int ps_alg_mgr_delete_rule
(
  alg_handle_type        del_handle,
  int16                 *errno_val
);

/*===========================================================================
FUNCTION PS_ALG_MGR_READ()

DESCRIPTION
  Performs read operation for a registered rule.

PARAMETERS
  (in)rule_handle - Rule for which packet is to be read

RETURNS
  Valid pointer to packet read, NULL otherwise
  .
DEPENDENCIES
  Rule should have been registered previously with ALG manager.
  Read callback for registered rule with ALG manager must have been invoked
===========================================================================*/
dsm_item_type * ps_alg_mgr_read
(
  alg_handle_type rule_handle
);

/*===========================================================================
FUNCTION PS_ALG_MGR_WRITE_TO_WWAN()

DESCRIPTION
  Performs write operation for a registered rule. Intention is to send
    packet to WWAN side.

PARAMETERS
  (in)rule_handle - Rule for which packet has to be written
  (in)pkt_chain_ptr - Ptr to packet
  (in)nat_global_port - NAT port to which this Rule applies

RETURNS
  0 on SUCCESS, -1 on FAILURE
  .
DEPENDENCIES
  Rule should have been registered previously with ALG manager 
===========================================================================*/
int ps_alg_mgr_write_to_wwan
(
  alg_handle_type  rule_handle,
  dsm_item_type  **pkt_chain_ptr,
  uint16           nat_global_port
);

/*===========================================================================
FUNCTION PS_ALG_MGR_WRITE_TO_LAN()

DESCRIPTION
  Performs write operation for a registered rule. Intention is to send
    packet to WLAN side.

PARAMETERS
  (in)rule_handle - Rule for which packet has to be written
  (in)pkt_chain_ptr - Ptr to packet
  (in)nat_global_port - NAT port to which this Rule applies

RETURNS
  0 on SUCCESS, -1 on FAILURE
  .
DEPENDENCIES
  Rule should have been registered previously with ALG manager 
===========================================================================*/
int ps_alg_mgr_write_to_lan
(
  alg_handle_type  rule_handle,
  dsm_item_type  **pkt_chain_ptr,
  uint16           nat_global_port
);

/*===========================================================================
FUNCTION PS_ALG_MGR_REGISTER_DYNAMIC_NAT_ENTRY()

DESCRIPTION
  Enables ALG client to add a dynamic entry to NAT Table.
  If entry exists, timeout value is updated.

PARAMETERS
  (in)iface_id - id for NAT iface
  (in)protocol - Transport layer protocol
  (in)priv_port - Local LAN port
  (in)src_port - Global NAT port
  (in)dst_port - Global target port
  (in)priv_ip - Local LAN IP
  (in)src_ip - Global NAT IP
  (in)dst_ip - Global target IP


RETURNS
  0 on SUCCESS, -1 on FAILURE
  .
DEPENDENCIES
  Rule should have been registered previously with ALG manager 
===========================================================================*/
int ps_alg_mgr_register_dynamic_nat_entry
(
  uint8  protocol,
  uint16 priv_port,
  uint16 src_port,
  uint16 dst_port,
  uint32 priv_ip,
  uint32 src_ip,
  uint32 dst_ip
);

/*===========================================================================
FUNCTION PS_ALG_MGR_REGISTER_DYNAMIC_NAT_ENTRY()

DESCRIPTION
  Enables ALG client to register start/stop callbacks.
  After sys start is called, ALG client has register new rule with manager.
  After sys stop is called, ALG client should start rule cleanups

PARAMETERS
  (in)start_cb - callback to start API
  (in)start_data - data to be passed along with start_cb
  (in)stop_cb - callback to stop API
  (in)stop_data - data to be passed along with stop_cb

RETURNS
  None
  .
DEPENDENCIES
  None
===========================================================================*/
void ps_alg_mgr_sys_register
(
  alg_client_start_cb_fn    start_cb,
  void                     *start_data,
  alg_client_stop_cb_fn     stop_cb,
  void                     *stop_data
);

#ifdef __cplusplus
}
#endif
#endif /* FEATURE_DATA_PS_SOFTAP */
#endif /* PS_ALG_MGR_H */
