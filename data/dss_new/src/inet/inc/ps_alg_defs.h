#ifndef PS_ALG_DEFS_H
#define PS_ALG_DEFS_H
/*===========================================================================

                P S _ A L G _ D E F S . H

DESCRIPTION
   Datatype Definitions used by ALG client/manager for ALG operations

Copyright (c) 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_alg_defs.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

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

/*===========================================================================

                            MACROS

===========================================================================*/
/*===========================================================================
MACRO ALG ERRNO DEFS

DESCRIPTION
  These macros specify the errno values returned in ALG manager operations.

  SUCCESS - operation performed successfully
  EXISTS - Rule specified already registered with ALG
  NO_MEM - No memory left to register rule
  EINVAL - Rule specification has an error
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/

#define ALG_MGR_RULE_REG_SUCCESS 0x500
#define ALG_MGR_RULE_REG_EXISTS 0x501
#define ALG_MGR_RULE_REG_NO_MEM 0x502
#define ALG_MGR_RULE_REG_EINVAL 0x503
#define ALG_MGR_RULE_REG_NOT_EXISTS 0x504

/*===========================================================================
ALG_HANDLE_TYPE

DESCRIPTION
  Rule handle passed back to ALG client by ALG manager for each
    Rule registered
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef int32 alg_handle_type;

/*===========================================================================
ALG_CLIENT_READ_CB_FN

DESCRIPTION
  Rule callback specified by ALG client to be notified by ALG manager
    when a payload has to be read for a particular Rule specified by client.
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef void (*alg_client_read_cb_fn)
(
  void    *cb_data
);

/*===========================================================================
ALG_CLIENT_START_CB_FN

DESCRIPTION
  Callback specified by ALG client to be notified by ALG manager when
    rules can be registered with ALG manager
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef void (*alg_client_start_cb_fn)
(
  void    *cb_data
);

/*===========================================================================
ALG_CLIENT_STOP_CB_FN

DESCRIPTION
  Callback specified by ALG client to be notified by ALG manager when
    rules have to be deregistered
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef void (*alg_client_stop_cb_fn)
(
  void    *cb_data
);

/*===========================================================================
ALG_RULE_CONFIG_S

DESCRIPTION
  Configuration to be used by ALG client to specify in Rule registration 
    to ALG manager.
  
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef struct 
{
  uint8  protocol;
  uint16 txprt_source_port;
  uint16 txprt_dest_port;
  boolean is_lan_lookup;
  alg_client_read_cb_fn rule_cback;
  void    *user_data;
} alg_rule_config_s;


#ifdef __cplusplus
}
#endif

#endif /* FEATURE_DATA_PS_SOFTAP */
#endif /* PS_ALG_DEFS_H */
