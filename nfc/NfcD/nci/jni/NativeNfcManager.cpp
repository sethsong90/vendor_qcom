/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */

/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include "OverrideLog.h"
#include "NfcJniUtil.h"
#include "NfcAdaptation.h"
#include "SyncEvent.h"
#include "PeerToPeer.h"
#include "SecureElement.h"
#include "RoutingManager.h"
#include "NfcTag.h"
#include "config.h"
#include "PowerSwitch.h"
#include "JavaClassConstants.h"
#include "Pn544Interop.h"
#include <ScopedLocalRef.h>
#include <ScopedUtfChars.h>
#include <ScopedPrimitiveArray.h>

extern "C"
{
    #include "nfa_api.h"
    #include "nfa_p2p_api.h"
    #include "rw_api.h"
    #include "nfa_ee_api.h"
    #include "nfc_brcm_defs.h"
    #include "ce_api.h"
    #ifdef DTA // <DTA>
    #include "dta_mode.h"
    #endif // </DTA>
}
#define NFCSERVICE_WATCHDOG_TIMER_EXPIRED 4
extern UINT8 *p_nfa_dm_start_up_cfg;
extern const UINT8 nfca_version_string [];
extern const UINT8 nfa_version_string [];
extern tNFA_DM_DISC_FREQ_CFG* p_nfa_dm_rf_disc_freq_cfg; //defined in stack
namespace android
{
    extern bool gIsTagDeactivating;
    extern bool gIsSelectingRfInterface;
    extern void nativeNfcTag_doTransceiveStatus (uint8_t * buf, uint32_t buflen);
    extern void nativeNfcTag_notifyRfTimeout ();
    extern void nativeNfcTag_doConnectStatus (jboolean is_connect_ok);
    extern void nativeNfcTag_doDeactivateStatus (int status);
    extern void nativeNfcTag_doWriteStatus (jboolean is_write_ok);
    extern void nativeNfcTag_doCheckNdefResult (tNFA_STATUS status, uint32_t max_size, uint32_t current_size, uint8_t flags);
    extern void nativeNfcTag_doMakeReadonlyResult (tNFA_STATUS status);
    extern void nativeNfcTag_doPresenceCheckResult (tNFA_STATUS status);
    extern void nativeNfcTag_formatStatus (bool is_ok);
    extern void nativeNfcTag_resetPresenceCheck ();
    extern void nativeNfcTag_doReadCompleted (tNFA_STATUS status);
    extern void nativeNfcTag_abortWaits ();
    extern void nativeLlcpConnectionlessSocket_abortWait ();
    extern void nativeNfcTag_registerNdefTypeHandler ();
    extern void nativeLlcpConnectionlessSocket_receiveData (uint8_t* data, uint32_t len, uint32_t remote_sap);
    #ifdef DTA // <DTA>
    extern struct ResponseMessage* nfcDepTransceive(uint8_t* buf, uint32_t bufLen);
    extern void nfcDepTransceiveStatus(uint8_t* buf, uint32_t bufLen);
    extern void debugMessage(uint8_t* array, uint32_t size);
    #endif // </DTA>
}


/*****************************************************************************
**
** public variables and functions
**
*****************************************************************************/
bool                        gActivated = false;
SyncEvent                   gDeactivatedEvent;

namespace android
{
    jmethodID               gCachedNfcManagerNotifyNdefMessageListeners;
    jmethodID               gCachedNfcManagerNotifyTransactionListeners;
    jmethodID               gCachedNfcManagerNotifyLlcpLinkActivation;
    jmethodID               gCachedNfcManagerNotifyLlcpLinkDeactivated;
    jmethodID               gCachedNfcManagerNotifyLlcpFirstPacketReceived;
    jmethodID               gCachedNfcManagerNotifySeFieldActivated;
    jmethodID               gCachedNfcManagerNotifySeFieldDeactivated;
    jmethodID               gCachedNfcManagerNotifySeListenActivated;
    jmethodID               gCachedNfcManagerNotifySeListenDeactivated;
    jmethodID               gCachedNfcManagerNotifyHostEmuActivated;
    jmethodID               gCachedNfcManagerNotifyHostEmuData;
    jmethodID               gCachedNfcManagerNotifyHostEmuDeactivated;
    const char*             gNativeP2pDeviceClassName                 = "com/android/nfc/dhimpl/NativeP2pDevice";
    const char*             gNativeLlcpServiceSocketClassName         = "com/android/nfc/dhimpl/NativeLlcpServiceSocket";
    const char*             gNativeLlcpConnectionlessSocketClassName  = "com/android/nfc/dhimpl/NativeLlcpConnectionlessSocket";
    const char*             gNativeLlcpSocketClassName                = "com/android/nfc/dhimpl/NativeLlcpSocket";
    const char*             gNativeNfcTagClassName                    = "com/android/nfc/dhimpl/NativeNfcTag";
    const char*             gNativeNfcManagerClassName                = "com/android/nfc/dhimpl/NativeNfcManager";
    const char*             gNativeNfcSecureElementClassName          = "com/android/nfc/dhimpl/NativeNfcSecureElement";
    void                    doStartupConfig ();
    void                    startStopPolling (bool isStartPolling);
    void                    startRfDiscovery (bool isStart);
    void                    setUiccIdleTimeout (bool enable);
    void                    restartPollingWithTechMask(int mask);
    #ifdef DTA // </DTA>
    const int gNfcDepDslDeactivation = 1;
    const int gNfcDepRlsDeactivation = 2;
    const int gNfaDeactivationToSleep = 3;
    const int gNfaDeactivation = 4;
    #endif // </DTA>
}


/*****************************************************************************
**
** private variables and functions
**
*****************************************************************************/
namespace android
{
static jint                 sLastError = ERROR_BUFFER_TOO_SMALL;
static jmethodID            sCachedNfcManagerNotifySeApduReceived;
static jmethodID            sCachedNfcManagerNotifySeMifareAccess;
static jmethodID            sCachedNfcManagerNotifySeEmvCardRemoval;
static jmethodID            sCachedNfcManagerNotifyTargetDeselected;
static SyncEvent            sNfaEnableEvent;  //event for NFA_Enable()
static SyncEvent            sNfaDisableEvent;  //event for NFA_Disable()
static SyncEvent            sNfaEnableDisablePollingEvent;  //event for NFA_EnablePolling(), NFA_DisablePolling()
static SyncEvent            sNfaSetConfigEvent;  // event for Set_Config....
static SyncEvent            sNfaGetConfigEvent;  // event for Get_Config....
static bool                 sIsNfaEnabled = false;
static bool                 sDiscoveryEnabled = false;  //is polling for tag?
static bool                 sIsDisabling = false;
static bool                 sRfEnabled = false; // whether RF discovery is enabled
static bool                 sSeRfActive = false;  // whether RF with SE is likely active
static bool                 sReaderModeEnabled = false; // whether we're only reading tags, not allowing P2p/card emu
static bool                 sP2pActive = false; // whether p2p was last active
static bool                 sAbortConnlessWait = false;
static bool                 sIsSecElemSelected = false;  //has NFC service selected a sec elem
#ifdef DTA // <DTA>
// static int          sDtaPatternNumber = 0x00;
static int sDtaPatternNumber = -1;
/* should be the same as above but just incase backup last pattern set */
// static int          LastPatternNumber = 0x00;
static int LastPatternNumber = -1;
#endif // </DTA>
#define CONFIG_UPDATE_TECH_MASK     (1 << 1)
#define DEFAULT_TECH_MASK           (NFA_TECHNOLOGY_MASK_A \
                                     | NFA_TECHNOLOGY_MASK_B \
                                     | NFA_TECHNOLOGY_MASK_F \
                                     | NFA_TECHNOLOGY_MASK_ISO15693 \
                                     | NFA_TECHNOLOGY_MASK_B_PRIME \
                                     | NFA_TECHNOLOGY_MASK_A_ACTIVE \
                                     | NFA_TECHNOLOGY_MASK_F_ACTIVE \
                                     | NFA_TECHNOLOGY_MASK_KOVIO)
#define DEFAULT_DISCOVERY_DURATION       500
#define READER_MODE_DISCOVERY_DURATION   200

static void nfaConnectionCallback (UINT8 event, tNFA_CONN_EVT_DATA *eventData);
static void nfaDeviceManagementCallback (UINT8 event, tNFA_DM_CBACK_DATA *eventData);
static bool isPeerToPeer (tNFA_ACTIVATED& activated);
static bool isListenMode(tNFA_ACTIVATED& activated);

static UINT16 sCurrentConfigLen;
#ifdef DTA // <DTA>
#define T1T_BLK_SIZE 8
#ifdef GETCONFIG_IMPLEMENT
static void getNciConfigurationValues();
#endif
static bool enablePassivePollMode = true;
static bool AnyPollModeSet = false;

static void myType4ListenLoopback(uint8_t* p_buf, uint32_t len);
UINT8 t1t_dyn_activated = FALSE;
extern UINT8 t1t_init_blocks[24];
extern UINT8 * t1t_tag_data;
UINT8 packet_count=1;
UINT8 data_packets_cnt = 0;
#endif // </DTA>
static UINT8 sConfig[256];
static UINT8 sLongGuardTime[] = { 0x00, 0x20 };
static UINT8 sDefaultGuardTime[] = { 0x00, 0x11 };
bool sIsDeviceReset = false;
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


/*******************************************************************************
**
** Function:        getNative
**
** Description:     Get native data
**
** Returns:         Native data structure.
**
*******************************************************************************/
nfc_jni_native_data *getNative (JNIEnv* e, jobject o)
{
    static struct nfc_jni_native_data *sCachedNat = NULL;
    if (e)
    {
        sCachedNat = nfc_jni_get_nat(e, o);
    }
    return sCachedNat;
}


/*******************************************************************************
**
** Function:        handleRfDiscoveryEvent
**
** Description:     Handle RF-discovery events from the stack.
**                  discoveredDevice: Discovered device.
**
** Returns:         None
**
*******************************************************************************/
static void handleRfDiscoveryEvent (tNFC_RESULT_DEVT* discoveredDevice)
{
    if (discoveredDevice->more)
    {
        //there is more discovery notification coming
        return;
    }

    bool isP2p = NfcTag::getInstance ().isP2pDiscovered ();
    if (!sReaderModeEnabled && isP2p)
    {
        //select the peer that supports P2P
        NfcTag::getInstance ().selectP2p();
    }
    else
    {
        //select the first of multiple tags that is discovered
        NfcTag::getInstance ().selectFirstTag();
    }
}

#ifdef DTA // <DTA>
/*******************************************************************************
**
** Function:        nfcManager_dta_set_pattern_number
**
** Description:     Set DTA pattern number.
**                  e: JVM environment.
**                  o: Java object.
**        pattern: DTA pattern number.
**
** Returns:         None.
**
*******************************************************************************/
static void nfcManager_dta_set_pattern_number(JNIEnv *e, jobject o, jint pattern)
{
        ALOGD ("[DTA] %s: enter", __FUNCTION__);
        ALOGD ("[DTA] pattern: %d", pattern);
        sDtaPatternNumber = (int)pattern;
        NFA_DTA_Set_Pattern_Number(sDtaPatternNumber);
}

/*******************************************************************************
**
** Function:        nfcManager_dta_get_pattern_number
**
** Description:     Get DTA pattern number.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Pattern number.
**
*******************************************************************************/
static jint nfcManager_dta_get_pattern_number(JNIEnv *e, jobject o)
{
        ALOGD ("[DTA] %s: enter", __FUNCTION__);
        jint pattern = (jint)NFA_DTA_Get_Pattern_Number();
        ALOGD ("[DTA] pattern: %d", pattern);
        return pattern;
}

static void dta_ndef_callback(tNFA_NDEF_EVT event, tNFA_NDEF_EVT_DATA *p_data) {
    // This function is intentionally empty.
}

#include "DtaHelper.h"
void sendtitdata(UINT8 i)
{
    UINT8 t1t_tag_reserved_data[24] = {0x55,0x55,0xAA,0xAA,0x12,0x49,0x06,0x00,0x01,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    ALOGD ("sendtitdata: i=%d",i);
    switch(i)
    {
        case 1:
        {
            ALOGD ("0th block: i=%d",i);
            NFA_RwT1tWrite8(0,t1t_init_blocks,1);
        }
        break;
        case 2:
        {
            ALOGD ("2nd block: i=%d",i);
            NFA_RwT1tWrite8(i,t1t_init_blocks+16,1);
        }
        break;
        default:
        {
            ALOGD ("now writing to block: i=%d",i);
            ALOGD ("t1t_tag_data=%X",t1t_tag_data);
            if((i==13) || (i==14) || (i==15) ||(i==0))
            {
                if(i == 13)
                {
                    NFA_RwT1tWrite8(i,t1t_tag_reserved_data,1);
                }
                else if(i==14)
                {
                    NFA_RwT1tWrite8(i,&t1t_tag_reserved_data[8],1);
                }
                else if(i == 15)
                {
                    NFA_RwT1tWrite8(i,&t1t_tag_reserved_data[16],1);
                }
                else
                {
                    //send CC byte E1 now
                    t1t_init_blocks[8] = 0xE1;
                    NFA_RwT1tWrite8(1,&t1t_init_blocks[8],1);
                }

            }
            else
            {
                NFA_RwT1tWrite8(i,t1t_tag_data,1);
                ALOGD ("i=%d: data_packets_cnt=%d",i,data_packets_cnt);
                if(i<(data_packets_cnt+5))
                {
                    t1t_tag_data = t1t_tag_data+8;
                    ALOGD ("t1t_tag_data=%X",t1t_tag_data);
                }
                else
                {
                    //ALOGD ("Setting packet_count=%X",packet_count);
                    packet_count = 0;
                    ALOGD ("Setting packet_count=%X",packet_count);
                }
            }
        }
    // FIXME: possible location for free
    // free(t1t_tag_data);
    // t1t_tag_data = NULL;
        break;
    }
}
#endif // </DTA>

/*******************************************************************************
**
** Function:        nfaConnectionCallback
**
** Description:     Receive connection-related events from stack.
**                  connEvent: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
static void nfaConnectionCallback (UINT8 connEvent, tNFA_CONN_EVT_DATA* eventData)
{
    #ifdef DTA // <DTA>
    UINT8 x = 0;
    #endif // </DTA>
    tNFA_STATUS status = NFA_STATUS_FAILED;
    ALOGD("%s: event= %u", __FUNCTION__, connEvent);

    switch (connEvent)
    {
    case NFA_POLL_ENABLED_EVT: // whether polling successfully started
        {
            ALOGD("%s: NFA_POLL_ENABLED_EVT: status = %u", __FUNCTION__, eventData->status);

            SyncEventGuard guard (sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne ();
        }
        break;

    case NFA_POLL_DISABLED_EVT: // Listening/Polling stopped
        {
            ALOGD("%s: NFA_POLL_DISABLED_EVT: status = %u", __FUNCTION__, eventData->status);

            SyncEventGuard guard (sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne ();
        }
        break;

    #ifdef DTA // <DTA>
    case NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT:
       {
             ALOGD("%s: [DTA] NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT: status = %u", __FUNCTION__, eventData->status);
       }
    #endif // </DTA>
    case NFA_RF_DISCOVERY_STARTED_EVT: // RF Discovery started
        {
            ALOGD("%s: NFA_RF_DISCOVERY_STARTED_EVT: status = %u", __FUNCTION__, eventData->status);

            SyncEventGuard guard (sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne ();
        }
        break;
    #ifdef DTA // <DTA>
    case NFA_EXCLUSIVE_RF_CONTROL_STOPPED_EVT:
        {
            ALOGD("%s: [DTA] NFA_EXCLUSIVE_RF_CONTROL_STOPPED_EVT: status = %u", __FUNCTION__, eventData->status);
        }
    #endif // <DTA>

    case NFA_RF_DISCOVERY_STOPPED_EVT: // RF Discovery stopped event
        {
            ALOGD("%s: NFA_RF_DISCOVERY_STOPPED_EVT: status = %u", __FUNCTION__, eventData->status);

            SyncEventGuard guard (sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne ();
        }
        break;

    case NFA_DISC_RESULT_EVT: // NFC link/protocol discovery notificaiton
        status = eventData->disc_result.status;
        ALOGD("%s: NFA_DISC_RESULT_EVT: status = %d", __FUNCTION__, status);
        if (status != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_DISC_RESULT_EVT error: status = %d", __FUNCTION__, status);
        }
        else
        {
            NfcTag::getInstance().connectionEventHandler(connEvent, eventData);
            handleRfDiscoveryEvent(&eventData->disc_result.discovery_ntf);
        }
        break;

    case NFA_SELECT_RESULT_EVT: // NFC link/protocol discovery select response
        ALOGD("%s: NFA_SELECT_RESULT_EVT: status = %d, gIsSelectingRfInterface = %d, sIsDisabling=%d", __FUNCTION__, eventData->status, gIsSelectingRfInterface, sIsDisabling);

        #ifdef DTA // <DTA>
        if (!in_dta_mode() ) {
        #endif
        if (sIsDisabling)
            break;
        #ifdef DTA // <DTA>
        } else {
            ALOGD("%s: in DTA mode", __FUNCTION__);
            if (sIsDisabling
                || NfcTag::getInstance().isNfcDep && !NfcTag::getInstance().isActive )
            break;

        }
        #endif // </DTA>

        if (eventData->status != NFA_STATUS_OK)
        {
            if (gIsSelectingRfInterface)
            {
                nativeNfcTag_doConnectStatus(false);
            }

            ALOGE("%s: NFA_SELECT_RESULT_EVT error: status = %d", __FUNCTION__, eventData->status);
            NFA_Deactivate (FALSE);
        }
        break;

    case NFA_DEACTIVATE_FAIL_EVT:
        ALOGD("%s: NFA_DEACTIVATE_FAIL_EVT: status = %d", __FUNCTION__, eventData->status);
        break;

    case NFA_ACTIVATED_EVT: // NFC link/protocol activated
        ALOGD("%s: NFA_ACTIVATED_EVT: gIsSelectingRfInterface=%d, sIsDisabling=%d", __FUNCTION__, gIsSelectingRfInterface, sIsDisabling);
        if(!in_dta_mode())
        {
          NfcTag::getInstance().setActive(true);
        }
        if (sIsDisabling || !sIsNfaEnabled)
            break;
        if(!in_dta_mode())
        {
          gActivated = true;
        }
        #ifdef DTA // <DTA>
        if(in_dta_mode())
        {
            ALOGD("checking dta mode: %x", eventData->activated.activate_ntf.rf_tech_param.mode);
            /* Check if we're in POLL or LISTEN mode following activation CB */
            if (
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_A) || // 00b
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_B) || // 01b
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_F) || // 10b
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_A_ACTIVE) || // 11b
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_F_ACTIVE) ||
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_ISO15693) ||
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_B_PRIME)  ||
                ( (eventData->activated.activate_ntf.rf_tech_param.mode) == NCI_DISCOVERY_TYPE_POLL_KOVIO)
               )
            {
                AnyPollModeSet = true;
                ALOGD("POLL MODE");
            }
            else
            {
                AnyPollModeSet = false;
                ALOGD("LISTEN MODE");
            }
            if(eventData->activated.params.t1t.hr[0] == 0x12)
            {
                ALOGD("%s: in DTA mode,Dynamic memory T1T activated", __FUNCTION__);
                t1t_dyn_activated = TRUE;
            }
        }
        #endif // </DTA>
        NfcTag::getInstance().setActivationState ();
        if (gIsSelectingRfInterface)
        {
            nativeNfcTag_doConnectStatus(true);
            break;
        }

        nativeNfcTag_resetPresenceCheck();
        if (isPeerToPeer(eventData->activated))
        {
            if (sReaderModeEnabled)
            {
                ALOGD("%s: ignoring peer target in reader mode.", __FUNCTION__);
                NFA_Deactivate (FALSE);
                break;
            }
            sP2pActive = true;
            ALOGD("%s: NFA_ACTIVATED_EVT; is p2p", __FUNCTION__);
            // Disable RF field events in case of p2p
            #ifdef DTA // <DTA>
            /*Disable RF field info evt reset in DTA mode*/
            if (!( in_dta_mode() )) {
            #endif // <DTA>
            UINT8  nfa_disable_rf_events[] = { 0x00 };
            ALOGD ("%s: Disabling RF field events", __FUNCTION__);
            status = NFA_SetConfig(NCI_PARAM_ID_RF_FIELD_INFO, sizeof(nfa_disable_rf_events),
                    &nfa_disable_rf_events[0]);
            if (status == NFA_STATUS_OK) {
                ALOGD ("%s: Disabled RF field events", __FUNCTION__);
            } else {
                ALOGE ("%s: Failed to disable RF field events", __FUNCTION__);
            }
        #ifdef DTA // <DTA>
        }
        #endif // </DTA>
            // For the SE, consider the field to be on while p2p is active.
            SecureElement::getInstance().notifyRfFieldEvent (true);
        #ifdef DTA // <DTA>
        ALOGD("eventData->activated.params.t1t.hr[0]=%X",eventData->activated.params.t1t.hr[0]);
        if (in_dta_mode() ) {
            ALOGD("%s: in DTA mode: eventData->activated.params.t1t.hr[0]=%X", __FUNCTION__, eventData->activated.params.t1t.hr[0]);
            NfcTag::getInstance().handleP2pConnection (NFA_ACTIVATED_EVT, eventData);
            NfcTag::getInstance().isP2pDiscovered();
            NfcTag::getInstance().selectP2p();
        }
        #endif // </DTA>
        }
        else if (pn544InteropIsBusy() == false)
        {
            NfcTag::getInstance().connectionEventHandler (connEvent, eventData);

            // We know it is not activating for P2P.  If it activated in
            // listen mode then it is likely for an SE transaction.
            // Send the RF Event.
            if (isListenMode(eventData->activated))
            {
                sSeRfActive = true;
                SecureElement::getInstance().notifyListenModeState (true);
            }
        }
        break;

    case NFA_DEACTIVATED_EVT: // NFC link/protocol deactivated
        ALOGD("%s: NFA_DEACTIVATED_EVT   Type: %u, gIsTagDeactivating: %d", __FUNCTION__, eventData->deactivated.type,gIsTagDeactivating);
        NfcTag::getInstance().setDeactivationState (eventData->deactivated);
        if (eventData->deactivated.type != NFA_DEACTIVATE_TYPE_SLEEP)
        {
            UINT8 sIsWaiting = FALSE;
            // Deactivation is done . Update waitstatus for nfcservice call to 0.
            NfcTag::getInstance().WaitStatus(&sIsWaiting);
            {
                SyncEventGuard g (gDeactivatedEvent);
                gActivated = false; //guard this variable from multi-threaded access
                gDeactivatedEvent.notifyOne ();
            }
            nativeNfcTag_resetPresenceCheck();
            NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
            nativeNfcTag_abortWaits();
            NfcTag::getInstance().abort ();
        }
        else if (gIsTagDeactivating)
        {
            NfcTag::getInstance().setActive(false);
            nativeNfcTag_doDeactivateStatus(0);
        }

        #ifdef DTA // <DTA> CR510341 the lines below were commented
        if (in_dta_mode() ) {
            ALOGD("%s: NFA_DEACTIVATED_EVT in DTA mode", __FUNCTION__);
            //NfcTag::getInstance().handleP2pConnection (NFA_ACTIVATED_EVT, eventData);
            //NfcTag::getInstance().selectP2p();

            //CR 579297 if nfcc is already sleeping leave it alone
            if ((eventData->deactivated.type != 0) && (AnyPollModeSet == true)) { // additional check to prevent WTX
               ALOGD("%s: send DEACT_CMD", __FUNCTION__);
               NFA_NFC_Deactivate(TRUE);
            }
            /* restart discovery */
            //NFA_StartRfDiscovery();
            //startRfDiscovery(true);
            //NFC_Deactivate(NFC_DEACTIVATE_TYPE_IDLE);

        }
        #endif // </DTA> CR510341
        // If RF is activated for what we think is a Secure Element transaction
        // and it is deactivated to either IDLE or DISCOVERY mode, notify w/event.
        if ((eventData->deactivated.type == NFA_DEACTIVATE_TYPE_IDLE)
                || (eventData->deactivated.type == NFA_DEACTIVATE_TYPE_DISCOVERY))
        {
            if (sSeRfActive) {
                sSeRfActive = false;
                if (!sIsDisabling && sIsNfaEnabled)
                    SecureElement::getInstance().notifyListenModeState (false);
            } else if (sP2pActive) {
                sP2pActive = false;
                // Make sure RF field events are re-enabled
                ALOGD("%s: NFA_DEACTIVATED_EVT; is p2p", __FUNCTION__);
                // Disable RF field events in case of p2p
                UINT8  nfa_enable_rf_events[] = { 0x01 };

                if (!sIsDisabling && sIsNfaEnabled)
                {
                #ifdef DTA // <DTA>
                /*Disbale RF field info evt set in DTA mode*/
                    if (!(in_dta_mode() )){
                #endif // </!DTA>
                    ALOGD ("%s: Enabling RF field events", __FUNCTION__);
                    status = NFA_SetConfig(NCI_PARAM_ID_RF_FIELD_INFO, sizeof(nfa_enable_rf_events),
                            &nfa_enable_rf_events[0]);
                    if (status == NFA_STATUS_OK) {
                        ALOGD ("%s: Enabled RF field events", __FUNCTION__);
                    } else {
                        ALOGE ("%s: Failed to enable RF field events", __FUNCTION__);
                #ifdef DTA // <DTA>
                        }
                #endif // </DTA>
                    }
                    // Consider the field to be off at this point
                    SecureElement::getInstance().notifyRfFieldEvent (false);
                }
            }
        }

        break;

    case NFA_TLV_DETECT_EVT: // TLV Detection complete
        status = eventData->tlv_detect.status;
        ALOGD("%s: NFA_TLV_DETECT_EVT: status = %d, protocol = %d, num_tlvs = %d, num_bytes = %d",
             __FUNCTION__, status, eventData->tlv_detect.protocol,
             eventData->tlv_detect.num_tlvs, eventData->tlv_detect.num_bytes);
        if (status != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_TLV_DETECT_EVT error: status = %d", __FUNCTION__, status);
        }
        break;

    case NFA_NDEF_DETECT_EVT: // NDEF Detection complete;
        //if status is failure, it means the tag does not contain any or valid NDEF data;
        //pass the failure status to the NFC Service;
        status = eventData->ndef_detect.status;
        #ifdef DTA // <DTA>
        if(in_dta_mode() )
        {
            if(t1t_dyn_activated == TRUE)
            {
                /*calculate the number of data packets to be written in T1T_BV4*/
                 if(eventData->ndef_detect.cur_size != 0)
                 {
                     data_packets_cnt = (eventData->ndef_detect.cur_size)/T1T_BLK_SIZE;
                     ALOGD("data_packets_cnt= %d", data_packets_cnt);
                     x = (eventData->ndef_detect.cur_size)%T1T_BLK_SIZE;
                     ALOGD("x= %d", x);
                     if(x > 0)
                     {
                         data_packets_cnt += 1;
                         ALOGD("Final data_packets_cnt= %d", data_packets_cnt);
                     }
                 }
            }
        }
        #endif // <!DTA>

        ALOGD("%s: NFA_NDEF_DETECT_EVT: status = 0x%X, protocol = %u, "
             "max_size = %lu, cur_size = %lu, flags = 0x%X", __FUNCTION__,
             status,
             eventData->ndef_detect.protocol, eventData->ndef_detect.max_size,
             eventData->ndef_detect.cur_size, eventData->ndef_detect.flags);
        NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
        nativeNfcTag_doCheckNdefResult(status,
            eventData->ndef_detect.max_size, eventData->ndef_detect.cur_size,
            eventData->ndef_detect.flags);
        break;

    case NFA_DATA_EVT: // Data message received (for non-NDEF reads)
        ALOGD("%s: NFA_DATA_EVT:  len = %d", __FUNCTION__, eventData->data.len);
        #ifdef DTA // <DTA>
        if (in_dta_mode() && (sSeRfActive == true)) {
          ALOGD("%s: in DTA mode", __FUNCTION__);
          // If we get transceive data in listen mode, always assume Type 4 Listen Mode DTA.
          myType4ListenLoopback(eventData->data.p_data, eventData->data.len);
          break;
        }


        if (in_dta_mode() && (dta::nfcdepListenLoopbackOn)) {
            ALOGD("%s: in DTA mode", __FUNCTION__);
            dta::nfcDepLoopBackInListenMode(eventData->data.p_data, eventData->data.len);
        }
        #endif // </DTA>
        nativeNfcTag_doTransceiveStatus(eventData->data.p_data,eventData->data.len);
        break;
    case NFA_RW_INTF_ERROR_EVT:

#ifdef DTA // <DTA>
        ALOGD("%s: NVA_RW_INTF_ERROR_EVT", __FUNCTION__);
        {
            ALOGD("%s: NFA_RW_INTF_ERROR_EVT: status = %u", __FUNCTION__, eventData->status);

            if (in_dta_mode()) {
                NFA_NFC_Deactivate(TRUE);
            }
            else
            {
                ALOGD("%s: NFC_RW_INTF_ERROR_EVT", __FUNCTION__);
                nativeNfcTag_notifyRfTimeout();
            }
        }
#else
                ALOGD("%s: NFC_RW_INTF_ERROR_EVT", __FUNCTION__);
                nativeNfcTag_notifyRfTimeout();
#endif // </DTA>
        break;
    case NFA_SELECT_CPLT_EVT: // Select completed
        status = eventData->status;
        ALOGD("%s: NFA_SELECT_CPLT_EVT: status = %d", __FUNCTION__, status);
        if (status != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_SELECT_CPLT_EVT error: status = %d", __FUNCTION__, status);
        }
        break;

    case NFA_READ_CPLT_EVT: // NDEF-read or tag-specific-read completed
        ALOGD("%s: NFA_READ_CPLT_EVT: status = 0x%X", __FUNCTION__, eventData->status);
        nativeNfcTag_doReadCompleted (eventData->status);
        NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
        break;

    case NFA_WRITE_CPLT_EVT: // Write completed
        ALOGD("%s: NFA_WRITE_CPLT_EVT: status = %d", __FUNCTION__, eventData->status);
        #ifdef DTA // <DTA>
        if (in_dta_mode())
        {
            if(t1t_dyn_activated == TRUE)
            {
               sendtitdata(packet_count);
               if(packet_count !=0)
               {
                   packet_count++;
               }
            }
            else
            {
        #endif // </DTA>
                nativeNfcTag_doWriteStatus (eventData->status == NFA_STATUS_OK);
        #ifdef DTA // <DTA>
            }
        }
        else
        {
        nativeNfcTag_doWriteStatus (eventData->status == NFA_STATUS_OK);
        }
        #endif // </DTA>
        break;

    case NFA_SET_TAG_RO_EVT: // Tag set as Read only
        ALOGD("%s: NFA_SET_TAG_RO_EVT: status = %d", __FUNCTION__, eventData->status);
        nativeNfcTag_doMakeReadonlyResult(eventData->status);
        break;

    case NFA_CE_NDEF_WRITE_START_EVT: // NDEF write started
        ALOGD("%s: NFA_CE_NDEF_WRITE_START_EVT: status: %d", __FUNCTION__, eventData->status);

        if (eventData->status != NFA_STATUS_OK)
            ALOGE("%s: NFA_CE_NDEF_WRITE_START_EVT error: status = %d", __FUNCTION__, eventData->status);
        break;

    case NFA_CE_NDEF_WRITE_CPLT_EVT: // NDEF write completed
        ALOGD("%s: FA_CE_NDEF_WRITE_CPLT_EVT: len = %lu", __FUNCTION__, eventData->ndef_write_cplt.len);
        break;

    case NFA_LLCP_ACTIVATED_EVT: // LLCP link is activated
        ALOGD("%s: NFA_LLCP_ACTIVATED_EVT: is_initiator: %d  remote_wks: %d, remote_lsc: %d, remote_link_miu: %d, local_link_miu: %d",
             __FUNCTION__,
             eventData->llcp_activated.is_initiator,
             eventData->llcp_activated.remote_wks,
             eventData->llcp_activated.remote_lsc,
             eventData->llcp_activated.remote_link_miu,
             eventData->llcp_activated.local_link_miu);

        PeerToPeer::getInstance().llcpActivatedHandler (getNative(0, 0), eventData->llcp_activated);
        break;

    case NFA_LLCP_DEACTIVATED_EVT: // LLCP link is deactivated
        ALOGD("%s: NFA_LLCP_DEACTIVATED_EVT", __FUNCTION__);
        PeerToPeer::getInstance().llcpDeactivatedHandler (getNative(0, 0), eventData->llcp_deactivated);
        break;
    case NFA_LLCP_FIRST_PACKET_RECEIVED_EVT: // Received first packet over llcp
        ALOGD("%s: NFA_LLCP_FIRST_PACKET_RECEIVED_EVT", __FUNCTION__);
        PeerToPeer::getInstance().llcpFirstPacketHandler (getNative(0, 0));
        break;
    case NFA_PRESENCE_CHECK_EVT:
        ALOGD("%s: NFA_PRESENCE_CHECK_EVT", __FUNCTION__);
        nativeNfcTag_doPresenceCheckResult (eventData->status);
        break;
    case NFA_FORMAT_CPLT_EVT:
        ALOGD("%s: NFA_FORMAT_CPLT_EVT: status=0x%X", __FUNCTION__, eventData->status);
        nativeNfcTag_formatStatus (eventData->status == NFA_STATUS_OK);
        break;

    case NFA_I93_CMD_CPLT_EVT:
        ALOGD("%s: NFA_I93_CMD_CPLT_EVT: status=0x%X", __FUNCTION__, eventData->status);
        break;

    case NFA_CE_UICC_LISTEN_CONFIGURED_EVT :
        ALOGD("%s: NFA_CE_UICC_LISTEN_CONFIGURED_EVT : status=0x%X", __FUNCTION__, eventData->status);
        SecureElement::getInstance().connectionEventHandler (connEvent, eventData);
        break;

    case NFA_SET_P2P_LISTEN_TECH_EVT:
        ALOGD("%s: NFA_SET_P2P_LISTEN_TECH_EVT", __FUNCTION__);
        PeerToPeer::getInstance().connectionEventHandler (connEvent, eventData);
        break;
    default:
        ALOGE("%s: unknown event ????", __FUNCTION__);
        break;
    }
}
#ifdef DTA // <DTA>
/* Requests to get NCI configuration parameter values, which will then be printed. */
#ifdef GETCONFIG_IMPLEMENT
static void getNciConfigurationValues()
{
        ALOGD ("[DTA] %s: enter", __FUNCTION__);

        UINT8 query_size = 20;
        tNFA_PMID requested_ids[query_size];

        ALOGD("%s: [DTA][CFG] Getting standard NCI configuration parameters", __FUNCTION__);
        // Get all standard NCI configuration parameters:
        requested_ids[0] = NFC_PMID_PA_BAILOUT;
        requested_ids[1] = NFC_PMID_PB_AFI;
        requested_ids[2] = NFC_PMID_PB_BAILOUT;
        requested_ids[3] = NFC_PMID_PB_ATTRIB_PARAM1;
        requested_ids[4] = NFC_PMID_PF_BIT_RATE;
        requested_ids[5] = NFC_PMID_PB_H_INFO;
        requested_ids[6] = NFC_PMID_BITR_NFC_DEP;
        requested_ids[7] = NFC_PMID_ATR_REQ_GEN_BYTES;
        requested_ids[8] = NFC_PMID_ATR_REQ_CONFIG;
        requested_ids[9] = NFC_PMID_LA_HIST_BY;
        requested_ids[10] = NFC_PMID_LA_NFCID1;
        requested_ids[11] = NFC_PMID_PI_BIT_RATE;
        requested_ids[12] = NFC_PMID_LA_BIT_FRAME_SDD;
        requested_ids[13] = NFC_PMID_LA_PLATFORM_CONFIG;
        requested_ids[14] = NFC_PMID_LA_SEL_INFO;
        requested_ids[15] = NFC_PMID_LI_BIT_RATE;
        requested_ids[16] = NFC_PMID_LB_SENSB_INFO;
        requested_ids[17] = NFC_PMID_LB_PROTOCOL;
        requested_ids[18] = NFC_PMID_LB_H_INFO;
        requested_ids[19] = NFC_PMID_LB_NFCID0;
        NFA_GetConfig(query_size, (tNFA_PMID*) &requested_ids);

        requested_ids[0] = NFC_PMID_TOTAL_DURATION;
        requested_ids[1] = NFC_PMID_CON_DEVICES_LIMIT;
        requested_ids[2] = NFC_PMID_LB_APPDATA;
        requested_ids[3] = NFC_PMID_LB_SFGI;
        requested_ids[4] = NFC_PMID_LB_ADC_FO;
        requested_ids[5] = NFC_PMID_LF_T3T_ID1;
        requested_ids[6] = NFC_PMID_LF_T3T_ID2;
        requested_ids[7] = NFC_PMID_LF_T3T_ID3;
        requested_ids[8] = NFC_PMID_LF_T3T_ID4;
        requested_ids[9] = NFC_PMID_LF_T3T_ID5;
        requested_ids[10] = NFC_PMID_LF_T3T_ID6;
        requested_ids[11] = NFC_PMID_LF_T3T_ID7;
        requested_ids[12] = NFC_PMID_LF_T3T_ID8;
        requested_ids[13] = NFC_PMID_LF_T3T_ID9;
        requested_ids[14] = NFC_PMID_LF_T3T_ID10;
        requested_ids[15] = NFC_PMID_LF_T3T_ID11;
        requested_ids[16] = NFC_PMID_LF_T3T_ID12;
        requested_ids[17] = NFC_PMID_LF_T3T_ID13;
        requested_ids[18] = NFC_PMID_LF_T3T_ID14;
        requested_ids[19] = NFC_PMID_LF_T3T_ID15;
        NFA_GetConfig(query_size, (tNFA_PMID*) &requested_ids);

        requested_ids[0] = NFC_PMID_LF_T3T_ID16;
        requested_ids[1] = NFC_PMID_LF_PROTOCOL;
        requested_ids[2] = NFC_PMID_LF_T3T_PMM;
        requested_ids[3] = NFC_PMID_LF_T3T_MAX;
        requested_ids[4] = NFC_PMID_LF_T3T_FLAGS2;
        requested_ids[5] = NFC_PMID_FWI;
        requested_ids[6] = NFC_PMID_LF_CON_BITR_F;
        requested_ids[7] = NFC_PMID_WT;
        requested_ids[8] = NFC_PMID_ATR_RES_GEN_BYTES;
        requested_ids[9] = NFC_PMID_ATR_RSP_CONFIG;
        requested_ids[10] = NFC_PMID_RF_FIELD_INFO;
        requested_ids[11] = NFC_PMID_NFC_DEP_OP;
        //requested_ids[12] = NFC_PARAM_ID_RF_EE_ACTION;  // These two have no actual NCI pmid
        //requested_ids[13] = NFC_PARAM_ID_ISO_DEP_OP;
        NFA_GetConfig(14, (tNFA_PMID*) &requested_ids);

        ALOGD("%s: [DTA][CFG] Getting Broadcom proprietary NCI configuration parameters", __FUNCTION__);
        // Get all Broadcom proprietary NCI parameters:
        requested_ids[0] = NCI_PARAM_ID_LA_FSDI;
        requested_ids[1] = NCI_PARAM_ID_LB_FSDI;
        requested_ids[2] = NCI_PARAM_ID_HOST_LISTEN_MASK;
        requested_ids[3] = NCI_PARAM_ID_CHIP_TYPE;
        requested_ids[4] = NCI_PARAM_ID_PA_ANTICOLL;
        requested_ids[5] = NCI_PARAM_ID_CONTINUE_MODE;
        requested_ids[6] = NCI_PARAM_ID_LBP;
        requested_ids[7] = NCI_PARAM_ID_T1T_RDR_ONLY;
        requested_ids[8] = NCI_PARAM_ID_LA_SENS_RES;
        requested_ids[9] = NCI_PARAM_ID_PWR_SETTING_BITMAP;
        requested_ids[10] = NCI_PARAM_ID_WI_NTF_ENABLE;
        requested_ids[11] = NCI_PARAM_ID_LN_BITRATE;
        requested_ids[12] = NCI_PARAM_ID_LF_BITRATE;
        requested_ids[13] = NCI_PARAM_ID_SWP_BITRATE_MASK;
        requested_ids[14] = NCI_PARAM_ID_KOVIO;
        requested_ids[15] = NCI_PARAM_ID_UICC_NTF_TO;
        requested_ids[16] = NCI_PARAM_ID_NFCDEP;
        requested_ids[17] = NCI_PARAM_ID_CLF_REGS_CFG;
        requested_ids[18] = NCI_PARAM_ID_NFCDEP_TRANS_TIME;
        requested_ids[19] = NCI_PARAM_ID_CREDIT_TIMER;
        NFA_GetConfig(query_size, (tNFA_PMID*) &requested_ids);

        requested_ids[0] = NCI_PARAM_ID_CORRUPT_RX;
        requested_ids[1] = NCI_PARAM_ID_ISODEP;
        requested_ids[2] = NCI_PARAM_ID_LF_CONFIG;
        requested_ids[3] = NCI_PARAM_ID_I93_DATARATE;
        requested_ids[4] = NCI_PARAM_ID_CREDITS_THRESHOLD;
        requested_ids[5] = NCI_PARAM_ID_TAGSNIFF_CFG;
        requested_ids[6] = NCI_PARAM_ID_PA_FSDI;
        requested_ids[7] = NCI_PARAM_ID_PB_FSDI;
        requested_ids[8] = NCI_PARAM_ID_FRAME_INTF_RETXN;
        requested_ids[9] = NCI_PARAM_ID_UICC_RDR_PRIORITY;
        requested_ids[10] = NCI_PARAM_ID_GUARD_TIME;
        requested_ids[11] = NCI_PARAM_ID_MAXTRY2ACTIVATE;
        requested_ids[12] = NCI_PARAM_ID_SWPCFG;
        requested_ids[13] = NCI_PARAM_ID_CLF_LPM_CFG;
        requested_ids[14] = NCI_PARAM_ID_DCLB;
        requested_ids[15] = NCI_PARAM_ID_ACT_ORDER;
        requested_ids[16] = NCI_PARAM_ID_DEP_DELAY_ACT;
        requested_ids[17] = NCI_PARAM_ID_DH_PARITY_CRC_CTL;
        requested_ids[18] = NCI_PARAM_ID_PREINIT_DSP_CFG;
        requested_ids[19] = NCI_PARAM_ID_FW_WORKAROUND;
        NFA_GetConfig(query_size, (tNFA_PMID*) &requested_ids);

        requested_ids[0] = NCI_PARAM_ID_RFU_CONFIG;
        requested_ids[1] = NCI_PARAM_ID_EMVCO_ENABLE;
        requested_ids[2] = NCI_PARAM_ID_ANTDRIVER_PARAM;
        requested_ids[3] = NCI_PARAM_ID_PLL325_CFG_PARAM;
        requested_ids[4] = NCI_PARAM_ID_OPNLP_ADPLL_ENABLE;
        requested_ids[5] = NCI_PARAM_ID_CONFORMANCE_MODE;
        requested_ids[6] = NCI_PARAM_ID_LPO_ON_OFF_ENABLE;
        requested_ids[7] = NCI_PARAM_ID_FORCE_VANT;
        requested_ids[8] = NCI_PARAM_ID_COEX_CONFIG;
        requested_ids[9] = NCI_PARAM_ID_INTEL_MODE;
        NFA_GetConfig(10, (tNFA_PMID*) &requested_ids);

        requested_ids[0] = NCI_PARAM_ID_AID; // NOTE: getting this fails for some reason
        NFA_GetConfig(1, (tNFA_PMID*) &requested_ids);

        //requested_ids[0] = NCI_PARAM_ID_STDCONFIG; /* dont not use this config item */
        //requested_ids[1] = NCI_PARAM_ID_PROPCFG; /* dont not use this config item */
}
#endif
#endif // </DTA>


/*******************************************************************************
**
** Function:        nfcManager_initNativeStruc
**
** Description:     Initialize variables.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_initNativeStruc (JNIEnv* e, jobject o)
{
    ALOGD ("%s: enter", __FUNCTION__);

    nfc_jni_native_data* nat = (nfc_jni_native_data*)malloc(sizeof(struct nfc_jni_native_data));
    if (nat == NULL)
    {
        ALOGE ("%s: fail allocate native data", __FUNCTION__);
        return JNI_FALSE;
    }

    // Set sIsDeviceReset to tru to indicate in further calls that it is device reset.
    sIsDeviceReset = true;
    memset (nat, 0, sizeof(*nat));
    e->GetJavaVM(&(nat->vm));
    nat->env_version = e->GetVersion();
    nat->manager = e->NewGlobalRef(o);

    ScopedLocalRef<jclass> cls(e, e->GetObjectClass(o));
    jfieldID f = e->GetFieldID(cls.get(), "mNative", "I");
    e->SetIntField(o, f, (jint)nat);

    /* Initialize native cached references */
    gCachedNfcManagerNotifyNdefMessageListeners = e->GetMethodID(cls.get(),
            "notifyNdefMessageListeners", "(Lcom/android/nfc/dhimpl/NativeNfcTag;)V");
    gCachedNfcManagerNotifyTransactionListeners = e->GetMethodID(cls.get(),
            "notifyTransactionListeners", "([B)V");
    gCachedNfcManagerNotifyLlcpLinkActivation = e->GetMethodID(cls.get(),
            "notifyLlcpLinkActivation", "(Lcom/android/nfc/dhimpl/NativeP2pDevice;)V");
    gCachedNfcManagerNotifyLlcpLinkDeactivated = e->GetMethodID(cls.get(),
            "notifyLlcpLinkDeactivated", "(Lcom/android/nfc/dhimpl/NativeP2pDevice;)V");
    gCachedNfcManagerNotifyLlcpFirstPacketReceived = e->GetMethodID(cls.get(),
            "notifyLlcpLinkFirstPacketReceived", "(Lcom/android/nfc/dhimpl/NativeP2pDevice;)V");
    sCachedNfcManagerNotifyTargetDeselected = e->GetMethodID(cls.get(),
            "notifyTargetDeselected","()V");
    gCachedNfcManagerNotifySeFieldActivated = e->GetMethodID(cls.get(),
            "notifySeFieldActivated", "()V");
    gCachedNfcManagerNotifySeFieldDeactivated = e->GetMethodID(cls.get(),
            "notifySeFieldDeactivated", "()V");
    gCachedNfcManagerNotifySeListenActivated = e->GetMethodID(cls.get(),
            "notifySeListenActivated", "()V");
    gCachedNfcManagerNotifySeListenDeactivated = e->GetMethodID(cls.get(),
            "notifySeListenDeactivated", "()V");

    gCachedNfcManagerNotifyHostEmuActivated = e->GetMethodID(cls.get(),
            "notifyHostEmuActivated", "()V");

    gCachedNfcManagerNotifyHostEmuData = e->GetMethodID(cls.get(),
            "notifyHostEmuData", "([B)V");

    gCachedNfcManagerNotifyHostEmuDeactivated = e->GetMethodID(cls.get(),
            "notifyHostEmuDeactivated", "()V");

    sCachedNfcManagerNotifySeApduReceived = e->GetMethodID(cls.get(),
            "notifySeApduReceived", "([B)V");

    sCachedNfcManagerNotifySeMifareAccess = e->GetMethodID(cls.get(),
            "notifySeMifareAccess", "([B)V");

    sCachedNfcManagerNotifySeEmvCardRemoval =  e->GetMethodID(cls.get(),
            "notifySeEmvCardRemoval", "()V");

    if (nfc_jni_cache_object(e, gNativeNfcTagClassName, &(nat->cached_NfcTag)) == -1)
    {
        ALOGE ("%s: fail cache NativeNfcTag", __FUNCTION__);
        return JNI_FALSE;
    }

    if (nfc_jni_cache_object(e, gNativeP2pDeviceClassName, &(nat->cached_P2pDevice)) == -1)
    {
        ALOGE ("%s: fail cache NativeP2pDevice", __FUNCTION__);
        return JNI_FALSE;
    }

    ALOGD ("%s: exit", __FUNCTION__);
    return JNI_TRUE;
}

#ifdef DTA // <DTA>
void print_nci_get_cfg_tlvs(UINT16 tlv_list_len, UINT8 *p_tlv_list) {

    UINT8 type, len, *p_value;
    UINT8 xx = 0;
    UINT8 num_param_fields = *p_tlv_list;
    p_tlv_list += 1;

    ALOGD ("[DTA][CFG] number of parameter fields: %d", num_param_fields);

    UINT8 count = 0;
    while (tlv_list_len - xx >= 2 && count < num_param_fields )
    {
        type    = *(p_tlv_list + xx);
        len     = *(p_tlv_list + xx + 1);
        p_value = p_tlv_list + xx + 2;

        //ALOGD  ("[DTA] type = 0x%02x", type);
        ALOGD("[DTA][CFG] %s =", nci_pmid_to_string(type));
        int y;
        for (y = 0; y < len; y++) {
            ALOGD ("[DTA][CFG] 0x%02x", (UINT8) *(p_value+y));
        }
        if (len == 0){
        ALOGD ("[DTA][CFG] <no value> ");
        }
            xx += len + 2;  /* move to next TLV */
            count++;
        }

}
#endif // </DTA>

/*******************************************************************************
**
** Function:        nfaDeviceManagementCallback
**
** Description:     Receive device management events from stack.
**                  dmEvent: Device-management event ID.
**                  eventData: Data associated with event ID.
**
** Returns:         None
**
*******************************************************************************/
void nfaDeviceManagementCallback (UINT8 dmEvent, tNFA_DM_CBACK_DATA* eventData)
{
    ALOGD ("%s: enter; event=0x%X", __FUNCTION__, dmEvent);

    switch (dmEvent)
    {
    case NFA_DM_ENABLE_EVT: /* Result of NFA_Enable */
        {
            SyncEventGuard guard (sNfaEnableEvent);
            ALOGD ("%s: NFA_DM_ENABLE_EVT; status=0x%X",
                    __FUNCTION__, eventData->status);
            sIsNfaEnabled = eventData->status == NFA_STATUS_OK;
            sIsDisabling = false;
            sNfaEnableEvent.notifyOne ();
        }
        break;

    case NFA_DM_DISABLE_EVT: /* Result of NFA_Disable */
        {
            SyncEventGuard guard (sNfaDisableEvent);
            ALOGD ("%s: NFA_DM_DISABLE_EVT", __FUNCTION__);
            sIsNfaEnabled = false;
            sIsDisabling = false;
            sNfaDisableEvent.notifyOne ();
        }
        break;

    case NFA_DM_SET_CONFIG_EVT: //result of NFA_SetConfig
        ALOGD ("%s: NFA_DM_SET_CONFIG_EVT", __FUNCTION__);
        {
            #ifdef DTA // <DTA>
            ALOGD ("%s: [DTA][CFG] NFA_DM_SET_CONFIG_EVT: set config status=%d: number of params:%d",
            __FUNCTION__, eventData->set_config.status, eventData->set_config.num_param_id);
            #endif // </DTA>
            SyncEventGuard guard (sNfaSetConfigEvent);
            sNfaSetConfigEvent.notifyOne();
        }
        break;

    case NFA_DM_GET_CONFIG_EVT: /* Result of NFA_GetConfig */
        ALOGD ("%s: NFA_DM_GET_CONFIG_EVT", __FUNCTION__);
        {
            #ifdef DTA // <DTA>
            tNFA_GET_CONFIG *p_get_config = (tNFA_GET_CONFIG*) eventData;
            ALOGD ("%s: [DTA][CFG] NFA_DM_GET_CONFIG_EVT: get config status=%d:", __FUNCTION__, p_get_config->status);

            if (p_get_config->status == NFA_STATUS_OK) {
                 print_nci_get_cfg_tlvs(p_get_config->tlv_size, p_get_config->param_tlvs);
            }
            #endif // </DTA>
            SyncEventGuard guard (sNfaGetConfigEvent);
            if (eventData->status == NFA_STATUS_OK &&
                    eventData->get_config.tlv_size <= sizeof(sConfig))
            {
                sCurrentConfigLen = eventData->get_config.tlv_size;
                memcpy(sConfig, eventData->get_config.param_tlvs, eventData->get_config.tlv_size);
            }
            else
            {
                ALOGE("%s: NFA_DM_GET_CONFIG failed", __FUNCTION__);
                sCurrentConfigLen = 0;
            }
            sNfaGetConfigEvent.notifyOne();
        }
        break;

    case NFA_DM_RF_FIELD_EVT:
        ALOGD ("%s: NFA_DM_RF_FIELD_EVT; status=0x%X; field status=%u", __FUNCTION__,
              eventData->rf_field.status, eventData->rf_field.rf_field_status);
        if (sIsDisabling || !sIsNfaEnabled)
            break;

        if (!sP2pActive && eventData->rf_field.status == NFA_STATUS_OK)
            SecureElement::getInstance().notifyRfFieldEvent (
                    eventData->rf_field.rf_field_status == NFA_DM_RF_FIELD_ON);
        break;

    case NFA_DM_NFCC_TRANSPORT_ERR_EVT:
    case NFA_DM_NFCC_TIMEOUT_EVT:
        {
            if (dmEvent == NFA_DM_NFCC_TIMEOUT_EVT)
                ALOGE ("%s: NFA_DM_NFCC_TIMEOUT_EVT; abort", __FUNCTION__);
            else if (dmEvent == NFA_DM_NFCC_TRANSPORT_ERR_EVT)
                ALOGE ("%s: NFA_DM_NFCC_TRANSPORT_ERR_EVT; abort", __FUNCTION__);

            nativeNfcTag_abortWaits();
            NfcTag::getInstance().abort ();
            sAbortConnlessWait = true;
            nativeLlcpConnectionlessSocket_abortWait();
            {
                ALOGD ("%s: aborting  sNfaEnableDisablePollingEvent", __FUNCTION__);
                SyncEventGuard guard (sNfaEnableDisablePollingEvent);
                sNfaEnableDisablePollingEvent.notifyOne();
            }
            {
                ALOGD ("%s: aborting  sNfaEnableEvent", __FUNCTION__);
                SyncEventGuard guard (sNfaEnableEvent);
                sNfaEnableEvent.notifyOne();
            }
            {
                ALOGD ("%s: aborting  sNfaDisableEvent", __FUNCTION__);
                SyncEventGuard guard (sNfaDisableEvent);
                sNfaDisableEvent.notifyOne();
            }
            sDiscoveryEnabled = false;
            PowerSwitch::getInstance ().abort ();

            if (!sIsDisabling && sIsNfaEnabled)
            {
                NFA_Disable(FALSE);
                sIsDisabling = true;
            }
            else
            {
                sIsNfaEnabled = false;
                sIsDisabling = false;
            }
            PowerSwitch::getInstance ().initialize (PowerSwitch::UNKNOWN_LEVEL);
            ALOGE ("%s: crash NFC service", __FUNCTION__);
            //////////////////////////////////////////////
            //crash the NFC service process so it can restart automatically
            //don't want to repeatedly crash the service if the hardware isn't there.
            //abort ();
            //////////////////////////////////////////////
        }
        break;

    case NFA_DM_PWR_MODE_CHANGE_EVT:
        PowerSwitch::getInstance ().deviceManagementCallback (dmEvent, eventData);
        break;

    default:
        ALOGD ("%s: unhandled event", __FUNCTION__);
        break;
    }
}

/*******************************************************************************
**
** Function:        nfcManager_sendRawFrame
**
** Description:     Send a raw frame.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_sendRawFrame (JNIEnv* e, jobject, jbyteArray data)
{
    ScopedByteArrayRO bytes(e, data);
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
    size_t bufLen = bytes.size();
    tNFA_STATUS status = NFA_SendRawFrame (buf, bufLen, 0);

    return (status == NFA_STATUS_OK);
}

/*******************************************************************************
**
** Function:        nfcManager_routeAid
**
** Description:     Route an AID to an EE
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_routeAid (JNIEnv* e, jobject, jbyteArray aid, jint route)
{
    ScopedByteArrayRO bytes(e, aid);
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
    size_t bufLen = bytes.size();
    bool result = RoutingManager::getInstance().addAidRouting(buf, bufLen, route);
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_unrouteAid
**
** Description:     Remove a AID routing
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_unrouteAid (JNIEnv* e, jobject, jbyteArray aid)
{
    ScopedByteArrayRO bytes(e, aid);
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
    size_t bufLen = bytes.size();
    bool result = RoutingManager::getInstance().removeAidRouting(buf, bufLen);
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_doInitialize
**
** Description:     Turn on NFC.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_doInitialize (JNIEnv* e, jobject o)
{
    ALOGD ("%s: enter; ver=%s nfa=%s NCI_VERSION=0x%02X",
        __FUNCTION__, nfca_version_string, nfa_version_string, NCI_VERSION);
    tNFA_STATUS stat = NFA_STATUS_OK;

    if (sIsNfaEnabled)
    {
        ALOGD ("%s: already enabled", __FUNCTION__);
        goto TheEnd;
    }

    PowerSwitch::getInstance ().initialize (PowerSwitch::FULL_POWER);

    {
        unsigned long num = 0;

        NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
        theInstance.Initialize(); //start GKI, NCI task, NFC task

        {
            SyncEventGuard guard (sNfaEnableEvent);
            tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs ();

            NFA_Init (halFuncEntries);

            NFA_CheckDeviceResetStatus(sIsDeviceReset);
            sIsDeviceReset = false;
            stat = NFA_Enable (nfaDeviceManagementCallback, nfaConnectionCallback);
            if (stat == NFA_STATUS_OK)
            {
                num = initializeGlobalAppLogLevel ();
                CE_SetTraceLevel (num);
                LLCP_SetTraceLevel (num);
                NFC_SetTraceLevel (num);
                RW_SetTraceLevel (num);
                NFA_SetTraceLevel (num);
                NFA_P2pSetTraceLevel (num);
                sNfaEnableEvent.wait(); //wait for NFA command to finish
            }
        }

        if (stat == NFA_STATUS_OK)
        {
            //sIsNfaEnabled indicates whether stack started successfully
            if (sIsNfaEnabled)
            {
                SecureElement::getInstance().initialize (getNative(e, o));
                RoutingManager::getInstance().initialize(getNative(e, o));
                nativeNfcTag_registerNdefTypeHandler ();
                NfcTag::getInstance().initialize (getNative(e, o));
                PeerToPeer::getInstance().initialize ();
                PeerToPeer::getInstance().handleNfcOnOff (true);

                /////////////////////////////////////////////////////////////////////////////////
                // Add extra configuration here (work-arounds, etc.)

                struct nfc_jni_native_data *nat = getNative(e, o);

                if ( nat )
                {
                    if (GetNumValue(NAME_POLLING_TECH_MASK, &num, sizeof(num)))
                        nat->tech_mask = num;
                    else
                        nat->tech_mask = DEFAULT_TECH_MASK;
                    ALOGD ("%s: tag polling tech mask=0x%X", __FUNCTION__, nat->tech_mask);
                }

                // if this value exists, set polling interval.
                if (GetNumValue(NAME_NFA_DM_DISC_DURATION_POLL, &num, sizeof(num)))
                    nat->discovery_duration = num;
                else
                    nat->discovery_duration = DEFAULT_DISCOVERY_DURATION;

                NFA_SetRfDiscoveryDuration(nat->discovery_duration);

                // Do custom NFCA startup configuration.
                doStartupConfig();
                goto TheEnd;
            }
        }

        ALOGE ("%s: fail nfa enable; error=0x%X", __FUNCTION__, stat);

        if (sIsNfaEnabled)
            stat = NFA_Disable (FALSE /* ungraceful */);

        theInstance.Finalize();
    }

TheEnd:
    if (sIsNfaEnabled)
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);
    ALOGD ("%s: exit", __FUNCTION__);
    return sIsNfaEnabled ? JNI_TRUE : JNI_FALSE;
}


/*******************************************************************************
**
** Function:        nfcManager_enableDiscovery
**
** Description:     Start polling and listening for devices.
**                  e: JVM environment.
**                  o: Java object.
**                  mode: Not used.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_enableDiscovery (JNIEnv* e, jobject o)
{
    tNFA_TECHNOLOGY_MASK tech_mask = DEFAULT_TECH_MASK;
    struct nfc_jni_native_data *nat = getNative(e, o);

    if (nat)
        tech_mask = (tNFA_TECHNOLOGY_MASK)nat->tech_mask;

    ALOGD ("%s: enter; tech_mask = %02x", __FUNCTION__, tech_mask);

    if (sDiscoveryEnabled)
    {
        ALOGE ("%s: already polling", __FUNCTION__);
        return;
    }

    tNFA_STATUS stat = NFA_STATUS_OK;

    ALOGD ("%s: sIsSecElemSelected=%u", __FUNCTION__, sIsSecElemSelected);

    PowerSwitch::getInstance ().setLevel (PowerSwitch::FULL_POWER);

    if (sRfEnabled) {
        // Stop RF discovery to reconfigure
        startRfDiscovery(false);
    }

    {
        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        stat = NFA_EnablePolling (tech_mask);
        if (stat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for enable event", __FUNCTION__);
            sDiscoveryEnabled = true;
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_ENABLED_EVT
            ALOGD ("%s: got enabled event", __FUNCTION__);
        }
        else
        {
            ALOGE ("%s: fail enable discovery; error=0x%X", __FUNCTION__, stat);
        }
    }

    // Start P2P listening if tag polling was enabled or the mask was 0.
    if (sDiscoveryEnabled || (tech_mask == 0))
    {
        ALOGD ("%s: Enable p2pListening", __FUNCTION__);
        PeerToPeer::getInstance().enableP2pListening (true);
    }

    // Actually start discovery.
    startRfDiscovery (true);

    PowerSwitch::getInstance ().setModeOn (PowerSwitch::DISCOVERY);

    ALOGD ("%s: exit", __FUNCTION__);
}


/*******************************************************************************
**
** Function:        nfcManager_disableDiscovery
**
** Description:     Stop polling and listening for devices.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
void nfcManager_disableDiscovery (JNIEnv*, jobject)
{
    UINT32 nfc_screen_off_polling_on = 0;
    tNFA_STATUS status = NFA_STATUS_OK;
    UINT8            cmd_params_len = 0;
    UINT8            *p_cmd_params = NULL;
    UINT8            oid = 0x3;
    ALOGD ("%s: enter;", __FUNCTION__);
    GetNumValue("NFC_SCREEN_OFF_POLL_ON", &nfc_screen_off_polling_on, sizeof(nfc_screen_off_polling_on));
    if(nfc_screen_off_polling_on == 0x01)
    {
        goto TheEnd;
    }
    pn544InteropAbortNow ();
    if (sDiscoveryEnabled == false)
    {
#ifdef DTA // <DTA>
        if(!(in_dta_mode() ) ) {
#endif
        ALOGD ("%s: already disabled", __FUNCTION__);
        NFA_SendVsCommand(oid, cmd_params_len, p_cmd_params, NULL);
        goto TheEnd;
#ifdef DTA // <DTA>
       }
#endif // </DTA>
    }

    // Stop RF Discovery.
    startRfDiscovery (false);

    if (sDiscoveryEnabled)
    {
        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        status = NFA_DisablePolling ();
        if (status == NFA_STATUS_OK)
        {
            sDiscoveryEnabled = false;
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_DISABLED_EVT
        }
        else
            ALOGE ("%s: Failed to disable polling; error=0x%X", __FUNCTION__, status);
    }

    PeerToPeer::getInstance().enableP2pListening (false);

    if(!sIsSecElemSelected)
    {
        //if nothing is active after this, then tell the controller to power down
        if (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::DISCOVERY))
            PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);
    }
    else
    {
        //continue discovery if card emulation is enabled.
        startRfDiscovery (true);
    }

    // We may have had RF field notifications that did not cause
    // any activate/deactive events. For example, caused by wireless
    // charging orbs. Those may cause us to go to sleep while the last
    // field event was indicating a field. To prevent sticking in that
    // state, always reset the rf field status when we disable discovery.
    SecureElement::getInstance().resetRfFieldStatus();
TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
}

void enableDisableLongGuardTime (bool enable)
{
    // TODO
    // This is basically a work-around for an issue
    // in BCM20791B5: if a reader is configured as follows
    // 1) Only polls for NFC-A
    // 2) Cuts field between polls
    // 3) Has a short guard time (~5ms)
    // the BCM20791B5 doesn't wake up when such a reader
    // is polling it. Unfortunately the default reader
    // mode configuration on Android matches those
    // criteria. To avoid the issue, increase the guard
    // time when in reader mode.
    //
    // Proper fix is firmware patch for B5 controllers.
    SyncEventGuard guard(sNfaSetConfigEvent);
    tNFA_STATUS stat = NFA_SetConfig(NCI_PARAM_ID_T1T_RDR_ONLY, 2,
            enable ? sLongGuardTime : sDefaultGuardTime);
    if (stat == NFA_STATUS_OK)
        sNfaSetConfigEvent.wait ();
    else
        ALOGE("%s: Could not configure longer guard time", __FUNCTION__);
    return;
}

void enableDisableLptd (bool enable)
{
    // This method is *NOT* thread-safe. Right now
    // it is only called from the same thread so it's
    // not an issue.
    static bool sCheckedLptd = false;
    static bool sHasLptd = false;

    tNFA_STATUS stat = NFA_STATUS_OK;
    if (!sCheckedLptd)
    {
        sCheckedLptd = true;
        SyncEventGuard guard (sNfaGetConfigEvent);
        tNFA_PMID configParam[1] = {NCI_PARAM_ID_TAGSNIFF_CFG};
        stat = NFA_GetConfig(1, configParam);
        if (stat != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_GetConfig failed", __FUNCTION__);
            return;
        }
        sNfaGetConfigEvent.wait ();
        if (sCurrentConfigLen < 4 || sConfig[1] != NCI_PARAM_ID_TAGSNIFF_CFG) {
            ALOGE("%s: Config TLV length %d returned is too short", __FUNCTION__,
                    sCurrentConfigLen);
            return;
        }
        sHasLptd = true;
    }
    // Bail if we checked and didn't find any LPTD config before
    if (!sHasLptd) return;
    UINT8 enable_byte = enable ? 0x01 : 0x00;

    SyncEventGuard guard(sNfaSetConfigEvent);

    stat = NFA_SetConfig(NCI_PARAM_ID_TAGSNIFF_CFG, 1, &enable_byte);
    if (stat == NFA_STATUS_OK)
        sNfaSetConfigEvent.wait ();
    else
        ALOGE("%s: Could not configure LPTD feature", __FUNCTION__);
    return;
}

#ifndef CONFIG_UICC_IDLE_TIMEOUT_SUPPORTED
void sendVscCallback (UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    UINT8 oid = (event & 0x3F);
    SyncEventGuard guard (sNfaGetConfigEvent);

    if ((event & 0xC0) == NCI_RSP_BIT)
    {
        if ((oid == 0x01)||(oid == 0x05))
        {
            if ((param_len >= 4)&&(*(p_param + 3) == NFA_STATUS_OK))
            {
                ALOGD("%s: event = 0x%x success", __FUNCTION__, event);
            }
            else
            {
                ALOGE("%s: event = 0x%x failed", __FUNCTION__, event);
            }
            sNfaGetConfigEvent.notifyOne();
        }
        else if (oid == 0x04)
        {
            if ((param_len >= 4)&&(*(p_param + 3) == NFA_STATUS_OK))
            {
                ALOGD("%s: event = 0x%x success", __FUNCTION__, event);
            }
            else
            {
                ALOGE("%s: event = 0x%x failed", __FUNCTION__, event);
                sNfaGetConfigEvent.notifyOne(); // don't wait for NTF
            }
        }
    }
    else if ((event & 0xC0) == NCI_NTF_BIT)
    {
        if (oid == 0x04)
        {
            if ((param_len >= 4)&&(*(p_param + 3) == NFA_STATUS_OK))
            {
                ALOGD("%s: event = 0x%x success", __FUNCTION__, event);
            }
            else
            {
                ALOGE("%s: event = 0x%x failed", __FUNCTION__, event);
            }
            sNfaGetConfigEvent.notifyOne();
        }
    }
}
#endif

void setUiccIdleTimeout (bool enable)
{
    // This method is *NOT* thread-safe. Right now
    // it is only called from the same thread so it's
    // not an issue.
    tNFA_STATUS stat = NFA_STATUS_OK;

#ifndef CONFIG_UICC_IDLE_TIMEOUT_SUPPORTED
    UINT8 cmd_params[5];

    ALOGD("%s: enable = %d", __FUNCTION__, enable);

    cmd_params[0] = 0x0A; // Command
    cmd_params[1] = 0x01; // number of TLV
    if (enable)
        cmd_params[2] = 0x01; // enable
    else
        cmd_params[2] = 0x00; // disable

    cmd_params[3] = 0x01;     // length
    cmd_params[4] = (SecureElement::getInstance().mActiveEeHandle & 0x00FF);     // NFCEE ID

    {
        SyncEventGuard guard (sNfaGetConfigEvent);
        stat = NFA_SendVsCommand (0x01, // oid
                                  0x05, // cmd_params_len,
                                  cmd_params,
                                  sendVscCallback);

        if (stat != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_SendVsCommand failed", __FUNCTION__);
            return;
        }
        sNfaGetConfigEvent.wait ();
    }

    if (!enable)
    {
        SyncEventGuard guard (sNfaGetConfigEvent);

        cmd_params[0] = (SecureElement::getInstance().mActiveEeHandle & 0x00FF);     // NFCEE ID

        stat = NFA_SendVsCommand (0x04, // oid
                                  0x01, // cmd_params_len,
                                  cmd_params,
                                  sendVscCallback);

        if (stat == NFA_STATUS_OK)
        {
            stat = NFA_RegVSCback (true, sendVscCallback);
            if (stat != NFA_STATUS_OK)
            {
                ALOGE("%s: NFA_RegVSCback failed", __FUNCTION__);
                return;
            }
        }
        else
        {
            ALOGE("%s: NFA_SendVsCommand failed", __FUNCTION__);
            return;
        }
        sNfaGetConfigEvent.wait (1000); // continue even if no NTF
        NFA_RegVSCback (false, sendVscCallback);

        ALOGD("%s: Start delay", __FUNCTION__);
        sNfaGetConfigEvent.wait (40); // let add delay
    }
    return;
#else

    UINT8 swp_cfg_byte0 = 0x00;
    {
        SyncEventGuard guard (sNfaGetConfigEvent);
        tNFA_PMID configParam[1] = {0xC2};
        stat = NFA_GetConfig(1, configParam);
        if (stat != NFA_STATUS_OK)
        {
            ALOGE("%s: NFA_GetConfig failed", __FUNCTION__);
            return;
        }
        sNfaGetConfigEvent.wait ();
        if (sCurrentConfigLen < 4 || sConfig[1] != 0xC2) {
            ALOGE("%s: Config TLV length %d returned is too short", __FUNCTION__,
                    sCurrentConfigLen);
            return;
        }
        swp_cfg_byte0 = sConfig[3];
    }
    SyncEventGuard guard(sNfaSetConfigEvent);
    if (enable)
        swp_cfg_byte0 |= 0x01;
    else
        swp_cfg_byte0 &= ~0x01;

    stat = NFA_SetConfig(0xC2, 1, &swp_cfg_byte0);
    if (stat == NFA_STATUS_OK)
        sNfaSetConfigEvent.wait ();
    else
        ALOGE("%s: Could not configure UICC idle timeout feature", __FUNCTION__);
    return;
#endif
}
/*******************************************************************************
**
** Function         nfc_jni_cache_object_local
**
** Description      Allocates a java object and calls it's constructor
**
** Returns          -1 on failure, 0 on success
**
*******************************************************************************/
static int nfc_jni_cache_object_local (JNIEnv *e, const char *className, jobject *cachedObj)
{
    ScopedLocalRef<jclass> cls(e, e->FindClass(className));
    if(cls.get() == NULL) {
        ALOGE ("%s: find class error", __FUNCTION__);
        return -1;
    }

    jmethodID ctor = e->GetMethodID(cls.get(), "<init>", "()V");
    jobject obj = e->NewObject(cls.get(), ctor);
    if (obj == NULL) {
       ALOGE ("%s: create object error", __FUNCTION__);
       return -1;
    }

    *cachedObj = obj;
    if (*cachedObj == NULL) {
        ALOGE ("%s: global ref error", __FUNCTION__);
        return -1;
    }
    return 0;
}


/*******************************************************************************
**
** Function:        nfcManager_doCreateLlcpServiceSocket
**
** Description:     Create a new LLCP server socket.
**                  e: JVM environment.
**                  o: Java object.
**                  nSap: Service access point.
**                  sn: Service name
**                  miu: Maximum information unit.
**                  rw: Receive window size.
**                  linearBufferLength: Max buffer size.
**
** Returns:         NativeLlcpServiceSocket Java object.
**
*******************************************************************************/
static jobject nfcManager_doCreateLlcpServiceSocket (JNIEnv* e, jobject, jint nSap, jstring sn, jint miu, jint rw, jint linearBufferLength)
{
    PeerToPeer::tJNI_HANDLE jniHandle = PeerToPeer::getInstance().getNewJniHandle ();

    ScopedUtfChars serviceName(e, sn);

    ALOGD ("%s: enter: sap=%i; name=%s; miu=%i; rw=%i; buffLen=%i", __FUNCTION__, nSap, serviceName.c_str(), miu, rw, linearBufferLength);

    /* Create new NativeLlcpServiceSocket object */
    jobject serviceSocket = NULL;
    if (nfc_jni_cache_object(e, gNativeLlcpServiceSocketClassName, &(serviceSocket)) == -1)
    {
        ALOGE ("%s: Llcp socket object creation error", __FUNCTION__);
        return NULL;
    }

    /* Get NativeLlcpServiceSocket class object */
    ScopedLocalRef<jclass> clsNativeLlcpServiceSocket(e, e->GetObjectClass(serviceSocket));
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE("%s: Llcp Socket get object class error", __FUNCTION__);
        return NULL;
    }

    if (!PeerToPeer::getInstance().registerServer (jniHandle, serviceName.c_str()))
    {
        ALOGE("%s: RegisterServer error", __FUNCTION__);
        return NULL;
    }

    jfieldID f;

    /* Set socket handle to be the same as the NfaHandle*/
    f = e->GetFieldID(clsNativeLlcpServiceSocket.get(), "mHandle", "I");
    e->SetIntField(serviceSocket, f, (jint) jniHandle);
    ALOGD ("%s: socket Handle = 0x%X", __FUNCTION__, jniHandle);

    /* Set socket linear buffer length */
    f = e->GetFieldID(clsNativeLlcpServiceSocket.get(), "mLocalLinearBufferLength", "I");
    e->SetIntField(serviceSocket, f,(jint)linearBufferLength);
    ALOGD ("%s: buffer length = %d", __FUNCTION__, linearBufferLength);

    /* Set socket MIU */
    f = e->GetFieldID(clsNativeLlcpServiceSocket.get(), "mLocalMiu", "I");
    e->SetIntField(serviceSocket, f,(jint)miu);
    ALOGD ("%s: MIU = %d", __FUNCTION__, miu);

    /* Set socket RW */
    f = e->GetFieldID(clsNativeLlcpServiceSocket.get(), "mLocalRw", "I");
    e->SetIntField(serviceSocket, f,(jint)rw);
    ALOGD ("%s:  RW = %d", __FUNCTION__, rw);

    sLastError = 0;
    ALOGD ("%s: exit", __FUNCTION__);
    return serviceSocket;
}


/*******************************************************************************
**
** Function:        nfcManager_doGetLastError
**
** Description:     Get the last error code.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Last error code.
**
*******************************************************************************/
static jint nfcManager_doGetLastError(JNIEnv*, jobject)
{
    ALOGD ("%s: last error=%i", __FUNCTION__, sLastError);
    return sLastError;
}


/*******************************************************************************
**
** Function:        nfcManager_doDeinitialize
**
** Description:     Turn off NFC.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_doDeinitialize (JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);

    sIsDisabling = true;
    pn544InteropAbortNow ();
    SecureElement::getInstance().finalize ();
    PowerSwitch::getInstance ().initialize (PowerSwitch::UNKNOWN_LEVEL);

    if (sIsNfaEnabled)
    {
        SyncEventGuard guard (sNfaDisableEvent);
        tNFA_STATUS stat = NFA_Disable (TRUE /* graceful */);
        if (stat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for completion", __FUNCTION__);
            sNfaDisableEvent.wait (); //wait for NFA command to finish
            PeerToPeer::getInstance ().handleNfcOnOff (false);
        }
        else
        {
            ALOGE ("%s: fail disable; error=0x%X", __FUNCTION__, stat);
        }
    }
    nativeNfcTag_abortWaits();
    NfcTag::getInstance().abort ();
    sAbortConnlessWait = true;
    nativeLlcpConnectionlessSocket_abortWait();
    sIsNfaEnabled = false;
    sDiscoveryEnabled = false;
    sIsDisabling = false;
    sIsSecElemSelected = false;
    gActivated = false;

    {
        //unblock NFA_EnablePolling() and NFA_DisablePolling()
        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        sNfaEnableDisablePollingEvent.notifyOne ();
    }

    NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
    theInstance.Finalize();

    ALOGD ("%s: exit", __FUNCTION__);
    return JNI_TRUE;
}


/*******************************************************************************
**
** Function:        nfcManager_doCreateLlcpSocket
**
** Description:     Create a LLCP connection-oriented socket.
**                  e: JVM environment.
**                  o: Java object.
**                  nSap: Service access point.
**                  miu: Maximum information unit.
**                  rw: Receive window size.
**                  linearBufferLength: Max buffer size.
**
** Returns:         NativeLlcpSocket Java object.
**
*******************************************************************************/
static jobject nfcManager_doCreateLlcpSocket (JNIEnv* e, jobject, jint nSap, jint miu, jint rw, jint linearBufferLength)
{
    ALOGD ("%s: enter; sap=%d; miu=%d; rw=%d; buffer len=%d", __FUNCTION__, nSap, miu, rw, linearBufferLength);

    PeerToPeer::tJNI_HANDLE jniHandle = PeerToPeer::getInstance().getNewJniHandle ();
    PeerToPeer::getInstance().createClient (jniHandle, miu, rw);

    /* Create new NativeLlcpSocket object */
    jobject clientSocket = NULL;
    if (nfc_jni_cache_object_local(e, gNativeLlcpSocketClassName, &(clientSocket)) == -1)
    {
        ALOGE ("%s: fail Llcp socket creation", __FUNCTION__);
        return clientSocket;
    }

    /* Get NativeConnectionless class object */
    ScopedLocalRef<jclass> clsNativeLlcpSocket(e, e->GetObjectClass(clientSocket));
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail get class object", __FUNCTION__);
        return clientSocket;
    }

    jfieldID f;

    /* Set socket SAP */
    f = e->GetFieldID (clsNativeLlcpSocket.get(), "mSap", "I");
    e->SetIntField (clientSocket, f, (jint) nSap);

    /* Set socket handle */
    f = e->GetFieldID (clsNativeLlcpSocket.get(), "mHandle", "I");
    e->SetIntField (clientSocket, f, (jint) jniHandle);

    /* Set socket MIU */
    f = e->GetFieldID (clsNativeLlcpSocket.get(), "mLocalMiu", "I");
    e->SetIntField (clientSocket, f, (jint) miu);

    /* Set socket RW */
    f = e->GetFieldID (clsNativeLlcpSocket.get(), "mLocalRw", "I");
    e->SetIntField (clientSocket, f, (jint) rw);

    ALOGD ("%s: exit", __FUNCTION__);
    return clientSocket;
}


/*******************************************************************************
**
** Function:        nfcManager_doCreateLlcpConnectionlessSocket
**
** Description:     Create a connection-less socket.
**                  e: JVM environment.
**                  o: Java object.
**                  nSap: Service access point.
**                  sn: Service name.
**
** Returns:         NativeLlcpConnectionlessSocket Java object.
**
*******************************************************************************/
static jobject nfcManager_doCreateLlcpConnectionlessSocket (JNIEnv *, jobject, jint nSap, jstring /*sn*/)
{
    ALOGD ("%s: nSap=0x%X", __FUNCTION__, nSap);
    return NULL;
}


/*******************************************************************************
**
** Function:        nfcManager_doGetSecureElementList
**
** Description:     Get a list of secure element handles.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         List of secure element handles.
**
*******************************************************************************/
static jintArray nfcManager_doGetSecureElementList(JNIEnv* e, jobject)
{
    ALOGD ("%s", __FUNCTION__);
    return SecureElement::getInstance().getSecureElementIdList (e);
}

/*******************************************************************************
**
** Function:        nfcManager_enableRoutingToHost
**
** Description:     NFC controller starts routing data to host.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_enableRoutingToHost(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    PowerSwitch::getInstance ().setLevel (PowerSwitch::FULL_POWER);
    PowerSwitch::getInstance ().setModeOn (PowerSwitch::HOST_ROUTING);
    if (sRfEnabled) {
        // Stop RF discovery to reconfigure
        startRfDiscovery(false);
    }
    RoutingManager::getInstance().commitRouting();
    startRfDiscovery(true);
    ALOGD ("%s: exit", __FUNCTION__);
}

/*******************************************************************************
**
** Function:        nfcManager_disableRoutingToHost
**
** Description:     NFC controller stops routing data to host.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_disableRoutingToHost(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    bool rfWasEnabled = false;

    if (PowerSwitch::getInstance ().getLevel() == PowerSwitch::LOW_POWER)
    {
        ALOGD ("%s: no need to disable routing while power is OFF", __FUNCTION__);
        goto TheEnd;
    }

    if (sRfEnabled) {
        rfWasEnabled = true;
        // Stop RF discovery to reconfigure
        startRfDiscovery(false);
    }
    RoutingManager::getInstance().commitRouting();
    if (rfWasEnabled)
    {
        startRfDiscovery(true);
    }
    if (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::HOST_ROUTING))
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);
TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
}

/*******************************************************************************
**
** Function:        nfcManager_doSelectSecureElement
**
** Description:     NFC controller starts routing data in listen mode.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         JNI_TRUE if successful, JNI_FALSE, otherwise
**
*******************************************************************************/
static jboolean nfcManager_doSelectSecureElement(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    bool stat = true;

    if (sIsSecElemSelected)
    {
        ALOGD ("%s: already selected", __FUNCTION__);
        goto TheEnd;
    }

    PowerSwitch::getInstance ().setLevel (PowerSwitch::FULL_POWER);

    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }


    stat = SecureElement::getInstance().activate (0xABCDEF);
    if (stat) {
        sIsSecElemSelected = true;
    } else {
        ALOGD ("%s: failed to activate", __FUNCTION__);
    }

    startRfDiscovery (true);
    PowerSwitch::getInstance ().setModeOn (PowerSwitch::SE_ROUTING);
TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);

    if (stat)
        return JNI_TRUE;
    else
        return JNI_FALSE;
}


/*******************************************************************************
**
** Function:        nfcManager_doDeselectSecureElement
**
** Description:     NFC controller stops routing data in listen mode.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_doDeselectSecureElement(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    bool bRestartDiscovery = false;

    if (! sIsSecElemSelected)
    {
        ALOGE ("%s: already deselected", __FUNCTION__);
        goto TheEnd2;
    }

    if (PowerSwitch::getInstance ().getLevel() == PowerSwitch::LOW_POWER)
    {
        ALOGD ("%s: do not deselect while power is OFF", __FUNCTION__);
        sIsSecElemSelected = false;
        goto TheEnd;
    }

    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
        bRestartDiscovery = true;
    }

    //if controller is not routing to sec elems AND there is no pipe connected,
    //then turn off the sec elems
    if (SecureElement::getInstance().isBusy() == false)
        SecureElement::getInstance().deactivate (0xABCDEF);

    sIsSecElemSelected = false;
TheEnd:
    if (bRestartDiscovery)
        startRfDiscovery (true);

    //if nothing is active after this, then tell the controller to power down
    if (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::SE_ROUTING))
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);

TheEnd2:
    ALOGD ("%s: exit", __FUNCTION__);
}


/*******************************************************************************
**
** Function:        isPeerToPeer
**
** Description:     Whether the activation data indicates the peer supports NFC-DEP.
**                  activated: Activation data.
**
** Returns:         True if the peer supports NFC-DEP.
**
*******************************************************************************/
static bool isPeerToPeer (tNFA_ACTIVATED& activated)
{
    #ifdef DTA // <DTA>
    bool isP2p = activated.activate_ntf.protocol == NFA_PROTOCOL_NFC_DEP;

    if (isP2p) {
        ALOGD("%s: P2P CONNECTION ACTIVATED", __FUNCTION__);
    } else {
        ALOGD("%s: Non-p2p connection activated", __FUNCTION__);
    }

    if (in_dta_mode()) {
        ALOGD("%s: in DTA mode", __FUNCTION__);
        dta::nfcdepListenLoopbackOn = isP2p && isListenMode(activated);
        if (dta::nfcdepListenLoopbackOn) {
            ALOGD("%s: P2P LOOPBACK IN LISTEN-MODE ACTIVATED", __FUNCTION__);
            dta::setResponseWaitTime(activated);
        }
    }

    return isP2p;
    #else
    return activated.activate_ntf.protocol == NFA_PROTOCOL_NFC_DEP;
    #endif // </DTA>
}

/*******************************************************************************
**
** Function:        isListenMode
**
** Description:     Indicates whether the activation data indicates it is
**                  listen mode.
**
** Returns:         True if this listen mode.
**
*******************************************************************************/
static bool isListenMode(tNFA_ACTIVATED& activated)
{
    return ((NFC_DISCOVERY_TYPE_LISTEN_A == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_B == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_F == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_ISO15693 == activated.activate_ntf.rf_tech_param.mode)
            || (NFC_DISCOVERY_TYPE_LISTEN_B_PRIME == activated.activate_ntf.rf_tech_param.mode));
}

/*******************************************************************************
**
** Function:        nfcManager_doCheckLlcp
**
** Description:     Not used.
**
** Returns:         True
**
*******************************************************************************/
static jboolean nfcManager_doCheckLlcp(JNIEnv*, jobject)
{
    ALOGD("%s", __FUNCTION__);
    return JNI_TRUE;
}


/*******************************************************************************
**
** Function:        nfcManager_doActivateLlcp
**
** Description:     Not used.
**
** Returns:         True
**
*******************************************************************************/
static jboolean nfcManager_doActivateLlcp(JNIEnv*, jobject)
{
    ALOGD("%s", __FUNCTION__);
    return JNI_TRUE;
}


/*******************************************************************************
**
** Function:        nfcManager_doAbort
**
** Description:     Not used.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_doAbort(JNIEnv*, jobject)
{
    ALOGE("%s: abort()", __FUNCTION__);
    /* Check if nfa enabled and then only store information of watchdog trigger*/
    if(sIsNfaEnabled)
    {
        /*Watchdog expired .nfcservice being aborted.Inform ncihal*/
        NFA_StoreShutdownReason(NFCSERVICE_WATCHDOG_TIMER_EXPIRED);
        usleep(100);
    }
    abort();
}


/*******************************************************************************
**
** Function:        nfcManager_doDownload
**
** Description:     Download firmware patch files.  Do not turn on NFC.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nfcManager_doDownload(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
#ifdef JNI_FIRMWARE_DOWNLOAD
    NfcAdaptation& theInstance = NfcAdaptation::GetInstance();

    theInstance.Initialize(); //start GKI, NCI task, NFC task
    theInstance.DownloadFirmware ();
    theInstance.Finalize();
#endif
    ALOGD ("%s: exit", __FUNCTION__);
    return JNI_TRUE;
}


/*******************************************************************************
**
** Function:        nfcManager_doResetTimeouts
**
** Description:     Not used.
**
** Returns:         None
**
*******************************************************************************/
static void nfcManager_doResetTimeouts(JNIEnv*, jobject)
{
    ALOGD ("%s", __FUNCTION__);
    NfcTag::getInstance().resetAllTransceiveTimeouts ();
}


/*******************************************************************************
**
** Function:        nfcManager_doSetTimeout
**
** Description:     Set timeout value.
**                  e: JVM environment.
**                  o: Java object.
**                  tech: technology ID.
**                  timeout: Timeout value.
**
** Returns:         True if ok.
**
*******************************************************************************/
static bool nfcManager_doSetTimeout(JNIEnv*, jobject, jint tech, jint timeout)
{
    if (timeout <= 0)
    {
        ALOGE("%s: Timeout must be positive.",__FUNCTION__);
        return false;
    }
    ALOGD ("%s: tech=%d, timeout=%d", __FUNCTION__, tech, timeout);
    NfcTag::getInstance().setTransceiveTimeout (tech, timeout);
    return true;
}


/*******************************************************************************
**
** Function:        nfcManager_doGetTimeout
**
** Description:     Get timeout value.
**                  e: JVM environment.
**                  o: Java object.
**                  tech: technology ID.
**
** Returns:         Timeout value.
**
*******************************************************************************/
static jint nfcManager_doGetTimeout(JNIEnv*, jobject, jint tech)
{
    int timeout = NfcTag::getInstance().getTransceiveTimeout (tech);
    ALOGD ("%s: tech=%d, timeout=%d", __FUNCTION__, tech, timeout);
    return timeout;
}

#ifdef DTA // <DTA>
/*******************************************************************************
**
** Function:        nfcManager_doNfcDeactivate
**
** Description:     Deactivate NFC target with selected command.
**                  e: JVM environment.
**                  o: Java object.
**                  deactivationType: 1 = NFC-DEP DSL request
**                                    2 = NFC-DEP RLS request
**                                    3 = General deactivation to sleep mode
**                                    4 = General deactivation
**
** Returns:         Whether the command is succesfully passed to lower level.
**
*******************************************************************************/
static jboolean nfcManager_doNfcDeactivate(JNIEnv *e, jobject o, jint deactivationType)
{
    ALOGD ("%s: deactivationType=%d", __FUNCTION__, deactivationType);

    tNFA_STATUS stat = NFA_STATUS_FAILED;

    switch (deactivationType)
    {
    case gNfcDepDslDeactivation:
    {
        // false = Sleep mode, DSL request
        stat = NFA_NFC_Deactivate(false);
        break;
    }
    case gNfcDepRlsDeactivation:
    {
        // true = Full deactivation, RLS request
        stat = NFA_NFC_Deactivate(true);
        break;
    }
    case gNfaDeactivationToSleep:
    {
        // sleep_mode = true
        stat = NFA_Deactivate(true);
        break;
    }
    case gNfaDeactivation:
    {
        // sleep_mode = false
        stat = NFA_Deactivate(false);
        break;
    }
    default:
    {
        break;
    }
    }

    if (stat == NFA_STATUS_OK) {
        // Command sent but most likely not finished yet
        return true;
    }
    else {
        ALOGE ("%s: failed to deactivate.", __FUNCTION__);
        return false;
    }
}
#endif // </DTA>
/*******************************************************************************
**
** Function:        nfcManager_doDump
**
** Description:     Not used.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Text dump.
**
*******************************************************************************/
static jstring nfcManager_doDump(JNIEnv* e, jobject)
{
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "libnfc llc error_count=%u", /*libnfc_llc_error_count*/ 0);
    return e->NewStringUTF(buffer);
}


/*******************************************************************************
**
** Function:        nfcManager_doSetP2pInitiatorModes
**
** Description:     Set P2P initiator's activation modes.
**                  e: JVM environment.
**                  o: Java object.
**                  modes: Active and/or passive modes.  The values are specified
**                          in external/libnfc-nxp/inc/phNfcTypes.h.  See
**                          enum phNfc_eP2PMode_t.
**
** Returns:         None.
**
*******************************************************************************/
static void nfcManager_doSetP2pInitiatorModes (JNIEnv *e, jobject o, jint modes)
{
    ALOGD ("%s: modes=0x%X", __FUNCTION__, modes);
    struct nfc_jni_native_data *nat = getNative(e, o);

    tNFA_TECHNOLOGY_MASK mask = 0;
    if (modes & 0x01) mask |= NFA_TECHNOLOGY_MASK_A;
    if (modes & 0x02) mask |= NFA_TECHNOLOGY_MASK_F;
    if (modes & 0x04) mask |= NFA_TECHNOLOGY_MASK_F;
    if (modes & 0x08) mask |= NFA_TECHNOLOGY_MASK_A_ACTIVE;
    if (modes & 0x10) mask |= NFA_TECHNOLOGY_MASK_F_ACTIVE;
    if (modes & 0x20) mask |= NFA_TECHNOLOGY_MASK_F_ACTIVE;
    nat->tech_mask = mask;
}


/*******************************************************************************
**
** Function:        nfcManager_doSetP2pTargetModes
**
** Description:     Set P2P target's activation modes.
**                  e: JVM environment.
**                  o: Java object.
**                  modes: Active and/or passive modes.
**
** Returns:         None.
**
*******************************************************************************/
static void nfcManager_doSetP2pTargetModes (JNIEnv*, jobject, jint modes)
{
    ALOGD ("%s: modes=0x%X", __FUNCTION__, modes);
    // Map in the right modes
    tNFA_TECHNOLOGY_MASK mask = 0;
    if (modes & 0x01) mask |= NFA_TECHNOLOGY_MASK_A;
    if (modes & 0x02) mask |= NFA_TECHNOLOGY_MASK_F;
    if (modes & 0x04) mask |= NFA_TECHNOLOGY_MASK_F;
    if (modes & 0x08) mask |= NFA_TECHNOLOGY_MASK_A_ACTIVE | NFA_TECHNOLOGY_MASK_F_ACTIVE;

    PeerToPeer::getInstance().setP2pListenMask(mask);
}

static void nfcManager_doEnableReaderMode (JNIEnv*, jobject, jint technologies)
{
    if (sDiscoveryEnabled) {
        sReaderModeEnabled = true;
        PeerToPeer::getInstance().enableP2pListening(false);
        NFA_PauseP2p();
        NFA_DisableListening();
        // Limit polling to these technologies
        int tech_mask = 0;
        if (technologies & 0x01)
           tech_mask |= NFA_TECHNOLOGY_MASK_A;
        if (technologies & 0x02)
           tech_mask |= NFA_TECHNOLOGY_MASK_B;
        if (technologies & 0x04)
           tech_mask |= NFA_TECHNOLOGY_MASK_F;
        if (technologies & 0x08)
           tech_mask |= NFA_TECHNOLOGY_MASK_ISO15693;
        if (technologies & 0x10)
           tech_mask |= NFA_TECHNOLOGY_MASK_KOVIO;

        enableDisableLptd(false);
        enableDisableLongGuardTime(true);
        NFA_SetRfDiscoveryDuration(READER_MODE_DISCOVERY_DURATION);
        restartPollingWithTechMask(tech_mask);
    }
}

static void nfcManager_doDisableReaderMode (JNIEnv* e, jobject o)
{
    struct nfc_jni_native_data *nat = getNative(e, o);
    if (sDiscoveryEnabled) {
        sReaderModeEnabled = false;
        NFA_ResumeP2p();
        PeerToPeer::getInstance().enableP2pListening(true);
        NFA_EnableListening();

        enableDisableLptd(true);
        enableDisableLongGuardTime(false);
        NFA_SetRfDiscoveryDuration(nat->discovery_duration);
        restartPollingWithTechMask(nat->tech_mask);
    }
}


/********************************************************************************************************
**
** Function:        nfcManager_doReportReason
**
** Description:     Gets reason of Shutdown from Java layer(Shutdown due to NFC disabled by User or not)
**                  e: JVM environment.
**                  o: Java object.
**                  shutdownReason: if 1 - NFC disabled by User otherwise 0.
**
** Returns:         None.
**
******************************************************************************************************/
static void nfcManager_doReportReason(JNIEnv *e, jobject o, jint shutdownReason)
{
    ALOGD ("%s: shutdownReason=%d", __FUNCTION__, shutdownReason);
    NFA_StoreShutdownReason (shutdownReason);
}

/*******************************************************************************
**
** Function:        nfcManager_doGetEeRoutingState
**
** Description:     Get EeRouting State value from the conf file.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         EeRouting State.
**
*******************************************************************************/
static jint nfcManager_doGetEeRoutingState(JNIEnv*, jobject)
{
    int num;

    if (GetNumValue("CE_SCREEN_STATE_CONFIG", &num, sizeof(num)))
       return num;
    else
       return 2;
}


/*******************************************************************************
**
** Function:        nfcManager_doGetEeRoutingReloadAtReboot
**
** Description:     Use conf file over prefs file at boot.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         pick routing state from conf file at NFC Service load
**
*******************************************************************************/
static jboolean nfcManager_doGetEeRoutingReloadAtReboot(JNIEnv*, jobject)
{
    int num;

    if (GetNumValue("CE_SCREEN_STATE_CONFIG_LOAD_AT_BOOT", &num, sizeof(num)))
    {
        if( num==1)
          return true;
        else
          return false;
    }
    else
       return false;
}
#ifdef DTA // <DTA>
/**
 * Type 4 DTA Loopback handling.
 */
static void myType4ListenLoopback(uint8_t* p_buf, uint32_t len)
{
    tNFA_STATUS sendStatus = 0;
    static const uint8_t select_dta_capdu[] = {0x00, 0xA4, 0x04, 0x00, 0x0E,
              0x31, 0x4E, 0x46, 0x43, 0x2E, 0x53,
              0x59, 0x53, 0x2E, 0x44, 0x44, 0x46,
              0x30, 0x31,
              0x00};
    static uint8_t select_dta_rapdu[] = {0x01, 0x00, 0x90, 0x00};

    // DTA 6.4.2.2 - data received from IUT
    if (0 == memcmp(p_buf, select_dta_capdu, len)) {
        ALOGD("LOOPBACK - Responding to SELECT DTA.");

        // C-APDU equals SELECT DTA, proceed to symbol 3
        sendStatus = NFA_SendRawFrame (select_dta_rapdu, sizeof(select_dta_rapdu), NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY );
    } else {
        // Symbol 4: Convert received C-APDU into the R-APDU
        // C-APDU format: '80 EE 00 00' + Lc + Data_C + '00'
        uint8_t Data_C_len = 0;
        uint8_t rapdu[252];

        ALOGD("Starting LOOPBACK for Type 4 Listen.");

        // Min len for C-APDU is 6 bytes (Lc = 0)
        if (len < 6) {
            ALOGD("LOOPBACK - Received C-APDU is too short: got %d bytes.", len);
            return;
}

        Data_C_len = p_buf[4];

        // Max len for Data_C is 250 bytes as defined in DTA
        if (Data_C_len > 250) {
            ALOGD("LOOPBACK - Detected Data_C len in  C-APDU is too long: %d bytes.", Data_C_len);
            return;
}

        // OK, update the R-APDU.
        memcpy(rapdu, &(p_buf[5]), Data_C_len);
        // End R-APDU with STATUS OK.
        rapdu[Data_C_len] = 0x90;
        rapdu[Data_C_len + 1] = 0x00;

        sendStatus = NFA_SendRawFrame (rapdu, Data_C_len + 2, NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY );
    }

    if (sendStatus == NFA_STATUS_OK) {
        ALOGD ("%s: [DTA] T4 LISTEN-LOOPBACK NFA_SendRawFrame(), retVal=NFA_STATUS_OK", __FUNCTION__);
    }
    else {
        ALOGD ("%s: [DTA] T4 LISTEN-LOOPBACK NFA_SendRawFrame(), retVal=NFA_STATUS_FAILED", __FUNCTION__);
    }

}
#endif // </DTA>

/*****************************************************************************
**
** JNI functions for android-4.0.1_r1
**
*****************************************************************************/
static JNINativeMethod gMethods[] =
{
    {"doDownload", "()Z",
            (void *)nfcManager_doDownload},

    {"initializeNativeStructure", "()Z",
            (void*) nfcManager_initNativeStruc},

    {"doInitialize", "()Z",
            (void*) nfcManager_doInitialize},

    {"doDeinitialize", "()Z",
            (void*) nfcManager_doDeinitialize},

    {"sendRawFrame", "([B)Z",
            (void*) nfcManager_sendRawFrame},

    {"routeAid", "([BI)Z",
            (void*) nfcManager_routeAid},

    {"unrouteAid", "([B)Z",
            (void*) nfcManager_unrouteAid},

    {"enableDiscovery", "()V",
            (void*) nfcManager_enableDiscovery},

    {"enableRoutingToHost", "()V",
            (void*) nfcManager_enableRoutingToHost},

    {"disableRoutingToHost", "()V",
            (void*) nfcManager_disableRoutingToHost},

    {"doGetSecureElementList", "()[I",
            (void *)nfcManager_doGetSecureElementList},

    {"doSelectSecureElement", "()Z",
            (void *)nfcManager_doSelectSecureElement},

    {"doDeselectSecureElement", "()V",
            (void *)nfcManager_doDeselectSecureElement},

    {"doCheckLlcp", "()Z",
            (void *)nfcManager_doCheckLlcp},

    {"doActivateLlcp", "()Z",
            (void *)nfcManager_doActivateLlcp},

    {"doCreateLlcpConnectionlessSocket", "(ILjava/lang/String;)Lcom/android/nfc/dhimpl/NativeLlcpConnectionlessSocket;",
            (void *)nfcManager_doCreateLlcpConnectionlessSocket},

    {"doCreateLlcpServiceSocket", "(ILjava/lang/String;III)Lcom/android/nfc/dhimpl/NativeLlcpServiceSocket;",
            (void*) nfcManager_doCreateLlcpServiceSocket},

    {"doCreateLlcpSocket", "(IIII)Lcom/android/nfc/dhimpl/NativeLlcpSocket;",
            (void*) nfcManager_doCreateLlcpSocket},

    {"doGetLastError", "()I",
            (void*) nfcManager_doGetLastError},

    {"disableDiscovery", "()V",
            (void*) nfcManager_disableDiscovery},

    {"doSetTimeout", "(II)Z",
            (void *)nfcManager_doSetTimeout},

    {"doGetTimeout", "(I)I",
            (void *)nfcManager_doGetTimeout},

    {"doResetTimeouts", "()V",
            (void *)nfcManager_doResetTimeouts},

    {"doAbort", "()V",
            (void *)nfcManager_doAbort},

    {"doSetP2pInitiatorModes", "(I)V",
            (void *)nfcManager_doSetP2pInitiatorModes},

    {"doSetP2pTargetModes", "(I)V",
            (void *)nfcManager_doSetP2pTargetModes},

    {"doEnableReaderMode", "(I)V",
            (void *)nfcManager_doEnableReaderMode},

    {"doDisableReaderMode", "()V",
            (void *)nfcManager_doDisableReaderMode},

    {"doDump", "()Ljava/lang/String;",
            (void *)nfcManager_doDump},

    {"doReportReason", "(I)V",
            (void *)nfcManager_doReportReason},

    {"doGetEeRoutingState", "()I",
            (void *)nfcManager_doGetEeRoutingState},

    {"doGetEeRoutingReloadAtReboot", "()Z",
            (void *)nfcManager_doGetEeRoutingReloadAtReboot},

#ifdef DTA // <DTA>
    {"do_dta_set_pattern_number", "(I)V",
        (void *)nfcManager_dta_set_pattern_number},

    {"do_dta_get_pattern_number", "()I",
            (void *)nfcManager_dta_get_pattern_number},

    {"doNfcDeactivate", "(I)Z",
     (void *)nfcManager_doNfcDeactivate},
    #endif // </DTA>
};


/*******************************************************************************
**
** Function:        register_com_android_nfc_NativeNfcManager
**
** Description:     Regisgter JNI functions with Java Virtual Machine.
**                  e: Environment of JVM.
**
** Returns:         Status of registration.
**
*******************************************************************************/
int register_com_android_nfc_NativeNfcManager (JNIEnv *e)
{
    ALOGD ("%s: enter", __FUNCTION__);
    PowerSwitch::getInstance ().initialize (PowerSwitch::UNKNOWN_LEVEL);
    ALOGD ("%s: exit", __FUNCTION__);
    return jniRegisterNativeMethods (e, gNativeNfcManagerClassName, gMethods, NELEM (gMethods));
}


/*******************************************************************************
**
** Function:        startRfDiscovery
**
** Description:     Ask stack to start polling and listening for devices.
**                  isStart: Whether to start.
**
** Returns:         None
**
*******************************************************************************/
void startRfDiscovery(bool isStart)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    #ifdef DTA // <DTA>
    UINT8 nfc_f_condevlimit_param[] = { 0x00 };
    UINT8 con_bailout_param[1]={0};
    //UINT32 config_fwi = 0;
    #endif // </DTA>

    ALOGD ("%s: is start=%d", __FUNCTION__, isStart);
    SyncEventGuard guard (sNfaEnableDisablePollingEvent);
    #ifdef DTA // <DTA>
    if (!in_dta_mode()) {
        ALOGD ("%s: normal operation", __FUNCTION__);
    #endif // </DTA>
    status  = isStart ? NFA_StartRfDiscovery () : NFA_StopRfDiscovery ();
    #ifdef DTA // <DTA>
    } else {
        ALOGD ("%s: [DTA] DTA mode", __FUNCTION__);

        ALOGD ("%s: [DTA] Use exclusive RF control instead of normal RF discovery", __FUNCTION__);


        /** DTA Specification v2.0 configuration parameter mapping
      ----------------------------------------------------------

      Information of each parameter
      -ok/update needeed/correct by default
      -reference number that points to a place in the code below


      CON_LISTEN_DEP_A
      -correct by default (verified when testing CON_LISTEN_DEP_F)
      -ref: (DTA_CFG_01)

      CON_LISTEN_DEP_F
      -ok
      -configured in tNFA_LISTEN_CFG: lf_protocol_type
      -ref: (DTA_CFG_02)

      CON_LISTEN_T3TP
      -ok
      -configured in tNFA_LISTEN_CFG: lf_t3t_flags
      -ref: (DTA_CFG_03)

      CON_LISTEN4ATP
      -ok
      -configured in tNFA_LISTEN_CFG: la_sel_info
      -ref: (DTA_CFG_04)

      CON_LISTEN_T4BTP
      -ok
      -configured in tNFA_LISTEN_CFG: lb_sensb_info
      -ref (DTA_CFG_05)

      CON_ADV_FEAT
      -correct by default
      -ref (DTA_CFG_06)

      CON_SYS_CODE[2]
      -ok
      -the FFFF version is configured with lf_t3t_identifier and the other one is not
       needed as tag type 3 listen mode is not supported
      -note: NFA_LF_MAX_SC_NFCID2 is defined as 1 (related to the lf_t3t_identifier)
      -ref (DTA_CFG_07)

      CON_SENSF_RES[2]
      -ok
      -NFCID2: generated automatically
      -PAD0, PAD1, MRTIcheck, MRTIupdate and PAD2: configured in tNFA_LISTEN_CFG: lf_t3t_pmm
        -PAD0 is left as it is in stock Nexus 4 (20h 79h)
      -ref (DTA_CFG_08)

      CON_ATR_RES
      -NFCC handles this
      -ref (DTA_CFG_09)

      CON_ATS
      -ok
      -FWI and SFGI configured in tNFA_LISTEN_CFG: li_fwi and lb_sfgi
      -other parameters are in the default values of the chip
      -ref (DTA_CFG_10)

      CON_SENSB_RES
      -ok
      -Byte1: is what it is, not found from structs
      -Byte2: configured in tNFA_LISTEN_CFG: lb_sensb_info
      -Byte3: configured in tNFA_LISTEN_CFG: lb_adc_fo
        -current way of setting the value has no effect based on logs (set to 0x75, is 0x05 in logs)
        -could be set with NFA_SetConfig, which works
        -is left as it is currently as the value is 0x05 in stock Nexus 4 is also
      -ref: (DTA_CFG_11)

      CON_ATTRIB_RES
      -is formed from B parameters, nothing done to this one specifically
      -ref: (DTA_CFG_12)

      CON_BITR_F
      -ok
      -set with NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE,...
      -ref: (DTA_CFG_13)

      CON_POLL_A
      -ok
      -configured in tNFA_TECHNOLOGY_MASK
      -ref: (DTA_CFG_14)

      CON_POLL_B
      -ok, verified by trying to read type B tags
      -configured in tNFA_TECHNOLOGY_MASK
      -ref: (DTA_CFG_15)

      CON_POLL_F
      -ok
      -configured in tNFA_TECHNOLOGY_MASK
      -ref: (DTA_CFG_16)

      CON_POLL_P
      -ok
      -NFA_TECHNOLOGY_MASK_B_PRIME and NFA_TECHNOLOGY_MASK_KOVIO disabled in tNFA_TECHNOLOGY_MASK
      -ref: (DTA_CFG_17)

      CON_BAIL_OUT_A
      -ok, verified from logs
      -set with NFA_SetConfig(NCI_PARAM_ID_PA_BAILOUT,...
      -ref: (DTA_CFG_18)

      CON_BAIL_OUT_B
      -ok, verified from logs
      -set with NFA_SetConfig(NCI_PARAM_ID_PB_BAILOUT,...
      -ref: (DTA_CFG_19)

      CON_DEVICES_LIMIT
      -value defined in ICS, assuming that chip default is fine
      -can be set with NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT,...
      -ref: (DTA_CFG_20)

      CON_ADV_FEAT
      -correct by default
      -ref: (DTA_CFG_21)

      CON_ANTICOLL
      -ok, verified from logs
      -set with NFA_SetConfig(NCI_PARAM_ID_PA_ANTICOLL,...
      -note: brcm proprietary param id
      -ref: (DTA_CFG_22)

      CON_ATR
      -is formed from NFC-DEP parameters, nothing done to this one specifically
      -ref: (DTA_CFG_23)

      CON_GB
      -is formed from ISO-DEP parameters, nothing done to this one specifically
      -note: NCI_PARAM_ID_ATR_REQ_GEN_BYTES is available
      -ref: (DTA_CFG_24)

      CON_RATS
      -no changes made, handled automatically
      -note: LA_FSDI, LB_FSDI, PA_FSDI and PB_FSDI NCI parameters are available
      -ref: (DTA_CFG_25)

      CON_ATTRIB
      -no changes made, handled automatically
      -ref: (DTA_CFG_26)

      CON_BITR_NFC_DEP
      -ok, verified from logs
      -configured by calling NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,...
      -ref: (DTA_CFG_27)
         **/

        // Use tNFA_TECHNOLOGY_MASK, tNFA_LISTEN_CFG and NCI parameters to set the parameters.
    tNFA_TECHNOLOGY_MASK poll_mask;
    poll_mask = DEFAULT_TECH_MASK;  // This one polls for all techs. (DTA_CFG_14), (DTA_CFG_16)
    poll_mask &= ~NFA_TECHNOLOGY_MASK_A_ACTIVE;
    poll_mask &= ~NFA_TECHNOLOGY_MASK_F_ACTIVE;
    poll_mask &= ~NFA_TECHNOLOGY_MASK_B_PRIME;  // (DTA_CFG_17)
    poll_mask &= ~NFA_TECHNOLOGY_MASK_KOVIO;  // (DTA_CFG_17)
    poll_mask &= ~NFA_TECHNOLOGY_MASK_ISO15693; // disabling poll mask for ISO15693 as this is not supported in QCA 2.1/2.0.

    /*TODO: Remove it later. only to disable poll for listen mode test*/
    /*poll_mask &= ~NFA_TECHNOLOGY_MASK_A;
    poll_mask &= ~NFA_TECHNOLOGY_MASK_B;
    poll_mask &= ~NFA_TECHNOLOGY_MASK_F;*/

    tNFA_LISTEN_CFG listen_cfg = { };
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    SyncEventGuard guard(sNfaSetConfigEvent);

    /* backup last pattern number */
    LastPatternNumber = sDtaPatternNumber;
    sDtaPatternNumber = NFA_DTA_Get_Pattern_Number();
    ALOGD("[DTA] sDtaPatternNumber = %d LastPatternNumber = %d", sDtaPatternNumber, LastPatternNumber);
    if (!isStart)
    {
        ALOGD("[DTA] isStart = %d , disable discovery", isStart);
        goto TheEnd;
    }
    // Set the pattern number dependant parameters.
    switch (sDtaPatternNumber) {
      case 0x00: {
        ALOGD("%s: [DTA] pattern number 0000h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly

        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        /* Set RTOX parameters when in POLL mode */
        if (AnyPollModeSet == true) {
            UINT8 bitr_nfc_dep_param2[] = { 0x0E };
            stat = NFA_SetConfig(NCI_PARAM_ID_NFC_DEP_OP,
                               sizeof(bitr_nfc_dep_param2),
                               &bitr_nfc_dep_param2[0]);

            ALOGD("POLL MODE , NCI_PARAM_ID_NFC_DEP_OP = 0xE\n");
            if (stat == NFA_STATUS_OK)
                sNfaSetConfigEvent.wait();
        }
        else
        {
            ALOGD("LISTEN MODE , not setting NCI_PARAM_ID_NFC_DEP_OP\n");
        }


        if (enablePassivePollMode) {
            poll_mask &= ~NFA_TECHNOLOGY_MASK_A_ACTIVE;
            poll_mask &= ~NFA_TECHNOLOGY_MASK_F_ACTIVE;
        }

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
            sNfaSetConfigEvent.wait();
        nfc_f_condevlimit_param[0] = 0x01;
        stat = NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT, sizeof(nfc_f_condevlimit_param),
                             &nfc_f_condevlimit_param[0]);

        if (stat == NFA_STATUS_OK)
            sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        ALOGD("%s: [DTA] pattern number 0000h selected : exit", __FUNCTION__);

        break;
    }

      case 0x01: {
        ALOGD("%s : [DTA] pattern number 0001h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();


        // NFC_PMID_CON_DEVICES_LIMIT
     //   UINT8 nfc_f_condevlimit_param[] = { 0x00 };
        nfc_f_condevlimit_param[0] = 0x00;
        stat = NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT, sizeof(nfc_f_condevlimit_param),
                             &nfc_f_condevlimit_param[0]);

        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No
        break;
      }

      case 0x02: {
        ALOGD("%s : [DTA] pattern number 0002h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        //   UINT8 nfc_f_condevlimit_param[] = { 0x00 };
        nfc_f_condevlimit_param[0] = 0x00;
        stat = NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT, sizeof(nfc_f_condevlimit_param),
                             &nfc_f_condevlimit_param[0]);

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x03: {
        ALOGD("%s : [DTA] pattern number 0003h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x04: {
        ALOGD("%s : [DTA] pattern number 0004h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x05: {
        ALOGD("%s : [DTA] pattern number 0005h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        /*TODO: Change name of param for b. like nfc_b_condevlimit_param*/
        nfc_f_condevlimit_param[0] = 0x01;
        stat = NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT, sizeof(nfc_f_condevlimit_param),
                             &nfc_f_condevlimit_param[0]);

        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();


        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x06: {
        ALOGD("%s : [DTA] pattern number 0006h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_424 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_424;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x07: {
        ALOGD("%s : [DTA] pattern number 0007h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        /* CON_DEVICES_LIMIT = 0x00 for
           TC_POL_NFCF_UND_BV_1
           TC_AN_POL_NFCF_BV_02 LM min
           TC_AN_POL_NFCF_BV_02 LM max */
        nfc_f_condevlimit_param[0] = 0x00;
        stat = NFA_SetConfig(NCI_PARAM_ID_CON_DEVICES_LIMIT, sizeof(nfc_f_condevlimit_param),
                             &nfc_f_condevlimit_param[0]);

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - ?

        // Reactivation - Yes

        break;
      }

      case 0x08: {
        ALOGD("%s : [DTA] pattern number 0008h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_424 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_424;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - this call is done later in a separate function.

        // SENSF_REQ Reactivation - ?

        // Reactivation - NFC-F only

        break;
      }

      case 0x09: {
        ALOGD("%s : [DTA] pattern number 0009h selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { NFC_BIT_RATE_424 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x0A: {
        ALOGD("%s : [DTA] pattern number 000Ah selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK
        poll_mask &= ~NFA_TECHNOLOGY_MASK_B;  // (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_424 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_424;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x0B: {
        ALOGD("%s : [DTA] pattern number 000Bh selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK
        poll_mask &= ~NFA_TECHNOLOGY_MASK_B;  // (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x0C: {
        ALOGD("%s : [DTA] pattern number 000Ch selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = 0x00;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      case 0x0D: {
        ALOGD("%s : [DTA] pattern number 000Dh selected", __FUNCTION__);

        // CON_POLL_B - defined in tNFA_TEHCNOLOGY_MASK - no need to disable in this pattern (DTA_CFG_15)

        // CON_BITR_NFC_DEP - set via NCI parameter directly
        UINT8 bitr_nfc_dep_param[] = { 0x01 };  //(DTA_CFG_27)
        stat = NFA_SetConfig(NCI_PARAM_ID_BITR_NFC_DEP,
                             sizeof(bitr_nfc_dep_param),
                             &bitr_nfc_dep_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        // NFC-F_BIT_RATE
        UINT8 nfc_f_bitr_param[] = { NFC_BIT_RATE_212 };  //(DTA_CFG_13)
        stat = NFA_SetConfig(NCI_PARAM_ID_PF_BIT_RATE, sizeof(nfc_f_bitr_param),
                             &nfc_f_bitr_param[0]);
        if (stat == NFA_STATUS_OK)
          sNfaSetConfigEvent.wait();

        listen_cfg.lf_con_bitr_f = NFC_BIT_RATE_212;  //(DTA_CFG_13)

        // CON_LISTEN_DEP_F - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_protocol_type = NCI_LISTEN_PROTOCOL_NFC_DEP;  //(DTA_CFG_02)

        // CON_LISTEN_T3TP3 - defined in tNFA_LISTEN_CFG
        listen_cfg.lf_t3t_flags = 0x0000;  //(DTA_CFG_03)

        // SENSF_REQ - not needed for this pattern number

        // SENSF_REQ Reactivation - not needed for this pattern number

        // Reactivation - No

        break;
      }

      default: {
        ALOGD("%s : [DTA] no supported pattern number selected", __FUNCTION__);
        break;
      }

    }


    ALOGD("[DTA] Have set Pattern -  Continue");

    // Set other DTA parameters (not pattern number dependant).
    // Set which tag types to listen to.
    listen_cfg.la_enable = true;
    listen_cfg.lb_enable = false;
    listen_cfg.lf_enable = true;
    listen_cfg.li_enable = true;
    listen_cfg.ln_enable = true;

    // Simulate Lower Tester?
    // Disable listening altoghether:
    if (dta::simulateLowerTester) {
      listen_cfg.la_enable = false;
      listen_cfg.lb_enable = false;
      listen_cfg.lf_enable = false;
      listen_cfg.li_enable = false;
      listen_cfg.ln_enable = false;
    }

    listen_cfg.la_bit_frame_sdd = 0x04;
    listen_cfg.la_platform_config = 0x00;

    // CON_LISTEN4ATP (DTA_CFG_04)
    listen_cfg.la_sel_info = NCI_PARAM_SEL_INFO_NFCDEP
        | NCI_PARAM_SEL_INFO_ISODEP;

    listen_cfg.la_nfcid1_len = 0x00;

    //listen_cfg.la_nfcid1          // Leave empty -> NFCC will decide

    // Byte 11 of SENSB_RES (DTA_CFG_11), (DTA_CFG_05):
    listen_cfg.lb_sensb_info = 0x81;

    listen_cfg.lb_nfcid0_len = 0x00;
    //listen_cfg.lb_nfcid0          // Leave empty -> NFCC will decide

    int i;
    for (i = 0; i < NCI_PARAM_LEN_LB_APPDATA; i++) {
      listen_cfg.lb_app_data[i] = 0x00;
    }

    // CON_ATS (DTA_CFG_10):
    listen_cfg.lb_sfgi = 0x00;

    /*TODO: make FWI=8 after test .makeing FWI configurable for NOW for Testing only*/
    /*GetNumValue("FWI", &config_fwi, sizeof(config_fwi));
    listen_cfg.li_fwi = config_fwi;*/
    listen_cfg.li_fwi = 0x08; // DTA? changed from 7 to 8 need for nfc forum
    ALOGD("[DTA] : listen_cfg.li_fwi=%X",listen_cfg.li_fwi);

    // Byte 12 of SENSB_RES (DTA_CFG_11):
    /*note:  Setting the value like this has no effect according to logs.
     The value seems to be 0x05. The value could be set via NFA_SetConfig, but
     it's left as it is for now as it's like that in stock version of Nexus 4 also. */
    listen_cfg.lb_adc_fo = 0x75;

    /*
     UINT8  lb_adc_fo_param[] = { 0x75 };
     stat = NFA_SetConfig(NCI_PARAM_ID_LB_ADC_FO, sizeof(lb_adc_fo_param), &lb_adc_fo_param[0]);
     if (stat == NFA_STATUS_OK)
     sNfaSetConfigEvent.wait ();
     */

    // CON_SYS_CODE[2] (DTA_CFG_07)
    int j;
    for (i = 0; i < NFA_LF_MAX_SC_NFCID2; i++) {
      for (j = 0; j < NCI_SYSTEMCODE_LEN + NCI_NFCID2_LEN; j++) {
        if (j <= 1) {
          listen_cfg.lf_t3t_identifier[i][j] = 0xFF;
        } else if (j == 2) {
          listen_cfg.lf_t3t_identifier[i][j] = 0x02;
        } else if (j == 3) {
          listen_cfg.lf_t3t_identifier[i][j] = 0xFE;
        } else {
          listen_cfg.lf_t3t_identifier[i][j] = 0x00;
        }
      }
    }

    // CON_SENSF_RES[2] (DTA_CFG_08):
    listen_cfg.lf_t3t_pmm[0] = 0x20;
    listen_cfg.lf_t3t_pmm[1] = 0x79;
    for (i = 2; i < NCI_T3T_PMM_LEN; i++) {
      listen_cfg.lf_t3t_pmm[i] = 0xFF;
    }

    listen_cfg.la_hist_bytes_len = 0x00;
    //listen_cfg.la_hist_bytes[] - default value is empty
    listen_cfg.lb_h_info_resp_len = 0x00;
    //listen_cfg.lb_h_info_resp[] - default resp is empty

    listen_cfg.ln_wt = 0x07;
    listen_cfg.ln_atr_res_gen_bytes_len = 0x20;
    listen_cfg.ln_atr_res_gen_bytes[0] = 0x46;
    listen_cfg.ln_atr_res_gen_bytes[1] = 0x66;
    listen_cfg.ln_atr_res_gen_bytes[2] = 0x6D;
    listen_cfg.ln_atr_res_gen_bytes[3] = 0x01;
    listen_cfg.ln_atr_res_gen_bytes[4] = 0x01;
    listen_cfg.ln_atr_res_gen_bytes[5] = 0x11;
    listen_cfg.ln_atr_res_gen_bytes[6] = 0x02;
    listen_cfg.ln_atr_res_gen_bytes[7] = 0x02;
    listen_cfg.ln_atr_res_gen_bytes[8] = 0x07;
    listen_cfg.ln_atr_res_gen_bytes[9] = 0xFF;
    listen_cfg.ln_atr_res_gen_bytes[10] = 0x03;
    listen_cfg.ln_atr_res_gen_bytes[11] = 0x02;
    listen_cfg.ln_atr_res_gen_bytes[12] = 0x00;
    listen_cfg.ln_atr_res_gen_bytes[13] = 0x13;
    listen_cfg.ln_atr_res_gen_bytes[14] = 0x04;
    listen_cfg.ln_atr_res_gen_bytes[15] = 0x01;
    listen_cfg.ln_atr_res_gen_bytes[16] = 0x64;
    listen_cfg.ln_atr_res_gen_bytes[17] = 0x07;
    listen_cfg.ln_atr_res_gen_bytes[18] = 0x01;
    listen_cfg.ln_atr_res_gen_bytes[19] = 0x03;

    listen_cfg.ln_atr_res_config = 0x30;

    // CON_BAIL_OUT_A (DTA_CFG_18)
    con_bailout_param[0] = { 0x01 };
    stat = NFA_SetConfig(NCI_PARAM_ID_PA_BAILOUT, sizeof(con_bailout_param),
                         &con_bailout_param[0]);
    if (stat == NFA_STATUS_OK)
      sNfaSetConfigEvent.wait();

    // CON_BAIL_OUT_B (DTA_CFG_19)
    stat = NFA_SetConfig(NCI_PARAM_ID_PB_BAILOUT, sizeof(con_bailout_param),
                         &con_bailout_param[0]);
    if (stat == NFA_STATUS_OK)
      sNfaSetConfigEvent.wait();

    // CON_ANTICOLL (DTA_CFG_22)
   /* UINT8 con_anticoll_param[] = { 0x01 };
    stat = NFA_SetConfig(NCI_PARAM_ID_PA_ANTICOLL, sizeof(con_anticoll_param),
                         &con_anticoll_param[0]);
    if (stat == NFA_STATUS_OK)
      sNfaSetConfigEvent.wait();*/

TheEnd:

    if (isStart) {
      ALOGD(
          "%s : [DTA] Requesting exclusive RF control instead of normal RF discovery",
          __FUNCTION__);
      status = NFA_RequestExclusiveRfControl(poll_mask, &listen_cfg,
                                             nfaConnectionCallback,
                                             dta_ndef_callback);
    } else {
      ALOGD(
          "%s : [DTA] Releasing exclusive RF control instead of normal RF discovery",
          __FUNCTION__);
      status = NFA_ReleaseExclusiveRfControl();
    }

  }
  #endif // </DTA>
    if (status == NFA_STATUS_OK)
    {
        sNfaEnableDisablePollingEvent.wait (); //wait for NFA_RF_DISCOVERY_xxxx_EVT
        sRfEnabled = isStart;
    }
    else
    {
        ALOGE ("%s: Failed to start/stop RF discovery; error=0x%X", __FUNCTION__, status);
    }
}


/*******************************************************************************
**
** Function:        doStartupConfig
**
** Description:     Configure the NFC controller.
**
** Returns:         None
**
*******************************************************************************/
void doStartupConfig()
{
    int actualLen = 0;
#if defined(FEATURE_STARTUP_CONFIG_FLAG)
    struct nfc_jni_native_data *nat = getNative(0, 0);
    tNFA_STATUS stat = NFA_STATUS_FAILED;

    // If polling for Active mode, set the ordering so that we choose Active over Passive mode first.
    if (nat && (nat->tech_mask & (NFA_TECHNOLOGY_MASK_A_ACTIVE | NFA_TECHNOLOGY_MASK_F_ACTIVE)))
    {
        UINT8  act_mode_order_param[] = { 0x01 };
        SyncEventGuard guard (sNfaSetConfigEvent);
        stat = NFA_SetConfig(NCI_PARAM_ID_ACT_ORDER, sizeof(act_mode_order_param), &act_mode_order_param[0]);
        if (stat == NFA_STATUS_OK)
            sNfaSetConfigEvent.wait ();
    }
#endif /* End FEATURE_STARTUP_CONFIG_FLAG */
    //configure RF polling frequency for each technology
    static tNFA_DM_DISC_FREQ_CFG nfa_dm_disc_freq_cfg;
    //values in the polling_frequency[] map to members of nfa_dm_disc_freq_cfg
    UINT8 polling_frequency [8] = {1, 1, 1, 1, 1, 1, 1, 1};
    actualLen = GetStrValue(NAME_POLL_FREQUENCY, (char*)polling_frequency, 8);
    if (actualLen == 8)
    {
        ALOGD ("%s: polling frequency", __FUNCTION__);
        memset (&nfa_dm_disc_freq_cfg, 0, sizeof(nfa_dm_disc_freq_cfg));
        nfa_dm_disc_freq_cfg.pa = polling_frequency [0];
        nfa_dm_disc_freq_cfg.pb = polling_frequency [1];
        nfa_dm_disc_freq_cfg.pf = polling_frequency [2];
        nfa_dm_disc_freq_cfg.pi93 = polling_frequency [3];
        nfa_dm_disc_freq_cfg.pbp = polling_frequency [4];
        nfa_dm_disc_freq_cfg.pk = polling_frequency [5];
        nfa_dm_disc_freq_cfg.paa = polling_frequency [6];
        nfa_dm_disc_freq_cfg.pfa = polling_frequency [7];
        p_nfa_dm_rf_disc_freq_cfg = &nfa_dm_disc_freq_cfg;
    }
}


/*******************************************************************************
**
** Function:        nfcManager_isNfcActive
**
** Description:     Used externaly to determine if NFC is active or not.
**
** Returns:         'true' if the NFC stack is running, else 'false'.
**
*******************************************************************************/
bool nfcManager_isNfcActive()
{
    return sIsNfaEnabled;
}

void restartPollingWithTechMask (int tech_mask)
{
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    startRfDiscovery (false);
    {
        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        stat = NFA_DisablePolling ();
        if (stat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for enable event", __FUNCTION__);
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_DISABLED_EVT
        }
        else
        {
            ALOGE ("%s: failed to disable polling; error=0x%X", __FUNCTION__, stat);
            goto TheEnd;
        }
        stat = NFA_EnablePolling (tech_mask);
        if (stat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for enable event", __FUNCTION__);
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_ENABLED_EVT
        }
        else
            ALOGE ("%s: fail enable polling; error=0x%X", __FUNCTION__, stat);
    }
TheEnd:
    startRfDiscovery(true);
}

/*******************************************************************************
**
** Function:        startStopPolling
**
** Description:     Start or stop polling.
**                  isStartPolling: true to start polling; false to stop polling.
**
** Returns:         None.
**
*******************************************************************************/
void startStopPolling (bool isStartPolling)
{
    ALOGD ("%s: enter; isStart=%u", __FUNCTION__, isStartPolling);
    tNFA_STATUS stat = NFA_STATUS_FAILED;

    startRfDiscovery (false);
    if (isStartPolling)
    {
        tNFA_TECHNOLOGY_MASK tech_mask = DEFAULT_TECH_MASK;
        unsigned long num = 0;
        if (GetNumValue(NAME_POLLING_TECH_MASK, &num, sizeof(num)))
            tech_mask = num;

        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        ALOGD ("%s: enable polling", __FUNCTION__);
        stat = NFA_EnablePolling (tech_mask);
        if (stat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for enable event", __FUNCTION__);
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_ENABLED_EVT
        }
        else
            ALOGE ("%s: fail enable polling; error=0x%X", __FUNCTION__, stat);
    }
    else
    {
        SyncEventGuard guard (sNfaEnableDisablePollingEvent);
        ALOGD ("%s: disable polling", __FUNCTION__);
        stat = NFA_DisablePolling ();
        if (stat == NFA_STATUS_OK)
        {
            sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_DISABLED_EVT
        }
        else
            ALOGE ("%s: fail disable polling; error=0x%X", __FUNCTION__, stat);
    }
    startRfDiscovery (true);
    ALOGD ("%s: exit", __FUNCTION__);
}


} /* namespace android */
