/***************************************************************************************************
    @file
    hlos_csvt_core.c

    @brief
    Implements functions supported in hlos_csvt_core.h.

    Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
    Qualcomm Technologies Proprietary and Confidential.
***************************************************************************************************/

#include "hlos_csvt_core.h"
#include "hlos_core.h"
#include "core_handler.h"
#include "cri_csvt_core.h"
#include "cri_core.h"
#include "qcril_qmi_voice.h"
#include "qcril_qmi_ims_misc.h"
#include "qcril_qmi_ims_socket.h"

#define HLOS_CSVT_EXTENDED_CODE_MAX_LEN (10)







/***************************************************************************************************
    @function
    hlos_csvt_convert_csvt_call_state_to_ims_call_state

    @brief
    Converts the CRI CSVT call state to HLOS IMS call state.

    @param[in]
        csvt_call_state
           CRI CSVT call state that is being used for the conversion

    @param[out]
        none

    @retval
    HLOS IMS call state
***************************************************************************************************/
static Ims__CallState hlos_csvt_convert_csvt_call_state_to_ims_call_state(
    csvt_event_type_enum_v01 csvt_call_state
);





/***************************************************************************************************
    @function
    hlos_csvt_dial_response_handler

    @brief
    Handles CRI dial response.

    @param[in]
        cri_core_context
           context comprising of HLOS token id and subscription id
        cri_core_error
           error corresponding to the dial request
        hlos_cb_data
           HLOS callback data that was sent as part of dial request
        cri_resp_data
           CRI dial response data

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
static void hlos_csvt_dial_response_handler(cri_core_context_type context,
                                            cri_core_error_type cri_core_error,
                                            void *hlos_cb_data,
                                            void *cri_resp_data);





/***************************************************************************************************
    @function
    hlos_csvt_answer_response_handler

    @brief
    Handles CRI answer response.

    @param[in]
        cri_core_context
           context comprising of HLOS token id and subscription id
        cri_core_error
           error corresponding to the answer request
        hlos_cb_data
           HLOS callback data that was sent as part of answer request
        cri_resp_data
           CRI answer response data

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
static void hlos_csvt_answer_response_handler(cri_core_context_type context,
                                              cri_core_error_type cri_core_error,
                                              void *hlos_cb_data,
                                              void *cri_resp_data);





/***************************************************************************************************
    @function
    hlos_csvt_hangup_response_handler

    @brief
    Handles CRI hangup response.

    @param[in]
        cri_core_context
           context comprising of HLOS token id and subscription id
        cri_core_error
           error corresponding to the hangup request
        hlos_cb_data
           HLOS callback data that was sent as part of hangup request
        cri_resp_data
           CRI hangup response data

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
static void hlos_csvt_hangup_response_handler(cri_core_context_type context,
                                              cri_core_error_type cri_core_error,
                                              void *hlos_cb_data,
                                              void *cri_resp_data);






/***************************************************************************************************
    @function
    hlos_csvt_unsol_ind_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_unsol_ind_handler(unsigned long message_id,
                                 void *ind_data,
                                 int ind_data_len)
{
    UTIL_LOG_MSG("entry");

    qcril_qmi_ims_socket_send_empty_payload_unsol_resp(
        IMS__MSG_ID__UNSOL_RESPONSE_CALL_STATE_CHANGED
    );

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_request_handler

    @implementation detail
    None.
***************************************************************************************************/
int hlos_csvt_request_handler(Ims__MsgId request_id,
                              uint32_t token_id,
                              void *data,
                              size_t data_len)
{
    int ret_code;
    Ims__Dial *ims_dial_req_ptr;
    Ims__Hangup *ims_hangup_req_ptr;
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;

    UTIL_LOG_MSG("entry");

    ret_code = FALSE;
    ims_dial_req_ptr = NULL;
    ims_hangup_req_ptr = NULL;
    hlos_core_hlos_request_data = NULL;

    switch(request_id)
    {
        case IMS__MSG_ID__REQUEST_DIAL:
            if(data && data_len)
            {
                ims_dial_req_ptr = (Ims__Dial*) data;
                if(TRUE == ims_dial_req_ptr->calldetails->has_calldomain &&
                   IMS__CALL_DOMAIN__CALL_DOMAIN_CS == ims_dial_req_ptr->calldetails->calldomain &&
                   TRUE == ims_dial_req_ptr->calldetails->has_calltype &&
                   IMS__CALL_TYPE__CALL_TYPE_VT == ims_dial_req_ptr->calldetails->calltype)
                {
                    ret_code = TRUE;
                }
            }
            break;

        case IMS__MSG_ID__REQUEST_HANGUP:
            if(data && data_len)
            {
                ims_hangup_req_ptr = (Ims__Hangup*) data;
                if(TRUE == cri_csvt_utils_is_hlos_call_id_belongs_to_csvt_call(
                               ims_hangup_req_ptr->conn_index
                           )
                   )
                {
                    ret_code = TRUE;
                }
            }
            break;

        case IMS__MSG_ID__REQUEST_ANSWER:
        case IMS__MSG_ID__REQUEST_GET_CURRENT_CALLS:
        case IMS__MSG_ID__REQUEST_LAST_CALL_FAIL_CAUSE:
            ret_code = cri_csvt_utils_is_csvt_calls_present();
            break;

        default:
            break;
    }

    if(ret_code)
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*)
                                            util_memory_alloc(sizeof(*hlos_core_hlos_request_data));
        if(hlos_core_hlos_request_data)
        {
            hlos_core_hlos_request_data->event_id = (unsigned long) request_id;
            hlos_core_hlos_request_data->token_id =
                                            qcril_qmi_ims_convert_ims_token_to_ril_token(token_id);
            hlos_core_hlos_request_data->data = data;
            hlos_core_hlos_request_data->data_len = data_len;
            core_handler_add_event(CORE_HANDLER_HLOS_REQUEST,
                                   hlos_core_hlos_request_data);
        }
    }

    UTIL_LOG_MSG("request id %d, token id %d, data %p, data len %d - is csvt req %d",
                 request_id,
                 token_id,
                 data,
                 data_len,
                 ret_code);
    return ret_code;
}

/***************************************************************************************************
    @function
    hlos_csvt_convert_csvt_call_state_to_ims_call_state

    @implementation detail
    None.
***************************************************************************************************/
Ims__CallState hlos_csvt_convert_csvt_call_state_to_ims_call_state(
    csvt_event_type_enum_v01 csvt_call_state
)
{
    Ims__CallState ims_call_state;

    ims_call_state = IMS__CALL_STATE__CALL_DIALING;

    switch(csvt_call_state)
    {
        case CSVT_EVENT_TYPE_CONFIRM_V01:
            ims_call_state = IMS__CALL_STATE__CALL_DIALING;
            break;

        case CSVT_EVENT_TYPE_PROGRESS_V01:
            ims_call_state = IMS__CALL_STATE__CALL_ALERTING;
            break;

        case CSVT_EVENT_TYPE_CONNECT_V01:
            ims_call_state = IMS__CALL_STATE__CALL_ACTIVE;
            break;

        case CSVT_EVENT_TYPE_INCOMING_V01:
            ims_call_state = IMS__CALL_STATE__CALL_INCOMING;
            break;

        default:
            ims_call_state = IMS__CALL_STATE__CALL_DIALING;
            break;
    }

    UTIL_LOG_MSG("ims call state %d",
                 ims_call_state);
    return ims_call_state;
}

/***************************************************************************************************
    @function
    hlos_csvt_dial_request_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_dial_request_handler(void *event_data)
{
    Ims__Dial *event_msg_ptr;
    cri_core_error_type ret_val;
    cri_core_context_type cri_core_context;
    csvt_originate_call_req_msg_v01 csvt_originate_call_req_msg;
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;

    UTIL_LOG_MSG("entry");

    event_msg_ptr = NULL;
    ret_val = QMI_ERR_INTERNAL_V01;
    cri_core_context = NIL;
    memset(&csvt_originate_call_req_msg,
           NIL,
           sizeof(csvt_originate_call_req_msg));
    hlos_core_hlos_request_data = NULL;

    if(event_data)
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) event_data;
        event_msg_ptr = (Ims__Dial *) hlos_core_hlos_request_data->data;
        if(event_msg_ptr)
        {
            cri_core_context =
                cri_core_generate_context_using_subscription_id__hlos_token_id(
                    NIL,
                    hlos_core_get_token_id_value(hlos_core_hlos_request_data->token_id)
                );
            strlcpy(csvt_originate_call_req_msg.dial_string,
                    event_msg_ptr->address,
                    sizeof(csvt_originate_call_req_msg.dial_string));
            ret_val = cri_csvt_core_dial_request_handler(cri_core_context,
                                                         (void *)&csvt_originate_call_req_msg,
                                                         event_data,
                                                         hlos_csvt_dial_response_handler);

            if(QMI_ERR_NONE_V01 != ret_val)
            {
                hlos_core_send_response(IMS_PIPE,
                                        ret_val,
                                        event_data,
                                        NULL,
                                        NIL);
                qcril_qmi_ims__dial__free_unpacked(event_msg_ptr,
                                                   NULL);
            }
        }
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_dial_response_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_dial_response_handler(cri_core_context_type context,
                                     cri_core_error_type cri_core_error,
                                     void *hlos_cb_data,
                                     void *cri_resp_data)
{
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;
    cri_core_subscription_id_type subscription_id;
    cri_core_hlos_token_id_type hlos_token_id;

    UTIL_LOG_MSG("entry");

    hlos_core_hlos_request_data = NULL;
    subscription_id = NIL;
    hlos_token_id = NIL;

    cri_core_retrieve_subscription_id__hlos_token_id_from_context(context,
                                                                  &subscription_id,
                                                                  &hlos_token_id);
    if(TRUE == core_queue_util_is_event_present_with_hlos_token_id(hlos_token_id))
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) hlos_cb_data;
        hlos_core_send_response(IMS_PIPE,
                                cri_core_error,
                                hlos_cb_data,
                                NULL,
                                NIL);
        qcril_qmi_ims__dial__free_unpacked(hlos_core_hlos_request_data->data,
                                           NULL);
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_answer_request_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_answer_request_handler(void *event_data)
{
    Ims__Answer *event_msg_ptr;
    cri_core_error_type ret_val;
    cri_core_context_type cri_core_context;
    int hlos_call_id;
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;

    UTIL_LOG_MSG("entry");

    event_msg_ptr = NULL;
    ret_val = QMI_ERR_INTERNAL_V01;
    cri_core_context = NIL;
    hlos_call_id = NIL;
    hlos_core_hlos_request_data = NULL;


    if(event_data)
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) event_data;
        event_msg_ptr = (Ims__Answer *) hlos_core_hlos_request_data->data;
        if(event_msg_ptr)
        {
            cri_core_context =
                cri_core_generate_context_using_subscription_id__hlos_token_id(
                    NIL,
                    hlos_core_get_token_id_value(hlos_core_hlos_request_data->token_id)
                );

            hlos_call_id = cri_csvt_utils_find_hlos_call_id_in_csvt_call_state(
                                CSVT_EVENT_TYPE_INCOMING_V01
                           );
            if(NIL != hlos_call_id)
            {
                ret_val = cri_csvt_core_answer_request_handler(cri_core_context,
                                                               hlos_call_id,
                                                               TRUE,
                                                               NIL,
                                                               event_data,
                                                               hlos_csvt_answer_response_handler);
            }

            if(QMI_ERR_NONE_V01 != ret_val)
            {
                hlos_core_send_response(IMS_PIPE,
                                        ret_val,
                                        event_data,
                                        NULL,
                                        NIL);
                qcril_qmi_ims__answer__free_unpacked(event_msg_ptr,
                                                     NULL);
            }
        }
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_answer_response_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_answer_response_handler(cri_core_context_type context,
                                       cri_core_error_type cri_core_error,
                                       void *hlos_cb_data,
                                       void *cri_resp_data)
{
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;
    cri_core_subscription_id_type subscription_id;
    cri_core_hlos_token_id_type hlos_token_id;

    UTIL_LOG_MSG("entry");

    hlos_core_hlos_request_data = NULL;
    subscription_id = NIL;
    hlos_token_id = NIL;

    cri_core_retrieve_subscription_id__hlos_token_id_from_context(context,
                                                                  &subscription_id,
                                                                  &hlos_token_id);
    if(TRUE == core_queue_util_is_event_present_with_hlos_token_id(hlos_token_id))
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) hlos_cb_data;
        hlos_core_send_response(IMS_PIPE,
                                cri_core_error,
                                hlos_cb_data,
                                NULL,
                                NIL);
        qcril_qmi_ims__answer__free_unpacked(hlos_core_hlos_request_data->data,
                                             NULL);
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_hangup_request_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_hangup_request_handler(void *event_data)
{
    Ims__Hangup *event_msg_ptr;
    cri_core_error_type ret_val;
    cri_core_context_type cri_core_context;
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;
    Ims__CallFailCause fail_cause;

    UTIL_LOG_MSG("entry");

    event_msg_ptr = NULL;
    ret_val = QMI_ERR_INTERNAL_V01;
    cri_core_context = NIL;
    hlos_core_hlos_request_data = NULL;
    fail_cause = IMS__CALL_FAIL_CAUSE__CALL_FAIL_NORMAL;

    if(event_data)
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) event_data;
        event_msg_ptr = (Ims__Hangup *) hlos_core_hlos_request_data->data;
        if(event_msg_ptr)
        {
            cri_core_context =
                cri_core_generate_context_using_subscription_id__hlos_token_id(
                    NIL,
                    hlos_core_get_token_id_value(hlos_core_hlos_request_data->token_id)
                );

            if(TRUE == cri_csvt_utils_is_csvt_call_with_hlos_call_id_in_csvt_call_state(
                            event_msg_ptr->conn_index,
                            CSVT_EVENT_TYPE_INCOMING_V01
                       )
               )
            {
                if (event_msg_ptr->failcauseresponse &&
                    event_msg_ptr->failcauseresponse->has_failcause)
                {
                    fail_cause = event_msg_ptr->failcauseresponse->failcause;
                }
                ret_val = cri_csvt_core_answer_request_handler(cri_core_context,
                                                               event_msg_ptr->conn_index,
                                                               FALSE,
                                                               fail_cause,
                                                               event_data,
                                                               hlos_csvt_hangup_response_handler);
            }
            else
            {
                ret_val = cri_csvt_core_end_request_handler(cri_core_context,
                                                            event_msg_ptr->conn_index,
                                                            event_data,
                                                            hlos_csvt_hangup_response_handler);
            }

            if(QMI_ERR_NONE_V01 != ret_val)
            {
                hlos_core_send_response(IMS_PIPE,
                                        ret_val,
                                        event_data,
                                        NULL,
                                        NIL);
                qcril_qmi_ims__hangup__free_unpacked(event_msg_ptr,
                                                     NULL);
            }
        }
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_hangup_response_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_hangup_response_handler(cri_core_context_type context,
                                       cri_core_error_type cri_core_error,
                                       void *hlos_cb_data,
                                       void *cri_resp_data)
{
    hlos_core_hlos_request_data_type *hlos_core_hlos_request_data;
    cri_core_subscription_id_type subscription_id;
    cri_core_hlos_token_id_type hlos_token_id;

    UTIL_LOG_MSG("entry");

    hlos_core_hlos_request_data = NULL;
    subscription_id = NIL;
    hlos_token_id = NIL;

    cri_core_retrieve_subscription_id__hlos_token_id_from_context(context,
                                                                  &subscription_id,
                                                                  &hlos_token_id);
    if(TRUE == core_queue_util_is_event_present_with_hlos_token_id(hlos_token_id))
    {
        hlos_core_hlos_request_data = (hlos_core_hlos_request_data_type*) hlos_cb_data;
        hlos_core_send_response(IMS_PIPE,
                                cri_core_error,
                                hlos_cb_data,
                                NULL,
                                NIL);
        qcril_qmi_ims__hangup__free_unpacked(hlos_core_hlos_request_data->data,
                                             NULL);
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_get_current_calls_request_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_get_current_calls_request_handler(void *event_data)
{
    cri_core_error_type ret_val;
    int number_of_csvt_calls;
    int iter_resp_object;
    cri_csvt_utils_hlos_call_object_type *cri_csvt_utils_hlos_ongoing_call_objects;
    void *get_current_calls_payload;
    size_t get_current_calls_payload_len;

    UTIL_LOG_MSG("entry");

    ret_val = QMI_ERR_NONE_V01;
    number_of_csvt_calls = NIL;
    iter_resp_object = NIL;
    get_current_calls_payload = NULL;
    get_current_calls_payload_len = NIL;

    if(event_data)
    {
        number_of_csvt_calls = cri_csvt_utils_retrieve_number_of_ongoing_csvt_calls();

        Ims__CallList call_list = IMS__CALL_LIST__INIT;
        call_list.n_callattributes = number_of_csvt_calls;
        call_list.callattributes =
                            util_memory_alloc(sizeof (Ims__CallList__Call*) * number_of_csvt_calls);
        Ims__CallList__Call *calls =
                            util_memory_alloc(sizeof(Ims__CallList__Call) * number_of_csvt_calls);
        Ims__CallDetails *call_details =
                            util_memory_alloc(sizeof(Ims__CallDetails) * number_of_csvt_calls);

        if ( call_list.callattributes && calls && call_details)
        {
            cri_csvt_utils_hlos_ongoing_call_objects =
                                                cri_csvt_utils_retrieve_hlos_ongoing_call_objects();

            if(NULL != cri_csvt_utils_hlos_ongoing_call_objects)
            {
                for ( iter_resp_object = 0;
                      iter_resp_object < number_of_csvt_calls;
                      iter_resp_object++ )
                {
                    call_list.callattributes[iter_resp_object] = &calls[iter_resp_object];

                    Ims__CallList__Call callTemp = IMS__CALL_LIST__CALL__INIT;
                    memcpy(&(calls[iter_resp_object]), &callTemp, sizeof(Ims__CallList__Call));

                    calls[iter_resp_object].has_state = TRUE;
                    calls[iter_resp_object].state     =
                        hlos_csvt_convert_csvt_call_state_to_ims_call_state(
                         cri_csvt_utils_hlos_ongoing_call_objects[iter_resp_object].csvt_call_state
                        );
                    calls[iter_resp_object].has_index = TRUE;
                    calls[iter_resp_object].index     =
                        cri_csvt_utils_hlos_ongoing_call_objects[iter_resp_object].hlos_call_id;
                    calls[iter_resp_object].has_ismpty = TRUE;
                    calls[iter_resp_object].ismpty    = FALSE;
                    calls[iter_resp_object].isvoice   = FALSE;
                    calls[iter_resp_object].name   = NULL;
                    calls[iter_resp_object].number   =
                     cri_csvt_utils_hlos_ongoing_call_objects[iter_resp_object].remote_party_number;
                    calls[iter_resp_object].numberpresentation = QCRIL_QMI_VOICE_RIL_PI_ALLOWED;
                    calls[iter_resp_object].has_ismt  = TRUE;
                    calls[iter_resp_object].ismt      =
                        cri_csvt_utils_hlos_ongoing_call_objects[iter_resp_object].is_mt;
                    calls[iter_resp_object].has_toa   = TRUE;
                    if ( calls[iter_resp_object].number[0] == QCRIl_QMI_VOICE_SS_TA_INTER_PREFIX )
                    {
                      calls[iter_resp_object].toa = QCRIL_QMI_VOICE_SS_TA_INTERNATIONAL;
                    }
                    else
                    {
                      calls[iter_resp_object].toa = QCRIL_QMI_VOICE_SS_TA_UNKNOWN;
                    }

                    calls[iter_resp_object].calldetails = &(call_details[iter_resp_object]);
                    Ims__CallDetails call_detailsTemp = IMS__CALL_DETAILS__INIT;
                    memcpy(&(call_details[iter_resp_object]),
                           &call_detailsTemp,
                           sizeof(Ims__CallDetails));
                    call_details[iter_resp_object].has_calltype = TRUE;
                    call_details[iter_resp_object].calltype = IMS__CALL_TYPE__CALL_TYPE_VT;
                    call_details[iter_resp_object].has_calldomain = TRUE;
                    call_details[iter_resp_object].calldomain = IMS__CALL_DOMAIN__CALL_DOMAIN_CS;
                }
                util_memory_free((void**) &cri_csvt_utils_hlos_ongoing_call_objects);
            }

            for ( iter_resp_object = 0;
                  iter_resp_object < number_of_csvt_calls;
                  iter_resp_object++ )
            {
                UTIL_LOG_MSG("hlos call object id %d, call id %d, state %d, is mt %d, number %s",
                             (iter_resp_object + 1),
                             calls[iter_resp_object].index,
                             calls[iter_resp_object].state,
                             calls[iter_resp_object].ismt,
                             calls[iter_resp_object].number);
            }

            get_current_calls_payload = &call_list;
            get_current_calls_payload_len = sizeof(call_list);
        }

        if(NULL == get_current_calls_payload)
        {
            UTIL_LOG_MSG("no hlos call objects to be reported");
        }

        hlos_core_send_response(IMS_PIPE,
                                ret_val,
                                event_data,
                                get_current_calls_payload,
                                get_current_calls_payload_len);

        if (call_list.callattributes)
        {
          util_memory_free((void**) &call_list.callattributes);
        }
        if (calls)
        {
          util_memory_free((void**) &calls);
        }
        if (call_details)
        {
          util_memory_free((void**) &call_details);
        }
    }

    UTIL_LOG_MSG("exit");
}

/***************************************************************************************************
    @function
    hlos_csvt_last_call_failure_cause_request_handler

    @implementation detail
    None.
***************************************************************************************************/
void hlos_csvt_last_call_failure_cause_request_handler(void *event_data)
{
    cri_core_error_type ret_val;
    char error_info_data[HLOS_CSVT_EXTENDED_CODE_MAX_LEN + 1];
    uint32_t csvt_last_call_failure_cause;
    void *last_call_failure_cause_payload;
    size_t last_call_failure_cause_payload_len;

    UTIL_LOG_MSG("entry");

    ret_val = QMI_ERR_INTERNAL_V01;
    memset(error_info_data,
           NIL,
           sizeof(error_info_data));
    csvt_last_call_failure_cause = NIL;
    last_call_failure_cause_payload = NULL;
    last_call_failure_cause_payload_len = NIL;

    if(event_data)
    {
        csvt_last_call_failure_cause = cri_csvt_utils_retrieve_last_call_failure_cause();
        if(NIL != csvt_last_call_failure_cause)
        {
            Ims__CallFailCauseResponse last_call_failure_cause =
                                                            IMS__CALL_FAIL_CAUSE_RESPONSE__INIT;
            last_call_failure_cause.has_failcause = TRUE;
            last_call_failure_cause.failcause =  IMS__CALL_FAIL_CAUSE__CALL_FAIL_MISC;

            snprintf(error_info_data, sizeof(error_info_data),
                     "%d",
                     csvt_last_call_failure_cause);
            last_call_failure_cause.has_errorinfo = TRUE;
            last_call_failure_cause.errorinfo.len =  strlen(error_info_data);
            last_call_failure_cause.errorinfo.data =  (uint8_t*) error_info_data;

            last_call_failure_cause_payload = &last_call_failure_cause;
            last_call_failure_cause_payload_len = sizeof(last_call_failure_cause);

            ret_val = QMI_ERR_NONE_V01;
        }
        hlos_core_send_response(IMS_PIPE,
                                ret_val,
                                event_data,
                                last_call_failure_cause_payload,
                                last_call_failure_cause_payload_len);
    }

    UTIL_LOG_MSG("exit");
}

