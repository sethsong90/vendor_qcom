/******************************************************************************
  @file    qcril_qmi_am.c
  @brief   qcril qmi - Audio Management

  DESCRIPTION
    Implements Audio Management APIs.

  ---------------------------------------------------------------------------

  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  ---------------------------------------------------------------------------
******************************************************************************/

#include <media/AudioSystem.h>
#include <cutils/properties.h>
#include <utils/String8.h>
#include <binder/IPCThreadState.h>
#include "qcril_am.h"

extern "C" {
#include "qcrili.h"
#include "qcril_log.h"
#include "qcril_qmi_voice.h"
#include "qcril_qmi_imsa.h"
}

using namespace android;

const int MAX_32_BIT_INT_DIGITS_NUM = 11;

static pthread_mutex_t am_state_mutex;
static boolean is_prim_rild;
static boolean is_dsda;
static uint8 num_of_rilds;

typedef struct {
    boolean is_valid;
    uint32 vsid;
} voice_vsid_type;

typedef struct {
    boolean is_valid;
    qcril_am_audio_api_param_type audio_api_param;
} pending_call_state_change_req;

typedef struct {
    // apply to all rild instances
    voice_vsid_type voice_vsid;
    boolean in_lch;

    // apply to primary rild only
    boolean in_srvcc;
    boolean ims_on_wlan;

    // only apply to DSDA primary rild
    qcril_am_call_state_type cur_state[QCRIL_MAX_INSTANCE_ID];
    pending_call_state_change_req pending_ril_req[QCRIL_MAX_INSTANCE_ID];
} qcril_am_state_type;

qcril_am_state_type am_state;

/***************************************************************************************************
    @function
    qcril_am_lock - this lock could be recursively calling within a thread
***************************************************************************************************/
static void qcril_am_lock()
{
  pthread_mutex_lock( &am_state_mutex );
}

/***************************************************************************************************
    @function
    qcril_am_unlock
***************************************************************************************************/
static void qcril_am_unlock()
{
  pthread_mutex_unlock( &am_state_mutex );
}

/***************************************************************************************************
    @function
    qcril_am_get_audio_vsid

    @implementation detail
    Gets vsid for the voice subsystem as queried.

    @param[in]
        vs_type
            voice subsystem type which is queried

    @param[out]
        vsid
            vsid of the voice subsystem type queried

    @retval
    QCRIL_AM_ERR_NONE if function is successful, QCRIL_AM_ERR_VSID_NOT_AVAILABLE if the vsid
    is not available.
***************************************************************************************************/
static RIL_Errno qcril_am_get_audio_vsid(qcril_am_vs_type vs_type, uint32 *vsid)
{
    RIL_Errno err = RIL_E_SUCCESS;

    // TODO: did not find IMS_VSID and VOICE_VSID value in header file
    switch (vs_type)
    {
    case QCRIL_AM_VS_IMS:
        *vsid = 0x10C02000; // IMS_VSID
        break;
    case QCRIL_AM_VS_IMS_WLAN:
        *vsid = 0x10002000; // IMS_WLAN_VSID
        break;
    case QCRIL_AM_VS_VOICE:
        qcril_am_lock();
        if (am_state.voice_vsid.is_valid)
        {
            *vsid = am_state.voice_vsid.vsid;
        }
        else
        {
            *vsid = 0x10C01000; // VOICE_VSID
        }
        qcril_am_unlock();
        break;
    default:
        err = RIL_E_GENERIC_FAILURE;
    }
    return err;
}

/***************************************************************************************************
    @function
    qcril_am_get_audio_call_state

    @implementation detail
    Maps qcril_am call_state to call_state as defined in AudioSystem.h
***************************************************************************************************/
static uint qcril_am_get_audio_call_state(qcril_am_call_state_type call_state)
{
    uint ret = QCRIL_AM_CALL_STATE_INACTIVE;
    if (call_state > QCRIL_AM_CALL_STATE_MIN && call_state < QCRIL_AM_CALL_STATE_MAX)
    {
        ret = call_state;
        if (QCRIL_AM_CALL_STATE_ACTIVE == ret)
        {
            qcril_am_lock();
            if (am_state.in_lch)
            {
                ret = 4; // local hold, TODO: do not find enum in audio header file
            }
            qcril_am_unlock();
        }
    }
    else
    {
        QCRIL_LOG_DEBUG("invalid call_state value: %d", call_state);
    }
    return ret;
}


/***************************************************************************************************
    @function
    qcril_am_is_active_call

    @implementation detail
    Checks if a call will be considered as an active call from AM perspective. It includes both
    current active call and the call expected to be changed to active by modem.
***************************************************************************************************/
static boolean qcril_am_is_active_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    boolean ret = FALSE;
    if (call_info_entry)
    {
        if ( CALL_STATE_CONVERSATION_V02 == call_info_entry->voice_scv_info.call_state ||
             CALL_STATE_ORIGINATING_V02 == call_info_entry->voice_scv_info.call_state ||
             CALL_STATE_ALERTING_V02 == call_info_entry->voice_scv_info.call_state ||
             CALL_STATE_DISCONNECTING_V02 == call_info_entry->voice_scv_info.call_state ||
             call_info_entry->elaboration & QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ANSWERING_CALL ||
             call_info_entry->elaboration & QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_SWITCHING_CALL_TO_ACTIVE
           )
        {
            ret = TRUE;
        }
    }
    return ret;
}

/***************************************************************************************************
    @function
    qcril_am_is_hold_call

    @implementation detail
    Checks if a call is a hold call.
***************************************************************************************************/
static boolean qcril_am_is_hold_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    boolean ret = FALSE;
    if (call_info_entry)
    {
        if ( CALL_STATE_HOLD_V02 == call_info_entry->voice_scv_info.call_state )
        {
            ret = TRUE;
        }
    }
    return ret;
}

/***************************************************************************************************
    @function
    qcril_am_is_active_ims_call

    @implementation detail
    Checks if a call will be considered as an active ims call from AM perspective.
***************************************************************************************************/
static boolean qcril_am_is_active_ims_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    return qcril_qmi_voice_is_ims_call(call_info_entry) &&
           qcril_am_is_active_call(call_info_entry);
}

/***************************************************************************************************
    @function
    qcril_am_is_hold_ims_call

    @implementation detail
    Checks if a call is a hold ims call from AM perspective.
***************************************************************************************************/
static boolean qcril_am_is_hold_ims_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    return qcril_qmi_voice_is_ims_call(call_info_entry) &&
           qcril_am_is_hold_call(call_info_entry);
}

/***************************************************************************************************
    @function
    qcril_am_is_active_voice_call

    @implementation detail
    Checks if a call will be considered as an active voice call from AM perspective.
***************************************************************************************************/
static boolean qcril_am_is_active_voice_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    return !qcril_qmi_voice_is_ims_call(call_info_entry) &&
           qcril_am_is_active_call(call_info_entry);
}

/***************************************************************************************************
    @function
    qcril_am_is_hold_voice_call

    @implementation detail
    Checks if a call is a hold ims call from AM perspective.
***************************************************************************************************/
static boolean qcril_am_is_hold_voice_call(const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry)
{
    return !qcril_qmi_voice_is_ims_call(call_info_entry) &&
           qcril_am_is_hold_call(call_info_entry);
}

/***************************************************************************************************
    @function
    qcril_am_set_voice_call_audio_driver_by_call_state
***************************************************************************************************/
static RIL_Errno qcril_am_set_voice_call_audio_driver_by_call_state()
{
    RIL_Errno ret = RIL_E_SUCCESS;
    qcril_am_lock();
    boolean in_srvcc = am_state.in_srvcc;
    qcril_am_unlock();

    if (!in_srvcc)
    {
        if (qcril_qmi_voice_has_specific_call(qcril_am_is_active_voice_call))
        {
            QCRIL_LOG_INFO("has active voice call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_ACTIVE);
        }
        else if (qcril_qmi_voice_has_specific_call(qcril_am_is_hold_voice_call))
        {
            QCRIL_LOG_INFO("has hold voice call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_HOLD);
        }
        else
        {
            QCRIL_LOG_INFO("no active and hold voice call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_INACTIVE);
        }
    }
    else
    {
        if (qcril_qmi_voice_has_specific_call(qcril_am_is_active_call))
        {
            QCRIL_LOG_INFO("in srvcc - has active voice or ims call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_ACTIVE);
        }
        else if (qcril_qmi_voice_has_specific_call(qcril_am_is_hold_call))
        {
            QCRIL_LOG_INFO("in srvcc - has hold voice or ims call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_HOLD);
        }
        else
        {
            QCRIL_LOG_INFO("in srvcc - no active and hold voice/ims call");
            ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_INACTIVE);
        }
    }

    return ret;
}

/***************************************************************************************************
    @function
    qcril_am_set_ims_call_audio_driver_by_call_state
***************************************************************************************************/
static RIL_Errno qcril_am_set_ims_call_audio_driver_by_call_state()
{
    RIL_Errno ret = RIL_E_SUCCESS;

    if (qcril_qmi_voice_has_specific_call(qcril_am_is_active_ims_call))
    {
        if(am_state.ims_on_wlan)
        {
          QCRIL_LOG_INFO("has active ims call on WLAN");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_ACTIVE);
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
        }
        else
        {
          QCRIL_LOG_INFO("has active ims call");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_ACTIVE);
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);
        }
    }
    else if (qcril_qmi_voice_has_specific_call(qcril_am_is_hold_ims_call))
    {
        if(am_state.ims_on_wlan)
        {
          QCRIL_LOG_INFO("has hold ims call on WLAN");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_HOLD);
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
        }
        else
        {
          QCRIL_LOG_INFO("has hold ims call");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_HOLD);
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);
        }
    }
    else
    {
          QCRIL_LOG_INFO("no active and hold ims call on WLAN");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);

          QCRIL_LOG_INFO("no active and hold ims call");
          ret = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
    }

    return ret;
}

/***************************************************************************************************
    @function
    qcril_am_set_ims_srv_rat

    @brief
    Sets IMS service is on wlan or not.
***************************************************************************************************/
static void qcril_am_set_ims_on_wlan()
{
    qcril_qmi_imsa_srv_status_type srv_status = qcril_qmi_imsa_get_srv_status();
    qcril_am_lock();
    am_state.ims_on_wlan = FALSE;
    if (srv_status.voip_service_rat_valid)
    {
        if (IMSA_WLAN_V01 == srv_status.voip_service_rat)
        {
            am_state.ims_on_wlan = TRUE;
        }
    }
    else if (srv_status.vt_service_rat_valid)
    {
        if (IMSA_WLAN_V01 == srv_status.vt_service_rat)
        {
            am_state.ims_on_wlan = TRUE;
        }
    }
    QCRIL_LOG_INFO("am_state.ims_on_wlan: %d", am_state.ims_on_wlan);
    qcril_am_unlock();
}

/***************************************************************************************************
    @function
    qcril_am_convert_vsid_audio_call_state_to_string

    @brief
    Convert vsid and audio_call_state to a string formatted with "vsid=xxx;call_state=xxx"
***************************************************************************************************/
static String8 qcril_am_convert_vsid_audio_call_state_to_string(uint32 audio_vsid, uint32 audio_call_state)
{
    String8 keyValPairs;
    char buffer[MAX_32_BIT_INT_DIGITS_NUM+1];

    snprintf(buffer, sizeof(buffer), "%lu", audio_vsid);
    // TODO: did not find the key in header file
    keyValPairs = String8("vsid") + String8("=") + String8(buffer);

    snprintf(buffer, sizeof(buffer), "%lu", audio_call_state);
    // TODO: did not find the key in header file
    keyValPairs += String8(";") + String8("call_state") + String8("=") + String8(buffer);

    QCRIL_LOG_INFO("keyValPairs: %s", keyValPairs.string());

    return keyValPairs;
}

/***************************************************************************************************
    @function
    qcril_am_call_audio_api

    @brief
    Calls Audio SetParameter API with the provided auguments
***************************************************************************************************/
static RIL_Errno qcril_am_call_audio_api(uint32 audio_vsid, uint32 audio_call_state)
{
    RIL_Errno err = RIL_E_SUCCESS;

    if ( FALSE == qmi_ril_is_feature_supported(QMI_RIL_FEATURE_KK) )
    {
        QCRIL_LOG_INFO("RIL need not set audio params!");
    }
    else
    {
        status_t status = AudioSystem::setParameters(
                             0,
                             qcril_am_convert_vsid_audio_call_state_to_string(audio_vsid, audio_call_state));
        QCRIL_LOG_INFO("AudioSystem::setParameters return status: %d", status);
        if (status)
        {
            err = RIL_E_GENERIC_FAILURE;
        }
    }
    return err;
}

/***************************************************************************************************
    @function
    qcril_am_send_audio_state_change_oem_hook_unsol_resp

    @brief
    Send QCRIL_EVT_HOOK_UNSOL_AUDIO_STATE_CHANGED
***************************************************************************************************/
static void qcril_am_send_audio_state_change_oem_hook_unsol_resp
(
  const char* str
)
{
    if (str)
    {
        QCRIL_LOG_INFO("Sending QCRIL_EVT_HOOK_UNSOL_AUDIO_STATE_CHANGED: %s", str);
        qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID,
                                   QCRIL_EVT_HOOK_UNSOL_AUDIO_STATE_CHANGED,
                                   (char *) str,
                                   strlen(str) );
    }
    else
    {
        QCRIL_LOG_DEBUG("empty string for QCRIL_EVT_HOOK_UNSOL_AUDIO_STATE_CHANGED");
    }
} // qcril_am_send_audio_state_change_oem_hook_unsol_resp

/***************************************************************************************************
    @function
    qcril_am_is_same_audio_call_state

    @brief
    Checks if the call state we are trying to set is the same as current audio state.

    AudioSystem::getParameters will return a string in a format of:
        all_call_states=281022464:1,282857472:1,281026560:1,276836352:1
***************************************************************************************************/
boolean qcril_am_is_same_audio_call_state
(
    uint32 vsid,
    uint32 new_call_state
)
{
    boolean ret = FALSE;
    QCRIL_LOG_INFO("vsid: %d, call_state: %d", vsid, new_call_state);
    String8 all_call_states = AudioSystem::getParameters(0, String8("all_call_states"));
    QCRIL_LOG_INFO("getParameters: %s", all_call_states.string() );

    char vsid_str[MAX_32_BIT_INT_DIGITS_NUM+1];
    snprintf(vsid_str, sizeof(vsid_str), "%lu:", vsid);
    ssize_t vsid_idx = all_call_states.find(vsid_str, 0);
    if (vsid_idx != -1)
    {
        ssize_t colon_idx;
        colon_idx = all_call_states.find(":", vsid_idx);
        if (colon_idx+1 < all_call_states.length())
        {
            uint32 cur_call_state = all_call_states[colon_idx + 1] - '0';

            if ( QCRIL_AM_CALL_STATE_MIN >= cur_call_state || QCRIL_AM_CALL_STATE_MAX <= cur_call_state ||
                 QCRIL_AM_CALL_STATE_MIN >= new_call_state || QCRIL_AM_CALL_STATE_MAX <= new_call_state )
            {
                QCRIL_LOG_DEBUG("cur_call_state or new_call_state is not in a valid range");
            }

            if (new_call_state == cur_call_state)
            {
                ret = TRUE;
            }
        }
        else
        {
            QCRIL_LOG_DEBUG("no state value after vsid");
        }
    }
    else
    {
        QCRIL_LOG_DEBUG("vsid_idx == -1");
    }
    QCRIL_LOG_FUNC_RETURN_WITH_RET((int)ret);
    return ret;
}

/***************************************************************************************************
    @function
    qcril_am_handle_pending_req

    @brief
    Applies to DSDA only.
    Checks pending requests and calls Audio SetParameter API if applicable. Avoids to have two
    voices both on Active state.
***************************************************************************************************/
static void qcril_am_handle_pending_req()
{
    uint8 num_of_active_state = 0;
    uint8 num_of_pending_req = 0;
    uint8 i;

    qcril_am_lock();
    for (i=0; i<num_of_rilds; i++)
    {
        if (am_state.pending_ril_req[i].is_valid)
        {
            num_of_pending_req++;
            if (QCRIL_AM_CALL_STATE_ACTIVE == am_state.pending_ril_req[i].audio_api_param.call_state)
            {
                num_of_active_state++;
            }
        }
        else
        {
            if (QCRIL_AM_CALL_STATE_ACTIVE == am_state.cur_state[i])
            {
                num_of_active_state++;
            }
        }
    }

    QCRIL_LOG_INFO( "num_of_pending_req: %d, num_of_active_state: %d",
                    num_of_pending_req, num_of_active_state );
    if (num_of_pending_req && num_of_active_state <= 1)
    {
        String8 keyValPairs;
        for (i=0; i<num_of_rilds; i++)
        {
            if ( am_state.pending_ril_req[i].is_valid &&
                 QCRIL_AM_CALL_STATE_ACTIVE != am_state.pending_ril_req[i].audio_api_param.call_state )
            {
                if ( !qcril_am_is_same_audio_call_state(
                          am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                          am_state.pending_ril_req[i].audio_api_param.call_state) )
                {
                    qcril_am_call_audio_api( am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                                             am_state.pending_ril_req[i].audio_api_param.call_state);
                    if (!keyValPairs.isEmpty())
                    {
                        keyValPairs += ";";
                    }
                    keyValPairs += qcril_am_convert_vsid_audio_call_state_to_string(
                                       am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                                       am_state.pending_ril_req[i].audio_api_param.call_state );
                }
                am_state.cur_state[i] = am_state.pending_ril_req[i].audio_api_param.call_state;
                am_state.pending_ril_req[i].is_valid = FALSE;
            }
        }

        for (i=0; i<num_of_rilds; i++)
        {
            if (am_state.pending_ril_req[i].is_valid)
            {
                if ( !qcril_am_is_same_audio_call_state(
                          am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                          am_state.pending_ril_req[i].audio_api_param.call_state) )
                {
                    qcril_am_call_audio_api( am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                                             am_state.pending_ril_req[i].audio_api_param.call_state);
                    if (!keyValPairs.isEmpty())
                    {
                        keyValPairs += ";";
                    }
                    keyValPairs += qcril_am_convert_vsid_audio_call_state_to_string(
                                       am_state.pending_ril_req[i].audio_api_param.voice_vsid,
                                       am_state.pending_ril_req[i].audio_api_param.call_state );
                }
                am_state.cur_state[i] = am_state.pending_ril_req[i].audio_api_param.call_state;
                am_state.pending_ril_req[i].is_valid = FALSE;
                break; // at most one active state to set
            }
        }
        if (!keyValPairs.isEmpty())
        {
            qcril_am_send_audio_state_change_oem_hook_unsol_resp(keyValPairs.string());
        }
    }
    qcril_am_unlock();
}

extern "C" {

/***************************************************************************************************
    @function
    qcril_am_pre_init
***************************************************************************************************/
void qcril_am_pre_init()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init( &am_state_mutex, &attr );
    pthread_mutexattr_destroy(&attr);

    char prop_str[ PROPERTY_VALUE_MAX ];
    *prop_str = 0;
    property_get( QMI_RIL_SYS_PROP_NAME_MULTI_SIM, prop_str, "" );
    if ( strncmp(prop_str, "dsda", QMI_RIL_SYS_PROP_LENGTH_MULTI_SIM ) == 0)
    {
        is_dsda = TRUE;
    }

    if (QCRIL_DEFAULT_INSTANCE_ID == qmi_ril_get_process_instance_id())
    {
        is_prim_rild = TRUE;
    }

    num_of_rilds = qmi_ril_retrieve_number_of_rilds();

    ProcessState::self()->startThreadPool();
}

/***************************************************************************************************
    @function
    qcril_am_post_cleanup
***************************************************************************************************/
void qcril_am_post_cleanup()
{
   pthread_mutex_destroy(&am_state_mutex);
}

/***************************************************************************************************
    @function
    qcril_am_set_call_audio_driver

    @implementation detail
    Maps vs_type and call_state to the value defined in AudioSystem.h,
    and calls setParameters API of AudioSystem
***************************************************************************************************/
RIL_Errno qcril_am_set_call_audio_driver
(
    qcril_am_vs_type vs_type,
    qcril_am_call_state_type call_state
)
{
    QCRIL_LOG_FUNC_ENTRY();
    RIL_Errno err = RIL_E_SUCCESS;
    uint32 audio_vsid;
    uint32 audio_call_state;

    if ( FALSE == qmi_ril_is_feature_supported(QMI_RIL_FEATURE_KK) )
    {
        QCRIL_LOG_INFO("RIL need not set audio params!");
    }
    else
    {
        qcril_am_lock();
        do
        {
            QCRIL_LOG_INFO("vs_type: %d, call_state: %d", vs_type, call_state);
            if ( vs_type <= QCRIL_AM_VS_MIN || vs_type >= QCRIL_AM_VS_MAX ||
                 call_state <= QCRIL_AM_CALL_STATE_MIN || call_state >= QCRIL_AM_CALL_STATE_MAX )
            {
                err = RIL_E_REQUEST_NOT_SUPPORTED;
                break;
            }

            err = qcril_am_get_audio_vsid(vs_type, &audio_vsid);
            if (RIL_E_SUCCESS != err)
            {
                break;
            }

            audio_call_state  = qcril_am_get_audio_call_state(call_state);
        } while (FALSE);

        qcril_am_unlock();

        if (RIL_E_SUCCESS == err )
        {
            if (!is_prim_rild && is_dsda && QCRIL_AM_VS_VOICE == vs_type)
            {
                qcril_am_inter_rild_msg_type msg;
                msg.rild_id = qmi_ril_get_process_instance_id();
                msg.param.call_state = (qcril_am_call_state_type)audio_call_state;
                msg.param.voice_vsid = audio_vsid;
                QCRIL_LOG_INFO( "sending rild_id: %d, call_state: %d, voice_vsid: %d",
                                msg.rild_id, msg.param.call_state, msg.param.voice_vsid );
                qcril_multiple_rild_ipc_send_func( IPC_MESSAGE_AM_CALL_STATE,
                                                   &msg,
                                                   sizeof(msg),
                                                   0 );
            }
            else if (is_prim_rild && is_dsda && QCRIL_AM_VS_VOICE == vs_type)
            {
                qcril_am_lock();
                am_state.pending_ril_req[0].is_valid = TRUE;
                am_state.pending_ril_req[0].audio_api_param.voice_vsid = audio_vsid;
                am_state.pending_ril_req[0].audio_api_param.call_state = (qcril_am_call_state_type)audio_call_state;
                qcril_am_handle_pending_req();
                qcril_am_unlock();
            }
            else
            {
                if (!qcril_am_is_same_audio_call_state(audio_vsid, audio_call_state))
                {
                    qcril_am_call_audio_api(audio_vsid, audio_call_state);
                    qcril_am_send_audio_state_change_oem_hook_unsol_resp(
                        qcril_am_convert_vsid_audio_call_state_to_string(audio_vsid, audio_call_state).string() );
                }
            }
        }
    }

    QCRIL_LOG_FUNC_RETURN_WITH_RET((int)err);
    return err;
}

/***************************************************************************************************
    @function
    qcril_am_state_reset
***************************************************************************************************/
void qcril_am_state_reset()
{
    qcril_am_lock();
    qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);
    qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
    qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_INACTIVE);
    memset(&am_state, 0, sizeof(am_state));
    qcril_am_unlock();
}

/***************************************************************************************************
    @function
    qcril_am_handle_event
***************************************************************************************************/
RIL_Errno qcril_am_handle_event(qcril_am_event_type event, const void* data)
{
    RIL_Errno err = RIL_E_SUCCESS;

    if ( FALSE == qmi_ril_is_feature_supported(QMI_RIL_FEATURE_KK) )
    {
        QCRIL_LOG_INFO("Not handling event %d", event);
    }
    else
    {
        const char *EVT_LOG_STR[] = { "EVENT_MIN",
                                      "IMS_ANSWER",
                                      "IMS_ANSWER_FAIL",
                                      "VOICE_ANSWER",
                                      "VOICE_ANSWER_FAIL",
                                      "SWITCH_CALL",
                                      "SWITCH_CALL_FAIL",
                                      "CALL_STATE_CHANGED",
                                      "SRVCC_START",
                                      "SRVCC_COMPLETE",
                                      "SRVCC_FAIL",
                                      "SRVCC_CANCEL",
                                      "IMS_SRV_CHANGED",
                                      "LCH",
                                      "UNLCH",
                                      "INTER_RIL_CALL_STATE",
                                      "EVENT_MAX" };

        if ( QCRIL_AM_EVENT_MIN <= event && event < sizeof(EVT_LOG_STR)/sizeof(EVT_LOG_STR[0]) )
        {
            QCRIL_LOG_INFO("processing event: %s", EVT_LOG_STR[event]);
        }
        else
        {
            QCRIL_LOG_INFO("processing UNKNOWN event: %d", event);
        }

        switch (event)
        {
        case QCRIL_AM_EVENT_IMS_ANSWER:
            if(am_state.ims_on_wlan)
            {
              QCRIL_LOG_INFO( "Answer IMS call on WLAN");
              err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_ACTIVE);
              err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
            }
            else
            {
              QCRIL_LOG_INFO( "Answer IMS call");
              err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_ACTIVE);
              err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);
            }
            break;

        case QCRIL_AM_EVENT_VOICE_ANSWER:
            err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_VOICE, QCRIL_AM_CALL_STATE_ACTIVE);
            break;

        case QCRIL_AM_EVENT_IMS_ANSWER_FAIL:
            err = qcril_am_set_ims_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_VOICE_ANSWER_FAIL:
            err = qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_SWITCH_CALL:
        case QCRIL_AM_EVENT_SWITCH_CALL_FAIL:
        case QCRIL_AM_EVENT_CALL_STATE_CHANGED:
            err = qcril_am_set_ims_call_audio_driver_by_call_state();
            err = qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_SRVCC_START:
            qcril_am_lock();
            am_state.in_srvcc = TRUE;
            qcril_am_unlock();
            err = qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_SRVCC_COMPLETE:
            qcril_am_lock();
            am_state.in_srvcc = FALSE;
            qcril_am_unlock();
            err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS_WLAN, QCRIL_AM_CALL_STATE_INACTIVE);
            err = qcril_am_set_call_audio_driver(QCRIL_AM_VS_IMS, QCRIL_AM_CALL_STATE_INACTIVE);
            break;

        case QCRIL_AM_EVENT_SRVCC_FAIL:
        case QCRIL_AM_EVENT_SRVCC_CANCEL:
            qcril_am_lock();
            am_state.in_srvcc = FALSE;
            qcril_am_unlock();
            err = qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_IMS_SRV_CHANGED:
            qcril_am_set_ims_on_wlan();
            break;

        case QCRIL_AM_EVENT_LCH:
            qcril_am_lock();
            am_state.in_lch = TRUE;
            qcril_am_unlock();
            qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_UNLCH:
            qcril_am_lock();
            am_state.in_lch = FALSE;
            qcril_am_unlock();
            qcril_am_set_voice_call_audio_driver_by_call_state();
            break;

        case QCRIL_AM_EVENT_INTER_RIL_CALL_STATE:
            if (data)
            {
                const qcril_am_inter_rild_msg_type* msg = (const qcril_am_inter_rild_msg_type*)data;
                QCRIL_LOG_INFO( "received rild_id: %d, call_state: %d, voice_vsid: %d",
                                msg->rild_id, msg->param.call_state, msg->param.voice_vsid );
                if (msg->rild_id < num_of_rilds)
                {
                    qcril_am_lock();
                    am_state.pending_ril_req[msg->rild_id].is_valid = TRUE;
                    am_state.pending_ril_req[msg->rild_id].audio_api_param = msg->param;
                    qcril_am_handle_pending_req();
                    qcril_am_unlock();
                }
                else
                {
                    QCRIL_LOG_ERROR("num_of_rilds(%d) <= msg->rild_id", num_of_rilds);
                }
            }
            else
            {
                QCRIL_LOG_ERROR("Unexpected NULL data");
            }
            break;

        default:
            QCRIL_LOG_DEBUG("ignore unexpected event");
            err = RIL_E_REQUEST_NOT_SUPPORTED;
        }
    }

    QCRIL_LOG_FUNC_RETURN_WITH_RET((int)err);
    return err;
}

/***************************************************************************************************
    @function
    qcril_am_set_vsid
***************************************************************************************************/
RIL_Errno qcril_am_set_vsid(qcril_am_vs_type vs_type, uint32 vsid)
{
    QCRIL_LOG_INFO("set vs_type: %d to vsid: %u", vs_type, vsid);
    RIL_Errno err = RIL_E_SUCCESS;

    if (QCRIL_AM_VS_VOICE == vs_type)
    {
        qcril_am_lock();
        am_state.voice_vsid.is_valid = TRUE;
        am_state.voice_vsid.vsid = vsid;
        qcril_am_unlock();
    }
    else
    {
        err = RIL_E_REQUEST_NOT_SUPPORTED;
    }

    QCRIL_LOG_FUNC_RETURN_WITH_RET((int)err);
    return err;
}

} // end of extern "C"
