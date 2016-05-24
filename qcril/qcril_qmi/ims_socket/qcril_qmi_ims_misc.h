/*!
  @file
  qcril_qmi_ims_misc.h

  @brief

*/

/*===========================================================================

  Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

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

#ifndef QCRIL_QMI_IMS_MISC_H
#define QCRIL_QMI_IMS_MISC_H


#include "ril.h"
#include "qcrili.h"
#include "qcril_qmi_voice.h"
#include "qcril_qmi_imsa.h"
#include "ip_multimedia_subsystem_application_v01.h"

Ims__MsgId qcril_qmi_ims_map_event_to_request(int event);

Ims__Error qcril_qmi_ims_map_ril_error_to_ims_error(int ril_error);

Ims__Registration__RegState qcril_qmi_ims_map_ril_reg_state_to_ims_reg_state(int state);

Ims__CallType qcril_qmi_ims_map_ril_call_type_to_ims_call_type(RIL_Call_Type call_type);
RIL_Call_Type qcril_qmi_ims_map_ims_call_type_to_ril_call_type(boolean has_call_type, Ims__CallType call_type);

Ims__CallDomain qcril_qmi_ims_map_ril_call_domain_to_ims_call_domain(RIL_Call_Domain call_domain);
RIL_Call_Domain qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain(boolean has_call_domain, Ims__CallDomain call_domain);

RIL_Token qcril_qmi_ims_convert_ims_token_to_ril_token(uint32_t ims_token);
uint32_t qcril_qmi_ims_free_and_convert_ril_token_to_ims_token(RIL_Token ril_token);

void qcril_qmi_ims_translate_ril_callmodify_to_ims_callmodify(const RIL_Call_Modify* ril_data, Ims__CallModify* ims_data);

void qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails(const RIL_Call_Details* ril_data, Ims__CallDetails* ims_data);
void qcril_qmi_ims_translate_ims_calldetails_to_ril_calldetails(const Ims__CallDetails *ims_data, RIL_Call_Details* ril_data);

void qcril_qmi_ims_translate_ril_callforwdinfo_to_ims_callforwdinfo(const qcril_qmi_voice_callforwd_info_param_u_type* ril_data, int num, Ims__CallForwardInfoList* ims_data);

void qcril_qmi_ims_translate_ril_service_status_class_to_ims_callwaitinginfo(int service_status, int service_class, Ims__CallWaitingInfo* ims_data);
Ims__Registration__RegState qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state(uint8_t ims_registered);

void qcril_qmi_ims_translate_ril_suppsvcnotification_to_ims_suppsvcnotification(const RIL_SuppSvcNotification* ril_data, Ims__SuppSvcNotification* ims_data);

void qcril_qmi_ims_translate_ril_callcapabilities_to_ims_srvstatusinfo(const voice_ip_call_capabilities_info_type_v02* ril_data, Ims__SrvStatusList* ims_data);

Ims__Info* qcril_qmi_ims_create_ims_info(
    Ims__CallType type,
    imsa_service_status_enum_v01 status,
    boolean rat_valid,
    imsa_service_rat_enum_v01 rat
);
Ims__SrvStatusList* qcril_qmi_ims_create_ims_srvstatusinfo(const qcril_qmi_imsa_srv_status_type* qmi_data);

void qcril_qmi_ims_free_srvstatuslist(Ims__SrvStatusList* ims_srv_status_list_ptr);
void qcril_qmi_ims_free_ims_info(Ims__Info* ims_info_ptr);

Ims__Handover* qcril_qmi_ims_create_ims_handover_from_imsa_rat_info(const imsa_rat_handover_status_info_v01* qmi_data);
void qcril_qmi_ims_free_ims_handover(Ims__Handover* ims_handover_ptr);

Ims__CallFailCause qcril_qmi_ims_map_ril_failcause_to_ims_failcause(RIL_LastCallFailCause ril_failcause, int ims_extended_error_code);

Ims__SuppSvcFacilityType qcril_qmi_voice_map_qmi_reason_to_ims_facility
(
  /* Reason code from QMI Voice Get Call Barring response message */
  voice_cc_sups_result_reason_enum_v02 reason
);
#endif /* QCRIL_QMI_IMS_MISC_H */
