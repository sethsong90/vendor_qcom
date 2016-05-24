/******************************************************************************
  @file    qcril_log.c
  @brief   qcril qmi - logging utilities

  DESCRIPTION

  ---------------------------------------------------------------------------

  Copyright (c) 2008-2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


/*===========================================================================

                           INCLUDE FILES

===========================================================================*/


#define QCRIL_IPC_RILD_SOCKET_PATH_PREFIX    "/dev/socket/qmux_radio/rild_sync_"
#define QCRIL_IPC_MAX_SOCKET_PATH_LENGTH     48
#ifdef QMI_RIL_UTF
#include <netinet/in.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#else
#include <utils/Log.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <cutils/properties.h>
#include <pthread.h>
#include "qcril_log.h"
#include "qcril_am.h"
#include <sys/un.h>

// Required for glibc compile
#include <limits.h>
#include <signal.h>

#ifdef LOG_TAG
#undef LOG_TAG // It might be defined differently in diag header files
#endif
#define LOG_TAG "RILQ"

/*===========================================================================

                   INTERNAL DEFINITIONS AND TYPES

===========================================================================*/



/*===========================================================================

                         LOCAL VARIABLES

===========================================================================*/

char log_buf[ QCRIL_MAX_LOG_MSG_SIZE ];
char log_fmt[ QCRIL_MAX_LOG_MSG_SIZE ];
char thread_name[ QMI_RIL_THREAD_NAME_MAX_SIZE ];
#ifdef QMI_RIL_UTF
char log_buf_raw[ QCRIL_MAX_LOG_MSG_SIZE ];
#endif
pthread_mutex_t log_lock_mutex;
boolean diag_init_complete = FALSE;

/* Flag that controls whether QCRIL debug messages logged on ADB or not */
boolean qcril_log_adb_on = FALSE;

static pid_t qcril_qmi_instance_log_id;

/* QCRIL request names */
static const qcril_qmi_event_log_type qcril_request_name[]  =
{
  /* 1 */
  { RIL_REQUEST_GET_SIM_STATUS,                       "RIL_REQUEST_GET_SIM_STATUS" },
  { RIL_REQUEST_ENTER_SIM_PIN,                        "RIL_REQUEST_ENTER_SIM_PIN" },
  { RIL_REQUEST_ENTER_SIM_PUK,                        "RIL_REQUEST_ENTER_SIM_PUK" },
  { RIL_REQUEST_ENTER_SIM_PIN2,                       "RIL_REQUEST_ENTER_SIM_PIN2" },
  { RIL_REQUEST_ENTER_SIM_PUK2,                       "RIL_REQUEST_ENTER_SIM_PUK2" },
  { RIL_REQUEST_CHANGE_SIM_PIN,                       "RIL_REQUEST_CHANGE_SIM_PIN" },
  { RIL_REQUEST_CHANGE_SIM_PIN2,                      "RIL_REQUEST_CHANGE_SIM_PIN2" },
  { RIL_REQUEST_ENTER_DEPERSONALIZATION_CODE,         "RIL_REQUEST_ENTER_DEPERSONALIZATION_CODE" },
  { RIL_REQUEST_GET_CURRENT_CALLS,                    "RIL_REQUEST_GET_CURRENT_CALLS" },
  { RIL_REQUEST_DIAL,                                 "RIL_REQUEST_DIAL" },
  /* 11 */
  { RIL_REQUEST_GET_IMSI,                             "RIL_REQUEST_GET_IMSI" },
  { RIL_REQUEST_HANGUP,                               "RIL_REQUEST_HANGUP" },
  { RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND,         "RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND" },
  { RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND,  "RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND" },
  { RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, "RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE" },
  { RIL_REQUEST_CONFERENCE,                           "RIL_REQUEST_CONFERENCE" },
  { RIL_REQUEST_UDUB,                                 "RIL_REQUEST_UDUB" },
  { RIL_REQUEST_LAST_CALL_FAIL_CAUSE,                 "RIL_REQUEST_LAST_CALL_FAIL_CAUSE" },
  { RIL_REQUEST_SIGNAL_STRENGTH,                      "RIL_REQUEST_SIGNAL_STRENGTH" },
  { RIL_REQUEST_REGISTRATION_STATE,                   "RIL_REQUEST_REGISTRATION_STATE" },
  /* 21 */
  { RIL_REQUEST_DATA_REGISTRATION_STATE,              "RIL_REQUEST_DATA_REGISTRATION_STATE" },
  { RIL_REQUEST_OPERATOR,                             "RIL_REQUEST_OPERATOR" },
  { RIL_REQUEST_RADIO_POWER,                          "RIL_REQUEST_RADIO_POWER" },
  { RIL_REQUEST_DTMF,                                 "RIL_REQUEST_DTMF" },
  { RIL_REQUEST_SEND_SMS,                             "RIL_REQUEST_SEND_SMS" },
  { RIL_REQUEST_SEND_SMS_EXPECT_MORE,                 "RIL_REQUEST_SEND_SMS_EXPECT_MORE" },
  { RIL_REQUEST_SETUP_DATA_CALL,                      "RIL_REQUEST_SETUP_DATA_CALL" },
  { RIL_REQUEST_SIM_IO,                               "RIL_REQUEST_SIM_IO" },
  { RIL_REQUEST_SEND_USSD,                            "RIL_REQUEST_SEND_USSD" },
  { RIL_REQUEST_CANCEL_USSD,                          "RIL_REQUEST_CANCEL_USSD" },
  /* 31 */
  { RIL_REQUEST_GET_CLIR,                             "RIL_REQUEST_GET_CLIR" },
  { RIL_REQUEST_SET_CLIR,                             "RIL_REQUEST_SET_CLIR" },
  { RIL_REQUEST_QUERY_CALL_FORWARD_STATUS,            "RIL_REQUEST_QUERY_CALL_FORWARD_STATUS" },
  { RIL_REQUEST_SET_CALL_FORWARD,                     "RIL_REQUEST_SET_CALL_FORWARD" },
  { RIL_REQUEST_QUERY_CALL_WAITING,                   "RIL_REQUEST_QUERY_CALL_WAITING" },
  { RIL_REQUEST_SET_CALL_WAITING,                     "RIL_REQUEST_SET_CALL_WAITING" },
  { RIL_REQUEST_SMS_ACKNOWLEDGE,                      "RIL_REQUEST_SMS_ACKNOWLEDGE" },
  { RIL_REQUEST_GET_IMEI,                             "RIL_REQUEST_GET_IMEI" },
  { RIL_REQUEST_GET_IMEISV,                           "RIL_REQUEST_GET_IMEISV" },
  { RIL_REQUEST_ANSWER,                               "RIL_REQUEST_ANSWER" },
  /* 41 */
  { RIL_REQUEST_DEACTIVATE_DATA_CALL,                 "RIL_REQUEST_DEACTIVATE_DATA_CALL" },
  { RIL_REQUEST_QUERY_FACILITY_LOCK,                  "RIL_REQUEST_QUERY_FACILITY_LOCK" },
  { RIL_REQUEST_SET_FACILITY_LOCK,                    "RIL_REQUEST_SET_FACILITY_LOCK" },
  { RIL_REQUEST_CHANGE_BARRING_PASSWORD,              "RIL_REQUEST_CHANGE_BARRING_PASSWORD" },
  { RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE,         "RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE" },
  { RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC,      "RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC" },
  { RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL,         "RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL" },
  { RIL_REQUEST_QUERY_AVAILABLE_NETWORKS,             "RIL_REQUEST_QUERY_AVAILABLE_NETWORKS" },
  { RIL_REQUEST_DTMF_START,                           "RIL_REQUEST_DTMF_START" },
  { RIL_REQUEST_DTMF_STOP,                            "RIL_REQUEST_DTMF_STOP" },
  /* 51 */
  { RIL_REQUEST_BASEBAND_VERSION, "RIL_REQUEST_BASEBAND_VERSION" },
  { RIL_REQUEST_SEPARATE_CONNECTION, "RIL_REQUEST_SEPARATE_CONNECTION" },
  { RIL_REQUEST_SET_MUTE, "RIL_REQUEST_SET_MUTE" },
  { RIL_REQUEST_GET_MUTE, "RIL_REQUEST_GET_MUTE" },
  { RIL_REQUEST_QUERY_CLIP, "RIL_REQUEST_QUERY_CLIP" },
  { RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE, "RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE" },
  { RIL_REQUEST_DATA_CALL_LIST, "RIL_REQUEST_DATA_CALL_LIST" },
  { RIL_REQUEST_RESET_RADIO, "RIL_REQUEST_RESET_RADIO" },
  { RIL_REQUEST_OEM_HOOK_RAW, "RIL_REQUEST_OEM_HOOK_RAW" },
  { RIL_REQUEST_OEM_HOOK_STRINGS, "RIL_REQUEST_OEM_HOOK_STRINGS" },
  /* 61 */
  { RIL_REQUEST_SCREEN_STATE,                         "RIL_REQUEST_SCREEN_STATE" },
  { RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION,            "RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION" },
  { RIL_REQUEST_WRITE_SMS_TO_SIM,                     "RIL_REQUEST_WRITE_SMS_TO_SIM" },
  { RIL_REQUEST_DELETE_SMS_ON_SIM,                    "RIL_REQUEST_DELETE_SMS_ON_SIM" },
  { RIL_REQUEST_SET_BAND_MODE,                        "RIL_REQUEST_SET_BAND_MODE" },
  { RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE,            "RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE" },
  { RIL_REQUEST_STK_GET_PROFILE,                      "RIL_REQUEST_STK_GET_PROFILE" },
  { RIL_REQUEST_STK_SET_PROFILE,                      "RIL_REQUEST_STK_SET_PROFILE" },
  {RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND,             "RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND" },
  { RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE,           "RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE" },
  /* 71 */
  { RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM, "RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM" },
  { RIL_REQUEST_EXPLICIT_CALL_TRANSFER,               "RIL_REQUEST_EXPLICIT_CALL_TRANSFER" },
  { RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE,           "RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE" },
  { RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE,           "RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE" },
  { RIL_REQUEST_GET_NEIGHBORING_CELL_IDS,             "RIL_REQUEST_GET_NEIGHBORING_CELL_IDS" },
  { RIL_REQUEST_SET_LOCATION_UPDATES,                 "RIL_REQUEST_SET_LOCATION_UPDATES" },
  { RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE,         "RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE" },
  { RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE,          "RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE" },
  { RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE,        "RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE" },
  { RIL_REQUEST_SET_TTY_MODE,                         "RIL_REQUEST_SET_TTY_MODE" },
  /* 81 */
  { RIL_REQUEST_QUERY_TTY_MODE,                       "RIL_REQUEST_QUERY_TTY_MODE" },
  { RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE, "RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE" },
  { RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE, "RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE" },
  { RIL_REQUEST_CDMA_FLASH,                           "RIL_REQUEST_CDMA_FLASH" },
  { RIL_REQUEST_CDMA_BURST_DTMF,                      "RIL_REQUEST_CDMA_BURST_DTMF" },
  { RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY,         "RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY" },
  { RIL_REQUEST_CDMA_SEND_SMS,                        "RIL_REQUEST_CDMA_SEND_SMS" },
  { RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE,                 "RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE" },
  { RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG,         "RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG" },
  { RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG,         "RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG" },
  /* 91 */
  { RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION,         "RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION" },
  { RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG,        "RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG" },
  { RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG,        "RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG" },
  { RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION,        "RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION" },
  { RIL_REQUEST_CDMA_SUBSCRIPTION,                    "RIL_REQUEST_CDMA_SUBSCRIPTION" },
  { RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM,               "RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM" },
  { RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM,              "RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM" },
  { RIL_REQUEST_DEVICE_IDENTITY,                      "RIL_REQUEST_DEVICE_IDENTITY" },
  { RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE,         "RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE" },
  { RIL_REQUEST_GET_SMSC_ADDRESS,                     "RIL_REQUEST_GET_SMSC_ADDRESS" },
  /* 101 */
  { RIL_REQUEST_SET_SMSC_ADDRESS,                     "RIL_REQUEST_SET_SMSC_ADDRESS" },
  { RIL_REQUEST_REPORT_SMS_MEMORY_STATUS,             "RIL_REQUEST_REPORT_SMS_MEMORY_STATUS" },
  { RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING,        "RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING" },
  { RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE,         "RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE" },
  { RIL_REQUEST_ISIM_AUTHENTICATION,                  "RIL_REQUEST_ISIM_AUTHENTICATION" },
  { RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS,        "RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS" },
  { RIL_REQUEST_VOICE_RADIO_TECH,                     "RIL_REQUEST_VOICE_RADIO_TECH" },
  { RIL_REQUEST_GET_CELL_INFO_LIST,                   "RIL_REQUEST_GET_CELL_INFO_LIST"},
  { RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE,        "RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE"},
  /* 111 */
  { RIL_REQUEST_SET_INITIAL_ATTACH_APN,               "RIL_REQUEST_SET_INITIAL_ATTACH_APN"},
  { RIL_REQUEST_IMS_REGISTRATION_STATE,               "RIL_REQUEST_IMS_REGISTRATION_STATE" },
  { RIL_REQUEST_IMS_SEND_SMS,                         "RIL_REQUEST_IMS_SEND_SMS" },
  { RIL_REQUEST_GET_DATA_CALL_PROFILE,                "RIL_REQUEST_GET_DATA_CALL_PROFILE" },
  { RIL_REQUEST_SET_UICC_SUBSCRIPTION,                "RIL_REQUEST_SET_UICC_SUBSCRIPTION" },
  { RIL_REQUEST_SET_DATA_SUBSCRIPTION,                "RIL_REQUEST_SET_DATA_SUBSCRIPTION" },
  /* 10002 */
  { RIL_REQUEST_MODIFY_CALL_INITIATE,                 "RIL_REQUEST_MODIFY_CALL_INITIATE"},
  { RIL_REQUEST_MODIFY_CALL_CONFIRM,                  "RIL_REQUEST_MODIFY_CALL_CONFIRM"},
#if (RIL_QCOM_VERSION >= 2)
  { RIL_REQUEST_SETUP_QOS,                            "RIL_REQUEST_SETUP_QOS" },
  { RIL_REQUEST_RELEASE_QOS,                          "RIL_REQUEST_RELEASE_QOS" },
  { RIL_REQUEST_GET_QOS_STATUS,                       "RIL_REQUEST_GET_QOS_STATUS" },
  { RIL_REQUEST_MODIFY_QOS,                           "RIL_REQUEST_MODIFY_QOS" },
  { RIL_REQUEST_SUSPEND_QOS,                          "RIL_REQUEST_SUSPEND_QOS" },
  { RIL_REQUEST_RESUME_QOS,                           "RIL_REQUEST_RESUME_QOS"},
#endif
  { RIL_REQUEST_GET_UICC_SUBSCRIPTION,                "RIL_REQUEST_GET_UICC_SUBSCRIPTION" },
  { RIL_REQUEST_GET_DATA_SUBSCRIPTION,                "RIL_REQUEST_GET_DATA_SUBSCRIPTION" },
  { RIL_REQUEST_SET_SUBSCRIPTION_MODE,                "RIL_REQUEST_SET_SUBSCRIPTION_MODE" },
  { RIL_REQUEST_UNKOWN,                           "RIL_REQUEST_UNKOWN"},
};

static const qcril_qmi_event_log_type qcril_unsol_response_name[] =
{
  /* 1000 */
  { RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,           "RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED" },
  /* 1001 */
  { RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,            "RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED" },
  { RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED,         "RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED" },
  { RIL_UNSOL_RESPONSE_NEW_SMS,                       "RIL_UNSOL_RESPONSE_NEW_SMS" },
  { RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,         "RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT" },
  { RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM,                "RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM" },
  { RIL_UNSOL_ON_USSD,                                "RIL_UNSOL_ON_USSD" },
  { RIL_UNSOL_ON_USSD_REQUEST,                        "RIL_UNSOL_ON_USSD_REQUEST" },
  { RIL_UNSOL_NITZ_TIME_RECEIVED,                     "RIL_UNSOL_NITZ_TIME_RECEIVED" },
  { RIL_UNSOL_SIGNAL_STRENGTH,                        "RIL_UNSOL_SIGNAL_STRENGTH" },
  { RIL_UNSOL_DATA_CALL_LIST_CHANGED,                 "RIL_UNSOL_DATA_CALL_LIST_CHANGED" },
  /*1011 */
  { RIL_UNSOL_SUPP_SVC_NOTIFICATION,                  "RIL_UNSOL_SUPP_SVC_NOTIFICATION" },
  { RIL_UNSOL_STK_SESSION_END,                        "RIL_UNSOL_STK_SESSION_END" },
  { RIL_UNSOL_STK_PROACTIVE_COMMAND,                  "RIL_UNSOL_STK_PROACTIVE_COMMAND" },
  { RIL_UNSOL_STK_EVENT_NOTIFY,                       "RIL_UNSOL_STK_EVENT_NOTIFY" },
  { RIL_UNSOL_STK_CALL_SETUP,                         "RIL_UNSOL_STK_CALL_SETUP" },
  { RIL_UNSOL_SIM_SMS_STORAGE_FULL,                   "RIL_UNSOL_SIM_SMS_STORAGE_FULL" },
  { RIL_UNSOL_SIM_REFRESH,                            "RIL_UNSOL_SIM_REFRESH" },
  { RIL_UNSOL_CALL_RING,                              "RIL_UNSOL_CALL_RING" },
  { RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,            "RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED" },
  { RIL_UNSOL_RESPONSE_CDMA_NEW_SMS,                  "RIL_UNSOL_RESPONSE_CDMA_NEW_SMS" },
  /* 1021 */
  { RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS,             "RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS" },
  { RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL,             "RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL" },
  { RIL_UNSOL_RESTRICTED_STATE_CHANGED,               "RIL_UNSOL_RESTRICTED_STATE_CHANGED" },
  { RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE,          "RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE" },
  { RIL_UNSOL_CDMA_CALL_WAITING,                      "RIL_UNSOL_CDMA_CALL_WAITING" },
  { RIL_UNSOL_CDMA_OTA_PROVISION_STATUS,                "RIL_UNSOL_CDMA_OTA_PROVISION_STATUS" },
  { RIL_UNSOL_CDMA_INFO_REC,                          "RIL_UNSOL_CDMA_INFO_REC" },
  { RIL_UNSOL_OEM_HOOK_RAW,                           "RIL_UNSOL_OEM_HOOK_RAW" },
  { RIL_UNSOL_RINGBACK_TONE,                          "RIL_UNSOL_RINGBACK_TONE" },
  { RIL_UNSOL_RESEND_INCALL_MUTE,                     "RIL_UNSOL_RESEND_INCALL_MUTE" },
  /* 1031 */
  { RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED,       "RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED" },
  { RIL_UNSOL_CDMA_PRL_CHANGED,                       "RIL_UNSOL_CDMA_PRL_CHANGED" },
  { RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE,           "RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE" },
  { RIL_UNSOL_RIL_CONNECTED,                          "RIL_UNSOL_RIL_CONNECTED" },
  { RIL_UNSOL_VOICE_RADIO_TECH_CHANGED,               "RIL_UNSOL_VOICE_RADIO_TECH_CHANGED" },
  { RIL_UNSOL_CELL_INFO_LIST,                         "RIL_UNSOL_CELL_INFO_LIST"},
  { RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED,     "RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED" },
  { RIL_UNSOL_ON_SS,                                  "RIL_UNSOL_ON_SS" },
  { RIL_UNSOL_STK_CC_ALPHA_NOTIFY,                    "RIL_UNSOL_STK_CC_ALPHA_NOTIFY" },
  { RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED,       "RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED" },
#if (RIL_QCOM_VERSION >= 2)
  { RIL_UNSOL_QOS_STATE_CHANGED_IND,                  "RIL_UNSOL_QOS_STATE_CHANGED_IND" },
#endif
  { RIL_UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED,   "RIL_UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED" },
  { RIL_UNSOL_MODIFY_CALL,                            "RIL_UNSOL_MODIFY_CALL" },
  { RIL_UNSOL_UNKOWN,                                 "RIL_UNSOL_UNKOWN"}
 };

typedef struct
{
 qcril_instance_id_e_type rild_instance_id;
 size_t rild_addrlen;
 struct sockaddr_un rild_addr;
}other_rild_addr_info_type;

static struct inter_rild_info_type
{
  int info_valid;
  int my_sockid;
  other_rild_addr_info_type *other_rilds_addr_info;
  int other_rilds_addr_info_len;
  pthread_t recv_thread_id;
  pthread_attr_t recv_thread_attr;
  pthread_mutex_t send_lock_mutex;
}inter_rild_info;

typedef struct ipc_send_recv_data_info
{
  qcril_instance_id_e_type rild_instance_id;
  ipc_message_id_type message_id;
  char payload[QCRIL_MAX_IPC_PAYLOAD_SIZE];
  int payload_length;
}ipc_send_recv_data_info_type;

typedef struct
{
    int is_valid;
    int thread_id;
    char thread_name[QMI_RIL_THREAD_NAME_MAX_SIZE];
} qmi_ril_thread_name_info_type;

static qmi_ril_thread_name_info_type qmi_ril_thread_name_info[QMI_RIL_THREAD_INFO_MAX_SIZE];

/*===========================================================================

                    INTERNAL FUNCTION PROTOTYPES


===========================================================================*/

static void* qcril_multiple_rild_ipc_recv_func(void *arg);
static void qcril_multiple_rild_ipc_signal_handler_sigusr1(int arg);
static int qcril_ipc_evaluate_rilds_socket_paths(char *rild_socket_name);
static int qcril_log_lookup_ril_event_index
(
  int event,
  qcril_qmi_event_log_type *data,
  int min_index,
  int max_index
);


/*===========================================================================

                                FUNCTIONS

===========================================================================*/

/*=========================================================================
  FUNCTION:  qcril_log_init

===========================================================================*/
/*!
    @brief
    Initialization for logging.

    @return
    None
*/
/*=========================================================================*/
void qcril_log_init
(
  void
)
{
  char args[ PROPERTY_VALUE_MAX ];
  int len;
  char *end_ptr;
  unsigned long ret_val;

  qcril_qmi_instance_log_id = getpid();

  /* Initialize Diag for QCRIL logging */
  ret_val = Diag_LSM_Init(NULL);
  if ( !ret_val )
  {
#ifndef QMI_RIL_UTF
    RLOGE( "Fail to initialize Diag for QCRIL logging\n" );
#endif
  }
  else
  {
    diag_init_complete = TRUE;
  }

  QCRIL_LOG_DEBUG ( "qcril_log_init() 1" );

  property_get( QCRIL_LOG_ADB_ON, args, "" );
  len = strlen( args );
  if ( len > 0 )
  {
    ret_val = strtoul( args, &end_ptr, 0 );
    if ( ( errno == ERANGE ) && ( ( ret_val == ULONG_MAX ) || ( ret_val == 0 ) ) )
    {
      QCRIL_LOG_ERROR( "Fail to convert adb_log_on setting %s", args );
    }
    else if ( ret_val > 1 )
    {
      QCRIL_LOG_ERROR( "Invalid saved adb_log_on setting %ld, use default", ret_val );
    }
    else
    {
      qcril_log_adb_on = ( boolean ) ret_val;
    }
  }

  #ifdef FEATURE_QCRIL_ADB_LOG_ON
  qcril_log_adb_enabled = TRUE;
  #endif /* FEATURE_QCRIL_ADB_LOG_ON */

  QCRIL_LOG_DEBUG ( "qcril_log_init() 2" );

  QCRIL_LOG_DEBUG( "adb_log_on = %d", (int) qcril_log_adb_on );

  /* Save ADB Log Enabled setting to system property */
  QCRIL_SNPRINTF( args, sizeof( args ), "%d", (int) qcril_log_adb_on );
  if ( property_set( QCRIL_LOG_ADB_ON, args ) != E_SUCCESS )
  {
    QCRIL_LOG_ERROR( "Fail to save %s to system property", QCRIL_LOG_ADB_ON );
  }

  pthread_mutexattr_t mtx_atr;
  pthread_mutexattr_init(&mtx_atr);
  pthread_mutex_init(&log_lock_mutex, &mtx_atr);

} /* qcril_log_init */


/*=========================================================================
  FUNCTION:  qcril_format_log_msg

===========================================================================*/
/*!
    @brief
    Format debug message for logging.

    @return
    None
*/
/*=========================================================================*/
void qcril_format_log_msg
(
  char *buf_ptr,
  int buf_size,
  char *fmt,
  ...
)
{
  va_list ap;


  va_start( ap, fmt );

  if ( NULL != buf_ptr && buf_size > 0 )
  {
      vsnprintf( buf_ptr, buf_size, fmt, ap );
  }

  va_end( ap );

} /* qcril_format_log_msg */


/*=========================================================================
  FUNCTION:  qcril_log_call_flow_packet

===========================================================================*/
/*!
    @brief
    Log the call flow packet.

    @return
    None
*/
/*=========================================================================*/
void qcril_log_call_flow_packet
(
  qcril_call_flow_subsystem_e_type src_subsystem,
  qcril_call_flow_subsystem_e_type dest_subsystem,
  qcril_call_flow_arrow_e_type arrow,
  char *label
)
{
  #if !defined(FEATURE_UNIT_TEST) && !defined(QMI_RIL_UTF)
  qcril_call_flow_log_packet_type *log_buf;
  uint16 label_len, log_packet_size = 0;

  /* Calculate the size of the log packet */
  label_len = strlen( label );
  log_packet_size = sizeof( qcril_call_flow_log_packet_type ) + label_len;

  /* Allocate log buffer */
  log_buf = ( qcril_call_flow_log_packet_type * ) log_alloc( LOG_QCRIL_CALL_FLOW_C, log_packet_size );

  if ( log_buf != NULL )
  {
    /* Fill in the log buffer */
    log_buf->src_subsystem = (uint8) src_subsystem;
    log_buf->dest_subsystem = (uint8) dest_subsystem;
    log_buf->arrow = (uint8) arrow;
    log_buf->label[ 0 ] = '\0';
    if ( label_len > 0 )
    {
      memcpy( (void *) log_buf->label, label, label_len + 1 );
    }

    /* Commit log buffer */
    log_commit( log_buf );
  }
  #endif /* !FEATURE_UNIT_TEST */

} /* qcril_log_call_flow_packet */

#ifndef QMI_RIL_UTF
/*=========================================================================
  FUNCTION:  qcril_log_msg_to_adb

===========================================================================*/
/*!
    @brief
    Log debug message to ADB.

    @return
    None
*/
/*=========================================================================*/
void qcril_log_msg_to_adb
(
  int  lvl,
  char *msg_ptr
)
{
  switch ( lvl )
  {
      case MSG_LEGACY_ERROR:
      case MSG_LEGACY_FATAL:
#ifndef QMI_RIL_UTF
        RLOGE( "(%d/%d): %s",
               (int)qmi_ril_get_process_instance_id(),
               (int)qcril_qmi_instance_log_id,
               msg_ptr );
#endif
        break;

      case MSG_LEGACY_HIGH:                         // fall through
        RLOGW_IF( qcril_log_adb_on, "(%d/%d): %s",
                  (int)qmi_ril_get_process_instance_id(),
                  (int)qcril_qmi_instance_log_id,
                  msg_ptr );
        break;

      case MSG_LEGACY_ESSENTIAL:
        RLOGI_IF( qcril_log_adb_on, "(%d/%d):%s",
               (int)qmi_ril_get_process_instance_id(),
               (int)qcril_qmi_instance_log_id,
                msg_ptr );
        break;

      case MSG_LEGACY_MED:
        RLOGI_IF( qcril_log_adb_on, "(%d/%d): %s",
                  (int)qmi_ril_get_process_instance_id(),
                  (int)qcril_qmi_instance_log_id,
                  msg_ptr );
        break;

      default:
        RLOGV_IF( qcril_log_adb_on, "(%d/%d): %s",
                  (int)qmi_ril_get_process_instance_id(),
                  (int)qcril_qmi_instance_log_id,
                  msg_ptr );
        break;
  }

} /* qcril_log_msg_to_adb */

#endif
/*===========================================================================

  FUNCTION: QCRIL_LOG_GET_TOKEN_ID

===========================================================================*/
/*!
    @brief
    Return the value of the Token ID.

    @return
    The value of Token ID
*/
/*=========================================================================*/
int qcril_log_get_token_id
(
  RIL_Token t
)
{
  int token_id = 0;

  /*-----------------------------------------------------------------------*/

  if ( t == NULL )
  {
    token_id = 0xFFFE;
  }
  else if ( t == ( void * ) QCRIL_TOKEN_ID_INTERNAL )
  {
    token_id = 0xFFFF;
  }
  else
  {
    token_id =  *( (int *) t );
  }

  return token_id;

} /* qcril_log_get_token_id */


/*===========================================================================

  FUNCTION: QCRIL_LOG_LOOKUP_EVENT_NAME

===========================================================================*/
/*!
    @brief
    Lookup the name of a QCRIL event

    @return
    The string respresenting the name of a QCRIL request
*/
/*=========================================================================*/
const char *qcril_log_lookup_event_name
(
  int event_id
)
{
  /*-----------------------------------------------------------------------*/
  int index;

  /* Lookup the name of a RIL request */
  if ( event_id < RIL_UNSOL_RESPONSE_BASE )
  {
     index = qcril_log_lookup_ril_event_index( event_id, (qcril_qmi_event_log_type *)qcril_request_name, 0, sizeof(qcril_request_name)/sizeof(qcril_request_name[0]) - 1);
     return qcril_request_name[ index ].event_name;
  }
  /* Lookup the name of a RIL unsolicited response */
  else if ( event_id < (int) QCRIL_EVT_BASE )
  {
     index = qcril_log_lookup_ril_event_index( event_id, (qcril_qmi_event_log_type *)qcril_unsol_response_name, 0, sizeof(qcril_unsol_response_name)/sizeof(qcril_unsol_response_name[0]) - 1);
     return qcril_unsol_response_name[ index ].event_name;
  }
  /* Lookup the name of a QCRIL event */
  else
  {
    /* NOTE: All internal QCRIL events must return a string that prefix with "INTERNAL_" in order to support AMSS event profiling */
    switch( event_id )
    {
      case QCRIL_EVT_NONE:
        return "<none>";

      case QCRIL_EVT_CM_COMMAND_CALLBACK:
        return "CM_COMMAND_CALLBACK";

      case QCRIL_EVT_CM_UPDATE_FDN_STATUS:
        return "CM_UPDATE_FDN_STATUS";

      case QCRIL_EVT_CM_CARD_STATUS_UPDATED:
        return "CM_CARD_STATUS_UPDATED";

      case QCRIL_EVT_CM_ACTIVATE_PROVISION_STATUS:
        return "CM_ACTIVATE_PROVISION_STATUS";

      case QCRIL_EVT_CM_DEACTIVATE_PROVISION_STATUS:
        return "CM_DEACTIVATE_PROVISION_STATUS";


      case QCRIL_EVT_UIM_QMI_COMMAND_CALLBACK:
        return "UIM_QMI_COMMAND_CALLBACK";

      case QCRIL_EVT_UIM_QMI_INDICATION:
        return "UIM_QMI_INDICATION";

      case QCRIL_EVT_INTERNAL_UIM_VERIFY_PIN_COMMAND_CALLBACK:
        return "INTERNAL_UIM_VERIFY_PIN_COMMAND_CALLBACK";

      case QCRIL_EVT_INTERNAL_MMGSDI_CARD_POWER_UP:
        return "INTERNAL_MMGSDI_CARD_POWER_UP";

      case QCRIL_EVT_INTERNAL_MMGSDI_CARD_POWER_DOWN:
        return "INTERNAL_MMGSDI_CARD_POWER_DOWN";

      case QCRIL_EVT_INTERNAL_MMGSDI_GET_FDN_STATUS:
        return "INTERNAL_MMGSDI_GET_FDN_STATUS";

      case QCRIL_EVT_INTERNAL_MMGSDI_SET_FDN_STATUS:
        return "INTERNAL_MMGSDI_SET_FDN_STATUS";

      case QCRIL_EVT_INTERNAL_MMGSDI_GET_PIN1_STATUS:
        return "INTERNAL_MMGSDI_GET_PIN1_STATUS";

      case QCRIL_EVT_INTERNAL_MMGSDI_SET_PIN1_STATUS:
        return "INTERNAL_MMGSDI_SET_PIN1_STATUS";

      case QCRIL_EVT_INTERNAL_MMGSDI_FDN_PBM_RECORD_UPDATE:
        return "INTERNAL_MMGSDI_FDN_PBM_RECORD_UPDATE";

      case QCRIL_EVT_INTERNAL_MMGSDI_READ_UST_VALUE:
        return "INTERNAL_MMGSDI_READ_UST_VALUE";

      case QCRIL_EVT_INTERNAL_MMGSDI_ACTIVATE_SUBS:
        return "QCRIL_EVT_INTERNAL_MMGSDI_ACTIVATE_SUBS";

      case QCRIL_EVT_INTERNAL_MMGSDI_DEACTIVATE_SUBS:
        return "QCRIL_EVT_INTERNAL_MMGSDI_DEACTIVATE_SUBS";

      case QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_START:
        return "QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_START";

      case QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_COMPLETE:
        return "QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_COMPLETE";

      case QCRIL_EVT_GSTK_QMI_CAT_INDICATION:
        return "QCRIL_EVT_GSTK_QMI_CAT_INDICATION";

      case QCRIL_EVT_GSTK_QMI_NOTIFY_RIL_IS_READY:
        return "QCRIL_EVT_GSTK_QMI_NOTIFY_RIL_IS_READY";

      case QCRIL_EVT_GSTK_QMI_RAW_COMMAND_CALLBACK:
        return "QCRIL_EVT_GSTK_QMI_RAW_COMMAND_CALLBACK";

      case QCRIL_EVT_GSTK_QMI_COMMAND_CALLBACK:
        return "QCRIL_EVT_GSTK_QMI_COMMAND_CALLBACK";

      case QCRIL_EVT_GSTK_QMI_NOTIFY_CARD_ERROR:
        return "QCRIL_EVT_GSTK_QMI_NOTIFY_CARD_ERROR";

      case QCRIL_EVT_DATA_COMMAND_CALLBACK:
        return "DATA_COMMAND_CALLBACK";

      case QCRIL_EVT_DATA_EVENT_CALLBACK:
        return "DATA_EVENT_CALLBACK";

      case QCRIL_EVT_DATA_WDS_EVENT_CALLBACK:
        return "QCRIL_EVT_DATA_WDS_EVENT_CALLBACK";

      case QCRIL_EVT_DATA_DSD_EVENT_CALLBACK:
        return "QCRIL_EVT_DATA_DSD_EVENT_CALLBACK";

      case QCRIL_EVT_HOOK_NV_READ:
        return "RIL_REQUEST_OEM_HOOK_RAW(NV_READ)";

      case QCRIL_EVT_HOOK_INFORM_SHUTDOWN:
        return "RIL_REQUEST_OEM_HOOK_RAW(INFORM_SHUTDOWN)";

      case QCRIL_EVT_HOOK_CSG_PERFORM_NW_SCAN:
        return "RIL_REQUEST_OEM_HOOK_RAW(CSG_PERFORM_NW_SCAN)";

      case QCRIL_EVT_HOOK_CSG_SET_SYS_SEL_PREF:
        return "RIL_REQUEST_OEM_HOOK_RAW(CSG_SET_SYS_SEL_PREF)";

      case QCRIL_EVT_HOOK_CSG_GET_SYS_INFO:
        return "RIL_REQUEST_OEM_HOOK_RAW(CSG_GET_SYS_INFO)";

      case QCRIL_EVT_HOOK_ENABLE_ENGINEER_MODE:
        return "RIL_REQUEST_OEM_HOOK_RAW(QCRIL_EVT_HOOK_ENABLE_ENGINEER_MODE)";

      case QCRIL_EVT_HOOK_UNSOL_ENGINEER_MODE:
        return "QCRIL_EVT_HOOK_UNSOL_ENGINEER_MODE";

      case QCRIL_EVT_HOOK_NV_WRITE:
        return "RIL_REQUEST_OEM_HOOK_RAW(NV_WRITE)";

      case QCRIL_EVT_HOOK_ME_DEPERSONALIZATION:
        return "RIL_REQUEST_OEM_HOOK_RAW(ME_DEPERSONALIZATION)";

      case QCRIL_EVT_HOOK_SET_TUNE_AWAY:
        return "RIL_REQUEST_OEM_HOOK_RAW(SET_TUNE_AWAY)";

      case QCRIL_EVT_HOOK_GET_TUNE_AWAY:
        return "RIL_REQUEST_OEM_HOOK_RAW(GET_TUNE_AWAY)";

      case QCRIL_EVT_HOOK_SET_PAGING_PRIORITY:
        return "RIL_REQUEST_OEM_HOOK_RAW(SET_PAGING_PRIORITY)";

      case QCRIL_EVT_HOOK_GET_PAGING_PRIORITY:
        return "RIL_REQUEST_OEM_HOOK_RAW(GET_PAGING_PRIORITY)";

      case QCRIL_EVT_HOOK_SET_DEFAULT_VOICE_SUB:
        return "QCRIL_EVT_HOOK_SET_DEFAULT_VOICE_SUB";

      case QCRIL_EVT_HOOK_SET_LOCAL_CALL_HOLD:
        return "QCRIL_EVT_HOOK_SET_LOCAL_CALL_HOLD";

      case QCRIL_EVT_HOOK_SET_BUILTIN_PLMN_LIST:
        return "QCRIL_EVT_HOOK_SET_BUILTIN_PLMN_LIST";

      case QCRIL_EVT_HOOK_REJECT_INCOMING_CALL_CAUSE_21:
        return "QCRIL_EVT_HOOK_REJECT_INCOMING_CALL_CAUSE_21";

      case QCRIL_EVT_HOOK_UNSOL_CDMA_CONT_DTMF_START:
        return "RIL_UNSOL_OEM_HOOK_RAW(CDMA_CONT_FWD_DTMF_START)";

      case QCRIL_EVT_HOOK_UNSOL_CDMA_CONT_DTMF_STOP:
        return "RIL_UNSOL_OEM_HOOK_RAW(CDMA_CONT_FWD_DTMF_STOP)";

      case QCRIL_EVT_HOOK_UNSOL_CDMA_BURST_DTMF:
        return "RIL_UNSOL_OEM_HOOK_RAW(CDMA_CONT_FWD_DTMF_BURST)";

      case QCRIL_EVT_HOOK_UNSOL_CALL_EVT_PROGRESS_INFO_IND:
        return "RIL_UNSOL_OEM_HOOK_RAW(CALL_EVT_PROGRESS_INFO_IND)";

      case QCRIL_EVT_HOOK_UNSOL_NSS_RELEASE:
        return "QCRIL_EVT_HOOK_UNSOL_NSS_RELEASE";

      case QCRIL_EVT_HOOK_NEIGHBOR_CELL_INFO_RCVD:
        return "QCRIL_EVT_HOOK_NEIGHBOR_CELL_INFO_RCVD";

      case QCRIL_EVT_HOOK_UNSOL_EUTRA_STATUS:
        return "QCRIL_EVT_HOOK_UNSOL_EUTRA_STATUS";

      case QCRIL_EVT_HOOK_UNSOL_VOICE_SYSTEM_ID:
        return "QCRIL_EVT_HOOK_UNSOL_VOICE_SYSTEM_ID";

      case QCRIL_EVT_HOOK_PERFORM_INCREMENTAL_NW_SCAN:
        return "QCRIL_EVT_HOOK_PERFORM_INCREMENTAL_NW_SCAN";

      case QCRIL_EVT_HOOK_UNSOL_INCREMENTAL_NW_SCAN_IND:
        return "QCRIL_EVT_HOOK_UNSOL_INCREMENTAL_NW_SCAN_IND";

      case QCRIL_EVT_QMI_REQUEST_NW_SCAN:
        return "REQUEST_NW_SCAN";

      case QCRIL_EVT_QMI_REQUEST_NW_SELECT:
        return "REQUEST_NW_SELECT";

      case QCRIL_EVT_QMI_REQUEST_POWER_RADIO:
        return "REQUEST_POWER_RADIO";

      case QCRIL_EVT_QMI_RIL_COMMON_IND_SUBSCRIBE_CONSIDER_ACTION:
        return "QCRIL_EVT_QMI_RIL_COMMON_IND_SUBSCRIBE_CONSIDER_ACTION";

      case QCRIL_EVT_QMI_REQUEST_NEIGHBOR_CELL_INFO:
        return "QCRIL_EVT_QMI_REQUEST_NEIGHBOR_CELL_INFO";

      case QCRIL_EVT_QMI_NAS_CLEANUP_NW_SEL:
        return "QCRIL_EVT_QMI_NAS_CLEANUP_NW_SEL";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_REQ:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_REQ";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_CON";

      case QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_CON:
        return "QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_CON";

      case QCRIL_EVT_HOOK_EMBMS_ENABLE:
        return "QCRIL_EVT_HOOK_EMBMS_ENABLE";

      case QCRIL_EVT_HOOK_EMBMS_DISABLE:
          return "QCRIL_EVT_HOOK_EMBMS_DISABLE";

      case QCRIL_EVT_HOOK_EMBMS_ACTIVATE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_ACTIVATE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_DEACTIVATE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_DEACTIVATE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_ACTIVATE_DEACTIVATE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_ACTIVATE_DEACTIVATE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_RSSI_IND:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_RSSI_IND";

      case QCRIL_EVT_HOOK_EMBMS_GET_AVAILABLE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_GET_AVAILABLE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_GET_ACTIVE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_GET_ACTIVE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_ENABLE_RSSI:
          return "QCRIL_EVT_HOOK_EMBMS_ENABLE_RSSI";

      case QCRIL_EVT_HOOK_EMBMS_DISABLE_RSSI:
          return "QCRIL_EVT_HOOK_EMBMS_DISABLE_RSSI";

      case QCRIL_EVT_HOOK_EMBMS_GET_COVERAGE_STATE:
          return "QCRIL_EVT_HOOK_EMBMS_GET_COVERAGE_STATE";

      case QCRIL_EVT_HOOK_EMBMS_GET_RSSI:
          return "QCRIL_EVT_HOOK_EMBMS_GET_RSSI";

      case QCRIL_EVT_HOOK_EMBMS_GET_EMBMS_SVC_STATE:
          return "QCRIL_EVT_HOOK_EMBMS_GET_EMBMS_SVC_STATE";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_SVC_STATE:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_SVC_STATE";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_ACTIVE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_ACTIVE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_COVERAGE:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_COVERAGE";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_OSS_WARNING:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_OSS_WARNING";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_AVAILABLE_TMGI:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_AVAILABLE_TMGI";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_CELL_INFO_CHANGED:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_CELL_INFO_CHANGED";

      case QCRIL_EVT_HOOK_EMBMS_UNSOL_SAI_LIST:
          return "QCRIL_EVT_HOOK_EMBMS_UNSOL_SAI_LIST";

      case QCRIL_EVT_HOOK_UNSOL_GENERIC:
          return "QCRIL_EVT_HOOK_UNSOL_GENERIC";

      case QCRIL_EVT_HOOK_REQ_GENERIC:
          return "QCRIL_EVT_HOOK_REQ_GENERIC";

      case QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_REQ:
          return "QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_REQ";

      case QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_CON:
          return "QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_CON";

      case QCRIL_EVT_QMI_RIL_EMBMS_DISABLE_DATA_IND:
          return "QCRIL_EVT_QMI_RIL_EMBMS_DISABLE_DATA_IND";

      case QCRIL_EVT_QMI_RIL_SEND_UNSOL_RADIO_STATE_CHANGED:
          return "QCRIL_EVT_QMI_RIL_SEND_UNSOL_RADIO_STATE_CHANGED";

      case QCRIL_EVT_QMI_RIL_CHECK_PRL_VER_CHANGE:
          return "QCRIL_EVT_QMI_RIL_CHECK_PRL_VER_CHANGE";

      case QCRIL_EVT_QMI_RIL_PRL_VER_FETCH_ATTEMPT:
          return "QCRIL_EVT_QMI_RIL_PRL_VER_FETCH_ATTEMPT";

      case QCRIL_EVT_QMI_REQUEST_EMBMS_ENABLE:
          return "QCRIL_EVT_QMI_REQUEST_EMBMS_ENABLE";

      case QCRIL_EVT_QMI_REQUEST_EMBMS_DISABLE:
          return "QCRIL_EVT_QMI_REQUEST_EMBMS_DISABLE";

      case QCRIL_EVT_QMI_REQUEST_MODIFY_INITIATE:
          return "QCRIL_EVT_QMI_REQUEST_MODIFY_INITIATE";

      case QCRIL_EVT_QMI_REQUEST_MODIFY_CONFIRM:
          return "QCRIL_EVT_QMI_REQUEST_MODIFY_CONFIRM";

      case QCRIL_EVT_QMI_REQUEST_POWER_WAIT_FOR_CARD_STATUS:
          return "QCRIL_EVT_QMI_REQUEST_POWER_WAIT_FOR_CARD_STATUS";

      case QCRIL_EVT_QMI_REQUEST_3GPP2_SUB:
          return "QCRIL_EVT_QMI_REQUEST_3GPP2_SUB";

      case QCRIL_EVT_QMI_REQUEST_SET_SUBS_MODE:
          return "QCRIL_EVT_QMI_REQUEST_SET_SUBS_MODE";

      case QCRIL_EVT_QMI_NAS_SIG_STRENGTH_UPDATE:
          return "QCRIL_EVT_QMI_NAS_SIG_STRENGTH_UPDATE";

      case QCRIL_EVT_QMI_NAS_DSDS_SUBS_FOLLOWUP:
          return "QCRIL_EVT_QMI_NAS_DSDS_SUBS_FOLLOWUP";

      case QCRIL_EVT_QMI_NAS_DSDS_SUBS_DEACTIVATE_FOLLOWUP:
          return "QCRIL_EVT_QMI_NAS_DSDS_SUBS_DEACTIVATE_FOLLOWUP";

      case QCRIL_EVT_QMI_VOICE_BURST_START_CONT_DTMF:
          return "QCRIL_EVT_QMI_VOICE_BURST_START_CONT_DTMF";

      case QCRIL_EVT_QMI_VOICE_BURST_STOP_CONT_DTMF:
          return "QCRIL_EVT_QMI_VOICE_BURST_STOP_CONT_DTMF";

      case QCRIL_EVT_QMI_VOICE_PENDING_MNG_CALLS_REQ_FINISHED:
          return "QCRIL_EVT_QMI_VOICE_PENDING_MNG_CALLS_REQ_FINISHED";

      case QCRIL_EVT_QMI_VOICE_GET_WAITING_CALL:
          return "QCRIL_EVT_QMI_VOICE_GET_WAITING_CALL";

      case QCRIL_EVT_QMI_VOICE_EMERGENCY_CALL_PENDING:
          return "QCRIL_EVT_QMI_VOICE_EMERGENCY_CALL_PENDING";

      case QCRIL_EVT_HOOK_VT_DIAL_CALL:
          return "QCRIL_EVT_HOOK_VT_DIAL_CALL";

      case QCRIL_EVT_HOOK_VT_END_CALL:
          return "QCRIL_EVT_HOOK_VT_END_CALL";

      case QCRIL_EVT_HOOK_VT_ANSWER_CALL:
          return "QCRIL_EVT_HOOK_VT_ANSWER_CALL";

      case QCRIL_EVT_HOOK_VT_GET_CALL_INFO:
          return "QCRIL_EVT_HOOK_VT_GET_CALL_INFO";

      case QCRIL_EVT_HOOK_VT_UNSOL_CALL_STATUS_IND:
          return "QCRIL_EVT_HOOK_VT_UNSOL_CALL_STATUS_IND";

      case QCRIL_EVT_HOOK_IMS_ENABLER_STATE_REQ:
          return "QCRIL_EVT_HOOK_IMS_ENABLER_STATE_REQ";

      case QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_REQ";

      case QCRIL_EVT_HOOK_IMS_SEND_UNPUBLISH_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_UNPUBLISH_REQ";

      case QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_REQ";

      case QCRIL_EVT_HOOK_IMS_SEND_UNSUBSCRIBE_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_UNSUBSCRIBE_REQ";

      case QCRIL_EVT_HOOK_IMS_NOTIFY_XML_IND_V01:
          return "QCRIL_EVT_HOOK_IMS_NOTIFY_XML_IND_V01";

      case QCRIL_EVT_HOOK_IMS_NOTIFY_IND_V01:
          return "QCRIL_EVT_HOOK_IMS_NOTIFY_IND_V01";

      case QCRIL_EVT_HOOK_IMS_ENABLER_STATUS_IND:
          return "QCRIL_EVT_HOOK_IMS_ENABLER_STATUS_IND";

      case QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_XML_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_XML_REQ";

      case QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_XML_REQ:
          return "QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_XML_REQ";

      case QCRIL_EVT_HOOK_IMS_SET_NOTIFY_FMT_REQ_V01:
          return "QCRIL_EVT_HOOK_IMS_SET_NOTIFY_FMT_REQ_V01";

      case QCRIL_EVT_HOOK_IMS_GET_NOTIFY_FMT_REQ_V01:
          return "QCRIL_EVT_HOOK_IMS_GET_NOTIFY_FMT_REQ_V01";

      case QCRIL_EVT_HOOK_IMS_SET_EVENT_REPORT_REQ_V01:
          return "QCRIL_EVT_HOOK_IMS_SET_EVENT_REPORT_REQ_V01";

      case QCRIL_EVT_HOOK_IMS_GET_EVENT_REPORT_REQ_V01:
          return "QCRIL_EVT_HOOK_IMS_GET_EVENT_REPORT_REQ_V01";

      case QCRIL_EVT_HOOK_IMS_PUBLISH_TRIGGER_IND_V01:
          return "QCRIL_EVT_HOOK_IMS_PUBLISH_TRIGGER_IND_V01";

      case QCRIL_EVT_QMI_RIL_POST_VOICE_RTE_CHANGE_IND:
          return "QCRIL_EVT_QMI_RIL_POST_VOICE_RTE_CHANGE_IND";

      case QCRIL_EVT_QMI_VOICE_HANDLE_COMM_CALLBACKS:
          return "QCRIL_EVT_QMI_VOICE_HANDLE_COMM_CALLBACKS";

      case QCRIL_EVT_QMI_VOICE_HANDLE_INDICATIONS:
          return "QCRIL_EVT_QMI_VOICE_HANDLE_INDICATIONS";

      case QCRIL_EVT_QMI_NAS_PASSOVER_NW_SEL_IND:
          return "QCRIL_EVT_QMI_NAS_PASSOVER_NW_SEL_IND";

      case QCRIL_EVT_QMI_NAS_HANDLE_INDICATIONS:
          return "QCRIL_EVT_QMI_NAS_HANDLE_INDICATIONS";

      case QCRIL_EVT_QMI_DMS_HANDLE_INDICATIONS:
          return "QCRIL_EVT_QMI_DMS_HANDLE_INDICATIONS";

      case QCRIL_EVT_HOOK_DATA_GO_DORMANT:
          return "QCRIL_EVT_HOOK_DATA_GO_DORMANT";

      case QCRIL_EVT_PBM_CARD_INSERTED:
          return "QCRIL_EVT_PBM_CARD_INSERTED";

      case QCRIL_EVT_PBM_CARD_INIT_COMPLETED:
          return "QCRIL_EVT_PBM_CARD_INIT_COMPLETED";

      case QCRIL_EVT_PBM_CARD_ERROR:
          return "QCRIL_EVT_PBM_CARD_ERROR";

      case QCRIL_EVT_PBM_UPDATE_OTA_ECC_LIST:
          return "QCRIL_EVT_PBM_UPDATE_OTA_ECC_LIST";

      case QCRIL_EVT_SMS_RAW_READ:
          return "QCRIL_EVT_SMS_RAW_READ";

      case QCRIL_EVT_QMI_RIL_ASSESS_EMRGENCY_NUMBER_LIST_DESIGNATED_COUNTRY:
          return "QCRIL_EVT_QMI_RIL_ASSESS_EMRGENCY_NUMBER_LIST_DESIGNATED_COUNTRY";

      case QCRIL_EVT_SMS_PERFORM_INITIAL_CONFIGURATION:
          return "QCRIL_EVT_SMS_PERFORM_INITIAL_CONFIGURATION";

      case QCRIL_EVT_QMI_RIL_POST_OPRT_ONLINE_ACTION:
          return "QCRIL_EVT_QMI_RIL_POST_OPRT_ONLINE_ACTION";

      case QCRIL_EVT_QMI_RIL_ENFORCE_DEFERRED_MODE_PREF_SET:
          return "QCRIL_EVT_QMI_RIL_ENFORCE_DEFERRED_MODE_PREF_SET";

      case QCRIL_EVT_HOOK_SET_RFM_SCENARIO_REQ:
          return "QCRIL_EVT_HOOK_SET_RFM_SCENARIO_REQ";

      case QCRIL_EVT_HOOK_GET_RFM_SCENARIO_REQ:
          return "QCRIL_EVT_HOOK_GET_RFM_SCENARIO_REQ";

      case QCRIL_EVT_HOOK_GET_PROVISIONED_TABLE_REVISION_REQ:
          return "QCRIL_EVT_HOOK_GET_PROVISIONED_TABLE_REVISION_REQ";

      case QCRIL_EVT_HOOK_SET_CDMA_SUB_SRC_WITH_SPC:
          return "QCRIL_EVT_HOOK_SET_CDMA_SUB_SRC_WITH_SPC";

      case QCRIL_EVT_HOOK_CDMA_AVOID_CUR_NWK:
          return "QCRIL_EVT_HOOK_CDMA_AVOID_CUR_NWK";

      case QCRIL_EVT_HOOK_CDMA_CLEAR_AVOIDANCE_LIST:
          return "QCRIL_EVT_HOOK_CDMA_CLEAR_AVOIDANCE_LIST";

      case QCRIL_EVT_HOOK_CDMA_GET_AVOIDANCE_LIST:
          return "QCRIL_EVT_HOOK_CDMA_GET_AVOIDANCE_LIST";

      case QCRIL_EVT_HOOK_GET_SAR_REV_KEY:
          return "QCRIL_EVT_HOOK_GET_SAR_REV_KEY";

      case QCRIL_EVT_HOOK_SET_TRANSMIT_POWER:
          return "QCRIL_EVT_HOOK_SET_TRANSMIT_POWER";

      case QCRIL_EVT_HOOK_SET_MODEM_TEST_MODE:
          return "QCRIL_EVT_HOOK_SET_MODEM_TEST_MODE";

      case QCRIL_EVT_HOOK_QUERY_MODEM_TEST_MODE:
          return "QCRIL_EVT_HOOK_QUERY_MODEM_TEST_MODE";

      case QCRIL_EVT_HOOK_CLEANUP_LOADED_CONFIGS:
          return "QCRIL_EVT_HOOK_CLEANUP_LOADED_CONFIGS";

      case QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION:
          return "QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION";

      case QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION:
          return "QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION";

      case QCRIL_EVT_QMI_RIL_PDC_ACTIVATE_CONFIGURATION:
          return "QCRIL_EVT_QMI_RIL_PDC_ACTIVATE_CONFIGURATION";

      case QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION:
          return "QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION";

      case QCRIL_EVT_QMI_RIL_PDC_LIST_CONFIGURATION:
          return "QCRIL_EVT_QMI_RIL_PDC_LIST_CONFIGURATION";

      case QCRIL_EVT_QMI_REQUEST_SET_SYS_SEL_PREF:
          return "QCRIL_EVT_QMI_REQUEST_SET_SYS_SEL_PREF";

      case QCRIL_EVT_HOOK_UNSOL_SIM_REFRESH:
          return "QCRIL_EVT_HOOK_UNSOL_SIM_REFRESH";

      case QCRIL_EVT_HOOK_SET_PREFERRED_NETWORK_ACQ_ORDER:
          return "QCRIL_EVT_HOOK_SET_PREFERRED_NETWORK_ACQ_ORDER";

      case QCRIL_EVT_HOOK_GET_PREFERRED_NETWORK_ACQ_ORDER:
          return "QCRIL_EVT_HOOK_GET_PREFERRED_NETWORK_ACQ_ORDER";

      default:
        return "<Unknown event> ?";
    } /* end switch */
  }

} /* qcril_log_lookup_event_name */


//============================================================================
// FUNCTION: qcril_ipc_init
//
// DESCRIPTION:
// create and bind sockets to respective ports, create receiver thread
// used for piping logs from 2nd ril instance to 1st instance so they can
// both be sent to diag
// Also used to send RADIO_POWER state change messages to sync RADIO_POWER
// state changes between multiple RILs.
//
// RETURN: None
//============================================================================
//
void qcril_ipc_init()
{
    int sockfd=0,rc=0,len=0;
    struct sockaddr_un local;
    char server[QCRIL_IPC_MAX_SOCKET_PATH_LENGTH];

    signal(SIGUSR1,qcril_multiple_rild_ipc_signal_handler_sigusr1);
    memset(&inter_rild_info,0,sizeof(inter_rild_info));
    rc = qcril_ipc_evaluate_rilds_socket_paths(server);
    if(rc)
    {
        unlink (server);

        //mutex initialization
        pthread_mutex_init(&inter_rild_info.send_lock_mutex, NULL);

        //server initialization
        if ((sockfd = socket(AF_UNIX,SOCK_DGRAM,0)) >= 0)
        {
            local.sun_family = AF_UNIX;
            strlcpy(local.sun_path, server, sizeof(local.sun_path));
            len = strlen(server) + sizeof(local.sun_family);

            if (bind(sockfd,(struct sockaddr *)&local, len) >= 0)
            {
                inter_rild_info.my_sockid = sockfd;
                pthread_attr_init(&inter_rild_info.recv_thread_attr);
                pthread_attr_setdetachstate(&inter_rild_info.recv_thread_attr, PTHREAD_CREATE_JOINABLE);
                if(!pthread_create(&inter_rild_info.recv_thread_id,&inter_rild_info.recv_thread_attr,
                                  qcril_multiple_rild_ipc_recv_func,(void*) &inter_rild_info))
                {
                    qmi_ril_set_thread_name(inter_rild_info.recv_thread_id, QMI_RIL_IPC_RECEIVER_THREAD_NAME);
                    inter_rild_info.info_valid = TRUE;
                }
                else
                {
                    QCRIL_LOG_ERROR("unable to spawn dedicated thread for rild IPC");
                    close(sockfd);
                }
            }
            else
            {
                QCRIL_LOG_ERROR("unable to bind socket for rild IPC");
                close(sockfd);
            }
        }
        else
        {
            QCRIL_LOG_ERROR("unable to open socket for rild IPC");
        }
    }
    else if(inter_rild_info.other_rilds_addr_info)
    {
        qcril_free(inter_rild_info.other_rilds_addr_info);
        inter_rild_info.other_rilds_addr_info_len = QMI_RIL_ZERO;
        inter_rild_info.other_rilds_addr_info = NULL;
    }
}

//============================================================================
// FUNCTION: qcril_ipc_release
//
// DESCRIPTION:
// release resources used to create inter-ril communication socket
//
// RETURN: None
//============================================================================
//
void qcril_ipc_release()
{
  if(inter_rild_info.info_valid)
  {
    inter_rild_info.info_valid = FALSE;
    pthread_mutex_destroy(&inter_rild_info.send_lock_mutex);
    pthread_kill(inter_rild_info.recv_thread_id,SIGUSR1);
    pthread_join(inter_rild_info.recv_thread_id,NULL);
    pthread_attr_destroy(&inter_rild_info.recv_thread_attr);
    close(inter_rild_info.my_sockid);

    if(inter_rild_info.other_rilds_addr_info)
    {
        qcril_free(inter_rild_info.other_rilds_addr_info);
        inter_rild_info.other_rilds_addr_info_len = QMI_RIL_ZERO;
        inter_rild_info.other_rilds_addr_info = NULL;
    }
  }
}

//============================================================================
// FUNCTION: qcril_ipc_evaluate_rilds_socket_paths
//
// DESCRIPTION:
// Evaluate socket paths for the current rild and the other rild's in mutiple rild scenario
//rild_socket_name will be updated to the current rild's socket path
//
// RETURN: FALSE If evaluation ended up with a error, TRUE otherwise
//============================================================================
//
int qcril_ipc_evaluate_rilds_socket_paths(char *rild_socket_name)
{
    int iter_i;
    int iter_other_rilds_addr_info;
    int num_of_rilds;
    int result = TRUE;
    struct sockaddr_un remote;

    num_of_rilds = qmi_ril_retrieve_number_of_rilds();

    iter_other_rilds_addr_info = 0;
    inter_rild_info.other_rilds_addr_info_len = num_of_rilds - 1;
    inter_rild_info.other_rilds_addr_info = qcril_malloc(sizeof(other_rild_addr_info_type) * (inter_rild_info.other_rilds_addr_info_len));
    if(inter_rild_info.other_rilds_addr_info)
    {
        for(iter_i = 0; iter_i < num_of_rilds; iter_i++)
        {
            if( iter_i == (int) qmi_ril_get_process_instance_id() )
            {
                snprintf( rild_socket_name, QCRIL_IPC_MAX_SOCKET_PATH_LENGTH, "%s%d", QCRIL_IPC_RILD_SOCKET_PATH_PREFIX,iter_i );
            }
            else
            {
                memset(&remote, 0, sizeof(remote));
                remote.sun_family = AF_UNIX;
                snprintf( remote.sun_path, sizeof(remote.sun_path), "%s%d", QCRIL_IPC_RILD_SOCKET_PATH_PREFIX,iter_i );
                inter_rild_info.other_rilds_addr_info[iter_other_rilds_addr_info].rild_addrlen =
                   strlen(remote.sun_path) + sizeof(remote.sun_family);
                inter_rild_info.other_rilds_addr_info[iter_other_rilds_addr_info].rild_addr = remote;
                inter_rild_info.other_rilds_addr_info[iter_other_rilds_addr_info].rild_instance_id = iter_i;
                iter_other_rilds_addr_info++;
            }
        }
    }
    else
    {
        result = FALSE;
        QCRIL_LOG_FATAL("Fail to allocate memory for inter_rild_info.other_rilds_addr_info");
    }
    return result;
} //qcril_ipc_evaluate_rilds_socket_paths

// prepare for sending radio power sync state to other rild
void qcril_multiple_rild_ipc_radio_power_propagation_helper_func(int is_genuine_signal)
{
    int iter_i;
    int num_of_rilds;

    if(inter_rild_info.other_rilds_addr_info)
    {
        num_of_rilds = qmi_ril_retrieve_number_of_rilds();

        for(iter_i = 0; iter_i < num_of_rilds; iter_i++)
        {
            if( iter_i != (int) qmi_ril_get_process_instance_id() )
            {
                qcril_multiple_rild_ipc_send_func(IPC_MESSAGE_RADIO_POWER, &is_genuine_signal, sizeof(is_genuine_signal), iter_i);
            }
        }
    }
}

int qcril_multiple_rild_ipc_send_func(ipc_message_id_type message_id, void * payload, int payload_length, int dest_rild_instance_id) //generic send function
{
    ipc_send_recv_data_info_type send_data;
    int iter_i;
    int match = FALSE;

    if(inter_rild_info.info_valid)
    {
          pthread_mutex_lock(&inter_rild_info.send_lock_mutex);
          if(inter_rild_info.other_rilds_addr_info)
          {
              for(iter_i = 0 ; iter_i < inter_rild_info.other_rilds_addr_info_len && !match; iter_i++)
              {
                  if(dest_rild_instance_id == (int) inter_rild_info.other_rilds_addr_info[iter_i].rild_instance_id)
                  {
                      match = TRUE;
                  }
              }
          }

          if(match)
          {
              memset(&send_data, 0, sizeof(send_data));         //HEADER = rild_instance_id + message_id
              send_data.rild_instance_id = qmi_ril_get_process_instance_id();
              send_data.message_id = message_id;
              send_data.payload_length = sizeof(send_data.message_id) + sizeof(send_data.rild_instance_id);

              if(NULL != payload && QMI_RIL_ZERO != payload_length)
              {
                  memcpy(&send_data.payload, payload, payload_length);
                  send_data.payload_length += payload_length;
              }
              sendto(inter_rild_info.my_sockid, &send_data, send_data.payload_length, 0,
                    (struct sockaddr *)(&inter_rild_info.other_rilds_addr_info[iter_i - 1].rild_addr),
                    inter_rild_info.other_rilds_addr_info[iter_i -1].rild_addrlen);
          }

          pthread_mutex_unlock(&inter_rild_info.send_lock_mutex);
    }
    return 0;
}

void* qcril_multiple_rild_ipc_recv_func(void *arg) //generic recv function
{
    int sockfd = ((struct inter_rild_info_type*)arg)->my_sockid;
    struct sockaddr_storage source_addr;
    socklen_t source_addr_len = 0;
    int received_buffer_length = 0;
    ipc_message_id_type message_id;

    int radio_power_is_genuine_signal;
    ipc_send_recv_data_info_type recv_data;

    source_addr_len = sizeof(source_addr);
    while(1)
    {
        memset(&source_addr,0,sizeof(source_addr));
        memset(&recv_data, 0, sizeof(recv_data));
        if ((received_buffer_length = recvfrom(sockfd, &recv_data, sizeof(recv_data) , 0,(struct sockaddr *)&source_addr, &source_addr_len)) == -1)
        {
            close(sockfd);
            break;
        }

        switch(recv_data.message_id)
        {
            case IPC_MESSAGE_RADIO_POWER:
                radio_power_is_genuine_signal = *((int *) recv_data.payload);
                qcril_qmi_nas_handle_multiple_rild_radio_power_state_propagation(radio_power_is_genuine_signal);
                break;

            case IPC_MESSAGE_AM_CALL_STATE:
                qcril_am_handle_event( QCRIL_AM_EVENT_INTER_RIL_CALL_STATE,
                                       (qcril_am_call_state_type*)recv_data.payload );
                break;

            default:
                break;
        }
    }

    qmi_ril_clear_thread_name(pthread_self());
    return NULL;
}

void qcril_multiple_rild_ipc_signal_handler_sigusr1(int arg)
{
  return;
}

//===========================================================================
// qmi_ril_set_thread_name
//===========================================================================
void qmi_ril_set_thread_name(pthread_t thread_id, const char *thread_name)
{
    int iter_i = 0;

    for(iter_i = 0; iter_i < QMI_RIL_THREAD_INFO_MAX_SIZE; iter_i++)
    {
        if(FALSE == qmi_ril_thread_name_info[iter_i].is_valid)
        {
            qmi_ril_thread_name_info[iter_i].is_valid = TRUE;
            qmi_ril_thread_name_info[iter_i].thread_id = thread_id;
            strlcpy(qmi_ril_thread_name_info[iter_i].thread_name, thread_name, QMI_RIL_THREAD_NAME_MAX_SIZE);
            break;
        }
    }

} //qmi_ril_set_thread_name

//===========================================================================
// qmi_ril_get_thread_name
//===========================================================================
int qmi_ril_get_thread_name(pthread_t thread_id, char *thread_name)
{
    int iter_i = 0,res = FALSE;

    for(iter_i = 0; iter_i < QMI_RIL_THREAD_INFO_MAX_SIZE; iter_i++)
    {
        if(TRUE == qmi_ril_thread_name_info[iter_i].is_valid && thread_id == qmi_ril_thread_name_info[iter_i].thread_id)
        {
            strlcpy(thread_name, qmi_ril_thread_name_info[iter_i].thread_name, QMI_RIL_THREAD_NAME_MAX_SIZE);
            res = TRUE;
            break;
        }
    }

    return res;
} //qmi_ril_get_thread_name

//===========================================================================
// qmi_ril_clear_thread_name
//===========================================================================
void qmi_ril_clear_thread_name(pthread_t thread_id)
{
    int iter_i = 0;

    for(iter_i = 0; iter_i < QMI_RIL_THREAD_INFO_MAX_SIZE; iter_i++)
    {
        if(TRUE == qmi_ril_thread_name_info[iter_i].is_valid && thread_id == qmi_ril_thread_name_info[iter_i].thread_id)
        {
            qmi_ril_thread_name_info[iter_i].is_valid = FALSE;
            break;
        }
    }

} //qmi_ril_clear_thread_name

/*===========================================================================

  FUNCTION: qcril_log_lookup_ril_event_index

===========================================================================*/
/*!
    @brief
    Lookup the index of  the given event in qcril_request_name[]

    @return
    Index of the event in data
*/
/*=========================================================================*/
int qcril_log_lookup_ril_event_index
(
  int event,
  qcril_qmi_event_log_type *data,
  int min_index,
  int max_index
)
{
   int min = min_index, max = max_index, index;

   while( min <= max )
   {
      index = (min+max)/2;

      if( data[index].event  == event )
      {
        return index;
      }
      else if( data[index].event  < event )
      {
        min = index + 1;
      }
      else
      {
        max = index - 1;
      }
   }

   // last element which is unkown event
   return max_index;
} /* qcril_log_lookup_ril_event_index */
