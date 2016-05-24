#ifndef PS_ALG_MGRI_H
#define PS_ALG_MGRI_H
/*===========================================================================

                P S _ A L G _ M G R I . H

DESCRIPTION
   Internal API definitions for ALG manager operations

Copyright (c) 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_alg_mgri.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS_SOFTAP

#ifdef __cplusplus
extern "C" {
#endif

#include "dsm.h"
#ifdef TEST_FRAMEWORK
#include "ps_nat.h"
#endif /* TEST_FRAMEWORK */

/*===========================================================================

                           EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
NAT_ALG_READ_CB_FN

DESCRIPTION
  Rule callback specified by ALG manager to ALG table for rule hits

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
typedef int (*nat_alg_read_cb_fn)
(
  uint32           rule_handle,
  dsm_item_type ** pkt_chain_ptr
);


/*===========================================================================
FUNCTION PS_ALG_MGR_INIT()

DESCRIPTION
  This API initiates the ALG manager

PARAMETERS
  Pointer to NAT iface
  .
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_alg_mgr_init
(
  void   *nat_iface_ptr
);

/*===========================================================================
FUNCTION PS_ALG_MGR_CLEANUP()

DESCRIPTION
  This API cleans up the ALG manager

PARAMETERS
  None
  .
DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_alg_mgr_cleanup
(
  void
);

#ifdef TEST_FRAMEWORK

typedef void (*ps_alg_mgr_test_cb_type)
(
  dsm_item_type      *pkt_chain_ptr,
  nat_pkt_direction_e pkt_tx_dir,
  uint32              user_data
);

void ps_alg_mgr_register_test_cb
(
  ps_alg_mgr_test_cb_type cb,
  uint32                  user_data
);

#endif /* TEST_FRAMEWORK*/

#ifdef __cplusplus
}
#endif

#endif /* FEATURE_DATA_PS_SOFTAP */
#endif /* PS_ALG_MGRI_H */
