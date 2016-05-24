/*==============================================================================
*        @file RTSPSession.cpp
*
*  @par DESCRIPTION:
*        RTSPSession class.
*
*
*  Copyright (c) 2011 - 2013 Qualcomm Technologies, Inc. All Rights Reserved.
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
#define LOG_TAG "RTSPSession_CPP"

#define LOG_NDEBUG  0
#define LOG_NDDEBUG 0
#ifndef WFD_ICS
#include <common_log.h>
#endif
#include <utils/Log.h>
#include "SessionManager.h"
#include "RTSPSession.h"
#include "WFDSession.h"
#include "wifidisplay.h"
#include <pthread.h>
#include <vector>
#include "rtsp_wfd.h"
#include "MMAdaptor.h"
#include "UIBCAdaptor.h"
#include "MMCapability.h"
#include "MMTimer.h"
#include <threads.h>
#include "wfd_netutils.h"
#include "wdsm_mm_interface.h"
#include "wfd_cfg_parser.h"
#ifdef HDCP_DISPLAY_ENABLED
#include "hdcpmgr_api.h"
#endif
#define WFD_MM_RTSP_THREAD_PRIORITY -19


using namespace std;

#define WFD_SRC_RTSP_KEEPALIVE_INTERVAL 50000

MMEventStatusType bHDCPStatus = MM_STATUS_INVALID;

cback::cback(RTSPSession* pRtspSess)
{
    pRtspSession = pRtspSess;
    m_hTimer = NULL;
}


void printMesg(rtspApiMesg &mesg)
{
   switch(mesg.error) {
    case noError:
        LOGD("Success");
    break;
    case badStateError:
        LOGD("Error: bad state");
    break;
    case timeoutError:
        LOGD("Error: timeout");
    break;
    case remoteError:
        LOGD("Error: remote error");
    break;
    default:
    break;
    }

    LOGD("Session: %d", mesg.session);
    LOGD("Port0: %d", mesg.rtpPort0);
    LOGD("Port1: %d", mesg.rtpPort1);

    if (!mesg.ipAddr.empty()) {
        LOGD("IP: %s", mesg.ipAddr.c_str());
    }
}

void cback::finishCallback()
{
   LOGD("Callback: finishCallback");
   if(m_hTimer)
   {
       int ret = MM_Timer_Release(m_hTimer);
       LOGI("Keep alive timer release returned %d",ret);
       m_hTimer = NULL;
   }
   if(pRtspSession != NULL && pRtspSession->rtspState != STOPPED)
   {
      /* destroy MM/UIBC session */
     MMAdaptor::destroySession();
     UIBCAdaptor::destroySession();
     LOGD("finishCallback: rtspState = %d", pRtspSession->rtspState);
     pRtspSession->rtspStateTransition(STOPPED);
   }
}
/*
 * Used to overwrite the WFD parameters before
 * RTSP begins negotiation
 */
void cback::openCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: openCallback");
    printMesg(mesg);
    mesg.wfd.dump();
    /*
     * Modify local wfd parameters in RTSP lib
     */
    #define NUMLOCALPARAMS 11
    string params[NUMLOCALPARAMS] = {
        "wfd_audio_codecs",
        "wfd_video_formats",
        "wfd_3d_video_formats",
        "wfd_content_protection",
        "wfd_display_edid",
        "wfd_coupled_sink",
        //"wfd_client_rtp_ports",
        "wfd_uibc_capability",
        "wfd_I2C",
        "wfd_connector_type",
        "wfd_presentation_URL",
        "wfd_standby_resume_capability"
    };
    SessionManager *pSM = SessionManager::Instance();
    if((pSM == NULL) || (pSM->pMyDevice == NULL) || (pSM->pMyDevice->pMMCapability == NULL))
        return;

    /* HDCP content protection is enabled in the config file, then check if the connected device supports it or not,
        if dosen't support then update the local capability structure wtih this information so that while capability
        negotiation Sink won't publish HDCP content protection.
       */
    if( mesg.wfd.contentProtection.getValid() == TRUE) {
#ifdef HDCP_DISPLAY_ENABLED
       if(HDCP1X_COMM_hdmi_status())
       {
         LOGE("RTSPSession  : HDMI is connected ");
         if((PRIMARY_SINK == pSM->pMyDevice->getDeviceType()) ||
            (SECONDARY_SINK == pSM->pMyDevice->getDeviceType())) {
            if (MM_STATUS_NOTSUPPORTED ==
                 pRtspSession->updateHdcpSupportedInConnectedDevice(
                       pSM->pMyDevice->pMMCapability->pCapability))
            {
              LOGE("Setting CP to FALSE in RTSPSession OpenCB");
              /*If UnAuthorised connection then no need to enable CP */
              mesg.wfd.contentProtection.setValid(FALSE);
            }
          }
       }
#endif
    }

    char buffer[1024];
    for (int i=0; i<NUMLOCALPARAMS; i++) {
        memset(buffer, 0, sizeof(buffer));
        strlcat(buffer, params[i].c_str(), sizeof(buffer));
        strlcat(buffer, ": ", sizeof(buffer));
        strlcat(buffer, pSM->pMyDevice->pMMCapability->getKeyValue((char*)params[i].c_str()), sizeof(buffer));
        params[i] = buffer;
        LOGD("Modify local wfd in rtsp lib....  %s", buffer);
    }
    if(!pSM->pMyDevice->pMMCapability->pUibcCapability->port_id &&
        SOURCE == pSM->pMyDevice->getDeviceType())
    {
      mesg.wfd.uibcCap.setValid(false);
    }
    rtspWfdParams type;
    bool isParamSet;
    for (int i=0; i<NUMLOCALPARAMS; i++) {
        isParamSet = false;
        if ((type=mesg.wfd.wfdType(params[i], isParamSet)) != wfd_invalid) {
            if (isParamSet) {
                mesg.wfd.wfdParse(type, params[i]);
            }
        }
    }

    LOGD("Configured RTSP wfd mesg in Open callback:");

    if(SOURCE != pSM->pMyDevice->getDeviceType())
    {
       mesg.wfd.client.setRtpPort0(
       pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.port1_id);
    }

    if (SOURCE == pSM->pMyDevice->getDeviceType())
    {
        if (pRtspSession->pPeerDevice->decoderLatency)
        {
            mesg.wfd.buffLen.setBufferLen(pRtspSession->pPeerDevice->decoderLatency);
        }
        mesg.wfd.tcpWindowSize.setWindowSize(0);
    }
    else if(PRIMARY_SINK == pSM->pMyDevice->getDeviceType())
    {
        mesg.wfd.tcpWindowSize.setValid(true);
        mesg.wfd.tcpWindowSize.setWindowSize(0);
    }

    mesg.wfd.dump();

    /* record sessionId */
    pRtspSession->rtspSessionId = mesg.session;

    /* record peer IP address */
    if (!mesg.ipAddr.empty()) {
        pRtspSession->pPeerDevice->ipAddr = mesg.ipAddr;
    }

    /* record Local IP if not done already */
    if (pSM->pMyDevice->ipAddr.empty()) {
       char ip[20];
       int ret = getLocalIpSocket (mesg.session, ip);
       if (ret == 0)
          pSM->pMyDevice->ipAddr = ip;
    }

    pRtspSession->rtspStateTransition(CAP_NEGOTIATING);
    if( mesg.wfd.contentProtection.getValid() == TRUE) {
        if( PRIMARY_SINK == pSM->pMyDevice->getDeviceType() || SECONDARY_SINK == pSM->pMyDevice->getDeviceType()) {
            LOGD("Callback: createHDCPSession()");
            MMAdaptor::createHDCPSession(pSM->pMyDevice->getDeviceType(),
                                         pSM->pMyDevice->pMMCapability,
                                         pRtspSession->pPeerDevice->pMMCapability,
                                         pRtspSession->pPeerDevice->pNegotiatedMMCapability);
      }
    }
}

void cback::getCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: getCallback");
    printMesg(mesg);
    mesg.wfd.dump();

    if (mesg.error != noError && mesg.error != pendingCmdError) {
        return;
    }
    SessionManager *pSM = SessionManager::Instance();

    if((pSM == NULL) || (pSM->pMyDevice == NULL))
    {
        LOGE("Something's Missing! Failed to get SessionManager instance");
        return;
    }
    /*If get parameter was not sent due to an already pending command, try resending*/
    if (mesg.error == pendingCmdError && pSM->pMyDevice->getDeviceType() == SOURCE) {
       LOGE("Pending cmd error, send keep alive again");
       pRtspSession->sendWFDKeepAliveMsg();
       return;
    }

    if (SOURCE == pSM->pMyDevice->getDeviceType()) {
        /*
         * Record wfd parameters of peer device
         */
        pRtspSession->pPeerDevice->pMMCapability->configure(mesg.wfd);

        LOGD("Peer MMCapability dump:");
        pRtspSession->pPeerDevice->pMMCapability->dump();

        if (mesg.wfd.tcpWindowSize.getValid())
        {
          pRtspSession->m_bTCPSupportedAtSink = true;
          LOGD("TCP Supported at Sink");
          if (pRtspSession->m_bTCPSupportStatusRequested) {
            if (pRtspSession->m_bTCPSupportedAtSink) {
                LOGD("Client waiting for TCP support status");
                eventMMUpdate(MM_RTP_EVENT_TCP_SUPPORTED_BY_SINK,MM_STATUS_SUCCESS,
                  0,0);
            }else{
                eventMMUpdate(MM_RTP_EVENT_TCP_SUPPORTED_BY_SINK,MM_STATUS_FAIL,
                  0,0);
            }
            pRtspSession->m_bTCPSupportStatusRequested = false;
          }
        }
        pRtspSession->m_bTCPSupportQueried = true;
    }
    else
    {
        /*
         *   If Peer has no HDCP capability reset local capability
         */

        if (mesg.wfd.contentProtection.getValid() == false) {
            pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.
                               content_protection_ake_port = 0;
            pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.
                               content_protection_capability = 0;
        }
    }
}

/*
 * Used to overwrite the WFD parameters before
 * RTSP begins negotiation
 */
void cback::intersectCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: intersectCallback");
    printMesg(mesg);
    mesg.wfd.dump();
    SessionManager *pSM = SessionManager::Instance();
    if((pSM == NULL) || (pSM->pMyDevice == NULL)  || (pSM->pMyDevice->pMMCapability == NULL))
        return;

    /*
     * Modify negotiated wfd parameters by querying the negotiated result from MM lib
     */

    LOGD("Get Negotiated MMCapability from MM lib");
    pRtspSession->pPeerDevice->pMMCapability->pCapability->peer_ip_addrs.ipv4_addr1 =
                                 inet_addr(pRtspSession->pPeerDevice->ipAddr.c_str());
    MMAdaptor::getNegotiatedCapability(pSM->pMyDevice->pMMCapability,
                                       pRtspSession->pPeerDevice->pMMCapability,
                                       pRtspSession->pPeerDevice->pNegotiatedMMCapability,
                                       pRtspSession->pPeerDevice->pCommonCapability);
    UIBCAdaptor::getNegotiatedCapability(pSM->pMyDevice->pMMCapability->pUibcCapability,
                                              pRtspSession->pPeerDevice->pMMCapability->pUibcCapability,
                                              pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability);
    //UIBC Capability does not have port param called.
    pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->ipv4_addr =
        inet_addr(pRtspSession->pPeerDevice->ipAddr.c_str());
    if(pSM->pMyDevice->pMMCapability->pUibcCapability != NULL)
    {
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->port_id =
            pSM->pMyDevice->pMMCapability->pUibcCapability->port_id;
    }
    LOGD("UIBC capability port = %d",
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->port_id);

    LOGD("Local MMCapability:");
    pSM->pMyDevice->pMMCapability->dump();
    LOGD("Peer MMCapability:");
    pRtspSession->pPeerDevice->pMMCapability->dump();
    LOGD("Negotiated MMCapability:");
    pRtspSession->pPeerDevice->pNegotiatedMMCapability->dump();

    LOGD("createHDCPSession()");
    if( SOURCE == pSM->pMyDevice->getDeviceType() &&
        ( pRtspSession->pPeerDevice->pNegotiatedMMCapability->isHDCPVersionSupported(
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_capability
            )))
    {
        LOGD("createHDCPSession(): SOURCE");
        MMAdaptor::createHDCPSession(pSM->pMyDevice->getDeviceType(),
                                     pSM->pMyDevice->pMMCapability,
                                     pRtspSession->pPeerDevice->pMMCapability,
                                     pRtspSession->pPeerDevice->pNegotiatedMMCapability);
    }

    #define NUMNEGPARAMS 10

    string params[NUMNEGPARAMS] = {
        "wfd_audio_codecs",
        "wfd_video_formats",
        "wfd_3d_video_formats",
        "wfd_content_protection",
        "wfd_display_edid",
        "wfd_coupled_sink",
        //"wfd_client_rtp_ports",
        "wfd_I2C",
        "wfd_uibc_capability",
        "wfd_connector_type"//,
        "wfd_standby_resume_capability"
    };
    mesg.wfd.standbyCap.setValid(false);

    //Check if A/V mode is set, otherwise wipe it out
    if (RTSPSession::m_eplayMode == AUDIO_ONLY)  {
      mesg.wfd.h264Cbp.setValid(false);
      mesg.wfd.h264Chp.setValid(false);
      mesg.wfd.h264Chi444p.setValid(false);
      mesg.wfd.videoHeader.setValid(false);

    }
    else if (RTSPSession::m_eplayMode == VIDEO_ONLY) {
       mesg.wfd.audioLpcm.setValid(false);
       mesg.wfd.audioAac.setValid(false);
       mesg.wfd.audioEac.setValid(false);
       mesg.wfd.audioDts.setValid(false);
    }
    else if (RTSPSession::m_eplayMode == AUDIO_VIDEO)  {
      mesg.wfd.h264Cbp.setValid(false);
      mesg.wfd.h264Chp.setValid(false);
      mesg.wfd.h264Chi444p.setValid(false);
    }
    // Hack to wipe out other audio codecs other than the negotiated one
    switch (pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_method) {
    case WFD_AUDIO_LPCM:
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec));
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec));
        break;
    case WFD_AUDIO_AAC:
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec));
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec));
        break;
    case WFD_AUDIO_DOLBY_DIGITAL:
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec));
        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec));
        break;
    default:
        break;
    }

    char buffer[500];
    rtspWfdParams type;
    bool isParamSet;
    for (int i=0; i<NUMNEGPARAMS; i++) {
        memset(buffer, 0, sizeof(buffer));
        strlcat(buffer, params[i].c_str(), sizeof(buffer));
        strlcat(buffer, ": ", sizeof(buffer));
        strlcat(buffer, pRtspSession->pPeerDevice->pNegotiatedMMCapability->getKeyValue((char*)params[i].c_str()), sizeof(buffer));
        params[i] = buffer;

        isParamSet = false;
        if ((type=mesg.wfd.wfdType(params[i], isParamSet)) != wfd_invalid) {
            if ((RTSPSession::m_eplayMode == VIDEO_ONLY && type == wfd_audio_codecs) ||
                (RTSPSession::m_eplayMode == AUDIO_ONLY && type == wfd_video_formats) ||
                (RTSPSession::m_eplayMode == AUDIO_ONLY && type == wfd_3d_video_formats))
              isParamSet = false;
            if(mesg.wfd.uibcCap.getValid()) {
               //parse for M14 support only if UIBC is supported in the first place
               pRtspSession->m_bUIBCSupported = true;
               if (type == wfd_uibc_capability) {
                   int m14Support = 0;
                   getCfgItem(UIBC_M14_KEY,&m14Support);
                   if(m14Support) {
                     //Since M14 suppport is enabled, don't set UIBC capabilties in M4
                     mesg.wfd.uibcCap.setValid(false);//Invalidate UIBC cap from RTSP message
                     isParamSet = false; //Do not add wfd_uibc in M4 message
                   }
               }
            }
            if (isParamSet) {
                mesg.wfd.wfdParse(type, params[i]);
            }
        }
    }
    if(pRtspSession->m_bUIBCSupported == true && (SOURCE == pSM->pMyDevice->getDeviceType()))
    { //Start server on source irrespective of M4/M14 to be able to accept connection at any point
       LOGE("Start UIBC Server on source");
       MMCapability tempCap;
       int32 negHeight =0, negWidth = 0;
       //TODO for source we can perhaps move the configure altogether here itself
       tempCap.configure(mesg.wfd);
       if(tempCap.pCapability)
       {
         negHeight = tempCap.pCapability->video_config.\
                    video_config.h264_codec[0].max_vres;
         negWidth = tempCap.pCapability->video_config.\
                    video_config.h264_codec[0].max_hres;
       }
       LOGE("RTSPSession_CPP ::UIBC negotiated_height %ld negotiated_width %ld",\
                                                negHeight, negWidth );
       pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->\
                                  negotiated_height = negHeight;
       pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->\
                                  negotiated_width = negWidth;
       UIBCAdaptor::createSession(pRtspSession->pPeerDevice->\
                                  pNegotiatedMMCapability->pUibcCapability);
    }
    if (pRtspSession->m_bTCPSupportedAtSink && pRtspSession->pPeerDevice->decoderLatency)
    {
        LOGD("Client requested decoder latency for Sink. Send in SET_PARAMETER");
        mesg.wfd.buffLen.setBufferLen(pRtspSession->pPeerDevice->decoderLatency);
    }
    else
    {
        mesg.wfd.buffLen.setValid(false);
    }
    LOGD("Configured rtsp mesg:");
    mesg.wfd.dump();

}

void cback::setCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: setCallback");
    printMesg(mesg);
    mesg.wfd.dump();

    if (mesg.error != noError) {

        if(mesg.wfd.tcpStreamControl.getValid())
        {
            rtsp_wfd::tcp_control_command eCmd = mesg.wfd.tcpStreamControl.getCommand();

            MMEventType eEvent = NO_EVENT;
            switch(eCmd) {
            case rtsp_wfd::flush:
                eEvent = BUFFERING_CONTROL_EVENT_FLUSH;
                LOGE("TCP Buffer Control Event FLush Fail");
                break;
            case rtsp_wfd::pause:
                eEvent = BUFFERING_CONTROL_EVENT_PAUSE;
                LOGE("TCP Buffer Control Event Pause Fail");
                break;
            case rtsp_wfd::play:
                eEvent = BUFFERING_CONTROL_EVENT_PLAY;
                LOGE("TCP Buffer Control Event Play Fail");
                break;
            case rtsp_wfd::status:
                eEvent = BUFFERING_CONTROL_EVENT_STATUS;
                LOGE("TCP Buffer Control Event Status Fail");
                break;
            default:
                LOGE("TCP Buffer Control Event invalid event");
                break;
            }

            LOGE("TCP Stream control failed");
            eventMMUpdate(eEvent,MM_STATUS_FAIL, 0,0);
            return;
        }
#ifdef TCP_LEGACY
        if(mesg.wfd.buffLen.getValid())
        {
            if(!mesg.wfd.tcpWindowSize.getValid())
            {
                LOGD("Buffering negotiation fail");
                eventMMUpdate(BUFFERING_NEGOTIATION,MM_STATUS_FAIL,
                      0,0);
            }
        }
#else
        if (pRtspSession->rtspState == SESS_ESTABLISHED)
        {
            if(mesg.wfd.client.getValid())
            {
                LOGD("Buffering negotiation fail");
                eventMMUpdate(BUFFERING_NEGOTIATION,MM_STATUS_FAIL,
                      0,0);
            }
            if (mesg.wfd.buffLen.getValid())
            {
                LOGD("Set decoder latency fail");
                eventMMUpdate(BUFFERING_CONTROL_EVENT_DECODER_LATENCY,MM_STATUS_FAIL,
                      0,0);
            }
        }
#endif

        return;
    }

    SessionManager *pSM = SessionManager::Instance();
    if((pSM == NULL) || (pSM->pMyDevice == NULL)) {
        LOGE("Something amiss!! SessionManager instance is null!!");
        return;
    }


    /*When SINK receives a SET PARAMETER with UIBC valid (either in M4/M14) go ahead and
          set SINK's parameters, spawn UIBC threads and attempt to connect to source [the server]
          Nothing to do for source since it should be already up at this point to accept connections*/
    if (mesg.wfd.uibcCap.getValid() && (SOURCE != pSM->pMyDevice->getDeviceType()))
    {
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->configure(mesg.wfd);
        pRtspSession->m_bUIBCSupported = true;
        //Populate UIBC Capabilities
        memcpy(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability,
                       pSM->pMyDevice->pMMCapability->pUibcCapability,
                       sizeof(*pSM->pMyDevice->pMMCapability->pUibcCapability));
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->ipv4_addr =
            inet_addr(pRtspSession->pPeerDevice->ipAddr.c_str());
        LOGD("UIBC IP IN network order %d",
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->ipv4_addr);
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->port_id =
            mesg.wfd.uibcCap.getPort();
        LOGD("UIBC port from SM %d",mesg.wfd.uibcCap.getPort());
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->negotiated_height =
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->\
                                       video_config.video_config.h264_codec[0].max_vres;
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->negotiated_width=
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->\
                                       video_config.video_config.h264_codec[0].max_hres;
        LOGD("RTSPSession_CPP :: UIBC negotiated_height %lu negotiated_width %lu",
           pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->negotiated_height,
           pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability->negotiated_width);
        UIBCAdaptor::createSession(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pUibcCapability);
    }

    if (pRtspSession->rtspState == CAP_NEGOTIATING) {
        /*
         * Configure negotiated MM capability
         */

        pRtspSession->pPeerDevice->pNegotiatedMMCapability->configure(mesg.wfd);
        LOGD("Dump final negotiated MMCapability:");
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->dump();
        pRtspSession->rtspStateTransition(CAP_NEGOTIATED);
        pRtspSession->rtspStateTransition(SESS_ESTABLISHING);

        if(mesg.wfd.standbyCap.getValid() && pRtspSession->pPeerDevice->getDeviceType() == SOURCE ){
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->standby_resume_support= true;
        }

        if(mesg.wfd.buffLen.getValid() && pRtspSession->pPeerDevice->getDeviceType() != SOURCE) {
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->decoder_latency
                      = mesg.wfd.buffLen.getBufferLen();
        }
    }
    else if (pRtspSession->rtspState == SESS_ESTABLISHED)
    {
        WFD_MM_capability_t *pMMCfg = pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability;
        if (SOURCE != pSM->pMyDevice->getDeviceType() && mesg.wfd.client.getValid())
        {
            if (pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->transport_capability_config.eRtpPortType == RTP_PORT_UDP)
            {
                if (mesg.wfd.client.getTCP())
                {
                    LOGE("Switch to TCP rquested from source");
                    MMAdaptor::streamPause();
                    getIPSockPair(true, &pMMCfg->transport_capability_config.rtpSock,
                        &pMMCfg->transport_capability_config.rtcpSock,
                        (int*)(&pMMCfg->transport_capability_config.port1_id),
                        (int*)(&pMMCfg->transport_capability_config.port1_rtcp_id), true );
                    close(pMMCfg->transport_capability_config.rtpSock);
                    close(pMMCfg->transport_capability_config.rtcpSock);
                    pMMCfg->transport_capability_config.rtpSock = -1;
                    pMMCfg->transport_capability_config.rtcpSock = -1;
                    pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->transport_capability_config.eRtpPortType = RTP_PORT_TCP;
                    MMAdaptor::updateSession(pRtspSession->pPeerDevice->pNegotiatedMMCapability);
                    MMAdaptor::streamPlay();
                    if (mesg.wfd.tcpWindowSize.getValid())
                    {
                      MMAdaptor::streamControl(WFD_DEVICE_PRIMARY_SINK, AV_CONTROL_SET_DECODER_LATENCY,
                                                (uint64)mesg.wfd.tcpWindowSize.getWindowSize());
                    }
                }
                else
                {
                    LOGE("Switch to TCP requested when in TCP. Ignore");
                }
            }
            else
            {
                if (!mesg.wfd.client.getTCP())
                {
                    LOGE("Switch to UDP rquested from source");
                    MMAdaptor::streamPause();
                    getIPSockPair(true, &pMMCfg->transport_capability_config.rtpSock,
                        &pMMCfg->transport_capability_config.rtcpSock,
                        (int*)(&pMMCfg->transport_capability_config.port1_id),
                        (int*)(&pMMCfg->transport_capability_config.port1_rtcp_id), false );
                    close(pMMCfg->transport_capability_config.rtpSock);
                    close(pMMCfg->transport_capability_config.rtcpSock);
                    pMMCfg->transport_capability_config.rtpSock = -1;
                    pMMCfg->transport_capability_config.rtcpSock = -1;
                    pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->transport_capability_config.eRtpPortType = RTP_PORT_UDP;
                    MMAdaptor::updateSession(pRtspSession->pPeerDevice->pNegotiatedMMCapability);
                    MMAdaptor::streamPlay();
                }
                else
                {
                    LOGE("Switch to UDP requested when in UDP. Ignore");
                }
            }
            mesg.wfd.client.setRtpPort0(pMMCfg->transport_capability_config.port1_id);
            if (mesg.wfd.client.getRtcpPort0())
            {
                mesg.wfd.client.setRtcpPort0(pMMCfg->transport_capability_config.port1_rtcp_id);
            }
        }
        else if (mesg.wfd.client.getValid())
        {
            LOGD("Received new port from Sink. Update it");
            pMMCfg->transport_capability_config.port1_id = mesg.wfd.client.getRtpPort0();
            pMMCfg->transport_capability_config.port1_rtcp_id = mesg.wfd.client.getRtcpPort0();
            pMMCfg->transport_capability_config.eRtpPortType = mesg.wfd.client.getTCP()?RTP_PORT_TCP:RTP_PORT_UDP;
            eventMMUpdate(BUFFERING_NEGOTIATION,MM_STATUS_SUCCESS,
                          mesg.wfd.client.getTCP()? TRANSPORT_TCP: TRANSPORT_UDP,
                          mesg.wfd.client.getTCP()? TRANSPORT_UDP: TRANSPORT_TCP);
            LOGD("Buffering negotiation success");
        }

        if (mesg.wfd.buffLen.getValid())
        {
            if (SOURCE != pSM->pMyDevice->getDeviceType())
            {
                MMAdaptor::streamControl(WFD_DEVICE_PRIMARY_SINK, AV_CONTROL_SET_DECODER_LATENCY,
                                                (uint64)mesg.wfd.buffLen.getBufferLen());
            }
            else
            {
                eventMMUpdate(BUFFERING_CONTROL_EVENT_DECODER_LATENCY,MM_STATUS_SUCCESS,0,0);
                LOGD("Set decoder latency success");
            }
        }
    }

    /* When source receives wfd_idr request, call MM lib to send idr frame */
    if ( mesg.wfd.idrReq.getValid() && (pSM->pMyDevice->getDeviceType() == SOURCE) &&
         RTSPSession::m_eplayMode != AUDIO_ONLY ) {
        MMAdaptor::sendIDRFrame();
    }



    /* Notify UIBCManager (in SessionManagerA) of UIBC enable/disable event */
    if (mesg.wfd.uibcSet.getValid()) {
        if (mesg.wfd.uibcSet.getSetting()) {
            // send UIBC enable event
            eventUIBCEnabled(pRtspSession->rtspSessionId);
        } else {
            // send UIBC disable event
            eventUIBCDisabled(pRtspSession->rtspSessionId);
        }
    }

    /* Capability renegotiation request*/
    if (mesg.wfd.timing.getValid()) {
        // Currently we support only video frame rate change.
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->configure(mesg.wfd);
        MMAdaptor::updateSession(pRtspSession->pPeerDevice->pNegotiatedMMCapability);
    }

    /* STANDBY request */
    if (mesg.wfd.halt.getValid()) {
        pRtspSession->rtspStateTransition(STANDBY);
        pauseCallback(mesg);
    }

    //B3
    MMEventStatusType status = MM_STATUS_SUCCESS;

    if(pSM->pMyDevice->getDeviceType() == SOURCE)
    {
        if(mesg.wfd.tcpStreamControl.getValid()) {
            LOGE("TCP Buffer Control Event");
            rtsp_wfd::tcp_control_command eCmd;

            eCmd = mesg.wfd.tcpStreamControl.getCommand();

            MMEventType eEvent = NO_EVENT;
            switch(eCmd) {
            case rtsp_wfd::flush:
                eEvent = BUFFERING_CONTROL_EVENT_FLUSH;
                LOGE("TCP Buffer Control Event FLush Success");
                break;
            case rtsp_wfd::pause:
                eEvent = BUFFERING_CONTROL_EVENT_PAUSE;
                LOGE("TCP Buffer Control Event Pause Success");
                break;
            case rtsp_wfd::play:
                eEvent = BUFFERING_CONTROL_EVENT_PLAY;
                LOGE("TCP Buffer Control Event Play success");
                break;
            case rtsp_wfd::status:
                eEvent = BUFFERING_CONTROL_EVENT_STATUS;
                LOGE("TCP Buffer Control Event Status success");
                break;
            default:
                LOGE("TCP Buffer Control Event invalid event");
                break;
            }

            int BuffLen = 0;
            int WindowSize = 0;

            if(mesg.wfd.tcpWindowSize.getValid()) {
                WindowSize = mesg.wfd.tcpWindowSize.getWindowSize();
            }

            if(mesg.wfd.buffLen.getValid()) {
                BuffLen = mesg.wfd.buffLen.getBufferLen();
            }
            LOGE("TCP Buffer Control Event windowSize = %d, BuffLen = %d",
                 WindowSize, BuffLen);
            eventMMUpdate(eEvent,status, WindowSize, BuffLen);
        }
#ifdef LEGACY_TCP
        if(mesg.wfd.tcpWindowSize.getValid()) {
            eventMMUpdate(BUFFERING_STATUS_UPDATE,status,
                          mesg.wfd.buffLen.getBufferLen(),
                           mesg.wfd.tcpWindowSize.getWindowSize());
            LOGD("Buffering update");
        }

        if(mesg.wfd.buffLen.getValid()) {

            unsigned int portNum = mesg.wfd.client.getRtpPort0();
            if(portNum > 65536) {
                LOGD("Invalid port num in response");
                eventMMUpdate(BUFFERING_NEGOTIATION,MM_STATUS_FAIL,
                          0,0);
                return;
            }
            pRtspSession->pPeerDevice->pNegotiatedMMCapability->
                pCapability->transport_capability_config.port1_id = mesg.wfd.client.getRtpPort0();
            eventMMUpdate(BUFFERING_NEGOTIATION,MM_STATUS_SUCCESS,
                          0,0);
            LOGD("Buffering negotiation success");
        }
#endif
    } else {
        if(mesg.wfd.tcpStreamControl.getValid()) {
            LOGE("TCP Buffer Control Event");
            rtsp_wfd::tcp_control_command eCmd;

            eCmd = mesg.wfd.tcpStreamControl.getCommand();

            if (eCmd == rtsp_wfd::flush)
            {
                MMAdaptor::streamControl(WFD_DEVICE_PRIMARY_SINK, AV_CONTROL_FLUSH,
                                  (uint64)mesg.wfd.tcpStreamControl.getDuration());
            }
        }
    }

}

void cback::setupCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: setupCallback");
    printMesg(mesg);
    mesg.wfd.dump();

    SessionManager *pSM = SessionManager::Instance();
    if((pSM == NULL) || (pSM->pMyDevice == NULL) || (pSM->pMyDevice->pMMCapability == NULL))
        return;

    if (mesg.error != noError) {
        return;
    }

    /* If Audio Only or video only mode is enabled, update the audio/video
     * method to INVALID (since it is NON zero and needs to be set explicitly
     */
    if (RTSPSession::m_eplayMode == AUDIO_ONLY) {
      pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_method = WFD_VIDEO_INVALID;
    mesg.wfd.videoHeader.setValid(false);
    mesg.wfd.h264Cbp.setValid(false);
    mesg.wfd.h264Chi444p.setValid(false);
    mesg.wfd.h264Chp.setValid(false);
    }
    else if (RTSPSession::m_eplayMode == VIDEO_ONLY) {
      pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_method = WFD_AUDIO_INVALID;
    mesg.wfd.audioAac.setValid(false);
    mesg.wfd.audioDts.setValid(false);
    mesg.wfd.audioEac.setValid(false);
    mesg.wfd.audioLpcm.setValid(false);
    }

    MMCapability* pNegotiatedMMCapability = pRtspSession->pPeerDevice->pNegotiatedMMCapability;

    /* Configure transport parameters */
    pNegotiatedMMCapability->pCapability->transport_capability_config.port1_id = mesg.rtpPort0;
    pNegotiatedMMCapability->pCapability->transport_capability_config.port2_id = mesg.rtpPort1;

    if(mesg.wfd.client.getRtcpPort0()) {
       pNegotiatedMMCapability->pCapability->transport_capability_config.port1_rtcp_id = mesg.wfd.client.getRtcpPort0();
    }
    pNegotiatedMMCapability->pCapability->peer_ip_addrs.ipv4_addr1=
             inet_addr(pRtspSession->pPeerDevice->ipAddr.c_str());
    strlcpy((char*)pNegotiatedMMCapability->pCapability->peer_ip_addrs.device_addr1,
                    pRtspSession->pPeerDevice->macAddr.c_str(),
                     sizeof(pNegotiatedMMCapability->pCapability->peer_ip_addrs.device_addr1));


    if(SOURCE != pSM->pMyDevice->getDeviceType())
    {
      LOGD("Setting Video Surface");
      if(pSM->pMyDevice->pMMCapability->pCapability != NULL)
      {
          pNegotiatedMMCapability->pCapability->pSurface = getStreamingSurface();
          pNegotiatedMMCapability->pCapability->
                          content_protection_config.content_protection_ake_port
                = pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.
                           content_protection_ake_port;
          pNegotiatedMMCapability->pCapability->content_protection_config.
                           content_protection_capability
                = pSM->pMyDevice->pMMCapability->pCapability->
                                content_protection_config.content_protection_capability;
      }
      LOGD("Updating local sockets");
      pNegotiatedMMCapability->pCapability->transport_capability_config.rtpSock =
           pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.rtpSock;
      pNegotiatedMMCapability->pCapability->transport_capability_config.rtcpSock =
           pSM->pMyDevice->pMMCapability->pCapability->transport_capability_config.rtcpSock;
    }

    LOGD("Dump MM capability used for mm_create_session():");
    pRtspSession->pPeerDevice->pNegotiatedMMCapability->dump();

    LOGD("Create MM session with peerDevice:  MacAddr=%s  IP=%s  port=%d", pRtspSession->pPeerDevice->macAddr.c_str(), pRtspSession->pPeerDevice->ipAddr.c_str(), pNegotiatedMMCapability->pCapability->transport_capability_config.port1_id);

    /* Configure content protection parameters */
    LOGD("RTSP setupCallback:negotiated capability:HDCP port %d,version %d",
                                        pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_ake_port,
                                        ((pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_capability == 0) ?
                                         pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_capability :
                                        (pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_capability - 1)));

    /* If link protection is enforeced, when SOURCE supports HDCP and SINK does not supports HDCP,
       the session should not be established. */
    int nHDCPEnforced = 0;
    getCfgItem(HDCP_ENFORCED_KEY, &nHDCPEnforced);
    if (nHDCPEnforced && SOURCE == pSM->pMyDevice->getDeviceType() &&
          ( pSM->pMyDevice->pMMCapability->isHDCPVersionSupported(
              pSM->pMyDevice->pMMCapability->pCapability->content_protection_config.content_protection_capability
           )))
    {
       if (!(pNegotiatedMMCapability->isHDCPVersionSupported(
                pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_capability)) ||
                (pNegotiatedMMCapability->pCapability->content_protection_config.content_protection_ake_port == 0))
       {
          eventMMUpdate(HDCP_UNSUPPORTED_BY_PEER, MM_STATUS_FAIL, 0, 0);
          LOGE("WFD Sink Doesn't Support HDCP. Teardown Session");
          mesg.error = badStateError;
          return;
       }
    }

    /* start MM session */
    MMAdaptor::createSession(pNegotiatedMMCapability, pSM->pMyDevice->getDeviceType());

    if(pRtspSession->m_sLocalTransportInfo.rtpPort) {
        LOGE("Set RTP Source port num");
        mesg.wfd.server.setRtpPort0(pRtspSession->m_sLocalTransportInfo.rtpPort);
    }

    if(pRtspSession->m_sLocalTransportInfo.rtcpPort &&
       pNegotiatedMMCapability->pCapability->transport_capability_config.port1_rtcp_id) {
        LOGE("Set RTCP Source Port Num");
        mesg.wfd.server.setRtcpPort0(pRtspSession->m_sLocalTransportInfo.rtcpPort);
    }

}

void cback::keepAliveTimerCallback(void *pMe)
{
   cback *pCback = (cback*)pMe;
   LOGD("Callback: keepAliveTimerCallback");
   pCback->pRtspSession->sendWFDKeepAliveMsg();
   return;
}

void cback::playCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: playCallback");
    printMesg(mesg);

    SessionManager *pSM = SessionManager::Instance();

    if (!pRtspSession || !pSM)
    {
        return;
    }

    if (mesg.error == noErrorPreSendCmdNotify) {
       SessionManager *pSM = SessionManager::Instance();
       if (pSM && pSM->pMyDevice &&
           (pSM->pMyDevice->getDeviceType() == PRIMARY_SINK ||
            pSM->pMyDevice->getDeviceType() == SECONDARY_SINK) &&
           pRtspSession->pPeerDevice->pNegotiatedMMCapability->
           pCapability->transport_capability_config.eRtpPortType != RTP_PORT_TCP) {
          LOGD("Call prepare play to setup multimedia in sink");
          if(false == MMAdaptor::streamPlayPrepare())
          {
              LOGE("cback::playCallback : stream play prepare returns false, tearing down session");
              eventMMUpdate(MM_VIDEO_EVENT_FAILURE,MM_STATUS_RUNTIME_ERROR,0,0);
              return;
          }
       }
       return;
    }

    if (mesg.error != noError) {
        return;
    }

    if (pRtspSession->rtspState == STANDBY) {
        pRtspSession->rtspStateTransition(SESS_ESTABLISHED);
    }

    LOGD("rtspState = %d", pRtspSession->rtspState);
    if (pRtspSession->rtspState == SESS_ESTABLISHING) {
        pRtspSession->rtspStateTransition(SESS_ESTABLISHED);
    }
    if (pRtspSession->rtspState == SESS_ESTABLISHED) {
        eventStreamControlCompleted(PLAY, pRtspSession->rtspSessionId);
    }

    bool bRet = false;

    if (pSM->pMyDevice->getDeviceType() != SOURCE &&
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->
         pCapability->transport_capability_config.eRtpPortType == RTP_PORT_TCP)
    {
        bRet = MMAdaptor::streamControl(WFD_DEVICE_PRIMARY_SINK,
                                        AV_CONTROL_PLAY,
                                        1);//1 is for flushing before play
        eventStreamControlCompleted(PLAY_DONE, pSM->getRTSPSessionId());
    }
    else
    {
        bRet = MMAdaptor::streamPlay();
    }
    if(false == bRet)
    {
        LOGE("cback::playCallback : stream play returns false, tearing down session");
        eventMMUpdate(MM_VIDEO_EVENT_FAILURE,MM_STATUS_RUNTIME_ERROR,0,0);
        return;
    }


    /* RTSP keepalive */
    if(pSM->pMyDevice == NULL)
      return;
    if (pSM->pMyDevice->getDeviceType() == SOURCE) {
    LOGD("Creating timer for RTSP Keep Alive");
    if (m_hTimer == NULL) {
      MM_Timer_Create(WFD_SRC_RTSP_KEEPALIVE_INTERVAL,1,cback::keepAliveTimerCallback, (void*)this, &m_hTimer);
     }
    }
}

void cback::pauseCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: pauseCallback");

    SessionManager *pSM = SessionManager::Instance();

    if (!pRtspSession || !pSM)
    {
        return;
    }

    printMesg(mesg);

    if (mesg.error != noError) {
        return;
    }

    bool bRet = false;

    if (pSM->pMyDevice->getDeviceType() != SOURCE &&
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->
         pCapability->transport_capability_config.eRtpPortType == RTP_PORT_TCP)
    {
        bRet = MMAdaptor::streamControl(WFD_DEVICE_PRIMARY_SINK,
                                        AV_CONTROL_PAUSE,
                                        0);
    }
    else
    {
        bRet = MMAdaptor::streamPause();
    }
    if(false == bRet)
    {
        LOGE("cback::pauseCallback : stream pause returns false, tearing down session");
        eventMMUpdate(MM_VIDEO_EVENT_FAILURE,MM_STATUS_RUNTIME_ERROR,0,0);
        return;
    }

    if (pRtspSession->rtspState == SESS_ESTABLISHED) {
        eventStreamControlCompleted(PAUSE, pRtspSession->rtspSessionId);
    }

    if (pSM->pMyDevice->getDeviceType() != SOURCE &&
        pRtspSession->pPeerDevice->pNegotiatedMMCapability->
         pCapability->transport_capability_config.eRtpPortType == RTP_PORT_TCP)
    {
        eventStreamControlCompleted(PAUSE_DONE, pSM->getRTSPSessionId());
    }

    LOGD("rtspState = %d", pRtspSession->rtspState);
}

void cback::teardownCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: teardownCallback");
    printMesg(mesg);

    if (pRtspSession->rtspState == TEARING_DOWN)
      return;

    pRtspSession->rtspState = TEARING_DOWN;

    if(m_hTimer)
    {
        int ret = MM_Timer_Release(m_hTimer);
        LOGI("Keep alive timer release returned %d",ret);
        m_hTimer = NULL;
    }


        eventStreamControlCompleted(TEARDOWN, pRtspSession->rtspSessionId);
        /* destroy MM/UIBC session */
        MMAdaptor::destroySession();
        UIBCAdaptor::destroySession();
        LOGD("rtspState = %d", pRtspSession->rtspState);
        pRtspSession->stop();

}

void cback::closeCallback(rtspApiMesg &mesg)
{
    LOGD("Callback: closeCallback");
    printMesg(mesg);

    if (pRtspSession->rtspState == TEARING_DOWN)
      return;

    pRtspSession->rtspState = TEARING_DOWN;
    eventError("RTSPCloseCallback");
    if(m_hTimer) {
        int ret = MM_Timer_Release(m_hTimer);
        LOGI("Keep alive timer release returned %d",ret);
        m_hTimer = NULL;
    }
    /* destroy MM/UIBC session */
    MMAdaptor::destroySession();
    UIBCAdaptor::destroySession();

    LOGD("rtspState = %d", pRtspSession->rtspState);
    pRtspSession->stop();
}





AVPlaybackMode RTSPSession::m_eplayMode = AUDIO_VIDEO;


RTSPSession::RTSPSession(WFDSession* pWfdSession, Device* pDev) {

    server = NULL;
    client = NULL;
    pWFDSession = pWfdSession;
    pPeerDevice = pDev;
    rtspState = STOPPED;
    rtspSessionId = -1;
    m_sLocalTransportInfo = {0};
    m_bUIBCSupported = false;
    m_bTCPSupportedAtSink = false;
    m_bTCPSupportQueried = false;
    m_bTCPSupportStatusRequested = false;
    events = new cback(this);
}


RTSPSession::~RTSPSession() {
    delete events;
}



void* RTSPSession::rtspServerLoopFunc(void *s)
{
    rtspServer* server = (rtspServer*)s;
    int tid = androidGetTid();
    LOGD("WFDD: RTSP thread Priority before = %d",androidGetThreadPriority(tid));
    androidSetThreadPriority(0,WFD_MM_RTSP_THREAD_PRIORITY);
    LOGD("WFDD: RTSP thread Priority after = %d",androidGetThreadPriority(tid));

    LOGD("Start rtspServer loop.");
    server->eventLoop();
    LOGD("Exit rtspServer loop.");
    delete server;
    return NULL;
}

void* RTSPSession::rtspClientLoopFunc(void *c)
{
    rtspClient* client = (rtspClient*)c;
    LOGD("Start rtspClient loop.");
    client->eventLoop();
    LOGD("Exit rtspClient loop.");
    delete client;
    return NULL;
}



bool RTSPSession::startServer(string ipAddr, int rtspPort, int uibcPort) {

    if (server != NULL) {
        return false;
    }

    server = new rtspServer(ipAddr, events, "", rtspPort, uibcPort, rtsp_wfd::source);
    if (server->createServer() < 0) {
        return false;
    }

    pthread_create(&session_thread, NULL, &RTSPSession::rtspServerLoopFunc, (void *)server);
    rtspStateTransition(STARTING); //RTSP server has started at this point

    return true;
}

bool RTSPSession::startClient(string ipAddr, int rtpPort0, int rtpPort1, int rtspPort, int hdcpPort) {

    if (client != NULL) {
        return false;
    }

    client = new rtspClient(rtpPort0, rtpPort1, hdcpPort,events, "", rtspPort, rtsp_wfd::sink, "");
    if (client->startClient(ipAddr) < 0) {
        return false;
    }

    pthread_create(&session_thread, NULL, &RTSPSession::rtspClientLoopFunc, (void *)client);

    return true;
}

void RTSPSession::rtspStateTransition(RTSPState newState) {
    if (rtspState != newState) {
        LOGD("RTSPSession state transition: %d --> %d  (sessionId=%d)", rtspState, newState, rtspSessionId);
        rtspState = newState;

        pWFDSession->updateSessionState();
    }
}


void RTSPSession::play() {
    if (server != NULL) {
        server->Play(rtspSessionId);
    } else if (client != NULL) {
        client->Play(rtspSessionId);
    } else {
        LOGE("Invalid RTSP session.");
    }
}

void RTSPSession::pause() {
    if (server != NULL) {
        server->Pause(rtspSessionId);
    } else if (client != NULL) {
        client->Pause(rtspSessionId);
    } else {
        LOGE("Invalid RTSP session.");
    }
}

void RTSPSession::teardown() {
    int err = 0;
    if (server != NULL) {
        err = server->Teardown(rtspSessionId);
    } else if (client != NULL) {
        err = client->Teardown(rtspSessionId);
    } else {
        LOGE("Invalid RTSP session.");
    }
    LOGE("Teardown return Code = %d", err);

    if(err < 0) {
        LOGE("Teardown Failed. Trying to Stop RTSP");
        stop();
    }
}

void RTSPSession::stop() {
    if (server != NULL) {
        LOGD("Stopping rtspServer");
        server->Stop();
    } else if (client != NULL) {
        LOGD("Stopping rtspClient");
        client->Stop();
    } else {
        LOGE("Invalid RTSP session.");
    }
}

bool RTSPSession::enableUIBC(bool enabled) {
    if(m_bUIBCSupported) {
       rtspWfd myWfd;
       myWfd.uibcSet.setSetting(enabled);
       if (server != NULL) {
          server->Set(rtspSessionId, myWfd);
       } else if (client != NULL) {
          client->Set(rtspSessionId, myWfd);
       } else {
          LOGE("Invalid RTSP session.");
          return false;
       }
     } else {
        LOGE("UIBC is not supported for session");
        return false;
     }
     LOGD("enableUIBC %d successful ",enabled);
     return true;
}


bool RTSPSession::getNegotiatedResolution(short int* pWidth, short int* pHeight) {
    vector<WFDSession*>::iterator it;
    SessionManager *pSM = SessionManager::Instance();
    if(pSM && pWidth && pHeight) {
      for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
          WFDSession* wfdSession = *it;
          RTSPSession* rtspSession = wfdSession->getRtspSession();
          if (rtspSession != NULL) {
              (*pWidth) = rtspSession->pPeerDevice->pNegotiatedMMCapability->\
                pCapability->video_config.video_config.h264_codec[0].max_hres;
              (*pHeight) = rtspSession->pPeerDevice->pNegotiatedMMCapability->\
                pCapability->video_config.video_config.h264_codec[0].max_vres;
              return true;
          }
      }//end for loop
    }
    return false;
}

void RTSPSession::streamControl(int rtspSessId, StreamCtrlType cmdType) {
    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL && rtspSession->rtspSessionId == rtspSessId) {
            switch (cmdType) {
            case PLAY:
                rtspSession->play();
                break;
            case PAUSE:
                rtspSession->pause();
                break;
            case TEARDOWN:
                rtspSession->teardown();
                break;
            default:
                break;
            }
            break;
        }
    }
}


bool RTSPSession::uibcControl(int rtspSessId, bool enabled) {
    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL && rtspSession->rtspSessionId == rtspSessId) {
                 LOGE("Valid RTSP session.");
            return rtspSession->enableUIBC(enabled);
        }
    }
    return false;
}

void RTSPSession::Cleanup()
{
    LOGD("RTSP Cleanup");

    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();
    RTSPSession *pRtspSession = NULL;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        if (it != NULL) {
           WFDSession* wfdSession = *it;
           if (wfdSession) {
              pRtspSession = wfdSession->getRtspSession();
           }

           if (pRtspSession != NULL) {
               LOGE("Valid RTSP session.");
               break;
           }
        }
    }
    if (pRtspSession)
    {

      if (pRtspSession->rtspState == TEARING_DOWN)
        return;

       pRtspSession->rtspState = TEARING_DOWN;


        /* destroy MM/UIBC session */
        MMAdaptor::destroySession();
        UIBCAdaptor::destroySession();
        LOGD("rtspState = %d", pRtspSession->rtspState);
        pRtspSession->stop();
    }
}

bool RTSPSession::setUIBC(int rtspSessId) {
    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();
    if(!pSM) {
       LOGE("SessionManager instance is null!!");
       return false;
    }
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* pRtspSession = wfdSession->getRtspSession();
        if (pRtspSession && pRtspSession->m_bUIBCSupported) {
            rtspWfd myWfd;
            myWfd.uibcCap.setValid(true);
            string uibcParam ("wfd_uibc_capability");
            if (pRtspSession->server != NULL) {
                rtspWfdParams type;
                bool isParamSet = false;
                uibcParam += string(": ") + string(pRtspSession->pPeerDevice->\
                    pNegotiatedMMCapability->getKeyValue((char*)uibcParam.c_str()));
                if ((type=myWfd.wfdType(uibcParam, isParamSet)) != wfd_invalid) {
                    if (isParamSet) {
                        myWfd.wfdParse(type, uibcParam);
                    } else {
                        LOGE("Something fishy!! UIBC Param not found!!");
                        return false;
                    }
                } else {
                    LOGE("Something amiss!! UIBC parameter is invalid!!");
                    return false;
                }
                LOGE("Configured RTSP M14 Message from Source");
                myWfd.dump();
                pRtspSession->server->Set(pRtspSession->rtspSessionId, myWfd);
            } else if (pRtspSession->client != NULL) {
                //pRtspSession->client->Set(pRtspSession->rtspSessionId, myWfd);
                //TODO Implement M14 for sink side
                return false;//For now
            }
        } else if (!pRtspSession){
            LOGE("No valid RTSP session.");
            return false;
        } else if (!(pRtspSession->m_bUIBCSupported)) {
            LOGE("UIBC not supported in current session");
            return false;
        }
        break;
    }
    return true;
}
bool RTSPSession::standby(int rtspSessId) {
    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* pRtspSession = wfdSession->getRtspSession();
        if (pRtspSession != NULL) {
            rtspWfd myWfd;
            if (pRtspSession->server != NULL
                &&
                pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->standby_resume_support) {
                myWfd.halt.setValid(true);
                pRtspSession->server->Set(pRtspSession->rtspSessionId, myWfd);
            } else if (pRtspSession->client != NULL
                       &&
                       pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->standby_resume_support) {
                myWfd.halt.setValid(true);
                pRtspSession->client->Set(pRtspSession->rtspSessionId, myWfd);
            } else if (!(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->standby_resume_support)){
                LOGE("No standby_resume_support ");
                return false;
            } else {
                LOGE("No valid RTSP session in progress");
                return false;
            }
            break;
        }
    }
    return true;
}

void RTSPSession::UpdateLocalTransportInfo(
    localTransportInfo *pTransport){
    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        LOGD("RTSPSession:: Update local rtp port numbers %d, %d",
             pTransport->rtpPort, pTransport->rtcpPort);
        if (rtspSession != NULL) {
            rtspSession->m_sLocalTransportInfo.rtpPort =
                               pTransport->rtpPort;
            rtspSession->m_sLocalTransportInfo.rtcpPort =
                               pTransport->rtcpPort;

        }
    }

}


void RTSPSession::sendIDRRequest() {

    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            rtspWfd myWfd;
            myWfd.idrReq.setValid(true);
            if (rtspSession->server != NULL) {
                rtspSession->server->Set(rtspSession->rtspSessionId, myWfd);
            } else if (rtspSession->client != NULL) {
                rtspSession->client->Set(rtspSession->rtspSessionId, myWfd);
            } else {
                LOGE("No valid RTSP session.");
            }
            break;
        }
    }
}

bool RTSPSession::setResolution (int type, int value, int *resParams)
{
  LOGD ("RTSPSession::setResolution");

  SessionManager *pSM                = SessionManager::Instance();
  MMCapability *commonCapability     = NULL;
  MMCapability *negotiatedCapability = NULL;
  bool bUpdate                       = false;
  CapabilityType resolutionType      = (CapabilityType)type;

  vector<WFDSession*>::iterator it;
  for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++)
  {
    WFDSession* wfdSession = *it;
    if (wfdSession == NULL)
    {
      LOGE("No Active WFD Session, can't set resolution");
      return false;
    }
    RTSPSession* rtspSession = wfdSession->getRtspSession();
    if (rtspSession != NULL)
    {
      commonCapability = rtspSession->pPeerDevice->pCommonCapability;
      negotiatedCapability = rtspSession->pPeerDevice->pNegotiatedMMCapability;
      switch (resolutionType)
      {
        case WFD_CEA_RESOLUTIONS_BITMAP:
          if (commonCapability->pCapability->video_config.video_config.h264_codec->supported_cea_mode & value)
          {
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_cea_mode = value;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_vesa_mode = 0;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_hh_mode = 0;
            bUpdate = true;
          }
          LOGE("Update CEA resolution");
          break;
        case WFD_VESA_RESOLUTIONS_BITMAP:
          if (commonCapability->pCapability->video_config.video_config.h264_codec->supported_vesa_mode & value)
          {
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_cea_mode = 0;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_vesa_mode = value;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_hh_mode = 0;
            bUpdate = true;
          }
          LOGE("Update VESA resolution");
          break;
        case WFD_HH_RESOLUTIONS_BITMAP:
          if (commonCapability->pCapability->video_config.video_config.h264_codec->supported_hh_mode & value)
          {
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_cea_mode = 0;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_vesa_mode = 0;
            negotiatedCapability->pCapability->video_config.video_config.h264_codec->supported_hh_mode = value;
            bUpdate = true;
          }
          LOGE("Update HH resolution");
          break;
        default:
          LOGE("Unknown resolution type");
      }
      if (bUpdate && MMAdaptor::updateSession(negotiatedCapability))
      {
        if(resParams)
        {
          //resParams has width at [0], height at [1] and FPS at [2]
          negotiatedCapability->pCapability->video_config.video_config.h264_codec->max_hres = resParams[0];
          negotiatedCapability->pCapability->video_config.video_config.h264_codec->max_vres = resParams[1];
        }
        return true;
      }
    } //if (rtspSession != NULL)

  } // end for
  return false;

}


void RTSPSession::sendTransportChangeRequest(int TransportType, int BufferLenMs, int portNum)
{
    LOGE("RTSPSession: sendTransportChangeRequest");

    SessionManager *pSM = SessionManager::Instance();

    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            rtspWfd myWfd;
            LOGE("Callback: sendTransportUpdate %d", (int)rtspSession);
            if (rtspSession->server != NULL) {
                LOGE("Calling Set: sendTransportUpdate");
                if(TransportType == 1) {
#ifdef LEGACY_TCP
                    myWfd.buffLen.setValid(true);
                    myWfd.buffLen.setBufferLen(BufferLenMs);
#else
                    myWfd.tcpWindowSize.setValid(true);
                    myWfd.tcpWindowSize.setWindowSize(BufferLenMs);
#endif
                    myWfd.client.setValid(true);
                    if(rtspSession->m_sLocalTransportInfo.rtpPort) {
                        myWfd.client.setRtpPort0(rtspSession->m_sLocalTransportInfo.rtpPort);
                    }

                    if(rtspSession->m_sLocalTransportInfo.rtcpPort) {
                        myWfd.client.setRtcpPort0(rtspSession->m_sLocalTransportInfo.rtcpPort);
                    }
                    myWfd.client.setTCP(true);
                }
                else
                {
                    myWfd.client.setValid(true);
                    if(rtspSession->m_sLocalTransportInfo.rtpPort) {
                        myWfd.client.setRtpPort0(rtspSession->m_sLocalTransportInfo.rtpPort);
                    }

                    if(rtspSession->m_sLocalTransportInfo.rtcpPort) {
                        myWfd.client.setRtcpPort0(rtspSession->m_sLocalTransportInfo.rtcpPort);
                    }
                    myWfd.client.setTCP(false);
                }
                rtspSession->server->Set(rtspSession->rtspSessionId, myWfd);
            } else if (rtspSession->client != NULL) {
                LOGE("Invalid request for client");
            } else {
                LOGE("No valid RTSP session.");
            }
            break;
        }
    }
    return;
}

void RTSPSession::setDecoderLatency(int latency)
{
    LOGE("RTSPSession: setDecoderLatency");

    SessionManager *pSM = SessionManager::Instance();

    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            rtspWfd myWfd;
            LOGE("Callback: setDecoderLatency %d", (int)rtspSession);
            if (rtspSession->server != NULL) {
                LOGE("Calling Set: setDecoderLatency");
                myWfd.buffLen.setValid(true);
                myWfd.buffLen.setBufferLen(latency);
                rtspSession->server->Set(rtspSession->rtspSessionId, myWfd);
            } else if (rtspSession->client != NULL) {
                LOGE("Invalid request for client");
            } else {
                LOGE("No valid RTSP session.");
            }
            break;
        }
    }
    return;
}

void RTSPSession::sendBufferingControlRequest(int ControlType, int cmdVal)
{
    LOGE("RTSPSession: sendBufferingControlRequest");

    SessionManager *pSM = SessionManager::Instance();

    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            rtspWfd myWfd;
            LOGE("Callback: sendBufferingControl %d", (int)rtspSession);
            if (rtspSession->server != NULL) {
                LOGE("Calling Set: sendBufferingControl");
                rtsp_wfd::tcp_control_command eCmd = rtsp_wfd::cmdNone;
                uint32 flushTimeStamp = 0;
                switch((ControlCmdType)ControlType) {
                case TCP_FLUSH:
                    LOGE("Fetching buffering timestamp");
                    flushTimeStamp = MMAdaptor::getCurrentPTS();
                    myWfd.tcpStreamControl.setDuration(flushTimeStamp);
                    LOGE("Fetching flush timestamp from rtsp = %lu",myWfd.tcpStreamControl.getDuration());
                    eCmd = rtsp_wfd::flush;
                    LOGE("Send Buffering Control FLUSH");
                    break;
                case TCP_PLAY:
                    eCmd = rtsp_wfd::play;
                    LOGE("Send Buffering Control PLAY");
                    break;
                case TCP_PAUSE:
                    eCmd = rtsp_wfd::pause;
                    LOGE("Send Buffering Control PAUSE");
                    break;
                case TCP_STATUS:
                    eCmd = rtsp_wfd::status;
                    LOGE("Send Buffering control STATUS");
                    break;
                default:
                    LOGE("Invalid Buffering Control Cmd");
                    eCmd = rtsp_wfd::cmdNone;
                    break;

                }

                if(eCmd == rtsp_wfd::cmdNone) {
                    return;
                }

                myWfd.tcpStreamControl.setCommand(eCmd);
                rtspSession->server->Set(rtspSession->rtspSessionId, myWfd);
            } else if (rtspSession->client != NULL) {
                LOGE("Invalid request for client");
            } else {
                LOGE("No valid RTSP session.");
            }
            break;
        }
    }
    return;
}


void RTSPSession::queryTCPTransportSupport()
{
    LOGD("RTSPSession: queryTCPTransportSupport");

    SessionManager *pSM = SessionManager::Instance();

    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            if (rtspSession->m_bTCPSupportQueried) {
                if (rtspSession->m_bTCPSupportedAtSink) {
                    eventMMUpdate(MM_RTP_EVENT_TCP_SUPPORTED_BY_SINK,MM_STATUS_SUCCESS,
                      0,0);
                }else{
                    eventMMUpdate(MM_RTP_EVENT_TCP_SUPPORTED_BY_SINK,MM_STATUS_FAIL,
                      0,0);
                }
            } else{
                rtspSession->m_bTCPSupportStatusRequested = true;
            }
            break;
        }
    }
}


void RTSPSession::setTransport (int value)
{
  LOGD ("RTSPSession::setResolution");

  SessionManager *pSM                = SessionManager::Instance();
  MMCapability *negotiatedCapability = NULL;
  bool bUpdate                       = false;

  vector<WFDSession*>::iterator it;
  for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++)
  {
    WFDSession* wfdSession = *it;
    if (wfdSession == NULL)
    {
      LOGE("No Active WFD Session, can't set transport");
      return;
    }
    RTSPSession* rtspSession = wfdSession->getRtspSession();
    if (rtspSession != NULL)
    {
      negotiatedCapability = rtspSession->pPeerDevice->pNegotiatedMMCapability;
    }
    if((negotiatedCapability!=NULL) && (negotiatedCapability->pCapability!=NULL))
    {
        negotiatedCapability->pCapability->transport_capability_config.eRtpPortType =
                (value == 0)? RTP_PORT_UDP:RTP_PORT_TCP;
        LOGE("Update Transport");
        if (MMAdaptor::updateSession(negotiatedCapability))
        {
            LOGE("succesfully set Transport");
            return;
        }
    }
  } // end for
  return;

}

void RTSPSession::sendWFDKeepAliveMsg()
{
    LOGD("RTSPSession: sendWFDKeepAliveMsg");

    SessionManager *pSM = SessionManager::Instance();

    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession != NULL) {
            rtspWfd myWfd;
            LOGD("Callback: sendWFDKeepAliveMsg %d", (int)rtspSession);
            if (rtspSession->server != NULL) {
                LOGD("Calling Get: sendWFDKeepAliveMsg");
                rtspSession->server->Get(rtspSession->rtspSessionId, myWfd);
            } else if (rtspSession->client != NULL) {
                rtspSession->client->Get(rtspSession->rtspSessionId, myWfd);
            } else {
                LOGE("No valid RTSP session.");
            }
            break;
        }
    }
}

void RTSPSession::sendAVFormatChangeRequest() {

    vector<WFDSession*>::iterator it;

    SessionManager *pSM = SessionManager::Instance();
    if((pSM == NULL) || (pSM->pMyDevice == NULL) || (pSM->pMyDevice->pMMCapability == NULL))
        return;

    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        RTSPSession* pRtspSession = wfdSession->getRtspSession();
        if (pRtspSession != NULL) {
            rtspWfd myWfd;
            // query the new proposed capability from MM
            if(MMAdaptor::getProposedCapability(pSM->pMyDevice->pMMCapability, pRtspSession->pPeerDevice->pMMCapability, pRtspSession->pPeerDevice->pNegotiatedMMCapability))
            {
                #if 1
                // Hack to wipe out other audio codecs other than the negotiated one
                switch (pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_method) {
                    case WFD_AUDIO_LPCM:
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec));
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec));
                        break;
                    case WFD_AUDIO_AAC:
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec));
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.dolby_digital_codec));
                        break;
                    case WFD_AUDIO_DOLBY_DIGITAL:
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.lpcm_codec));
                        memset(&(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec), 0, sizeof(pRtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->audio_config.aac_codec));
                        break;
                    default:
                        break;
                }
                #endif

                string params[2] = {
                    "wfd_audio_codecs",
                    "wfd_video_formats"
                };
                char buffer[200];
                rtspWfdParams type;
                bool isParamSet;
                for (int i=0; i<2; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    strlcat(buffer, params[i].c_str(), sizeof(buffer));
                    strlcat(buffer, ": ", sizeof(buffer));
                    strlcat(buffer, pRtspSession->pPeerDevice->pNegotiatedMMCapability->getKeyValue((char*)params[i].c_str()), sizeof(buffer));
                    params[i] = buffer;

                    isParamSet = false;
                    if ((type=myWfd.wfdType(params[i], isParamSet)) != wfd_invalid) {
                        if (isParamSet) {
                            myWfd.wfdParse(type, params[i]);
                        }
                    }
                }

                // query PTS/DTS from MM lib
                unsigned long int pts, dts;
                MMAdaptor::getAVFormatChangeTiming(&pts, &dts);
                myWfd.timing.setPts(pts);
                myWfd.timing.setDts(dts);
                myWfd.timing.setValid(true);

                if (pRtspSession->server != NULL) {
                    pRtspSession->server->Set(pRtspSession->rtspSessionId, myWfd);
                } else if (pRtspSession->client != NULL) {
                    pRtspSession->client->Set(pRtspSession->rtspSessionId, myWfd);
                } else {
                    LOGE("No valid RTSP session.");
                }
                break;
        }
        }
    }
}

bool RTSPSession::setAVPlaybackMode (AVPlaybackMode mode) {

  bool ret = false;
  //TODO , you can't set the playback mode at any time
  //Add checks for state when this is being called
  switch (mode) {

  case  AUDIO_ONLY:
  case  VIDEO_ONLY:
  case  AUDIO_VIDEO:
        m_eplayMode = mode;
        ret = true;
        break;
  case NO_AUDIO_VIDEO:
        // What are we supposed to do with this? May be only UIBC session?
        ret = false;

  }
  return ret;
}

void RTSPSession::setSinkSurface (void* surface) {
  SessionManager *pSM = SessionManager::Instance();
  MMCapability *negotiatedCapability = NULL;

  vector<WFDSession*>::iterator it;
  for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++)
  {
    WFDSession* wfdSession = *it;
    if (wfdSession == NULL)
    {
      LOGE("No Active WFD Session, can't set Sink Surface");
      return;
    }
    RTSPSession* rtspSession = wfdSession->getRtspSession();
    if (rtspSession != NULL && rtspSession->pPeerDevice != NULL)
    {
      negotiatedCapability = rtspSession->pPeerDevice->pNegotiatedMMCapability;
    }
    if((negotiatedCapability!=NULL) && (negotiatedCapability->pCapability!=NULL))
    {
        negotiatedCapability->pCapability->pSurface =  surface;
        LOGE("Update Surface");
        if (MMAdaptor::updateSession(negotiatedCapability))
        {
            LOGE("succesfully set surface");
            return;
        }
    }
  } //for loop end
  return;
}

uint32_t* RTSPSession::getCommonResloution(int* numProf) {
    LOGD("getCommonResloution");
    SessionManager *pSM = SessionManager::Instance();
    if(!pSM) {
       LOGE("SessionManager instance is null!!");
       return NULL;
    }
    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        if (wfdSession == NULL) {
          LOGE("No Active WFD Session, can't get Common Resloution");
          return NULL;
        }
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession == NULL || rtspSession->pPeerDevice == NULL ||
            rtspSession->pPeerDevice->pCommonCapability == NULL ||
            rtspSession->pPeerDevice->pCommonCapability->pCapability == NULL ||
            rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.h264_codec == NULL) {
          LOGE("Something's wrong. Can't get Common Resloution");
          return NULL;
        }
        /*For each of the H264 profiles there will be 3 values to consider CEA,
         *VESA and HH. So create an array with a size of multiple of
         * 4 = [1Profile + its 3 corresponding bitmaps]
         */
        *numProf = rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.num_h264_profiles;
        uint32_t* comRes = new uint32_t[4*(*numProf)];
        for(int i =0;i<*numProf;i++) {
            comRes[4*i]= rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.h264_codec[i].h264_profile;
            comRes[4*i +1] = rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.h264_codec[i].supported_cea_mode;
            comRes[4*i +2] = rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.h264_codec[i].supported_vesa_mode;
            comRes[4*i +3] = rtspSession->pPeerDevice->pCommonCapability->pCapability->video_config.video_config.h264_codec[i].supported_hh_mode;
            LOGD("For profile %d CEA mode is %u, VESA mode is %u, HH mode is %u",comRes[4*i],comRes[4*i +1],comRes[4*i +2],comRes[4*i +3]);
        }
        return comRes;
    }
    return NULL;
}

uint32_t* RTSPSession::getNegotiatedResloution() {
    LOGD("getNegotiatedResloution");
    SessionManager *pSM = SessionManager::Instance();
    if(!pSM) {
       LOGE("SessionManager instance is null!!");
       return NULL;
    }
    vector<WFDSession*>::iterator it;
    for (it=pSM->vecWFDSessions.begin(); it!=pSM->vecWFDSessions.end(); it++) {
        WFDSession* wfdSession = *it;
        if (wfdSession == NULL) {
          LOGE("No Active WFD Session, can't get Common Resloution");
          return NULL;
        }
        RTSPSession* rtspSession = wfdSession->getRtspSession();
        if (rtspSession == NULL || rtspSession->pPeerDevice == NULL ||
            rtspSession->pPeerDevice->pNegotiatedMMCapability == NULL ||
            rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability == NULL ||
            rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_config.video_config.h264_codec == NULL) {
          LOGE("Something's wrong. Can't get Negotiated Resloution");
          return NULL;
        }
        /*For the H264 profile there will be 3 values to consider CEA,VESA & HH
         *Create array of size 4 = [1Profile + its 3 corresponding bitmaps]
         */
        uint32_t* negRes = new uint32_t[4];
        negRes[0] = rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_config.video_config.h264_codec[0].h264_profile;
        negRes[1] = rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_config.video_config.h264_codec[0].supported_cea_mode;
        negRes[2] = rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_config.video_config.h264_codec[0].supported_vesa_mode;
        negRes[3] = rtspSession->pPeerDevice->pNegotiatedMMCapability->pCapability->video_config.video_config.h264_codec[0].supported_hh_mode;
        LOGD("For profile %d CEA mode is %u, VESA mode is %u, HH mode is %u",negRes[0],negRes[1],negRes[2],negRes[3]);
        return negRes;
    }
    return NULL;
}
MMEventStatusType RTSPSession::updateHdcpSupportedInConnectedDevice(void *pCfgCababilities)
{

  MMEventStatusType nStatus = MM_STATUS_SUCCESS;
#ifdef HDCP_DISPLAY_ENABLED
  WFD_MM_capability_t *pReadCababilities = (WFD_MM_capability_t *)pCfgCababilities;

  LOGV("Disp HDCP Manager Init");

  int ret = 0;
  ret = HDCP1X_COMM_Init(&RTSPSession::DISPhdcpCallBack);
  if (ret)
  {
     LOGE("Fail Initializing disp hdcp manager!");
     return MM_STATUS_FAIL;
  }
  else
  {
     LOGD("Initializing disp hdcp manager successful!");
  }


  bHDCPStatus = MM_STATUS_INVALID;

  ret = HDCP1X_COMM_Send_hdcp2x_event( EV_REQUEST_TOPOLOGY, NULL, NULL);

  if (ret)
  {
     bHDCPStatus = MM_STATUS_FAIL;
     LOGE("Fail sending request to disp hdcp manager!");

  }
  else
  {
     LOGD("Request_topology event to disp hdcp manager success!");
  }

  // wait here for the CB from DISPLAY.
  while (true)
  {
     if (bHDCPStatus == MM_STATUS_CONNECTED)
     {
        nStatus = MM_STATUS_SUCCESS;
        LOGE("HDCP Content protection is supprted in the connected device");
        break;
     }
     else if ((bHDCPStatus == MM_STATUS_NOTSUPPORTED) ||
              (bHDCPStatus == MM_STATUS_FAIL)) // FAIL means send req faild to display,
     {
       nStatus = bHDCPStatus;
       if (pReadCababilities)
       {
         // over write the config file read values wtih content protection not supported
         pReadCababilities->content_protection_config.content_protection_capability = 0;
         pReadCababilities->content_protection_config.content_protection_ake_port = 0;

         LOGD("HDCP Content protection is not supprted in the connected device");
       }
       break;
     }
     else
     {
       // wait for Display to notify us on the connected/authenticated status.
       MM_Timer_Sleep(5);
     }
  }

  //!Warning: Need to Enable
  ret = HDCP1X_COMM_Term();
  if (ret)
     LOGE("fail DeInit disp hdcp manager!");
  else
     LOGD("DeInit disp hdcp manager successful!");


  bHDCPStatus = MM_STATUS_INVALID;
#endif
  return nStatus;
}


int RTSPSession::DISPhdcpCallBack(int type, void *param)
{
    int ret = 0, *temp = NULL;
    //struct HDCP_V2V1_DS_TOPOLOGY *msg = NULL;
#ifdef HDCP_DISPLAY_ENABLED
    switch (type) {
    case EV_REQUEST_TOPOLOGY:
    case EV_SEND_TOPOLOGY:

      // supports HDCP
      bHDCPStatus = MM_STATUS_CONNECTED;
      break;

    case EV_ERROR:
        temp = (int *)param;
        LOGE("Received error from hdcp manager, error = %d!",*temp);

        bHDCPStatus = MM_STATUS_NOTSUPPORTED;
        break;
    }
#endif
    return ret;
}

/*bool RTSPSession::isHDMIConnected()
{
  char *path = "/sys/class/graphics/fb1/connected";
  int connected = 0;
  int fd = open(path, O_RDONLY, 0);
  if(fd > 0)
  {
    int datasize = read(fd, connected, sizeof(int));
    LOGE("RTSPSession::isHDMIConnected() connected = %d!",connected);
  }
  return connected;
}*/

