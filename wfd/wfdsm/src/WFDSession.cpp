/*==============================================================================
*        @file SessionState.cpp
*
*  @par DESCRIPTION:
*        SessionState class.
*
*
*  Copyright (c) 2012 - 2013 by Qualcomm Technologies, Inc. All Rights Reserved.
*  Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

/*==============================================================================

                      EDIT HISTORY FOR FILE

  $Header:


when        who         what, where, why
--------    ------      --------------------------------------------------------

================================================================================
*/

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WFDSession_CPP"

#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#ifndef WFD_ICS
#include "common_log.h"
#endif

#include "WFDSession.h"
#include "RTSPSession.h"
#include "SessionManager.h"
#include "Device.h"
#include "MMCapability.h"
#include "wifidisplay.h"


WFDSession::WFDSession(DeviceType myDevType, WfdDevice *pD, SessionType sType) {
    sessionState = SESSION_STOPPED;
    sessionType = sType;
    /*pSourcePeer = NULL;
    pPrimarySinkPeer = NULL;
    pSecondarySinkPeer = NULL;*/
    DeviceType tp = SOURCE;
    if (pD->deviceType == 0)
    {
      tp = SOURCE;
    }
    else if(pD->deviceType == 1)
    {
      tp = PRIMARY_SINK;
    }
    pPeerHandle = new Device(string(pD->macaddress),tp);
    pPeerHandle->ipAddr  = string(pD->ipaddress);
    pPeerHandle->coupledPeerMacAddr = string(pD->peermac);
    pPeerHandle->sessionMngtControlPort = pD->SMControlPort;
    pPeerHandle->decoderLatency = pD->decoderLatency;
    LOGD("start WFD Session with %s", pPeerHandle->macAddr.c_str());
    LOGD("Decoder latency = %d", pPeerHandle->decoderLatency);

    /*switch (myDevType) {
    case SOURCE:
        pPrimarySinkPeer = new Device(peerMacAddr, PRIMARY_SINK);
        pPeerHandle = pPrimarySinkPeer;
        LOGD("Source-PrimarySink session created.");
        break;
    case PRIMARY_SINK:
        if (sessionType == STREAMING) {
            pSourcePeer = new Device(peerMacAddr, SOURCE);
            pPeerHandle = pSourcePeer;
            LOGD("PrimarySink-Source session created.");
        } else {
            pSecondarySinkPeer = new Device(peerMacAddr, SECONDARY_SINK);
            pPeerHandle = pSecondarySinkPeer;
        }
        break;
    case SECONDARY_SINK:
        pPrimarySinkPeer = new Device(peerMacAddr, PRIMARY_SINK);
        pPeerHandle = pPrimarySinkPeer;
        break;
    default:
        pPeerHandle = NULL;
        break;
    }*/
}


WFDSession::~WFDSession() {
 if (pRtspSession) {
    delete pRtspSession;
    pRtspSession = NULL;
   }
 if(pPeerHandle != NULL) {
    LOGD("WFDSession:Deleting peer device");
    delete pPeerHandle;
    pPeerHandle = NULL;
    LOGD("WFDSession:Deleting peer device completed");
 }
}


/**
 * Function to start the WFD session state machine
 */
bool WFDSession::start() {
    bool bRetVal = true;
    if(!pPeerHandle) {
        LOGE("start WFD session....  pPeerHandle is NULL");
        return false;
    }
    /* get the peer device info */
//    LOGD("start WFD session....  pPeerHandle mac: %s", pPeerHandle->macAddr.c_str());
    //getWfdDeviceInfo(pPeerHandle);
    /*if (pPeerHandle==pPrimarySinkPeer && pPeerHandle->coupleSinkStatusBitmap==COUPLEDSINKSTATUS_COUPLED) {
        LOGD("Coupled Sinks: get secondary sink device info.");
        pSecondarySinkPeer = new Device(pPeerHandle->coupledPeerMacAddr, SECONDARY_SINK);
        getWfdDeviceInfo(pSecondarySinkPeer);
    }*/

    /* check p2p readiness */
    /*if (pSourcePeer != NULL && pSourcePeer->ipAddr.empty()) {
        eventError("pSourcePeer has no IP.");
    }
    if (pPrimarySinkPeer != NULL && pPrimarySinkPeer->ipAddr.empty()) {
        eventError("pPrimarySinkPeer has no IP.");
    }
    if (pSecondarySinkPeer != NULL && pSecondarySinkPeer->ipAddr.empty()) {
        eventError("pSecondarySinkPeer has no IP.");
    }*/

    SessionManager *pSM = SessionManager::Instance();

    if(!pSM->pMyDevice) {
        LOGE("pSM->pMyDevice is NULL");
        return false;
    }

    if (pPeerHandle->ipAddr.empty()) {
        LOGD("Peer has no IP");
    }


//    LOGD("start RTSP session with %s.  local ip = %s", pPeerHandle->ipAddr.c_str(), pSM->pMyDevice->ipAddr.c_str());

    /* start RTSP session */
    if (pPeerHandle->getDeviceType() == SOURCE) {
        LOGD("start RTSP session to SOURCE");
        pRtspSession = new RTSPSession(this, pPeerHandle);
        // run RTSP client to interact with Source
        LOGD("Start RTSP client.  Source ip=%s rtspPort=%d", pPeerHandle->ipAddr.c_str(), pPeerHandle->sessionMngtControlPort);
        bRetVal = pRtspSession->startClient(pPeerHandle->ipAddr,
                                               pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.port1_id,
                                               pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.port2_id,
                                               pPeerHandle->sessionMngtControlPort,
                                               pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.content_protection_ake_port);
    }
    if (pPeerHandle->getDeviceType() == PRIMARY_SINK) {
        LOGD("start RTSP session to PRIMARY_SINK");
        pRtspSession = new RTSPSession(this, pPeerHandle);
        if (pSM->pMyDevice->getDeviceType() == SOURCE) {
            // run RTSP server on Source to interact with Primary Sink
            LOGD("Start RTSP server.  localIP=%s rtspPort=%d", pSM->pMyDevice->ipAddr.c_str(), pSM->pMyDevice->sessionMngtControlPort);
            pRtspSession->startServer(pSM->pMyDevice->ipAddr, pSM->pMyDevice->sessionMngtControlPort, 6000);
        } else if (pSM->pMyDevice->getDeviceType() == SECONDARY_SINK){
            // run RTSP client on Secondary Sink to interact with Primary Sink
            LOGD("Start RTSP client.  PrimarySinkIP=%s", pPeerHandle->ipAddr.c_str());
            bRetVal = pRtspSession->startClient(pPeerHandle->ipAddr,
                                                   pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.port1_id,
                                                   pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.port2_id,
                                                   pPeerHandle->sessionMngtControlPort,
                                                   pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.content_protection_ake_port/*hdcpPort*/);
        } else {
            LOGE("Wrong local device type: %d", pSM->pMyDevice->getDeviceType());
        }
    }
    if (pPeerHandle->getDeviceType() == SECONDARY_SINK) {
        LOGD("start RTSP session to SECONDARY_SINK");
        pRtspSession = new RTSPSession(this, pPeerHandle);
        // run RTSP server to interact with Secondary Sink
        LOGD("Start RTSP server.  localIP=%s", pSM->pMyDevice->ipAddr.c_str());
        pRtspSession->startServer(pSM->pMyDevice->ipAddr, pSM->pMyDevice->sessionMngtControlPort, 6000);
    }
    if(!bRetVal) {
          LOGE("WFDSession::start() failed!");
          return false;
    }
    sessionState = SESSION_STARTING;
    return true;
}

/**
 * Function to transition session state
 * @param newState
 */
void WFDSession::sessionStateTransition(SessionState newState) {
    if (sessionState != newState) {
        sessionState = newState;

        char sessId[10];
        snprintf(sessId,10, "%d", pRtspSession->rtspSessionId);
        LOGD("Session state transition to %d. macAddr=%s sessionId=%s", sessionState, pPeerHandle->macAddr.c_str(), sessId);
        eventSessionStateChanged(sessionState, pPeerHandle->macAddr.c_str(), sessId);
    }
}


/**
 * This function is called by RTSPSession when RTSPState
 * changes, in order to update the WFDSessionState.
 */
void WFDSession::updateSessionState() {

#if MULTIPEER
    if (pPeerHandle->getDeviceType() == PRIMARY_SINK && pPeerHandle->coupleSinkStatusBitmap==COUPLEDSINKSTATUS_COUPLED) {
        // wfd session with coupled sinks
        if (pRtspSession->rtspState==STOPPED) {
            sessionStateTransition(SESSION_STOPPED);
        } else {
            switch (sessionState) {
            case SESSION_STOPPED:
                if (pRtspSession->rtspState==CAP_NEGOTIATING || pSecondarySinkPeer->pRtspSession->rtspState==CAP_NEGOTIATING) {
                    sessionStateTransition(SESSION_NEGOTIATING);
                }
                break;
            case SESSION_NEGOTIATING:
                if (pRtspSession->rtspState==CAP_NEGOTIATED && pSecondarySinkPeer->pRtspSession->rtspState==CAP_NEGOTIATED) {
                    sessionStateTransition(SESSION_NEGOTIATED);
                }
                break;
            case SESSION_NEGOTIATED:
                if (pRtspSession->rtspState==SESS_ESTABLISHING || pSecondarySinkPeer->pRtspSession->rtspState==SESS_ESTABLISHING) {
                    sessionStateTransition(SESSION_ESTABLISHING);
                }
                break;
            case SESSION_ESTABLISHING:
                if (pRtspSession->rtspState==SESS_ESTABLISHED && pSecondarySinkPeer->pRtspSession->rtspState==SESS_ESTABLISHED) {
                    sessionStateTransition(SESSION_ESTABLISHED);
                }
                break;
            case SESSION_ESTABLISHED:
                break;
            case STANDBY:
                if (pRtspSession->rtspState==SESS_ESTABLISHED && pSecondarySinkPeer->pRtspSession->rtspState==SESS_ESTABLISHED) {
                    sessionStateTransition(SESSION_STANDBY);
                } 
                break;
            default:
                break;
            }
        }


    } else {
#endif
        switch (pRtspSession->rtspState) {
        case STOPPED:
            sessionStateTransition(SESSION_STOPPED);
            break;
        case CAP_NEGOTIATING:
            sessionStateTransition(SESSION_NEGOTIATING);
            break;
        case CAP_NEGOTIATED:
            sessionStateTransition(SESSION_NEGOTIATED);
            break;
        case SESS_ESTABLISHING:
            sessionStateTransition(SESSION_ESTABLISHING);
            break;
        case SESS_ESTABLISHED:
            sessionStateTransition(SESSION_ESTABLISHED);
            break;
        case STANDBY:
            sessionStateTransition(SESSION_STANDBY);
            break;
        default:
            break;
        }
#if MULTIPEER
    }
#endif
}


/**
 * Query RTSPSession using session id
 * @param sessId
 *
 * @return RTSPSession*
 */
RTSPSession* WFDSession::getRtspSession() {

    if (pRtspSession != NULL) {
        return pRtspSession;
    }
    return NULL;
}
