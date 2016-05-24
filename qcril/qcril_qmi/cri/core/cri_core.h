/***************************************************************************************************
    @file
    cri_core.h

    @brief
    Supports functions for performing operations on/using qmi clients
    Primary use would be to create, release qmi clients and send, recv messages using the same.

    Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
    Qualcomm Technologies Proprietary and Confidential.
***************************************************************************************************/

#ifndef CRI_CORE
#define CRI_CORE

#include "utils_common.h"
#include "qmi.h"
#include "qmi_i.h"
#include "common_v01.h"
#include "qmi_client.h"

typedef uint32_t cri_core_hlos_token_id_type;
typedef uint16_t cri_core_token_id_type;
typedef uint64_t cri_core_context_type;
typedef qmi_error_type_v01 cri_core_error_type; //TODO : Can be a enum

typedef void (*hlos_ind_cb_type)(unsigned long message_id,
                                 void *ind_data,
                                 int ind_data_len);
typedef void (*hlos_resp_cb_type)(cri_core_context_type context,
                                  cri_core_error_type cri_core_error,
                                  void *hlos_cb_data,
                                  void *cri_resp_data);

typedef enum {
    CRI_CORE_PRIMARY_CRI_SUBSCRIPTION_ID = 0x1,
    CRI_CORE_SECONDARY_CRI_SUBSCRIPTION_ID = 0x2,
    CRI_CORE_TERTIARY_CRI_SUBSCRIPTION_ID = 0x3,
    CRI_CORE_MAX_CRI_SUBSCRIPTION_ID = 0xF
}cri_core_subscription_id_type;

#define CRI_CORE_MAX_CLIENTS (QMI_MAX_SERVICES)

#define CRI_CORE_MINIMAL_TIMEOUT (5)
#define CRI_CORE_STANDARD_TIMEOUT (30)
#define CRI_CORE_MAX_TIMEOUT (60)

typedef enum cri_core_message_category_type
{
    RESP = 1,
    IND,
    QMUXD
}cri_core_message_category_type;

typedef struct cri_core_cri_message_data_type
{
    cri_core_message_category_type cri_message_category;
    unsigned long event_id;
    qmi_client_type user_handle;
    void *data;
    size_t data_len;
    void *cb_data;
    qmi_client_error_type transport_error;
}cri_core_cri_message_data_type;

typedef struct cri_core_cri_client_init_service_info_type
{
    qmi_service_id_type cri_service_id;
    hlos_ind_cb_type hlos_ind_cb;
}cri_core_cri_client_init_service_info_type;

typedef struct cri_core_cri_client_init_info_type
{
    cri_core_subscription_id_type subscription_id;
    int number_of_cri_services_to_be_initialized;
    cri_core_cri_client_init_service_info_type service_info[CRI_CORE_MAX_CLIENTS];
}cri_core_cri_client_init_info_type;





/***************************************************************************************************
    @function
    cri_core_start

    @brief
    Initializes the CRI framework. Needs to be called before initializing any services via
    cri_core_cri_client_init.

    @param[in]
        none

    @param[out]
        none

    @retval
    ESUCCESS if CRI framework has been successfully initialized, appropriate error code otherwise
***************************************************************************************************/
int cri_core_start();






/***************************************************************************************************
    @function
    cri_core_cri_client_init

    @brief
    Initializes all the QMI services requested by the caller. Needs to be called before attempting
    to send or receive messages to/from modem through CRI.

    @param[in]
        client_init_info
            contains information about all the services that need to be initialized.

    @param[out]
        none

    @retval
    QMI_ERR_NONE_V01 if CRI framework has been successfully initialized, appropriate error code
    otherwise
***************************************************************************************************/
qmi_error_type_v01 cri_core_cri_client_init(cri_core_cri_client_init_info_type *client_init_info);





/***************************************************************************************************
    @function
    cri_core_cri_client_release

    @brief
    Releases the QMI services created using cri_core_cri_client_init().

    @param[in]
        none

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
void cri_core_cri_client_release();





/***************************************************************************************************
    @function
    cri_core_generate_context_using_subscription_id__hlos_token_id

    @brief
    Generates the context using subscription id and HLOS token id.

    @param[in]
        subscription_id
            subscription id for which the context is being requested for
        hlos_token_id
            HLOS token id for which the context is being requested for

    @param[out]
        none

    @retval
    context calculated using subscription id and HLOS token id
***************************************************************************************************/
cri_core_context_type cri_core_generate_context_using_subscription_id__hlos_token_id(
                            cri_core_subscription_id_type subscription_id,
                            cri_core_hlos_token_id_type hlos_token_id);





/***************************************************************************************************
    @function
    cri_core_generate_context_using_cri_token_id

    @brief
    Generates the context using CRI token id.

    @param[in]
        cri_core_context
           context to which CRI token id needs to be attached
        cri_core_token_id
            CRI token id for which the context is being requested for

    @param[out]
        none

    @retval
    context which would have HLOS token id, subscription id and CRI token id
***************************************************************************************************/
cri_core_context_type cri_core_generate_context_using_cri_token_id(
                            cri_core_context_type cri_core_context,
                            cri_core_token_id_type cri_core_token_id);





/***************************************************************************************************
    @function
    cri_core_retrieve_cri_token_id_from_context

    @brief
    Retrieves the CRI token id from the context.

    @param[in]
        cri_core_context
           context from which CRI token id needs to be retrieved

    @param[out]
        cri_core_token_id
            pointer to CRI token id which has been retrieved from the provided context

    @retval
    none
***************************************************************************************************/
void cri_core_retrieve_cri_token_id_from_context(cri_core_context_type cri_core_context,
                                                 cri_core_token_id_type *cri_core_token_id);





/***************************************************************************************************
    @function
    cri_core_retrieve_subscription_id__hlos_token_id_from_context

    @brief
    Retrieves the CRI token id from the context.

    @param[in]
        cri_core_context
           context from which subscription id and HLOS token id need to be retrieved

    @param[out]
        subscription_id
            pointer to subscription id which has been retrieved from the provided context
        hlos_token_id
            pointer to HLOS token id which has been retrieved from the provided context

    @retval
    none
***************************************************************************************************/
void cri_core_retrieve_subscription_id__hlos_token_id_from_context(
           cri_core_context_type cri_core_context,
           cri_core_subscription_id_type *subscription_id,
           cri_core_hlos_token_id_type *hlos_token_id);





/***************************************************************************************************
    @function
    cri_core_create_loggable_context

    @brief
    Calculates the string that can be used for logging the context.
    Should only be called from the core handler thread - Is NOT thread safe

    @param[in]
        cri_core_context
           context which needs to be logged.

    @param[out]
        none

    @retval
    string that contains the loggable format of the context
    format of the string is:
    context <context> (hlos token id <hlos token id>, sub id <sub id>, cri token id <cri token id>)
***************************************************************************************************/
char* cri_core_create_loggable_context(cri_core_context_type cri_core_context);





/***************************************************************************************************
    @function
    cri_core_retrieve_err_code

    @brief
    Calculates the error code from a qmi response.

    @param[in]
        transport_error
           error code returned by the qmi framework when a message is sent using sync/async
           mechanism
        resp_err
           QMI response TLV that is part of every QMI response message
           most of the QMI response messages have the response TLV as the first TLV of the message

    @param[out]
        none

    @retval
    QMI_ERR_NONE_V01 if message has been sent succesfully and response is also successful,
    appropriate error code otherwise
***************************************************************************************************/
qmi_error_type_v01 cri_core_retrieve_err_code(qmi_client_error_type transport_error,
                                              qmi_response_type_v01* resp_err);





/***************************************************************************************************
    @function
    cri_core_create_qmi_client

    @brief
    Initializes QMI framework that would be used by CRI.

    @param[in]
        sys_event_rx_hdlr
           callback function that would be called by the QMI framework for delivering
           framework specific notifications

    @param[out]
        none

    @retval
    QMI framework handle if QMI framework has been initialized succesfully,
    appropriate negative error code otherwise
***************************************************************************************************/
int cri_core_create_qmi_client(qmi_sys_event_rx_hdlr sys_event_rx_hdlr);





/***************************************************************************************************
    @function
    cri_core_create_qmi_service_client

    @brief
    Initializes QMI service client.

    @param[in]
        service_id
           service id of the service that needs to be initialized
        hlos_ind_cb
           callback function that would be called by the QMI framework for delivering
           service specific notifications

    @param[out]
        none

    @retval
    CRI client ID if QMI service has been initialized succesfully, QMI_INTERNAL_ERR otherwise
***************************************************************************************************/
int cri_core_create_qmi_service_client(qmi_service_id_type service_id,
                                       hlos_ind_cb_type hlos_ind_cb);






/***************************************************************************************************
    @function
    cri_core_release_qmi_client

    @brief
    Releases QMI framework.

    @param[in]
        qmi_client_handle
           QMI client handle returned by cri_core_create_qmi_client

    @param[out]
        none

    @retval
    QMI_NO_ERR if QMI framework has been released succesfully, appropriate error code otherwise
***************************************************************************************************/
int cri_core_release_qmi_client(int qmi_client_handle);






/***************************************************************************************************
    @function
    cri_core_release_qmi_service_client

    @brief
    Releases QMI service client.

    @param[in]
        qmi_service_client_id
           CRI client ID returned by cri_core_create_qmi_service_client

    @param[out]
        none

    @retval
    QMI_NO_ERR if QMI service client has been released succesfully, appropriate error code otherwise
***************************************************************************************************/
int cri_core_release_qmi_service_client(int qmi_service_client_id);





/***************************************************************************************************
    @function
    cri_core_qmi_send_msg_sync

    @brief
    Sends QMI message using sync mechanism to the QMI service server on modem.

    @param[in]
        qmi_service_client_id
           CRI client ID returned by cri_core_create_qmi_service_client
        message_id
           message id of the message that needs to be sent
        req_message
           pointer to the request message that needs to be sent
        req_message_len
           length of the request message that needs to be sent
        resp_message
           pointer to the response message that would be received in response to the sent request
        resp_message_len
           length of the response message that would be received
        timeout_secs
           number of seconds to wait for the response message from the QMI service server

    @param[out]
        none

    @retval
    QMI_ERR_NONE_V01 if QMI message has been sent successfully and a response has been received,
    appropriate error code otherwise
***************************************************************************************************/
qmi_error_type_v01 cri_core_qmi_send_msg_sync(int qmi_service_client_id,
                                              unsigned long message_id,
                                              void *req_message,
                                              int req_message_len,
                                              void *resp_message,
                                              int resp_message_len,
                                              int timeout_secs);





/***************************************************************************************************
    @function
    cri_core_qmi_send_msg_async

    @brief
    Sends QMI message using async mechanism to the QMI service server on modem.
    Creates a rule (condition on which the callback for this async request should be called)
    with a timeout.
    If caller of this function does not have any specific conditions on which the callback needs
    to be called, then the callback would be called as soon as the response arrives or the timer
    expires.

    @param[in]
        cri_core_context
           context comprising of HLOS token id and subscription id
        qmi_service_client_id
           CRI client ID returned by cri_core_create_qmi_service_client
        message_id
           message id of the message that needs to be sent
        req_message
           pointer to the request message that needs to be sent
        req_message_len
           length of the request message that needs to be sent
        resp_message_len
           length of the response message that would be received
        hlos_cb_data
           user callback data that needs to be passed when calling the callback
        hlos_resp_cb
           user callback that needs to be called when the rule has been met
        timeout_secs
           number of seconds to wait for the rule to be met
        rule_data
           pointer to data that needs to be used for checking If corresponding rule has been met
        rule_check_handler
           pointer to function that needs to be used checking If corresponding rule has been met
        rule_data_free_handler
           pointer to function that needs to be used for freeing rule data once the rule has been
           met

    @param[out]
        none

    @retval
    QMI_ERR_NONE_V01 if QMI message has been sent successfully and a rule has been created
    successfully, appropriate error code otherwise
***************************************************************************************************/
qmi_error_type_v01 cri_core_qmi_send_msg_async(cri_core_context_type cri_core_context,
                                               int qmi_service_client_id,
                                               unsigned long message_id,
                                               void *req_message,
                                               int req_message_len,
                                               int resp_message_len,
                                               void *hlos_cb_data,
                                               hlos_resp_cb_type hlos_resp_cb,
                                               int timeout_secs,
                                               void *rule_data,
                                               int (*rule_check_handler)(void *rule_data),
                                               void (*rule_data_free_handler)(void *rule_data));





/***************************************************************************************************
    @function
    cri_core_async_resp_handler

    @brief
    Handles the QMI async response in context of the core thread (as opposed to the QMI framework
    thread).
    QMI async response would have been queued up in the core queue by the QMI framework thread
    as soon as the response arrived.

    @param[in]
        event_data
           pointer to data that comprises of the QMI async response related information

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
void cri_core_async_resp_handler(void *event_data);





/***************************************************************************************************
    @function
    cri_core_unsol_ind_handler

    @brief
    Handles the QMI indication in context of the core thread (as opposed to the QMI framework
    thread).
    QMI indication would have been queued up in the core queue by the QMI framework thread
    as soon as the indication arrived.

    @param[in]
        event_data
           pointer to data that comprises of the QMI indication related information

    @param[out]
        none

    @retval
    none
***************************************************************************************************/
void cri_core_unsol_ind_handler(void *event_data);






/***************************************************************************************************
    @function
    cri_core_retrieve_hlos_ind_cb

    @brief
    Retrieves the HLOS indication callback provided through cri_core_cri_client_init while
    requesting for initialization of a QMI service.

    @param[in]
        qmi_service_client_id
           CRI client ID for which HLOS indication callback needs to be retrieved

    @param[out]
        none

    @retval
    pointer to HLOS indication callback function
***************************************************************************************************/
hlos_ind_cb_type cri_core_retrieve_hlos_ind_cb(int qmi_service_client_id);


#endif
