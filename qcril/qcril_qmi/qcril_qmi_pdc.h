/******************************************************************************
  @file    qcril_qmi_pdc.h
  @brief   qcril qmi - PDC

  DESCRIPTION
    Handles RIL requests, Callbacks, indications for QMI NAS.

  ---------------------------------------------------------------------------

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


#ifndef QCRIL_QMI_PDC_H
#define QCRIL_QMI_PDC_H


/*===========================================================================

                           INCLUDE FILES

============================================================================*/

#include "comdef.h"
#include "qmi_client.h"

/*===========================================================================

                        DEFINITIONS AND TYPES

 ===========================================================================*/


// modem test modes
typedef enum
{
    TM_CMCC_DEFAULT = 0,
    TM_CMCC_LAB,
    TM_CMCC_FIELD,
    TM_CMCC_LAB_CSFB,
    TM_CT_DEFAULT,
    TM_CTA_DEFAULT,
    TM_CTA_CDMA,
    TM_CTA_TDS_CDMA,
    TM_GCF_DEFAULT,
    TM_GCF_2G_PROTOCAL_343,
    TM_GCF_2G_SIM,
    TM_GCF_3G_SIM,
    TM_COUNTS,
} qmi_ril_modem_test_mode_type;


void qcril_qmi_pdc_set_modem_test_mode
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_query_modem_test_mode
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

qmi_client_error_type qcril_qmi_pdc_init
(
  void
);

void qcril_qmi_pdc_load_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);
void qcril_qmi_pdc_select_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_activate_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_deactivate_current_config
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_cleanup_loaded_configs
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_delete_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_list_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_pdc_unsol_ind_cb
(
  qmi_client_type       user_handle,
  unsigned long         msg_id,
  unsigned char         *ind_buf,
  int                   ind_buf_len,
  void                  *ind_cb_data
);

#endif
