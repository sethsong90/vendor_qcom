/*==============================================================================
*  @file wifidisplay.cpp
*
*  @par DESCRIPTION:
*       Wifi Display Native APIs
*       Interface between SM-B and SM-A JNI
*
*
*  Copyright (c) 2012 - 2013 by Qualcomm Technologies, Incorporated.
*  All Rights Reserved.
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
#define LOG_TAG "wifidisplay"

#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <utils/Log.h>
#ifndef WFD_ICS
#include "common_log.h"
#endif
#include "wifidisplay.h"
#include "wfd_util_mutex.h"

#include "SessionManager.h"
#include "WFDSession.h"
#include "RTSPSession.h"
#include "MMAdaptor.h"
#include "UIBCAdaptor.h"
#ifdef WFD_ICS
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#else
#include <Surface.h>
#endif

#define WIFIDISPLAY_STRING_ARR_SIZE 50
#define WIFIDISPLAY_STRING_SIZE     256
#define WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE (WIFIDISPLAY_STRING_SIZE - 1)
char strarray[WIFIDISPLAY_STRING_ARR_SIZE][WIFIDISPLAY_STRING_SIZE];

using namespace android;

void* m_pMutex= NULL;
static stringarray_callback gCallback = NULL;

/* TODO*/
static Surface* gSurface = NULL;

/*
 * Method:    enableWfd
 * This should be called first before calling any other WFD calls.
 */
int enableWfd (WfdDevice* thisDevice, stringarray_callback cb)
{
    MM_Debug_Initialize();
    LOGD("enableWfd called");
    if (cb != NULL) {
        gCallback = cb;
    }

    SessionManager *pSM = SessionManager::Instance();

    if (!pSM)
        return false;

    DeviceType tp = SOURCE;
    switch (thisDevice->deviceType) {
    case 0:
        tp = SOURCE;
        break;
    case 1:
        tp = PRIMARY_SINK;
        break;
    case 2:
        tp = SECONDARY_SINK;
        break;
    default:
        LOGE("Unknown devType: %d", thisDevice->deviceType);
        return false;
    }

    if (!m_pMutex && venc_mutex_create(&m_pMutex) != 0)
    {
      LOGE( "enableWfd: failed to init mutex");
    }
    LOGE( "enableWfd: created mutex %p",m_pMutex);

    Device *dev = new Device(string(thisDevice->macaddress), tp);
    dev->ipAddr  = string(thisDevice->ipaddress);
    dev->coupledPeerMacAddr = string(thisDevice->peermac);
    if (thisDevice->SMControlPort <= 0 ||
        thisDevice->SMControlPort > UINT16_MAX)
    {
      LOGE("Invalid SM Port: %d, Using default value %u",
           thisDevice->SMControlPort, SM_CONTROL_PORT);
      dev->sessionMngtControlPort = SM_CONTROL_PORT;
    }
    else
    {
      dev->sessionMngtControlPort = (unsigned short)thisDevice->SMControlPort;
    }

    LOGD("enable WFD for this device %s", dev->macAddr.c_str());
    pSM->enableWfd(dev);

    return true;
}

/*
 * Method:    getResolution
 *To retrieve the current resolution of the WFD session
 */
int getResolution(short int* pWidth, short  int* pHeight)
{
    return RTSPSession::getNegotiatedResolution(pWidth, pHeight);
}

/*
 * Method:    setUIBC
 *@Brief: To set UIBC parameters during WFD session using M14 message
 *@param : Session ID
 *@return : success/failure
*/
int setUIBC(int sessionId)
{
    return RTSPSession::setUIBC(sessionId);
}
/*
 * Method:    setSurface
 *
 */
void setSurface(void *surface)
{
    LOGD("setSurface called with value %p", surface);
    if (gSurface == NULL) {
        LOGD("gSurface was null");
    }
    gSurface = (Surface*) surface;
    RTSPSession::setSinkSurface(surface);
}

/*
 * Class:     com_qualcomm_wfd_WFDNative
 * Method:    disableWfd
 * Signature: ()Z
 */
int disableWfd ()
{
    LOGD("disableWfd called");

    SessionManager *pSM = SessionManager::Instance();

    if (pSM) {
        pSM->disableWfd();
        SessionManager::DeleteInstance();
        pSM = NULL;
    }

    if (gCallback != NULL)
        gCallback = NULL;

    if (venc_mutex_destroy(m_pMutex) != 0)
    {
     LOGE("disableWfd: failed to destroy mutex");
    }
    m_pMutex = NULL;
    gSurface = NULL;
    MM_Debug_Deinitialize();
    return true;
}

/*
 * Method:    startWfdSession
 *
 */
void startWfdSession (WfdDevice* pD)
{

    SessionManager *pSM = SessionManager::Instance();

    if (pSM) {
        if (false == pSM->startWfdSession(pD)){
            LOGE("Error in startWfdSession" );
            eventError("StartSessionFail");
        }
    }
}

void stopWfdSession (int sid)
{
  SessionManager *pSM = SessionManager::Instance();
  if (pSM)
  {
    pSM->stopWfdSession(sid);
  }
  else
  {
    LOGE("pSM is NULL");
  }
}

/*
 * Method:    play
 *
 */
void play_rtsp ( int sessionId, unsigned char secureFlag)
{
    LOGD("Play RTSP session.  Session id=%d, secureFlag = %d", sessionId, secureFlag);
    if (secureFlag) {
        MMAdaptor::streamPlay();
    } else {
        RTSPSession::streamControl(sessionId, PLAY);
    }
}

/*
 * Method:    pause
 *
 */
void pause_rtsp (int sessionId, unsigned char secureFlag)
{
    LOGD("Pause RTSP session.  sessionId=%d, secureFlag = %d", sessionId, secureFlag);
    if (secureFlag) {
        MMAdaptor::streamPause();
    } else {
        RTSPSession::streamControl(sessionId, PAUSE);
    }
}

/*
 * Method:    teardown
 *
 */
void teardown_rtsp   ( int sid, int isRTSP)
{
    if (isRTSP) {
    LOGD("Teardown RTSP session.  session id=%d", sid);
    RTSPSession::streamControl(sid, TEARDOWN);
    }
    else {
     RTSPSession::Cleanup();
    }
}

/*
 * Method:    standby
 *
 */
int standby_rtsp (int sessionId)
{
    LOGD("standby called");
    return RTSPSession::standby(sessionId);
}

/*
 * Method:    enableUIBC
 *
 */
int enableUIBC (int sessionId)
{
    // send RTSP request to enable UIBC
    LOGD("Enable UIBC.  RTSP sessionId=%d", sessionId);
    return RTSPSession::uibcControl(sessionId, true);
}

/*
 * Method:    disableUIBC
 *
 */
int disableUIBC (int sessionId)
{
    // send RTSP request to disable UIBC
    LOGD("Disable UIBC.  RTSP sessionId=%d", sessionId);
    return RTSPSession::uibcControl(sessionId, false);
}

/*
 * Method:    startUIBC
 *
 */
void startUIBC ()
{
    //LOGE("WFDNative_startUIBC");
    // call MM lib to start UIBC
    UIBCAdaptor::startUIBC();
}

/*
 * Method:    stopUIBC
 *
 */
void stopUIBC  ()
{
    // UIBC Adaptor to stop UIBC
    UIBCAdaptor::stopUIBC();
}

void registerUibcCallbacks(wfd_uibc_attach_cb Attach,
                           wfd_uibc_send_event_cb SendEvent,
                           wfd_uibc_hid_event_cb sendHIDEvent)
{
  UIBCAdaptor::registerUIBCCallbacks(Attach,SendEvent,sendHIDEvent);
}

/*
 * Method:    sendUIBCKeyEvent
 * arg event is of type WFD_uibc_event_t
 */
 int sendUIBCEvent
  (WFD_uibc_event_t* event)
{
    // send the captured Android event to peer device through MM lib

    return UIBCAdaptor::sendUIBCEvent(event);
}


int getRTSPSessionId()
{
   SessionManager *pSM = SessionManager::Instance();
    if (pSM)
        return pSM->getRTSPSessionId();
    else
        return 0;
}

void setCapability (int capType, void *value)
{
    SessionManager *pSM = SessionManager::Instance();
  if (pSM)
    pSM->setUserCapability(capType, value);

}

int setResolution (int type, int value, int* resParams)
{
  return RTSPSession::setResolution(type,value,resParams);
}

void setDecoderLatency (int latency)
{
  return RTSPSession::setDecoderLatency(latency);
}

void setRtpTransport (int transportType)
{
  return RTSPSession::setTransport(transportType);
}

void queryTCPTransportSupport ()
{
  return RTSPSession::queryTCPTransportSupport();
}

void tcpPlaybackControl(int cmdType, int cmdVal)
{
  return RTSPSession::sendBufferingControlRequest(cmdType, cmdVal);
}

void negotiateRtpTransport(int TransportType,int BufferLenMs, int portNum)
{
  return RTSPSession::sendTransportChangeRequest(TransportType, BufferLenMs, portNum);
}

void setBitrate(int value)
{
  MMAdaptor::setBitRate(value);
}

int setAVPlaybackMode (AVPlaybackMode mode)
{
  if (RTSPSession::setAVPlaybackMode(mode))
    return 1;
  else
    return 0;
}

int executeRuntimeCommand (int cmd)
{
  return MMAdaptor::executeRuntimeCommand(cmd);
}

uint32_t* getCommonResolution(int* numProf)
{
  return RTSPSession::getCommonResloution(numProf);
}

uint32_t* getNegotiatedResolution()
{
  return RTSPSession::getNegotiatedResloution();
}


/** =======================================================================
*                Callback functions
** ======================================================================= */


/**
 * Function to send notification to RTSPAdaptor with one string argument.
 * message is comprised of: eventName, eventObjectArray={single_string_argument}.
 */
static void singlestring_callback(const char* eName, const char* str_argument)
{
    size_t len = strlen(str_argument) + 1;
    if (venc_mutex_lock(m_pMutex) != 0)
        LOGE(" singlestring_callback : Failed to Acquire Mutex" );
    else
       LOGE(" singlestring_callback : Acquiring Mutex" );
    strlcpy(strarray[0], str_argument,len);
    if (gCallback != NULL)
        gCallback(eName, 1, strarray);
    else
        LOGD("singlestring_callback: Callback is NULL");
    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE(" singlestring_callback : Failed to Release Mutex" );
    else
       LOGE(" singlestring_callback : Releasing Mutex" );
}


void eventError(const char* reason)
{
    singlestring_callback(ERROR_EVENT, reason);
}

void eventServiceEnabled()
{
    singlestring_callback(SERVICE_CHANGED_EVENT, WFD_ENABLED);
}

void eventServiceDisabled()
{
    singlestring_callback(SERVICE_CHANGED_EVENT, WFD_DISABLED);
}

void eventSessionStateChanged(int sessionState, const char* peerHandleMac, const char* sessionId)
{
    if (venc_mutex_lock(m_pMutex) != 0)
        LOGE(" eventSessionStateChanged : Failed to Acquire Mutex" );
    else
       LOGE(" eventSessionStateChanged : Acquiring Mutex" );

    SessionManager *pSM = SessionManager::Instance();
    size_t len;
    switch (sessionState) {
    case SESSION_STOPPED:
    {
        const char* stateStr = "STOPPED";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        break;
    }
    case SESSION_NEGOTIATING:
    {
        const char* stateStr = "NEGOTIATING";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        break;
    }
    case SESSION_NEGOTIATED:
    {
        const char* stateStr = "NEGOTIATED";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        //Copying the Rtsp session ID
        pSM->rtspSessionId = atoi(sessionId);
        break;
    }
    case SESSION_ESTABLISHING:
    {
        const char* stateStr = "ESTABLISHING";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        //Copying the Rtsp session ID
        pSM->rtspSessionId = atoi(sessionId);
        break;
    }
    case SESSION_ESTABLISHED:
    {
        const char* stateStr = "ESTABLISHED";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        break;
    }
    case SESSION_STANDBY:
    {
        const char* stateStr = "STANDBY";
        len = strlen(stateStr) + 1;
        strlcpy(strarray[0], stateStr,len);
        break;
    }
    default:
        LOGE("Unknown session state: %d", sessionState);
        eventError("Unknown session state.");
        if (venc_mutex_unlock(m_pMutex) != 0)
         LOGE(" eventSessionStateChanged : Failed to Release Mutex" );
        else
         LOGE(" eventSessionStateChanged : Releasing Mutex" );
        return;
    }

    len = strlen(peerHandleMac) + 1;
    strlcpy(strarray[1], peerHandleMac,len);
    len = strlen(sessionId) + 1;
    strlcpy(strarray[2], sessionId,len);
    if (gCallback != NULL)
        gCallback(SESSION_CHANGED_EVENT, 3, strarray);
    else
        LOGD("eventSessionStateChanged: Callback is null");
    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE(" eventSessionStateChanged : Failed to Release Mutex" );
    else
       LOGE(" eventSessionStateChanged : Releasing Mutex" );

}

void eventStreamControlCompleted(int cmdType, int rtspSessId)
{
    size_t len;
    if (venc_mutex_lock(m_pMutex) != 0)
         LOGE("eventStreamControlCompleted : Failed to Acquire Mutex" );
     else
        LOGE("eventStreamControlCompleted : Acquiring Mutex" );


    snprintf(strarray[0],256, "%d", rtspSessId);
    switch (cmdType) {
    case PLAY:
    {
        const char* controlStr = "PLAY";
        len = strlen(controlStr) + 1;
        strlcpy(strarray[1], controlStr,len);
        break;
    }
    case PLAY_DONE:
    {
        const char* controlStr = "PLAY_DONE";
        len = strlen(controlStr) + 1;
        strlcpy(strarray[1], controlStr,len);
        break;
    }
    case PAUSE:
    {
        const char* controlStr = "PAUSE";
        len = strlen(controlStr) + 1;
        strlcpy(strarray[1], controlStr,len);
        break;
    }
    case PAUSE_DONE:
    {
        const char* controlStr = "PAUSE_DONE";
        len = strlen(controlStr) + 1;
        strlcpy(strarray[1], controlStr,len);
        break;
    }
    case TEARDOWN:
    {
        const char* controlStr = "TEARDOWN";
        len = strlen(controlStr) + 1;
        strlcpy(strarray[1], controlStr,len);
        break;
    }
    default:
        LOGE("Unknown stream control cmd: %d", cmdType);
        eventError("Unknown stream control command.");
        break;
    }
    if (gCallback != NULL)
        gCallback(STREAM_CONTROL_EVENT, 2, strarray);
    else
        LOGD("eventStreamControlCompleted: Callback is null");

    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE("eventStreamControlCompleted : Failed to Release Mutex" );
    else
       LOGE("eventStreamControlCompleted : Releasing Mutex" );

}

void eventUIBCEnabled(int rtspSessId)
{
    if (venc_mutex_lock(m_pMutex) != 0)
        LOGE("eventUIBCEnabled : Failed to Acquire Mutex" );
    else
       LOGE("eventUIBCEnabled : Acquiring Mutex" );
    snprintf(strarray[0],256, "%d", rtspSessId);
    const char* UIBCStr = "ENABLED";
    size_t len = strlen(UIBCStr) + 1;
    strlcpy(strarray[1],UIBCStr,len);
    if (gCallback != NULL)
        gCallback(UIBC_CONTROL_EVENT, 2, strarray);
    else
        LOGD("eventUIBCEnabled: Callback is NULL");
    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE("eventUIBCEnabled : Failed to Release Mutex" );
    else
       LOGE("eventUIBCEnabled : Releasing Mutex" );
}

void eventUIBCDisabled(int rtspSessId)
{
    if (venc_mutex_lock(m_pMutex) != 0)
        LOGE("eventUIBCDisabled : Failed to Acquire Mutex" );
    else
       LOGE("eventUIBCDisabled : Acquiring Mutex" );
    snprintf(strarray[0],256, "%d", rtspSessId);
    const char* UIBCStr = "DISABLED";
    size_t len = strlen(UIBCStr) + 1;
    strlcpy(strarray[1],UIBCStr,len);
    if (gCallback != NULL)
        gCallback(UIBC_CONTROL_EVENT, 2, strarray);
    else
        LOGD("eventUIBCDisabled: Callback is NULL");
    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE("eventUIBCDisabled : Failed to Release Mutex" );
    else
       LOGE("eventUIBCDisabled : Releasing Mutex" );
}

void eventUIBCRotate(int angle)
{
    if(venc_mutex_lock(m_pMutex) !=0 )
        LOGE("eventUIBCRotate: Failed to acquire mutex");
    snprintf(strarray[0],256,"%d", angle);
    if (gCallback != NULL)
    {
         LOGE("Sending Rotate event");
         gCallback(UIBC_ROTATE_EVENT,1,strarray);
    }
    if (venc_mutex_unlock(m_pMutex) != 0)
        LOGE("eventUIBCDisabled : Failed to Release Mutex" );
}

void eventMMUpdate (MMEventType mmEvent,MMEventStatusType status, int evtData1, int evtData2, int evtData3)
{
    if (venc_mutex_lock(m_pMutex) != 0)
        LOGE("eventMMUpdate : Failed to Acquire Mutex" );
    else
       LOGE("eventMMUpdate : Acquiring Mutex" );

  switch (mmEvent)
  {
    case HDCP_CONNECT_SUCCESS:
    {
      snprintf(strarray[0],256,"HDCP_CONNECT");
      const char* success = "SUCCESS";
      size_t len = strlen(success) + 1;
      strlcpy(strarray[1],success,len);
      if (gCallback != NULL)
        gCallback(MM_UPDATE_EVENT, 2, strarray);
    }
    break;

    case HDCP_CONNECT_FAIL:
    {
      snprintf(strarray[0],256,"HDCP_CONNECT");
      const char* fail = "FAIL";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
         gCallback(MM_UPDATE_EVENT, 2, strarray);
    }
    break;

    case HDCP_UNSUPPORTED_BY_PEER:
    {
      snprintf(strarray[0],256,"HDCP_CONNECT");
      const char* fail = "UNSUPPORTEDBYPEER";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
         gCallback(MM_UPDATE_EVENT, 2, strarray);
    }
    break;

    case MM_VIDEO_EVENT_FAILURE:
    {
      if(status == MM_STATUS_RUNTIME_ERROR ||
         status == MM_STATUS_FAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_VIDEO_EVENT_FAILURE-RuntimeError");
        snprintf(strarray[0],256, "RuntimeError");
      }
      else if(status == MM_STATUS_NOTSUPPORTED ||
              status == MM_STATUS_BADPARAM     ||
              status == MM_STATUS_MEMORYFAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_VIDEO_EVENT_FAILURE-ConfigureFailure");
        snprintf(strarray[0],256, "ConfigureFailure");
      }
      const char* fail = "FAIL";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
        gCallback(MM_VIDEO_EVENT, 2, strarray);
    }
    break;

    case MM_AUDIO_EVENT_FAILURE:
    {
      if(status == MM_STATUS_RUNTIME_ERROR ||
         status == MM_STATUS_FAIL)
      {
          LOGD("WFDDBG: eventMMUpdate: Callback is MM_AUDIO_EVENT_FAILURE- RuntimeError");
        snprintf(strarray[0],256, "RuntimeError");
      }
      else if(status == MM_STATUS_NOTSUPPORTED ||
              status == MM_STATUS_BADPARAM     ||
              status == MM_STATUS_MEMORYFAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_AUDIO_EVENT_FAILURE- ConfigureFailure");
        snprintf(strarray[0],256, "ConfigureFailure");
      }
      const char* fail = "FAIL";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
        gCallback(MM_AUDIO_EVENT, 2, strarray);
    }
    break;

    case MM_EVENT_STREAM_STARTED:
    {
        if (status == MM_STATUS_SUCCESS && evtData1)
        {
            int nNumStrings;

            snprintf(strarray[0],256,"MMStreamStarted");
            snprintf(strarray[1],256,"%d", evtData1);
            snprintf(strarray[2],256,"%d", evtData2);
            snprintf(strarray[3],256,"%d", evtData3);
            if (gCallback != NULL)
                gCallback(MM_UPDATE_EVENT, 4, strarray);

        }

    }
    break;

    case MM_HDCP_EVENT_FAILURE:
    {
      if(status == MM_STATUS_RUNTIME_ERROR ||
         status == MM_STATUS_FAIL)
      {
	      LOGD("WFDDBG: eventMMUpdate: Callback is MM_HDCP_EVENT_FAILURE- RuntimeError");
        snprintf(strarray[0],256, "RuntimeError");
      }
      else if(status == MM_STATUS_NOTSUPPORTED ||
              status == MM_STATUS_BADPARAM     ||
              status == MM_STATUS_MEMORYFAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_HDCP_EVENT_FAILURE- ConfigureFailure");
        snprintf(strarray[0],256, "ConfigureFailure");
      }
      const char* fail = "FAIL";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
        gCallback(MM_HDCP_EVENT, 2, strarray);
    }
    break;
    case MM_RTP_EVENT_FAILURE:
    {
      if(status == MM_STATUS_RUNTIME_ERROR ||
         status == MM_STATUS_FAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_RTP_EVENT_FAILURE- RuntimeError");
        snprintf(strarray[0],256, "RuntimeError");
      }
      else if(status == MM_STATUS_NOTSUPPORTED ||
              status == MM_STATUS_BADPARAM     ||
              status == MM_STATUS_MEMORYFAIL)
      {
        LOGD("WFDDBG: eventMMUpdate: Callback is MM_RTP_EVENT_FAILURE- ConfigureFailure");
        snprintf(strarray[0],256, "ConfigureFailure");
      }
      const char* fail = "FAIL";
      size_t len = strlen(fail) + 1;
      strlcpy(strarray[1],fail,len);
      if (gCallback != NULL)
        gCallback(MM_NETWORK_EVENT, 2, strarray);
    }
    break;

  case BUFFERING_STATUS_UPDATE:
    {
        snprintf(strarray[0],256,"BufferingUpdate");
        snprintf(strarray[1], 256,"%d", evtData1);
        snprintf(strarray[2], 256,"%d", evtData2);

        if (gCallback != NULL)
            gCallback(MM_NETWORK_EVENT, 3, strarray);
        else
            LOGD("eventUIBCDisabled: Callback is NULL");
    }
  break;

  case BUFFERING_NEGOTIATION:
    if(status != MM_STATUS_SUCCESS)
    {
        snprintf(strarray[0],256,"RtpTransportNegotiationFail");

        if (gCallback != NULL)
            gCallback(MM_NETWORK_EVENT, 1, strarray);
        else
            LOGD("eventUIBCDisabled: Callback is NULL");
    }
    else
    {
        snprintf(strarray[0],256,"RtpTransportNegotiationSuccess");
        snprintf(strarray[1],256,"%d", evtData1);
        snprintf(strarray[2],256,"%d", evtData2);

        if (gCallback != NULL)
            gCallback(MM_NETWORK_EVENT, 3, strarray);
        else
            LOGD(" Callback is NULL");
    }
  break;


  case BUFFERING_CONTROL_EVENT_FLUSH:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Flush");
          snprintf(strarray[2], 256, "0");
          snprintf(strarray[3], 256,"%d", evtData1);
          snprintf(strarray[4], 256,"%d", evtData2);
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 5, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Flush");
          snprintf(strarray[2], 256, "-1");

          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 3, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;
  case BUFFERING_CONTROL_EVENT_PAUSE:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Pause");
          snprintf(strarray[2], 256, "0");
          snprintf(strarray[3], 256,"%d", evtData1);
          snprintf(strarray[4], 256,"%d", evtData2);
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 5, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Pause");
          snprintf(strarray[2], 256, "-1");

          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 3, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;

  case BUFFERING_CONTROL_EVENT_PLAY:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Play");
          snprintf(strarray[2], 256, "0");
          snprintf(strarray[3], 256,"%d", evtData1);
          snprintf(strarray[4], 256,"%d", evtData2);
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 5, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Play");
          snprintf(strarray[2], 256, "-1");

          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 3, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;

  case BUFFERING_CONTROL_EVENT_STATUS:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Status");
          snprintf(strarray[2], 256, "0");
          snprintf(strarray[3], 256,"%d", evtData1);
          snprintf(strarray[4], 256,"%d", evtData2);
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 5, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "TCPPlaybackControl");
          snprintf(strarray[1], 256, "Status");
          snprintf(strarray[2], 256, "-1");

          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 3, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;
  case BUFFERING_CONTROL_EVENT_DECODER_LATENCY:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "setDecoderLatency");
          snprintf(strarray[1], 256, "0");
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 2, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "setDecoderLatency");
          snprintf(strarray[1], 256, "-1");
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 2, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;
  case MM_RTP_EVENT_TCP_SUPPORTED_BY_SINK:
      if(status == MM_STATUS_SUCCESS)
      {
          snprintf(strarray[0], 256, "TCPTransportSupport");
          snprintf(strarray[1], 256, "0");
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 2, strarray);
          else
              LOGD(" Callback is NULL");

      }
      else
      {
          snprintf(strarray[0], 256, "TCPTransportSupport");
          snprintf(strarray[1], 256, "-1");
          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 2, strarray);
          else
              LOGD(" Callback is NULL");

      }
      break;
  case MM_RTP_EVENT_RTCP_RR_MESSAGE:
      {
          LOGD("Recieved MM RTCP receiver report");
          snprintf(strarray[0], 256, "RTCPRRMessage");
          snprintf(strarray[1], 256,"%d", evtData1); //Len of message

          if(!evtData1 || !evtData2) {
              LOGE("RTCP Data Size 0 or Invalid ptr");
              if (venc_mutex_unlock(m_pMutex) != 0)
                  LOGE("eventMMUpdate : Failed to Release Mutex" );
              else
                 LOGE("eventMMUpdate : Releasing Mutex" );
              return;
          }

          //Need to split the RTCP message across multiple payloads.
          int numSplit = evtData1/WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE;

          if(evtData1 % WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE) {
            numSplit += 1;
          }

          if(numSplit > WIFIDISPLAY_STRING_ARR_SIZE - 2) {
              LOGE("RTCP message too large. Truncated");
              numSplit = WIFIDISPLAY_STRING_ARR_SIZE - 2;
          }
          // Split RTCP messages into units of 255 as different NULL terminated strings

          uint8 *pSrc = (uint8*)evtData2;
          uint32 nSize = evtData1 > WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE ?
                              WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE : evtData1;

          for(int i = 0; i < numSplit; i++ ) {
              memcpy(strarray[2 + i], pSrc, nSize);
              strarray[2 + i][WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE] = 0;
              evtData1 -= nSize;
              if(!evtData1) {
                if (venc_mutex_unlock(m_pMutex) != 0)
                  LOGE("eventMMUpdate : Failed to Release Mutex" );
                else
                  LOGE("eventMMUpdate : Releasing Mutex" );
                  return;
              }
              nSize = evtData1 > WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE ?
                  WIFIDISPLAY_SEGMENTED_MESSAGE_SIZE : evtData1;
          }

          if (gCallback != NULL)
              gCallback(MM_NETWORK_EVENT, 2 + numSplit, strarray);
          else
              LOGD(" Callback is NULL");
      }
      break;

   case  MM_AUDIO_PROXY_DEVICE_OPENED:
      LOGD("WFDDBG: eventMMUpdate: Callback is MM_AUDIO_PROXY_DEVICE_OPENED");
      snprintf(strarray[0],256, "AudioProxyOpened");
      if (gCallback != NULL)
        gCallback(MM_AUDIO_EVENT, 2, strarray);
      break;

    case  MM_AUDIO_PROXY_DEVICE_CLOSED:
      LOGD("WFDDBG: eventMMUpdate: Callback is MM_AUDIO_PROXY_DEVICE_CLOSED");
      snprintf(strarray[0],256, "AudioProxyClosed");
      if (gCallback != NULL)
        gCallback(MM_AUDIO_EVENT, 2, strarray);
      break;

    default:
      break;
  }
  if (venc_mutex_unlock(m_pMutex) != 0)
      LOGE("eventMMUpdate : Failed to Release Mutex" );
  else
     LOGE("eventMMUpdate : Releasing Mutex" );

}

void* getStreamingSurface()
{
  int32 nSleepTime = 0;
  while (gSurface == NULL && (nSleepTime < 500))//Framework yet to Set Surface
  {
     MM_Timer_Sleep(10);
     nSleepTime += 10;
  }
  if (gSurface == NULL)
     LOGE("WIFI_DISPLAY:: gSurface Null!");
  return gSurface;
}

/*
 * Informing sink to send a request to source
 * for generating an IDR frame.
 */
void sendIDRRequest()
{
  LOGE("Wifidisplay : sendIdrRequest : Called");
  RTSPSession::sendIDRRequest();
}
