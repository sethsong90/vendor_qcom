#ifndef QCRIL_UIM_CARD_H
#define QCRIL_UIM_CARD_H
/*===========================================================================

  Copyright (c) 2010-2011 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //depot/asic/sandbox/users/micheleb/ril/qcril_uim_card.h#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/10/13   at      Support for clearing proactive cmd cache on card error
04/09/12   at      Added support for RIL_REQUEST_SIM_GET_ATR
01/11/11   at      Removed refresh function declarations, added others
05/12/10   at      Initial version
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "qcril_uim.h"

/*===========================================================================

  FUNCTION:  qcril_uim_init_card_status

===========================================================================*/
/*!
    @brief
    Initializes the global card status structure.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_init_card_status
(
  qmi_uim_card_status_type  * card_status_ptr
);


/*===========================================================================

  FUNCTION:  qcril_uim_process_status_change_ind

===========================================================================*/
/*!
    @brief
    Main function for processing QMI card status changed indication. Based
    on the indication received, if needed, it updates the global card status,
    ret_ptr and sends card events internal to QCRIL (CM & PBM).

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_process_status_change_ind
(
  const qcril_uim_indication_params_type  * ind_param_ptr,
  qcril_request_return_type               * const ret_ptr /*!< Output parameter */
);


/*=========================================================================

  FUNCTION:  qcril_uim_process_power_down

===========================================================================*/
/*!
    @brief
    Process the power down request.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_process_power_down
(
  qcril_instance_id_e_type          instance_id,
  qcril_modem_id_e_type             modem_id,
  int                               slot
);


/*=========================================================================

  FUNCTION:  qcril_uim_process_power_down

===========================================================================*/
/*!
    @brief
    Process the power up request.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_process_power_up
(
  qcril_instance_id_e_type          instance_id,
  qcril_modem_id_e_type             modem_id,
  int                               slot
);


/*===========================================================================

  FUNCTION:  qcril_uim_update_cm_card_status

===========================================================================*/
/*!
    @brief
    Update QCRIL(CM) card status per UIM card state.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_update_cm_card_status
(
  qcril_instance_id_e_type instance_id,
  qcril_modem_id_e_type    modem_id,
  int                      slot,
  qcril_card_status_e_type new_card_status
);


/*===========================================================================

  FUNCTION:  qcril_uim_update_pbm_card_event

===========================================================================*/
/*!
    @brief
    Update QCRIL(PBM) card event per MMGSDI card state.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_update_pbm_card_event
(
  qcril_instance_id_e_type instance_id,
  qcril_modem_id_e_type    modem_id,
  int                      slot,
  qcril_evt_e_type         pbm_card_event
);


/*===========================================================================

  FUNCTION:  qcril_uim_update_gstk_card_event

===========================================================================*/
/*!
    @brief
    Update QCRIL GSTK with card error/absent status.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_update_gstk_card_event
(
  qcril_instance_id_e_type instance_id,
  qcril_modem_id_e_type    modem_id,
  int                      slot
);


/*=========================================================================

  FUNCTION:  qcril_uim_process_change_subscription

===========================================================================*/
/*!
    @brief
    Processes the subscription activation/deactivation requests from QCRIL.
    called as a result of QCRIL_EVT_INTERNAL_MMGSDI_ACTIVATE_SUB or
    QCRIL_EVT_INTERNAL_MMGSDI_DEACTIVATE_SUB.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_process_change_subscription
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
);

/*=========================================================================

  FUNCTION:  qcril_uim_update_prov_session_type

===========================================================================*/
/*!
    @brief
    Updates the global provisioning session status type based on the passed
    session type.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_update_prov_session_type
(
  qmi_uim_session_type                  session_type,
  qcril_uim_prov_session_state_type     session_state
);

#ifdef FEATURE_QCRIL_UIM_QMI_GET_ATR
/*=========================================================================

  FUNCTION:  qcril_uim_get_atr_resp

===========================================================================*/
/*!
    @brief
    Processes the response for get ATR command.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_get_atr_resp
(
  const qcril_uim_callback_params_type * const params_ptr
);
#endif /* FEATURE_QCRIL_UIM_QMI_GET_ATR */

RIL_Errno qcril_uim_direct_get_card_status( qcril_instance_id_e_type instance_id, RIL_CardStatus_v6 * ril_card_status );

#endif /* QCRIL_UIM_CARD_H */

