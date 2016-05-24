/*
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
/* =======================================================================
**               Include files for DASHMMIInterface.cpp
** ======================================================================= */
//#ifndef LOG_TAG
#define LOG_NDEBUG 0
#define LOG_TAG "DASHMMIInterface"
//#endif

#include "OMX_Index.h"
#include "OMX_Other.h"
#include "DASHMMIInterface.h"
#include "OMX_Types.h"

#include "common_log.h"

#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMCriticalSection.h"
#include "qtv_msg.h"
#include "OMX_CoreExt.h"
#include "QOMX_StreamingExtensions.h"
#include "QOMX_IVCommonExtensions.h"
#include "QOMX_SourceExtensions.h"
#include "DASHHTTPLiveSource.h"
#include "QOMX_StreamingExtensionsPrivate.h"

namespace android {

const char *DASH_MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";

#define MIN_LOW_WATERMARK_THRESHOLD 500000  //in Us
#define SEEK_DELAY_THRESHOLD 5000000
#define DEFAULT_PORT MMI_HTTP_AUDIO_PORT_INDEX
#define MARLIN_DRM_MIMETYPE "application/vnd.marlin.drm.mpd+xml"
//#define USE_DRM_PASSTHRU
//#define DRM_FILE_DUMP
#define REGISTER_AUDIO_CB 0
#define REGISTER_VIDEO_CB 1
#define DASH_MAX_NUM_TRACK 3

/** @brief   Constructor of DASH MMI Interface.
 *  @return
 */
DASHMMIInterface::DASHMMIInterface(const char *uri ,
                                   const KeyedVector<String8, String8> *headers,
                                   OMX_U32 &nReturn)
{
  nReturn = MMI_S_COMPLETE;

  QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
               "DASHMMIInterface::DASHMMIInterface");

  m_uri = NULL;

  if (uri)
  {
    uint32 size = std_strlen(uri);
    m_uri = (OMX_STRING) MM_Malloc((size+1) * sizeof(OMX_U8));
    if (m_uri)
    {
      std_strlcpy(m_uri, uri, size+1);
    }
  }

  memset(m_pHTTPAALStates, 0, sizeof(m_pHTTPAALStates));
  if ( MM_CriticalSection_Create(&m_hHTTPAALStateLock) != 0)
  {
    nReturn = MMI_S_EFAIL;
  }

  m_hHTTPAALSeekLock = NULL;

  if (IS_SUCCESS(nReturn))
  {
    if ( MM_CriticalSection_Create(&m_hHTTPAALSeekLock) != 0)
    {
      nReturn = MMI_S_EFAIL;
    }
  }

  m_hHTTPAALReadLock = NULL;
  m_hHTTPAALAudioReadLock = NULL;
  m_hHTTPAALVideoReadLock = NULL;
  m_hHTTPAALTextReadLock = NULL;
  m_pCurrentHTTPAALState = NULL;
  mDrmExtraSampleInfo = OMX_IndexComponentStartUnused;
  mDrmParamPsshInfo = OMX_IndexComponentStartUnused;
  mSMTPTimedTextDimensionsIndex = OMX_IndexComponentStartUnused;
  mSMTPTimedTextSubInfoIndex = OMX_IndexComponentStartUnused;
  mWatermarkExtIndex = OMX_IndexComponentStartUnused;
  mWatermarkStatusExtIndex = OMX_IndexComponentStartUnused;

  memset (&paramStructSMTPTimedTextDimensions,0x00,sizeof(MMI_GetExtensionCmdType));
  memset (&paramStructSMTPTimedTextSubInfo,0x00,sizeof(MMI_GetExtensionCmdType));

  if (IS_SUCCESS(nReturn))
  {
    if ( MM_CriticalSection_Create(&m_hHTTPAALReadLock) != 0)
    {
      nReturn = MMI_S_EFAIL;
    }
    if ( MM_CriticalSection_Create(&m_hHTTPAALAudioReadLock) != 0)
    {
      nReturn = MMI_S_EFAIL;
    }
    if ( MM_CriticalSection_Create(&m_hHTTPAALVideoReadLock) != 0)
    {
      nReturn = MMI_S_EFAIL;
    }
    if ( MM_CriticalSection_Create(&m_hHTTPAALTextReadLock) != 0)
    {
      nReturn = MMI_S_EFAIL;
    }
  }

  if (IS_SUCCESS(nReturn))
  {
    nReturn = InitializeHTTPAALStates();
  }

  if (IS_SUCCESS(nReturn))
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                 "HTTP ALL Initialized");
    // Initialize the HTTP ALL State to Idle
    m_pCurrentHTTPAALState = m_pHTTPAALStates[HTTPAALStateIdle];
  }

  m_handle = NULL;
  m_highWatermark = 0;
  m_lowWatermark = 0;
  m_bRebuffering = false;
  m_recentAudStatus = MMI_S_COMPLETE;
  m_recentVidStatus = MMI_S_COMPLETE;
  m_recentTextStatus = MMI_S_COMPLETE;
  m_bAudEos = true;
  m_bVidEos = true;
  m_bTextEos = true;

  bVideoDiscontinuity = false;
  bAudioDiscontinuity = false;

  mHttpLiveSrcObj = NULL;
  m_sessionSeek.Reset();

  eStreamType = DASH_STREAM_TYPE_VOD;

  mGetParamStruct = NULL;

  mDecryptHandle = NULL;
  mDrmManagerClient = NULL;
  mNumDecryptUnits = 0;
  mContentProtectionInfo = NULL;

  mLastVideoUniqueID = 0;
  mLastAudioUniqueID = 0;

  mAudioPSSHDataSize = 0;
  mVideoPSSHDataSize = 0;
  mTextPSSHDataSize  = 0;

  openDecryptBuffer = NULL;

  mValidAudioPort = new DASHMMIInterface::CPortsValidity(MMI_HTTP_AUDIO_PORT_INDEX);
  mAvailableValidPorts.push_back(mValidAudioPort);

  mValidVideoPort = new DASHMMIInterface::CPortsValidity(MMI_HTTP_VIDEO_PORT_INDEX);
  mAvailableValidPorts.push_back(mValidVideoPort);
  mBufferingIndication = 0x00;
  mDrmCorruptFrameCount = 0;
  setAALDRMError(DRM_NO_ERROR);
}

/** @brief   Destructor of DASH MMI Interface.
 *  @return
 */
DASHMMIInterface::~DASHMMIInterface()
{
  QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_DEBUG,
               "~DASHMMIInterface");

  if (m_pCurrentHTTPAALState)
  {
    HTTPAALState state = m_pCurrentHTTPAALState->GetState();
    if (state != HTTPAALStateIdle)
    {
      SetHTTPAALState(HTTPAALStateClosing);
      int timeout = 0;
      Wait(AAL_ABORT_SIG|AAL_WAKEUP_SIG,
           MMI_WAIT_INTERVAL, &timeout);
      if (timeout)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                     "DASHMMIInterface::~DASHMMIInterface "
                     "timeout on closing MMI");
      }
    }
  }

  if (m_handle)
  {
    HTTPMMIDeviceClose(m_handle);
    m_handle = NULL;
  }

  SetHTTPAALState(HTTPAALStateIdle);

  //destruct states
  for (int i=0; i<HTTPAALStateMax; i++)
  {
    if (m_pHTTPAALStates[i])
    {
      MM_Delete(m_pHTTPAALStates[i]);
      m_pHTTPAALStates[i] = NULL;
    }
  }

  if (m_hHTTPAALStateLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALStateLock);
  }

  if (m_hHTTPAALSeekLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALSeekLock);
  }

  if (m_hHTTPAALReadLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALReadLock);
  }

  if (m_hHTTPAALAudioReadLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALAudioReadLock);
  }

  if (m_hHTTPAALVideoReadLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALVideoReadLock);
  }
  if (m_hHTTPAALTextReadLock)
  {
    MM_CriticalSection_Release(m_hHTTPAALTextReadLock);
  }

  eStreamType = DASH_STREAM_TYPE_VOD;
  mHttpLiveSrcObj = NULL;

  if (mGetParamStruct) {
    free(mGetParamStruct);
    mGetParamStruct = NULL;
  }

  if(mDrmManagerClient != NULL)
  {
      delete mDrmManagerClient;
      mDrmManagerClient = NULL;
  }

  if(mContentProtectionInfo != NULL)
  {
     MM_Free(mContentProtectionInfo);
     mContentProtectionInfo = NULL;
  }

  if (openDecryptBuffer)
  {
    delete openDecryptBuffer;
    openDecryptBuffer = NULL;
  }

  if (m_uri)
  {
    MM_Free(m_uri);
    m_uri = NULL;
  }

  mValidAudioPort = NULL;
  mValidVideoPort = NULL;
  mBufferingIndication = 0x00;
  mDrmCorruptFrameCount  = 0;
  setAALDRMError(DRM_NO_ERROR);
}


/*
 * Initializes all the HTTP AAL States
 *
 * @return MMI_S_EFAIL on failure MMI_S_SUCCESS_COMPLETE on success
 */
int DASHMMIInterface::InitializeHTTPAALStates()
{
  uint32 nReturn = MMI_S_COMPLETE;

  m_pHTTPAALStates[HTTPAALStateIdle] =
    MM_New_Args( CHTTPAALStateIdle, (*this));
  if (m_pHTTPAALStates[HTTPAALStateIdle] == NULL )
  {
    nReturn = MMI_S_EFAIL;
  }
  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStateConnecting] =
      MM_New_Args( CHTTPAALStateConnecting, (*this));
    if (m_pHTTPAALStates[HTTPAALStateConnecting] == NULL )
    {
      nReturn = MMI_S_EFAIL;
    }
  }
  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStatePlaying] =
      MM_New_Args( CHTTPAALStatePlaying, (*this));
    if (m_pHTTPAALStates[HTTPAALStatePlaying] == NULL )
    {
      nReturn = MMI_S_EFAIL;
    }
  }

  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStatePausing] =
      MM_New_Args( CHTTPAALStatePausing, (*this));
    if (m_pHTTPAALStates[HTTPAALStatePausing] == NULL )
    {
      nReturn = MMI_S_EFAIL;
    }
  }
  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStatePaused] =
      MM_New_Args( CHTTPAALStatePaused, (*this));
    if (m_pHTTPAALStates[HTTPAALStatePaused] == NULL )
    {
      nReturn = MMI_S_EFAIL;
    }
  }
  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStateResuming] =
      MM_New_Args( CHTTPAALStateResuming, (*this));
    if (m_pHTTPAALStates[HTTPAALStateResuming] == NULL)
    {
      nReturn = MMI_S_EFAIL;
    }
  }
  if (nReturn == MMI_S_COMPLETE)
  {
    m_pHTTPAALStates[HTTPAALStateClosing] =
      MM_New_Args( CHTTPAALStateClosing, (*this));
    if (m_pHTTPAALStates[HTTPAALStateClosing] == NULL)
    {
      nReturn = MMI_S_EFAIL;
    }
  }

  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "DASHMMIInterface AAL State initialize return %lu", nReturn);
  return nReturn;
}

/** @brief   initcheck.
 *  @param[in] none
 *  @return status of operation
 */
status_t DASHMMIInterface::initCheck() const
{
  status_t ret = NO_INIT;
  HTTPAALState state = HTTPAALStateIdle;

  if (m_pCurrentHTTPAALState != NULL)
  {
    state = m_pCurrentHTTPAALState->GetState();
  }

  if ((state > HTTPAALStateIdle) && (state < HTTPAALStateClosing))
  {
    ret = OK;
  }

  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "HTTP DASH MMI - initcheck, state %d", state);
  return ret;
}


/** @brief  get enabled flags
 *  @param[in]
 *  @return enabled flags
 */
uint32_t DASHMMIInterface::flags()
{
  return 0;
}


DASHMMIInterface::DashStreamType DASHMMIInterface::GetStreamType()
{
    return eStreamType;
}

void DASHMMIInterface::SetStreamType(DASHMMIInterface::DashStreamType lStreamType)
{
    eStreamType = lStreamType;
}

void DASHMMIInterface::RegisterDecryptUnit(int track, int portIndex)
{
    mDecryptUnitIds[track] = portIndex;
    if(track >= mNumDecryptUnits)
    {
        mNumDecryptUnits = track + 1;
    }
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM, "mNumDecryptUnits %d ", mNumDecryptUnits);
}

int DASHMMIInterface::SetupDRMEnv()
{
  status_t drmErr = DRM_NO_ERROR;
  int err = -1;
  err = GetContentProtectionInfo();
  if (err == -1)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: GetContentProtectionInfo");
    return -1;
  }
  else if (err == 1)
  {
    // DRM type not MARLIN. Not An error
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "eDrmType != QOMX_MARLIN_DRM");
    return 0;
  }

  //Initializing  DRM
  if(DrmInit() != 0)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: DrmInit");
    return -1;
  }

  //Get PSSH info and  Initialization for each Decrypt Unit
  for(int i=0; i<mNumDecryptUnits; i++)
  {
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM, "Querying for mNumDecryptUnit %d", mDecryptUnitIds[i]);
    QOMX_PARAM_STREAMING_PSSHINFO* psshInfoPtr = NULL;
    const DrmBuffer* initDecryptBuffer = NULL;
    //Get PSSH info for each Decrypt Unit
    err = GetPSSHInfo(mDecryptUnitIds[i],&psshInfoPtr);

    if (err == 0 && (getPSSHDataSizeOnPort(mDecryptUnitIds[i]) == 0))
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "(%d) track doesn't have PSSH info.. returning",mDecryptUnitIds[i]);
      mDecryptUnitIds[i] = 0; // reset the array element which is not needed any more as the track is not encrypted
      continue;
    }

    if (psshInfoPtr)
    {
#ifndef USE_DRM_PASSTHRU
      if(err != 0)
      {
        return -1;
      }
      if (psshInfoPtr->nPsshDataBufSize > 0)
      {
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
              "(%lu) PSSHDataBufSize",psshInfoPtr->nPsshDataBufSize - 1);
        initDecryptBuffer = new DrmBuffer((char*)&psshInfoPtr->cPSSHData[0],psshInfoPtr->nPsshDataBufSize - 1);
#ifdef DRM_FILE_DUMP
// writing audio content to file before decryption - start
        if (IS_AUDIO_PORT(mDecryptUnitIds[i]))
        {
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
           "Audio PSSHDataBufSize %lu", psshInfoPtr->nPsshDataBufSize - 1);
          FILE *psshAudioFp = fopen("/data/psshAudio.yuv", "ab+");
          if (psshAudioFp)
          {
            int size = fwrite(psshInfoPtr->cPSSHData, 1, psshInfoPtr->nPsshDataBufSize - 1 ,  psshAudioFp );
            if (size <= 0)
            {
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
               "writing audio pssh data size");
            }

            fclose(psshAudioFp);
            psshAudioFp = NULL;
          }
          else
          {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(psshAudio.yuv)");
          }
        }
//writing audio content to file before decryption - end
#endif
      }
#else
      initDecryptBuffer = new DrmBuffer(NULL,0);
#endif
      if(initDecryptBuffer == NULL)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "initDecryptBuffer is NULL");
        return -1;
      }

      //Initializing  Decrypt Unit
#ifndef USE_DRM_PASSTHRU
      String8 key ;
      String8 value ;
      key   = (const char *)"default_KID";
      value = String8((const char *)psshInfoPtr->cDefaultKeyID, MAX_KID_SIZE);
      mDecryptHandle->extendedData.add(key, value);
      for (unsigned k = 0; k < mDecryptHandle->extendedData.size(); k++ )
      {
        QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                       "default Key [%s] value [%s] and size of value [%d]",
                      (char *)(mDecryptHandle->extendedData.keyAt(k)).string(),
                      (char *)(mDecryptHandle->extendedData.valueFor(mDecryptHandle->extendedData.keyAt(k))).string(),
                      (mDecryptHandle->extendedData.valueFor(mDecryptHandle->extendedData.keyAt(k))).size());
      }
      drmErr = mDrmManagerClient->initializeDecryptUnit(mDecryptHandle,psshInfoPtr->nUniqueID, initDecryptBuffer);
#else
      drmErr = mDrmManagerClient->initializeDecryptUnit(mDecryptHandle,mDecryptUnitIds[i],initDecryptBuffer);
#endif
      if(drmErr != DRM_NO_ERROR)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "initializeDecryptUnit:failed");
        if (initDecryptBuffer)
        {
           delete initDecryptBuffer;
           initDecryptBuffer = NULL;
        }
        return -1;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "initializeDecryptUnit:success");
      }
      if (initDecryptBuffer)
      {
        delete initDecryptBuffer;
        initDecryptBuffer = NULL;
      }
      if (psshInfoPtr)
      {
        MM_Free(psshInfoPtr);
        psshInfoPtr = NULL;
      }
    }
    else
    {
      return -1;
    }
  }

  //Set Playback status for DRM
  // TODO: Determine proper use/value of 'position' (milliseconds) - currently passing 0
  int64_t position = 0;
  drmErr = mDrmManagerClient->setPlaybackStatus(mDecryptHandle, Playback::START, position);
  if(drmErr != DRM_NO_ERROR)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "DrmManagerClient.setPlaybackStatus(Playback::START) failed but continuing decryption");
    HandleDRMError();
    return -1;
  }
  else
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "DrmManagerClient.setPlaybackStatus(Playback::START) success");
  }
  return 0;
}

int DASHMMIInterface::GetContentProtectionInfo()
{
  //Get Content Protection Info
  QOMX_PARAM_STREAMING_CONTENTPROTECTIONINFO cpInfo;
  int cpInfoAllocSize = 0;
  memset(&cpInfo, 0, sizeof(cpInfo));
  cpInfo.eDrmType = QOMX_NO_DRM;
  cpInfo.nSize = sizeof(cpInfo);
  OMX_INDEXTYPE cpIndex = OMX_IndexComponentStartUnused;
  int err = OMXPrefetch((OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_CONTENTPROTECTION_INFO,&cpInfo, &cpIndex);
  QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "DRM type [%d] size [%lu]", cpInfo.eDrmType, cpInfo.nContentProtectionInfoSize );

#ifndef USE_DRM_PASSTHRU
  if(err != 0 || cpInfo.nContentProtectionInfoSize == 0 )
  {
    return -1;
  }
  else if(cpInfo.eDrmType != QOMX_MARLIN_DRM)
  {
    // DRM type not MARLIN. Not An error
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "cpInfo.eDrmType != QOMX_MARLIN_DRM");
    return 1;
  }

  if(mContentProtectionInfo != NULL)
  {
    MM_Free(mContentProtectionInfo);
    mContentProtectionInfo = NULL;
  }

  OMX_HANDLETYPE handle = GetMMIHandle();
  cpInfoAllocSize = 2* sizeof(OMX_U32) +
             sizeof(OMX_VERSIONTYPE) +
             sizeof(QOMX_DRM_TYPE)+
             sizeof(OMX_U8)* cpInfo.nContentProtectionInfoSize;
  mContentProtectionInfo =
          (QOMX_PARAM_STREAMING_CONTENTPROTECTIONINFO*)MM_Malloc(cpInfoAllocSize);

  if (mContentProtectionInfo)
  {
    mContentProtectionInfo->eDrmType = cpInfo.eDrmType;
    mContentProtectionInfo->nContentProtectionInfoSize = cpInfo.nContentProtectionInfoSize;
    mContentProtectionInfo->nSize = cpInfoAllocSize;
    MMI_OmxParamCmdType cmd;
    cmd.nParamIndex = cpIndex;
    cmd.pParamStruct = mContentProtectionInfo;
    uint32 mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_STD_OMX_PARAM, &cmd);
    if(mmiret != MMI_S_COMPLETE)
    {
       QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "Get Content Protection Info failed %lu", mmiret);
      return -1;
    }
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "ContentProtection Element is %s", mContentProtectionInfo->cContentProtectionData);
  }
  else
  {
     QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
      "mContentProtectionInfo : memory allocation failed");
     return -1;
  }

#else
  mContentProtectionInfo =
          (QOMX_PARAM_STREAMING_CONTENTPROTECTIONINFO*)
              MM_Malloc(sizeof(QOMX_PARAM_STREAMING_CONTENTPROTECTIONINFO));
  if (mContentProtectionInfo)
  {
    mContentProtectionInfo->eDrmType = QOMX_NO_DRM;
  }
#endif
  return 0;
}


void DASHMMIInterface::HandleDRMError()
{
 status_t drmerr = DRM_NO_ERROR;
 for(int j=0; j < mNumDecryptUnits; j++)
 {
   (void)mDrmManagerClient->finalizeDecryptUnit(mDecryptHandle,mDecryptUnitIds[j]);
   if(drmerr != DRM_NO_ERROR)
   {
     QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  "Failed to finalize decryption unit on port %d",mDecryptUnitIds[j]);
   }
 }
 return;
}

void DASHMMIInterface::GetIndexForExtensions()
{
  //Query for Index of mDrmParamPsshInfo
  OMX_HANDLETYPE handle = GetMMIHandle();
  MMI_GetExtensionCmdType ext;

  mDrmParamPsshInfo = OMX_IndexComponentStartUnused;
  ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_PSSH_INFO;
  ext.pIndex = &mDrmParamPsshInfo;
  uint32 mmiret = MMI_S_EFAIL;
  mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
  if(mmiret != MMI_S_COMPLETE)
  {
      QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "Get Extension Index for %s failed %lu", ext.cParamName, (long unsigned int)ext.pIndex);
      return;
  }
  QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_DEBUG,
              "Get Extension Index for %s is  %x", ext.cParamName, mDrmParamPsshInfo);

  mDrmExtraSampleInfo = OMX_IndexComponentStartUnused;
  ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_EXTRASAMPLE_INFO;
  ext.pIndex = &mDrmExtraSampleInfo;
  mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
  if(mmiret != MMI_S_COMPLETE)
  {
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
               "Get Extension Index for %s failed %lu", ext.cParamName, (long unsigned int)ext.pIndex);
    return;
  }
  QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_DEBUG,
              "Get Extension Index for %s is  %x", ext.cParamName, mDrmExtraSampleInfo);
  return;
}
void DASHMMIInterface::GetIndexForQOEExtensions(bool bNotify)
{
  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : bNotify = %d", bNotify);
  if (bNotify)
  {
    //Query for Index of mQOEDataPlay
    OMX_HANDLETYPE handle = GetMMIHandle();
    MMI_GetExtensionCmdType ext;
    uint32 mmiret = MMI_S_EFAIL;

    mQOEDataPlay = OMX_IndexComponentStartUnused;
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_QOE_PLAY;
    ext.pIndex = &mQOEDataPlay;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if(mmiret != MMI_S_COMPLETE)
    {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : %s failed %lu", ext.cParamName,(long unsigned int)ext.pIndex);
        return;
    }
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
                "GetIndexForQOEExtensions : %s success %x", ext.cParamName, mQOEDataPlay);
    mmiret = MMI_S_EFAIL;

    mQOEDataStop = OMX_IndexComponentStartUnused;
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_QOE_STOP;
    ext.pIndex = &mQOEDataStop;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if(mmiret != MMI_S_COMPLETE)
    {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : %s failed %lu", ext.cParamName, (long unsigned int)ext.pIndex);
        return;
    }
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
                "GetIndexForQOEExtensions : %s success %x", ext.cParamName, mQOEDataStop);
    mmiret = MMI_S_EFAIL;

    mQOEDataSwitch = OMX_IndexComponentStartUnused;
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_QOE_SWITCH;
    ext.pIndex = &mQOEDataSwitch;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if(mmiret != MMI_S_COMPLETE)
    {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : %s failed %lu", ext.cParamName, (long unsigned int)ext.pIndex);
        return;
    }
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
                "GetIndexForQOEExtensions : %s success %x", ext.cParamName, mQOEDataSwitch);
    mmiret = MMI_S_EFAIL;

    mQOEDataPeriodic = OMX_IndexComponentStartUnused;
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_QOE_PERIODIC;
    ext.pIndex = &mQOEDataPeriodic;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if(mmiret != MMI_S_COMPLETE)
    {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : %s failed %lu", ext.cParamName, (long unsigned int)ext.pIndex);
        return;
    }
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
                "GetIndexForQOEExtensions : %s success %x", ext.cParamName, mQOEDataPeriodic);
  }
  else
  {
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "GetIndexForQOEExtensions : bNotify %d , setting OMX_IndexComponentStartUnused", bNotify);
    mQOEDataPlay     = OMX_IndexComponentStartUnused;
    mQOEDataStop     = OMX_IndexComponentStartUnused;
    mQOEDataSwitch   = OMX_IndexComponentStartUnused;
    mQOEDataPeriodic = OMX_IndexComponentStartUnused;
  }
  return;
}


QOMX_DRM_TYPE DASHMMIInterface::getDrmType()
{
  if (mContentProtectionInfo)
  {
    return mContentProtectionInfo->eDrmType;
  }
  return  QOMX_NO_DRM;
}

void DASHMMIInterface::setAALDRMError(status_t error)
{
  mAALDrmError = error;
  if (mAALDrmError != DRM_NO_ERROR)
  {
    if (mDrmCorruptFrameCount >= DRM_CORRUPT_FRAME_COUNT_LIMIT)
    {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
                  "Max Error frame limit exceeds");
        mAALDrmError = DRM_ERROR_CANNOT_HANDLE;
    }
    else
    {
      // changing mAALDrmError, keeping UNKNOWN
      mDrmCorruptFrameCount++;
      if (mAALDrmError != DRM_ERROR_UNKNOWN)
      {
        mAALDrmError = DRM_ERROR_UNKNOWN;
      }
    }
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "setAALDRMError (%d)",mAALDrmError);
  }
}


int DASHMMIInterface::DrmCheckAndDecrypt(MMI_BufferCmdType* pFillBuffCmdResp)
{
  int oldDecryptUnitId = -1;
  int newDecryptUnitId = -1;
  bool bUniqueIdChanged = false;

  OMX_BUFFERHEADERTYPE* pBufHdr = pFillBuffCmdResp ? (OMX_BUFFERHEADERTYPE*)pFillBuffCmdResp->pBufferHdr : NULL;
  QOMX_EXTRA_SAMPLE_INFO* extraSampleInfo = NULL;
  QOMX_PARAM_STREAMING_PSSHINFO* psshInfo = NULL;

  if(mContentProtectionInfo == NULL)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "mContentProtectionInfo is NULL");
    return DRM_NO_ERROR;
  }
  else if (pFillBuffCmdResp && getPSSHDataSizeOnPort(pFillBuffCmdResp->nPortIndex) == 0)
  {
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "PSSH DataSize is zero on pFillBuffCmdResp->nPortIndex (%lu)",
                pFillBuffCmdResp->nPortIndex);
    return DRM_NO_ERROR;
  }
  else if (mContentProtectionInfo &&
           mContentProtectionInfo->eDrmType != QOMX_MARLIN_DRM )
  {
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM, "DRM type  %d",
                  mContentProtectionInfo->eDrmType);
    // NOTE: For general DRM, check eDrmType == QOMX_NO_DRM
    return DRM_NO_ERROR;
  }

  if (pFillBuffCmdResp && (IS_AUDIO_PORT(pFillBuffCmdResp->nPortIndex) || IS_VIDEO_PORT(pFillBuffCmdResp->nPortIndex)) &&
      (pBufHdr->nFlags & OMX_BUFFERFLAG_EXTRADATA))
  // Traverse the list of extra data sections
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW, "Processing for Extra Data");

    OMX_OTHER_EXTRADATATYPE *pExtraData = NULL;
    uint32 ulAddr = (uint32)( pBufHdr->pBuffer) +  pBufHdr->nFilledLen;
    // Aligned address to DWORD boundary
    ulAddr = (ulAddr + 0x3) & (~0x3);
    pExtraData = (OMX_OTHER_EXTRADATATYPE *)ulAddr;

    // Traverse the list of extra data sections
    while(pExtraData && pExtraData->eType != OMX_ExtraDataNone)
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
              "Processing pExtraData->eType(0x%x)",
              (OMX_EXTRADATATYPE)pExtraData->eType);
      if((OMX_INDEXTYPE)pExtraData->eType == mDrmParamPsshInfo)
      {
        psshInfo = (QOMX_PARAM_STREAMING_PSSHINFO*)&pExtraData->data;
        //PrintPSSHInfo(psshInfo);
      }
      if((OMX_INDEXTYPE)pExtraData->eType == mDrmExtraSampleInfo)
      {
        extraSampleInfo = (QOMX_EXTRA_SAMPLE_INFO*)&pExtraData->data;
        PrintExtraSampleInfo(extraSampleInfo);
      }
      ulAddr = ulAddr + pExtraData->nSize;
      ulAddr = (ulAddr + 0x3) & (~0x3);
      pExtraData = (OMX_OTHER_EXTRADATATYPE*)ulAddr;
    }
    oldDecryptUnitId = GetLastUniqueID(pFillBuffCmdResp->nPortIndex);
    if (psshInfo)
    {
      newDecryptUnitId = psshInfo->nUniqueID;
      if (psshInfo->nUniqueID != oldDecryptUnitId && oldDecryptUnitId != 0)
      {

        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                      "UniqueId Changed..last UniqueId %d present UniqueId %ld",
                       oldDecryptUnitId, psshInfo->nUniqueID );
        bUniqueIdChanged = true;
        SetLastUniqueID(pFillBuffCmdResp->nPortIndex, newDecryptUnitId);
      }
    }
     //NOTE: While Playing OMX_BUFFERHEADERTYPE will have PSSH + Extra Sample Info (for both Audio/video)
    // For track chage (only applicable for Video) OMX_BUFFERHEADERTYPE will have only PSSH
    if (IS_VIDEO_PORT(pFillBuffCmdResp->nPortIndex))
    {
      if ((pBufHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG) && !psshInfo)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
            "Failed to obtain DRM psshInfo info in OMX_BUFFERFLAG_CODECCONFIG for Video port");
        return DRM_ERROR_UNKNOWN;
      }
      else if (!psshInfo && !extraSampleInfo)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
            "Failed to obtain DRM psshInfo/extra sample info in FTB for Video port");
        return DRM_ERROR_UNKNOWN;
      }
      QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,"Santiy Check: video port passed");
    }
    else if (IS_AUDIO_PORT(pFillBuffCmdResp->nPortIndex))
    {
      if (!psshInfo && !extraSampleInfo)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
            "Failed to obtain DRM psshInfo/extra sample info in FTB for audio port");
        return DRM_ERROR_UNKNOWN;
      }
      QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,"Santiy Check: audio port passed");
    }
  }

  const DrmBuffer* encBuffer = NULL;
  DrmBuffer* decBuffer = NULL;
  DrmBuffer* iv = NULL;
  status_t drmerr = DRM_NO_ERROR;

#ifndef USE_DRM_PASSTHRU

  if(pFillBuffCmdResp && (bUniqueIdChanged == true) && psshInfo)
  {
    // Track change - we need to reconfig the decryption unit
    // TODO: Handle initial playback case (unnecessary reconfig)
    status_t drmerr = mDrmManagerClient->finalizeDecryptUnit(mDecryptHandle,oldDecryptUnitId);
    if(drmerr != DRM_NO_ERROR)
    {
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "Failed to finalize decryption unit on DecryptUnit %d",oldDecryptUnitId);
      return drmerr;
    }

    QOMX_PARAM_STREAMING_PSSHINFO* psshInfoPtr = NULL;
    int err = GetPSSHInfo(pFillBuffCmdResp->nPortIndex,&psshInfoPtr, psshInfo->nUniqueID);
    if (psshInfoPtr)
    {
      if(err != 0)
      {
         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                    "Failed to GetPSSHInfo on DecryptUnit %d",newDecryptUnitId);
         return DRM_ERROR_UNKNOWN;
      }

      const DrmBuffer* drmBuffer = new DrmBuffer((char*)&psshInfoPtr->cPSSHData[0],psshInfoPtr->nPsshDataBufSize - 1);
      String8 key ;
      String8 value ;
      key   = (const char *)"default_KID";
      value = String8((const char *)psshInfoPtr->cDefaultKeyID, MAX_KID_SIZE);
      mDecryptHandle->extendedData.add(key, value);
      for (unsigned k = 0; k < mDecryptHandle->extendedData.size(); k++ )
      {
        QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                       "default Key [%s] value [%s] and size of value [%d]",
                      (char *)(mDecryptHandle->extendedData.keyAt(k)).string(),
                      (char *)(mDecryptHandle->extendedData.valueFor(mDecryptHandle->extendedData.keyAt(k))).string(),
                      (mDecryptHandle->extendedData.valueFor(mDecryptHandle->extendedData.keyAt(k))).size());
      }
      drmerr = mDrmManagerClient->initializeDecryptUnit(mDecryptHandle,newDecryptUnitId,drmBuffer);
      //should newDecryptUnitId be passed in place of decryptUnit
      delete drmBuffer;
      drmBuffer = NULL;
      MM_Free(psshInfoPtr);
      psshInfoPtr = NULL;
      if(drmerr != DRM_NO_ERROR)
      {
          QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  "Failed to initialize decryption unit on DecryptUnit %d drmerr %d ",newDecryptUnitId,drmerr);
        return drmerr;
      }
    }
    else
    {
      return DRM_ERROR_UNKNOWN;
    }
    bUniqueIdChanged = false;
  }

  if(extraSampleInfo != NULL && extraSampleInfo->nIsEncrypted == 0)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,"Extra sample Non Encrypted..skip decryption");
    return DRM_NO_ERROR;
  }

  if (pFillBuffCmdResp && IS_VIDEO_PORT(pFillBuffCmdResp->nPortIndex) &&
      (pBufHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG))
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,"Video Sample contains OMX_BUFFERFLAG_CODECCONFIG");
    return DRM_NO_ERROR;
  }

  int decryptSize = 0;
  uint32 encSampleSize = 0;
  for(int i=0;extraSampleInfo && i<extraSampleInfo->nSubSampleCount;i++)
  {
     decryptSize += extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData;
     encSampleSize += extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData +
                      extraSampleInfo->sEncSubsampleInfo[i].nSizeOfClearData;
  }
  char* destptr = NULL;
  char* srcptr = NULL;

  if(extraSampleInfo && extraSampleInfo->nIsEncrypted)
  {
    if (extraSampleInfo->nSubSampleCount == 0 && decryptSize == 0 )
    {
#ifdef DRM_FILE_DUMP
// writing audio content to file before decryption - start
      QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
        "size of encrypted Audio pBufHdr->pBuffer is %d offset %lu decryptSize %d",
        pBufHdr->nFilledLen, pBufHdr->nOffset, decryptSize);
      FILE *encAudioFp = fopen("/data/encryptAudio.yuv", "ab+");
      if (encAudioFp)
      {
        int size = fwrite(pBufHdr->pBuffer, 1, pBufHdr->nFilledLen ,  encAudioFp );
        if (size <= 0)
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
            "error in writing encrypted pBufHdr->pBuffer");
        }

        fclose(encAudioFp);
        encAudioFp = NULL;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(encryptAudio.yuv)");
      }

//writing audio content to file before decryption - end
#endif
      decryptSize =  pBufHdr->nFilledLen;
      encBuffer = new DrmBuffer((char*)&pBufHdr->pBuffer[0],decryptSize);
      if (encBuffer == NULL)
      {
        return DRM_ERROR_UNKNOWN;
      }
      char* decrypted
          = (char *)malloc(sizeof(OMX_U8) * decryptSize);
      if (decrypted == NULL)
      {
        delete encBuffer;
        encBuffer = NULL;
        return DRM_ERROR_UNKNOWN;
      }
      memset(decrypted, 0 , sizeof(OMX_U8) * decryptSize);
      decBuffer = new DrmBuffer(&decrypted[0],decryptSize);
      if (decBuffer == NULL)
      {
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        return DRM_ERROR_UNKNOWN;
      }
#ifdef DRM_FILE_DUMP
// writing audio IV content to file - start
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "size of audio IV content  %d",
                    extraSampleInfo->nIVSize);
      FILE *audioIVFp = fopen("/data/audioIVContent.yuv", "ab+");
      if (audioIVFp)
      {
        int size = fwrite(extraSampleInfo->nInitVector, 1, MAX_IV_SIZE,  audioIVFp );
        if (size <= 0)
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
          "error in writing audio IV content");
        }

        fclose(audioIVFp);
        audioIVFp = NULL;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(audioIVContent.yuv)");
      }

//writing audio IV content to file - end
#endif
      iv = new DrmBuffer((char*)&extraSampleInfo->nInitVector[0],extraSampleInfo->nIVSize);
      if (iv == NULL)
      {
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        return DRM_ERROR_UNKNOWN;
      }

      QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
          "[DRM type:%d] decrypt: on port %lu with DecryptUnitId %d ",
      mContentProtectionInfo->eDrmType, pFillBuffCmdResp->nPortIndex, newDecryptUnitId);
      drmerr = mDrmManagerClient->decrypt(
               mDecryptHandle, newDecryptUnitId, encBuffer, &decBuffer, iv);
      if(drmerr != DRM_NO_ERROR)
      {
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        delete iv;
        iv = NULL;
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "decrypt:failed with error %d",drmerr);
        return drmerr;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "decrypt:succeess");
        srcptr = (char*)&decBuffer->data[0];
        destptr = (char*)&pBufHdr->pBuffer[0];
        memcpy(destptr, srcptr, decBuffer->length);
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "decrypted audio content copied of size %d destptr %x",
              decBuffer->length,
              (uint32_t)destptr );
#ifdef DRM_FILE_DUMP
// writing audio content to file after decryption - start
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
           "size of decrypted Audio pBufHdr->pBuffer is %lu offset %lu",
             pBufHdr->nFilledLen, pBufHdr->nOffset);
        FILE *decAudioFp = fopen("/data/decryptAudio.yuv", "ab+");
        if (decAudioFp)
        {
          int size = fwrite(pBufHdr->pBuffer, 1, pBufHdr->nFilledLen,  decAudioFp );
          if (size <= 0)
          {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "error in writing decrypted pBufHdr->pBuffer");
          }

          fclose(decAudioFp);
          decAudioFp = NULL;
        }
        else
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(decryptAudio.yuv)");
        }

// writing audio content to file after decryption - end
#endif
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        delete iv;
        iv = NULL;
      }
    }
    else //(nSubSampleCount > 0 and decryptSize > 0)
    {
      // Strict sanity check
      if (encSampleSize != pBufHdr->nFilledLen)
      {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
                     "returning, encSampleSize (%lu) != pBufHdr->nFilledLen (%lu)",
                     encSampleSize, pBufHdr->nFilledLen );
        return DRM_ERROR_TAMPER_DETECTED;
      }
#ifdef DRM_FILE_DUMP
// writing video content to file before decryption - start
      QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
        "size of encrypted Video pBufHdr->pBuffer is %lu offset %d decryptSize %d",
        pBufHdr->nFilledLen, pBufHdr->nOffset, decryptSize);
      FILE *encVideoFp = fopen("/data/encryptVideo.yuv", "ab+");
      if (encVideoFp)
      {
        int size = fwrite(pBufHdr->pBuffer, 1, pBufHdr->nFilledLen ,  encVideoFp );
        if (size <= 0)
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
          "error in writing encrypted pBufHdr->pBuffer");
        }

        fclose(encVideoFp);
        encVideoFp = NULL;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(encryptVideo.yuv)");
      }

//writing video content to file before decryption - end
#endif
      char* toDecrypt
          = (char *)malloc(sizeof(OMX_U8) * decryptSize);
      if (toDecrypt == NULL)
      {
         return DRM_ERROR_UNKNOWN;
      }
      memset(toDecrypt, 0, sizeof(OMX_U8) * decryptSize);
      destptr = &toDecrypt[0];
      srcptr = NULL;
      for(int i = 0; extraSampleInfo && i<extraSampleInfo->nSubSampleCount;i++)
      {
        if (extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData > 0)
        {
          srcptr = (char*)&pBufHdr->pBuffer[extraSampleInfo->sEncSubsampleInfo[i].nOffsetEncryptedData];
          memcpy(destptr, srcptr, extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData);
          QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "encypted video content copied from Offset %lu of size %lu srcptr 0x%x",
              extraSampleInfo->sEncSubsampleInfo[i].nOffsetEncryptedData,
              extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData, (unsigned)srcptr );
          destptr += extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData;
        }
      }

      encBuffer = new DrmBuffer(&toDecrypt[0],decryptSize);
      if (encBuffer == NULL)
      {
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        return DRM_ERROR_UNKNOWN;
      }
      char* decrypted
          = (char *)malloc(sizeof(OMX_U8) * decryptSize);
      if (decrypted == NULL)
      {
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        delete encBuffer;
        encBuffer = NULL;
        return DRM_ERROR_UNKNOWN;
      }
      memset(decrypted, 0 , sizeof(OMX_U8) * decryptSize);
      decBuffer = new DrmBuffer(&decrypted[0],decryptSize);
      if (decBuffer == NULL)
      {
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        return DRM_ERROR_UNKNOWN;
      }
      iv = new DrmBuffer((char*)&extraSampleInfo->nInitVector[0],extraSampleInfo->nIVSize);
      if (iv == NULL)
      {
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        return DRM_ERROR_UNKNOWN;
      }
#ifdef DRM_FILE_DUMP
// writing video IV content to file - start
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "size of video IV content  %d",
                     extraSampleInfo->nIVSize);
      FILE *videoIVFp = fopen("/data/videoIVContent.yuv", "ab+");
      if (videoIVFp)
      {
        int size = fwrite(extraSampleInfo->nInitVector, 1, MAX_IV_SIZE ,  videoIVFp );
        if (size <= 0)
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                        "error in writing video IV content");
        }

        fclose(videoIVFp);
        videoIVFp = NULL;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(videoIVContent.yuv)");
      }

//writing video IV content to file - end
#endif
      QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
            "[DRM type:%d] decrypt: on port %lu with DecryptUnitId %d ",
            mContentProtectionInfo->eDrmType, pFillBuffCmdResp->nPortIndex, newDecryptUnitId);
      drmerr = mDrmManagerClient->decrypt(
       mDecryptHandle, newDecryptUnitId, encBuffer, &decBuffer, iv);
      if(drmerr != DRM_NO_ERROR)
      {
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        delete iv;
        iv = NULL;
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "decrypt:failed with error %d",drmerr);
        return drmerr;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "decrypt:succeess");
        srcptr = (char*)&decBuffer->data[0];
        for(int i=0;extraSampleInfo && i<extraSampleInfo->nSubSampleCount;i++)
        {
          //QOMX_ENCRYPTED_SUBSAMPLE_INFO ssinfo = extraSampleInfo->sEncSubsampleInfo[i];
          if (extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData > 0)
          {
            destptr = (char*)&pBufHdr->pBuffer[extraSampleInfo->sEncSubsampleInfo[i].nOffsetEncryptedData];
            memcpy(destptr, srcptr, extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData);
            QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "decrypted video content copied to Offset %lu of size %lu destptr 0x%x",
              extraSampleInfo->sEncSubsampleInfo[i].nOffsetEncryptedData,
              extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData,
              (unsigned)destptr );
            srcptr += extraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData;
          }
        }
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "Done:preparing decrypted pbuffer");
        MM_Free(toDecrypt);
        toDecrypt = NULL;
        delete encBuffer;
        encBuffer = NULL;
        MM_Free(decrypted);
        decrypted = NULL;
        delete decBuffer;
        decBuffer = NULL;
        delete iv;
        iv = NULL;
#ifdef DRM_FILE_DUMP
// writing video content to file after decryption - start
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
           "size of decrypted Video pBufHdr->pBuffer is %lu offset %lu",
           pBufHdr->nFilledLen, pBufHdr->nOffset);
        FILE *decVideoFp = fopen("/data/decryptVideo.yuv", "ab+");
        if (decVideoFp)
        {
          int size = fwrite(pBufHdr->pBuffer, 1, pBufHdr->nFilledLen,  decVideoFp );
          if (size <= 0)
          {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "error in writing decrypted pBufHdr->pBuffer");
          }

          fclose(decVideoFp);
          decVideoFp = NULL;
        }
        else
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "Error: fopen(decryptVideo.yuv)");
        }

// writing video content to file after decryption - end
#endif
      }
    }
  }
#else
  encBuffer = new DrmBuffer((char*)&pBufHdr->pBuffer[0],pBufHdr->nAllocLen);
  if (encBuffer == NULL)
  {
    return DRM_ERROR_UNKNOWN;
  }
  char toDecrypt[pBufHdr->nAllocLen];
  decBuffer = new DrmBuffer(&toDecrypt[0],pBufHdr->nAllocLen);
  if (decBuffer == NULL)
  {
    delete encBuffer;
    encBuffer = NULL;
    return DRM_ERROR_UNKNOWN;
  }
  iv = NULL;
  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
      "[Pass Through]decrypt:on port %d",
       pFillBuffCmdResp->nPortIndex);
  drmerr = mDrmManagerClient->decrypt(
        mDecryptHandle, pFillBuffCmdResp->nPortIndex, encBuffer, &decBuffer, iv);
    delete encBuffer;
    encBuffer = NULL;
    delete decBuffer;
    decBuffer = NULL;
#endif
  return drmerr;
}

void DASHMMIInterface::SetLastUniqueID(int nPortIndex, int uniqueId)
{
  if(IS_AUDIO_PORT(nPortIndex))
  {
    mLastAudioUniqueID = uniqueId;
  }
  else if(IS_VIDEO_PORT(nPortIndex))
  {
    mLastVideoUniqueID = uniqueId;
  }
  return;
}


int DASHMMIInterface::GetLastUniqueID(int nPortIndex)
{
  int uniqueId = 0;
  if(IS_AUDIO_PORT(nPortIndex))
  {
    uniqueId = mLastAudioUniqueID;
  }
  else if(IS_VIDEO_PORT(nPortIndex))
  {
    uniqueId = mLastVideoUniqueID;
  }
  return uniqueId;
}

int DASHMMIInterface::DrmInit()
{
  Mutex::Autolock autoLock(mDrmLock);

  if (mDrmManagerClient == NULL)
  {
    mDrmManagerClient = new DrmManagerClient();
    if (mDrmManagerClient == NULL)
    {
      return -1;
    }
  }

  if (mDecryptHandle == NULL)
  {
    // NOTE: openDecryptSession() parameters change for Jellybean
    String8 mimeType("");
    if (mContentProtectionInfo->eDrmType == QOMX_MARLIN_DRM)
    {
      mimeType = MARLIN_DRM_MIMETYPE;
    }
#ifndef USE_DRM_PASSTHRU
    openDecryptBuffer = new DrmBuffer((char*)&mContentProtectionInfo->cContentProtectionData[0],
                                   mContentProtectionInfo->nContentProtectionInfoSize);
#else
    openDecryptBuffer = new DrmBuffer(NULL,0);
#endif
    /*Open Decrypt Session*/
    if (openDecryptBuffer)
    {
      mDecryptHandle = mDrmManagerClient->openDecryptSession(*openDecryptBuffer,mimeType);
      if (mDecryptHandle == NULL)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,"openDecryptSession: Fail");
        delete mDrmManagerClient;
        mDrmManagerClient = NULL;
        return -1;
      }
      else
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "openDecryptSession:success");
      }
    }
    else
    {
      QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "openDecryptSession:openDecryptBuffer Allocation Failed");
      return -1;
    }
  }
  return 0;
}

void DASHMMIInterface::DrmCleanup()
{
  Mutex::Autolock autoLock(mDrmLock);

  if(mDrmManagerClient != NULL && mDecryptHandle != NULL)
  {
    int i;
    status_t drmerr = DRM_NO_ERROR;
    // TODO: determine proper use/value of 'position'
    int64_t position = 0;
    mDrmManagerClient->setPlaybackStatus(mDecryptHandle,Playback::STOP,position);

    for(i=0;i<mNumDecryptUnits;i++)
    {
      drmerr = mDrmManagerClient->finalizeDecryptUnit(mDecryptHandle,mDecryptUnitIds[i]);
      if(drmerr != DRM_NO_ERROR)
      {
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              "Failed to finalize decryption unit on port %d",mDecryptUnitIds[i]);
      }
    }
    mDrmManagerClient->closeDecryptSession(mDecryptHandle);
    mDecryptHandle = NULL;
  }
}

int DASHMMIInterface::OMXPrefetch(OMX_STRING param, void* omxStruct, OMX_INDEXTYPE *omxIndex)
{
    OMX_HANDLETYPE handle = GetMMIHandle();
    MMI_GetExtensionCmdType ext;
    ext.cParamName = param;
    ext.pIndex = omxIndex;
    uint32 mmiret = MMI_S_EFAIL;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if(mmiret != MMI_S_COMPLETE)
    {
      QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "Get Extension Index for %s failed %lu", param, (unsigned long)ext.pIndex);
      return -1;
    }

    omxIndex = ext.pIndex;
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "Get Extension Index for %s is  %x", param, *omxIndex);

    MMI_OmxParamCmdType cmd;
    cmd.nParamIndex = *omxIndex;
    cmd.pParamStruct = omxStruct;
    mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_STD_OMX_PARAM, &cmd);
    if(mmiret != MMI_S_COMPLETE)
    {
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "Get Size for %s failed %x", param, (uint32_t)mmiret);
       return -1;
    }
    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "Get Size for %s: Success", param);
    return 0;
}

void DASHMMIInterface::setPSSHDataSizeOnPort(int portIndex, uint32_t size)
{
   if (IS_AUDIO_PORT(portIndex))
   {
     mAudioPSSHDataSize = size;
   }
   else if (IS_VIDEO_PORT(portIndex))
   {
     mVideoPSSHDataSize = size;
   }
   else  if (IS_TEXT_PORT(portIndex))
   {
     mTextPSSHDataSize = size;
   }
}
uint32_t DASHMMIInterface::getPSSHDataSizeOnPort(int portIndex)
{
  uint32_t nSize = 0;

  if (IS_AUDIO_PORT(portIndex))
  {
    nSize = mAudioPSSHDataSize;
  }
  else if (IS_VIDEO_PORT(portIndex))
  {
   nSize = mVideoPSSHDataSize;
  }
  else  if (IS_TEXT_PORT(portIndex))
  {
    nSize = mTextPSSHDataSize;
  }
  return nSize;
}


/* Only writes to psshPtr on success
 * WARNING: Caller is responsible for freeing the memory
 * allocated to psshPtr via MM_Free()
 */
int DASHMMIInterface::GetPSSHInfo(int portIndex, QOMX_PARAM_STREAMING_PSSHINFO** psshPtr, int uniqueID)
{
    OMX_HANDLETYPE handle = GetMMIHandle();
    QOMX_PARAM_STREAMING_PSSHINFO psshInfo;
    QOMX_STRUCT_INIT(psshInfo,QOMX_PARAM_STREAMING_PSSHINFO);
    psshInfo.nUniqueID = uniqueID;
    psshInfo.nPortIndex = portIndex;
    OMX_INDEXTYPE psshIndex;
    int err = OMXPrefetch((OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_PSSH_INFO,&psshInfo, &psshIndex);

    setPSSHDataSizeOnPort(portIndex, psshInfo.nPsshDataBufSize);

     QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "PSSHDataSize %lu", psshInfo.nPsshDataBufSize);
    if(err != 0)
    {
      if (psshInfo.nPsshDataBufSize == 0)
      {
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                "PsshDataBufSize 0.. track doesn't have PSSH info");
        return 0;
      }
        return -1;
    }

    int psshInfoAllocSize = 4 * sizeof (OMX_U32) +
               sizeof(OMX_VERSIONTYPE) +
               sizeof(OMX_U8) * sizeof(psshInfo.cDefaultKeyID) +
               sizeof(OMX_U8) * psshInfo.nPsshDataBufSize;
    QOMX_PARAM_STREAMING_PSSHINFO* psshInfoPtr
          = (QOMX_PARAM_STREAMING_PSSHINFO *)malloc(sizeof(OMX_U8) * psshInfoAllocSize);
  if (psshInfoPtr)
  {
    memset(psshInfoPtr, 0, psshInfoAllocSize);
    psshInfoPtr->nSize = psshInfoAllocSize;
    psshInfoPtr->nPortIndex = psshInfo.nPortIndex;
    psshInfoPtr->nUniqueID =  psshInfo.nUniqueID;
    psshInfoPtr->nPsshDataBufSize = psshInfo.nPsshDataBufSize;
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "AAL querying using nUniqueID %ld nPsshDataBufSize %lu", psshInfoPtr->nUniqueID,
                 psshInfoPtr->nPsshDataBufSize);

    MMI_OmxParamCmdType cmd;
    cmd.nParamIndex = psshIndex;
    cmd.pParamStruct = psshInfoPtr;
    uint32 mmiret = HTTPMMIDeviceCommand(handle, MMI_CMD_GET_STD_OMX_PARAM, &cmd);
    if(mmiret != MMI_S_COMPLETE)
    {
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "Get PSSH Data failed %x for port %d", (unsigned int)mmiret, portIndex);
        MM_Free(psshInfoPtr);
        return -1;
    }
    SetLastUniqueID(psshInfoPtr->nPortIndex, psshInfoPtr->nUniqueID);
    //PrintPSSHInfo(psshInfoPtr);
    *psshPtr = psshInfoPtr;
  }
  else
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,"Allocate: psshInfoPtr Failed");
    return -1;
  }
    return 0;
}

void DASHMMIInterface::PrintPSSHInfo(QOMX_PARAM_STREAMING_PSSHINFO *pPsshInfo)
{
  QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
              "AAL:UniqueID %ld PSSH buffer size %lu",pPsshInfo->nUniqueID,
               pPsshInfo->nPsshDataBufSize);
  if (pPsshInfo->cDefaultKeyID[0] != '\0')
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                " -- AAL:DefaultKeyID [Byte] --");
    for (uint32 i = 0 ; i < MAX_KID_SIZE; i++)
    {
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                    " DefaultKeyID:[%lu]:0x%x ",i,pPsshInfo->cDefaultKeyID[i]);
    }
  }
  if (pPsshInfo->nPsshDataBufSize > 0)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                " -- AAL:PSSH Atom [Byte] --");
    for (uint32 i = 0 ; i < pPsshInfo->nPsshDataBufSize - 1; i++)
    {
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                    " PSSH Atom:[%lu]0x%x ",i,pPsshInfo->cPSSHData[i]);
    }
  }
  return;
}

void DASHMMIInterface::PrintExtraSampleInfo(QOMX_EXTRA_SAMPLE_INFO *pExtraSampleInfo)
{
  QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "AAL:IsEncrypted %d KeyId_size %d IV_Size %d",
                pExtraSampleInfo->nIsEncrypted, pExtraSampleInfo->nKeyIDSize,
                pExtraSampleInfo->nIVSize);

#if 0
  if (pExtraSampleInfo->nDefaultKeyID[0] != '\0')
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                " -- AAL:DefaultKeyID [Byte Print] --");
    for (int i = 0 ; i < MAX_KID_SIZE; i++)
    {
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                    "DefaultKeyID:[%d]0x%x ",i,pExtraSampleInfo->nDefaultKeyID[i]);
    }
  }

  if (pExtraSampleInfo->nKeyIDSize > 0)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              " -- AAL:KeyID [Byte Print] --");
    for (int i = 0 ; i < MAX_KID_SIZE; i++)
    {
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  " KeyID:[%d]0x%x ",i,pExtraSampleInfo->nKeyID[i]);
    }
  }

  if (pExtraSampleInfo->nIVSize > 0)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
              " -- AAL:InitVector [Byte Print] --");
    for (int i = 0 ; i < pExtraSampleInfo->nIVSize; i++)
    {
      QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  "InitVector:[%d]0x%x ",i,pExtraSampleInfo->nInitVector[i]);
    }
  }

#endif
  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                "AAL:SubsampleCount %d",
                pExtraSampleInfo->nSubSampleCount);

  for (int i = 0; i < pExtraSampleInfo->nSubSampleCount ; i++)
  {
    QTV_MSG_PRIO5(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                  "[%d]SizeOfClearData [%d] OffsetClearData [%lu] SizeOfEncryptedData [%lu] OffsetEncryptedData[%lu]",
                  i,
                  pExtraSampleInfo->sEncSubsampleInfo[i].nSizeOfClearData,
                  pExtraSampleInfo->sEncSubsampleInfo[i].nOffsetClearData,
                  pExtraSampleInfo->sEncSubsampleInfo[i].nSizeOfEncryptedData,
                  pExtraSampleInfo->sEncSubsampleInfo[i].nOffsetEncryptedData);
  }

  return;
}

bool DASHMMIInterface::IsSeekable()
{
  bool bSeekable = false;

  MMI_GetExtensionCmdType extnData;
  OMX_INDEXTYPE extnIndex;
  extnData.cParamName = OMX_QCOM_INDEX_PARAM_SEEK_ACCESS;
  extnData.pIndex = &extnIndex;

  if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &extnData) == MMI_S_COMPLETE)
  {
    MMI_OmxParamCmdType paramData;
    QOMX_PARAM_SEEKACCESSTYPE seekAccess;
    QOMX_STRUCT_INIT(seekAccess, QOMX_PARAM_SEEKACCESSTYPE);
    seekAccess.nPortIndex = OMX_ALL;
    seekAccess.nSize = sizeof(QOMX_PARAM_SEEKACCESSTYPE);
    seekAccess.bSeekAllowed = OMX_FALSE;
    paramData.nParamIndex = extnIndex;
    paramData.pParamStruct = &seekAccess;
    if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &paramData) == MMI_S_COMPLETE)
    {
      bSeekable = (seekAccess.bSeekAllowed == OMX_TRUE);
    }
  }

  return bSeekable;
}

status_t DASHMMIInterface::getRepositionRange(uint64_t* pMin, uint64_t* pMax, uint64_t* pMaxDepth)
{
  status_t ret = UNKNOWN_ERROR;
  MMI_GetExtensionCmdType extnData;
  OMX_INDEXTYPE extnIndex;
  extnData.cParamName = OMX_QUALCOMM_INDEX_CONFIG_DASH_REPOSITION_RANGE;
  extnData.pIndex = &extnIndex;

  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
        "DASHMMIInterface::getRepositionRange MMI_CMD_GET_EXTENSION_INDEX %s", extnData.cParamName);

  if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &extnData) == MMI_S_COMPLETE)
  {
    MMI_OmxParamCmdType paramData;
    QOMX_DASH_REPOSITION_RANGE reposRange;
    QOMX_STRUCT_INIT(reposRange, QOMX_DASH_REPOSITION_RANGE);
    reposRange.nPortIndex = OMX_ALL;
    reposRange.nSize = sizeof(QOMX_DASH_REPOSITION_RANGE);
    reposRange.nMin = reposRange.nMax = 0;
    paramData.nParamIndex = extnIndex;
    paramData.pParamStruct = &reposRange;
    if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &paramData) == MMI_S_COMPLETE)
    {
      *pMin = reposRange.nMin;
      *pMax = reposRange.nMax;
      *pMaxDepth = reposRange.nMaxDepth;
       ret = OK;
       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
        "DASHMMIInterface::getRepositionRange %lld %lld", *pMin, *pMax);
    }
  }

  return ret;
}

bool DASHMMIInterface::isPlaybackDiscontinued()
{
  bool bDiscontinuity = false;
  MMI_GetExtensionCmdType extnData;
  OMX_INDEXTYPE extnIndex;

  extnData.cParamName = OMX_QUALCOMM_INDEX_CONFIG_DASH_RESUME_DISCONTINUITY;
  extnData.pIndex = &extnIndex;
  if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &extnData) == MMI_S_COMPLETE)
  {
    MMI_OmxParamCmdType paramData;
    QOMX_DASH_RESUME_DISCONTINUITY resumeDisc;
    QOMX_STRUCT_INIT(resumeDisc, QOMX_DASH_RESUME_DISCONTINUITY);
    resumeDisc.nPortIndex = OMX_ALL;
    resumeDisc.nSize = sizeof(QOMX_DASH_RESUME_DISCONTINUITY);
    resumeDisc.bDiscontinuity = OMX_FALSE;
    paramData.nParamIndex = extnIndex;
    paramData.pParamStruct = &resumeDisc;
    if (HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &paramData) == MMI_S_COMPLETE)
    {
      bDiscontinuity = (resumeDisc.bDiscontinuity == OMX_TRUE);
    }
  }

  return bDiscontinuity;
}

void DASHMMIInterface::QueryStreamType(OMX_U32 nPort)
{

    // Query Dash for the Stream type info and set that with in DASH Interface

    QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                 "Query stream type on Port : %lu",nPort);
    OMX_HANDLETYPE handle = NULL;
    MMI_GetExtensionCmdType ext;
    OMX_INDEXTYPE bufDurIndex = OMX_IndexComponentStartUnused;
    OMX_U32 ret = MMI_S_EFAIL;

    ext.cParamName = (OMX_STRING)OMX_QCOM_INDEX_CONFIG_MEDIAINFO;
    ext.pIndex = &bufDurIndex;
    handle = GetMMIHandle();

    ret = postCmd(handle,MMI_CMD_GET_EXTENSION_INDEX,&ext);

    if (ret == MMI_S_COMPLETE)
    {
      MMI_OmxParamCmdType cmd;
      QOMX_MEDIAINFOTYPE *pExtMediaInfo;
      uint32 size = sizeof(QOMX_MEDIAINFOTYPE) + sizeof(QOMX_MEDIASTREAMTYPE);
      pExtMediaInfo = (QOMX_MEDIAINFOTYPE*)MM_Malloc(size * sizeof(OMX_U8));

      if (pExtMediaInfo)
      {
        QOMX_STRUCT_INIT(*pExtMediaInfo, QOMX_MEDIAINFOTYPE);
        pExtMediaInfo->nSize = size;
        pExtMediaInfo->nPortIndex = nPort;
        pExtMediaInfo->eTag = QOMX_MediaInfoTagMediaStreamType;
        pExtMediaInfo->nDataSize = sizeof(QOMX_MEDIASTREAMTYPE);
        cmd.nParamIndex = bufDurIndex;
        cmd.pParamStruct = pExtMediaInfo;
        ret = postCmd(handle,MMI_CMD_GET_STD_OMX_PARAM,&cmd);
        if (ret == MMI_S_COMPLETE)
        {
          QOMX_MEDIASTREAMTYPE *pStreamType = (QOMX_MEDIASTREAMTYPE *)pExtMediaInfo->cData;
          DashStreamType streamType = DASHMMIInterface::DASH_STREAM_TYPE_VOD;

          if(pStreamType->eStreamType == QOMX_STREAMTYPE_LIVE)
          {
             streamType = DASHMMIInterface::DASH_STREAM_TYPE_LIVE;
          }
          else if (pStreamType->eStreamType == QOMX_STREAMTYPE_VOD)
          {
            streamType = DASHMMIInterface::DASH_STREAM_TYPE_VOD;
          }
          QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                       "Stream type on Port %lu set to %d ",nPort,streamType);
          SetStreamType(streamType);
        }
        MM_Free(pExtMediaInfo);
      } /*if (pExtMediaInfo)*/
    }
}



/*
 * Returns the current HTTP AAL State
 *
 * return refernce to HTTPAALStateBase derived State
 */
CHTTPAALStateBase *DASHMMIInterface::GetCurrentHTTPAALState()
{
  CHTTPAALStateBase *pHTTPAALCurrentState = NULL;
  MM_CriticalSection_Enter(m_hHTTPAALStateLock);
  pHTTPAALCurrentState = m_pCurrentHTTPAALState;
  MM_CriticalSection_Leave(m_hHTTPAALStateLock);
  return pHTTPAALCurrentState;
}


/*
 * Initiates the transtion to the new state
 *
 * @param[in] eHTTPAALState RTP MMI State to transtion to
 * @param[in] HTTPAALRsp MMI response that is sent after the transition is
 * complete
 *
 * @return true on success else failure
 */
bool DASHMMIInterface::SetHTTPAALState(HTTPAALState eHTTPAALState)
{
  bool bReturn = false;
  CHTTPAALStateBase *pHTTPAALCurrentState = GetCurrentHTTPAALState();
  if(pHTTPAALCurrentState)
  {
    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                  "SetHTTPAALState from %d to %d",
                  pHTTPAALCurrentState->GetState(), eHTTPAALState);
    bReturn = pHTTPAALCurrentState->ExitHandler();
    if (bReturn)
    {
      MM_CriticalSection_Enter(m_hHTTPAALStateLock);
      m_pCurrentHTTPAALState = m_pHTTPAALStates[eHTTPAALState];
      pHTTPAALCurrentState = m_pCurrentHTTPAALState;
      MM_CriticalSection_Leave(m_hHTTPAALStateLock);
      bReturn = pHTTPAALCurrentState->EntryHandler();
    }
  }
  return bReturn;
}

/*
 * handles asynchronous events from MMI
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 * @param[in] pClientData client data provided by AAL at registration
 *
 * @return
 */
void DASHMMIInterface::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                    OMX_IN OMX_U32 nEvtStatus,
                                    OMX_IN OMX_U32 nPayloadLen,
                                    OMX_IN OMX_PTR pEvtData,
                                    OMX_IN OMX_PTR pClientData)
{
  DASHMMIInterface *pDataCache = (DASHMMIInterface*)pClientData;

  if (pDataCache)
  {
    CHTTPAALStateBase *pCurrState = pDataCache->GetCurrentHTTPAALState();
    QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_DEBUG,
                  "EventHandler Process event %lu %lu by state %d",
                  nEvtCode, nEvtStatus, pCurrState->GetState());

    pCurrState->EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
  }
  else
  {
    QTV_MSG_PRIO3(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  "EventHandler Drop event %lu %lu %x",
                  nEvtCode, nEvtStatus, (unsigned)pEvtData );
  }
}

/*
 * process commands given by framework
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
status_t DASHMMIInterface::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                      int32 arg1, int32 arg2, int64 arg3)
{
  status_t err = UNKNOWN_ERROR;
  OMX_U32 nReturn = 0;

  CHTTPAALStateBase *pCurrState = GetCurrentHTTPAALState();
  if (pCurrState)
  {
    nReturn = pCurrState->ProcessCmd(cmd, arg1, arg2, arg3);
    err = MapMMIToAALStatus(nReturn);
  }

  return err;
}

uint32 DASHMMIInterface::ProcessSeek(int32_t seekMode, int64_t seekTimeUs)
{
  uint32 ret = MMI_S_EFAIL;
  MMI_CustomParamCmdType data;

  MM_CriticalSection_Enter(m_hHTTPAALSeekLock);

  if (!IsSeekable())
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
      "DASHMMIInterface::seek - stream not seekable" );
    ret = MMI_S_EFAIL;
  }
  else
  {
    OMX_TIME_CONFIG_SEEKMODETYPE paramStruct;
    QOMX_STRUCT_INIT(paramStruct, OMX_TIME_CONFIG_SEEKMODETYPE);
    paramStruct.eType = (OMX_TIME_SEEKMODETYPE) seekMode;
    data.nParamIndex = (MMI_IndexType) OMX_IndexConfigTimeSeekMode;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &data);
    if (ret == MMI_S_COMPLETE)
    {
      OMX_TIME_CONFIG_TIMESTAMPTYPE paramTSStruct;
      QOMX_STRUCT_INIT(paramTSStruct, OMX_TIME_CONFIG_TIMESTAMPTYPE);
      paramTSStruct.nTimestamp = seekTimeUs;
      paramTSStruct.nPortIndex = OMX_ALL;
      data.nParamIndex = (MMI_IndexType) OMX_IndexConfigTimePosition;
      data.pParamStruct = &paramTSStruct;
      ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &data);
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
        "DASHMMIInterface::seek - sent to DASH source - result = %lu", ret);
    }
  }

  MM_CriticalSection_Leave(m_hHTTPAALSeekLock);

  return ret;
}

/*
 * Wait for a signal
 *
 * @param[in] time is the time need to wait for
 * @param[out] timeout if 'time' interval expires, this is set
 *
 * @return status of the wait
 */
uint32 DASHMMIInterface::Wait(uint32 signalset, int time, int *timeout)
{
  return m_signalHandler.Wait((HTTPAALSignalSet)signalset, time, timeout);
}

/*
 * Signal (wakeup) the waiting thread upon recving a signal
 *
 * @param[in] signal signal to be invoked
 *
 * @return status of the Signal
 */
int32 DASHMMIInterface::Signal(HTTPAALSignalSet signal)
{
  return m_signalHandler.Signal(signal);
}

/*
 * Get MMI Handle
 *
 * @param[in]
 *
 * @return MMI handle
 */
OMX_HANDLETYPE DASHMMIInterface::GetMMIHandle()
{
  return m_handle;
}

/*
 * Set MMI Handle
 *
 * @param[in] hanlde
 *
 * @return none
 */
void DASHMMIInterface::SetMMIHandle(OMX_HANDLETYPE handle)
{
  m_handle = handle;
}

/*
 * Maps MMI to AAL (stagefright) error codes
 *
 * @param[in] nValue MMI status code
 *
 * @return mapped error code
 */
status_t DASHMMIInterface::MapMMIToAALStatus(uint32 nValue)
{
  status_t status;

  switch(nValue)
  {
  case MMI_S_COMPLETE:
    status = OK;
    break;
  case MMI_S_PENDING:
    status = WOULD_BLOCK;
    break;
  case MMI_S_ENOTIMPL:
    status = ERROR_UNSUPPORTED;  //!Warnign
    break;
    //todo
  //case MMI_S_
    //status = ERROR_END_OF_STREAM;
  case MMI_S_EBUFFREQ:
    status = ERROR_BUFFER_TOO_SMALL;
    break;
  case MMI_S_ECMDQFULL:
    status = NOT_ENOUGH_DATA;
    break;
  case MMI_S_EINVALSTATE:
    status = INVALID_OPERATION;
    break;
  case MMI_S_EFAIL:
  case MMI_S_EFATAL:
  default:
    status = UNKNOWN_ERROR;
    break;
  }

  return status;
}

/*
 * Open HTTP MMI device
 *
 * @param[in]
 *
 * @return status of the operation
 */
OMX_U32 DASHMMIInterface::OpenMMI(const KeyedVector<String8, String8> *headers)
{
  OMX_U32 ret = MMI_S_EFAIL;

  MMI_CustomParamCmdType data;

  //audio init
  MMI_ParamBuffersReqType paramStruct;
  paramStruct.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
  data.nParamIndex = MMI_IndexBuffersReq;
  data.pParamStruct = &paramStruct;
  ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);

  if (IS_SUCCESS(ret))
  {
    MMI_ParamDomainDefType paramStruct;
    paramStruct.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
    data.nParamIndex = MMI_IndexDomainDef;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);
  }


  //video init
  if (IS_SUCCESS(ret))
  {
    MMI_ParamBuffersReqType paramStruct;
    paramStruct.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
    data.nParamIndex = MMI_IndexBuffersReq;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);
  }

  if (IS_SUCCESS(ret))
  {
    MMI_ParamDomainDefType paramStruct;
    paramStruct.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
    data.nParamIndex = MMI_IndexDomainDef;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);
  }

  if (IS_SUCCESS(ret))
  {
    MMI_ParamBuffersReqType paramStruct;
    paramStruct.nPortIndex = MMI_HTTP_OTHER_PORT_INDEX;
    data.nParamIndex = MMI_IndexBuffersReq;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);
  }

  if (IS_SUCCESS(ret))
  {
    MMI_ParamDomainDefType paramStruct;
    paramStruct.nPortIndex = MMI_HTTP_OTHER_PORT_INDEX;
    data.nParamIndex = MMI_IndexDomainDef;
    data.pParamStruct = &paramStruct;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_CUSTOM_PARAM, &data);
  }

  if (IS_SUCCESS(ret))
  {
    AddOemHeaders(headers);
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                 "Add OEM Headers)");
  }

  if (IS_SUCCESS(ret))
  {
    //set audio port to auto detect
    MMI_CustomParamCmdType autoDetect;
    MMI_ParamDomainDefType paramAutoDetect;
    paramAutoDetect.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
    paramAutoDetect.format.audio.eEncoding = OMX_AUDIO_CodingAutoDetect;
    autoDetect.nParamIndex = MMI_IndexDomainDef;
    autoDetect.pParamStruct = &paramAutoDetect;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_CUSTOM_PARAM, &autoDetect);

    //set video port to auto detect
    paramAutoDetect.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
    paramAutoDetect.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    autoDetect.nParamIndex = MMI_IndexDomainDef;
    autoDetect.pParamStruct = &paramAutoDetect;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_CUSTOM_PARAM, &autoDetect);

    //set text port to auto detect
    paramAutoDetect.nPortIndex = MMI_HTTP_OTHER_PORT_INDEX;
    paramAutoDetect.format.other.eFormat = (OMX_OTHER_FORMATTYPE)QOMX_OTHER_CodingAutoDetect;
    autoDetect.nParamIndex = MMI_IndexDomainDef;
    autoDetect.pParamStruct = &paramAutoDetect;
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_CUSTOM_PARAM, &autoDetect);
  }

  if (IS_SUCCESS(ret))
  {
     MMI_GetExtensionCmdType ext;
     ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_CONFIG_WATERMARK;

     ret = GetOmxIndexByExtensionString(ext);
     if (IS_SUCCESS(ret))
     {
        MMI_OmxParamCmdType cmd;
        QOMX_BUFFERINGWATERMARKTYPE watermark;
        watermark.nSize = sizeof(QOMX_BUFFERINGWATERMARKTYPE);
        watermark.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
        watermark.eWaterMark = QOMX_WATERMARK_NORMAL;
        watermark.eUnitsType = QOMX_WATERMARKUNITSTYPE_Time;
        watermark.nLevel = MAX_UINT32;
        watermark.bEnable = OMX_TRUE;

        cmd.nParamIndex = mWatermarkExtIndex;
        cmd.pParamStruct = &watermark;
        HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &cmd);

        watermark.nLevel = MAX_UINT32;
        watermark.eWaterMark = QOMX_WATERMARK_UNDERRUN;
        watermark.bEnable = OMX_TRUE;
        cmd.nParamIndex = mWatermarkExtIndex;
        cmd.pParamStruct = &watermark;
        ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &cmd);


        MMI_OmxParamCmdType cmd1;
        QOMX_BUFFERINGWATERMARKTYPE watermark1;
        watermark1.nSize = sizeof(QOMX_BUFFERINGWATERMARKTYPE);
        watermark1.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
        watermark1.eWaterMark = QOMX_WATERMARK_NORMAL;
        watermark1.eUnitsType = QOMX_WATERMARKUNITSTYPE_Time;
        watermark1.nLevel = MAX_UINT32;
        watermark1.bEnable = OMX_TRUE;

        cmd1.nParamIndex = mWatermarkExtIndex;
        cmd1.pParamStruct = &watermark1;
        HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &cmd1);

        watermark.nLevel = MAX_UINT32;
        watermark1.eWaterMark = QOMX_WATERMARK_UNDERRUN;
        watermark.bEnable = OMX_TRUE;
        cmd1.nParamIndex = mWatermarkExtIndex;
        cmd1.pParamStruct = &watermark1;
        ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &cmd1);


        MMI_GetExtensionCmdType ext;
        ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_CONFIG_WATERMARKSTATUS;
        ret = GetOmxIndexByExtensionString(ext);
        if (IS_SUCCESS(ret))
        {
           QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                        "mWatermarkStatusExtIndex val (%d)",mWatermarkStatusExtIndex);
        }
     }
  }
  if (IS_SUCCESS(ret))
  {
    //issue start to mmi
    ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_START, NULL);
  }

  if (IS_SUCCESS(ret))
  {
    MM_CriticalSection_Enter(m_hHTTPAALStateLock);
    SetHTTPAALState(HTTPAALStateConnecting);
    MM_CriticalSection_Leave(m_hHTTPAALStateLock);
  }
  setAALDRMError(DRM_NO_ERROR);
  return ret;
}


OMX_U32 DASHMMIInterface::GetOmxIndexByExtensionString(MMI_GetExtensionCmdType &nMMICmdType)
{
    OMX_U32 ret = MMI_S_EFAIL;

    if (strncmp((char *)OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_DIMENSIONS,
                 nMMICmdType.cParamName,
                 strlen(nMMICmdType.cParamName)) == 0)
    {
         nMMICmdType.pIndex = &mSMTPTimedTextDimensionsIndex;

         ret = HTTPMMIDeviceCommand(GetMMIHandle(), MMI_CMD_GET_EXTENSION_INDEX, &nMMICmdType);
         if (IS_SUCCESS(ret))
         {
            QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                          "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_DIMENSIONS) Index success (0x%x)",
                           (OMX_EXTRADATATYPE)*nMMICmdType.pIndex);
            ret = MMI_S_COMPLETE;
         }
         else
         {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_DIMENSIONS) Index Failed");
         }
    }
    else if (strncmp((char *)OMX_QUALCOMM_INDEX_CONFIG_WATERMARK,
                      nMMICmdType.cParamName,
                      strlen(nMMICmdType.cParamName)) == 0)
    {
         nMMICmdType.pIndex = &mWatermarkExtIndex;
         ret = HTTPMMIDeviceCommand(GetMMIHandle(), MMI_CMD_GET_EXTENSION_INDEX, &nMMICmdType);
         if (IS_SUCCESS(ret))
         {
            QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                            "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_CONFIG_WATERMARK) success (0x%x)",
                            (OMX_EXTRADATATYPE)*nMMICmdType.pIndex);
            ret = MMI_S_COMPLETE;
         }
         else
         {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                          "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_CONFIG_WATERMARK) Index Failed");
         }
     }
     else if (strncmp((char *)OMX_QUALCOMM_INDEX_CONFIG_WATERMARKSTATUS,
                       nMMICmdType.cParamName,
                       strlen(nMMICmdType.cParamName)) == 0)
     {
             nMMICmdType.pIndex = &mWatermarkStatusExtIndex;
             ret = HTTPMMIDeviceCommand(GetMMIHandle(), MMI_CMD_GET_EXTENSION_INDEX, &nMMICmdType);
             if (IS_SUCCESS(ret))
             {
                 QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                               "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_CONFIG_WATERMARKSTATUS) success (0x%x)",
                                (OMX_EXTRADATATYPE)*nMMICmdType.pIndex);
                 ret = MMI_S_COMPLETE;
             }
             else
             {
                 QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                              "CHTTPAALStateConnecting GetOmxIndexByExtensionString (OMX_QUALCOMM_INDEX_CONFIG_WATERMARKSTATUS) Index Failed");
             }
    }
    else if (strncmp((char *)OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_SUBINFO,
                     nMMICmdType.cParamName,
                     strlen(nMMICmdType.cParamName)) == 0)
    {
         char *str = (char *)OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_SUBINFO;
         nMMICmdType.cParamName = str;
         nMMICmdType.pIndex = &mSMTPTimedTextSubInfoIndex;
         ret = HTTPMMIDeviceCommand(GetMMIHandle(), MMI_CMD_GET_EXTENSION_INDEX, &nMMICmdType);
         if (IS_SUCCESS(ret))
         {
             QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
             "CHTTPAALStateConnectin::GetOmxIndexByExtensionString GetTimedTextSubInfo Index success (0x%x)",
             (OMX_EXTRADATATYPE)*nMMICmdType.pIndex);
             ret = MMI_S_COMPLETE;
         }
         else
         {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "CHTTPAALStateConnecting::GetOmxIndexByExtensionString GetTimedTextSubInfo Index Failed");
         }
    }
    return ret;
}

void DASHMMIInterface::AddOemHeaders(const KeyedVector<String8, String8> *headers)
{
  OMX_INDEXTYPE protocolHeaderIndex;
  OMX_U32 err = MMI_S_EBADPARAM;
  int numHeaders = 0;

  if (!headers)
  {
    return;
  }

  numHeaders = headers->size();
  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "DASHMMIInterface::AddOemHeaders size=%d",
                numHeaders);

  for (int i=0; i<numHeaders; i++)
  {
    const char* hdrMsgToAdd = "";
    const char* hdrName = headers->keyAt(i);
    const char* hdrValue = headers->valueAt(i);

    QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                  "DASHMMIInterface::AddOemHeaders Header[\"%s\",\"%s\"]",
                  hdrName, hdrValue);

    MMI_GetExtensionCmdType ext;
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_CONFIG_STREAMING_PROTOCOLHEADER;
    ext.pIndex = &protocolHeaderIndex;
    err = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if (err == MMI_S_COMPLETE)
    {
      MMI_OmxParamCmdType cmd;
      int msgHdrSize = std_strlen(hdrMsgToAdd) + std_strlen(hdrName) +
                       std_strlen(hdrValue) + 1;
      int size = sizeof(QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE) + msgHdrSize;
      QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE* configHdr =
                  (QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE *)MM_Malloc(size);
      if (configHdr)
      {
        QOMX_STRUCT_INIT(*configHdr, QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE);
        configHdr->nSize = size;
        configHdr->eMessageType = QOMX_STREAMING_PROTOCOLMESSAGE_REQUEST;
        configHdr->eActionType = QOMX_STREAMING_PROTOCOLHEADERACTION_ADD;
        configHdr->nMessageClassSize = std_strlen(hdrMsgToAdd);
        configHdr->nHeaderNameSize = std_strlen(hdrName);
        configHdr->nHeaderValueSize = std_strlen(hdrValue);
        std_strlprintf((char *)configHdr->messageHeader, msgHdrSize, "%s%s%s",
                     hdrMsgToAdd, hdrName, hdrValue);
        cmd.nParamIndex = protocolHeaderIndex;
        cmd.pParamStruct = configHdr;
        HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &cmd);
        MM_Free(configHdr);
      } /*if (configHdr)*/
    }
  } /*for (int i=0; i< headers.size(); i++)*/
}

/*
 * Get specified AAL attribute
 *
 * @param[in] nKey Attribute key
 * @param[out] nVal Attribute value
 *
 * @return status of the get operation
 */
bool DASHMMIInterface::GetAttribute
(
 const HTTPAALAttributeKey nKey,
 HTTPAALAttributeValue& nVal
)
{
  bool bOk = false;

  CHTTPAALStateBase *pCurrState = GetCurrentHTTPAALState();
  if (pCurrState)
  {
    bOk = pCurrState->GetAttribute(nKey, nVal);
  }

  return bOk;
}

/*
 * Set specified AAL attribute
 *
 * @param[in] nKey Attribute key
 * @param[in] nVal Attribute value
 *
 * @return status of the get operation
 */
bool DASHMMIInterface::SetAttribute
(
 const HTTPAALAttributeKey nKey,
 const HTTPAALAttributeValue nVal
)
{
  bool bOk = false;

  CHTTPAALStateBase *pCurrState = GetCurrentHTTPAALState();
  if (pCurrState)
  {
    bOk = pCurrState->SetAttribute(nKey, nVal);
  }

  return bOk;
}

/**
 * C'tor
 * @param[in]
 *
 * @return
 */
CHTTPAALStateBase::CHTTPAALStateBase(DASHMMIInterface &pDataCache,
                                     HTTPAALState eHTTPAALState):
                                     m_pMMHTTPDataCache(pDataCache),
                                     m_eHTTPAALState(eHTTPAALState)
{
}

/**
 * D'tor
 * @param[in]
 *
 * @return
 */
CHTTPAALStateBase::~CHTTPAALStateBase()
{
}

/*
 * EntryHandler, called when enters state
 *
 * @param[in] none
 *
 * @return true when successful, false otherwise
 */
bool CHTTPAALStateBase::EntryHandler()
{
  QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
               "CHTTPAALStateBase::EntryHandler");
  return true;
}

/*
 * process commands given by framework
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 * @param[in] arg3 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStateBase::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                           int32 arg1, int32 arg2, int64 arg3)
{
  OMX_U32 ret = MMI_S_EFAIL;
  if (DASHMMIInterface::AAL_STOP == cmd)
  {
    MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    m_pMMHTTPDataCache.SetHTTPAALState(HTTPAALStateClosing);
    if (m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
    {
      m_pMMHTTPDataCache.mHttpLiveSrcObj->setFinalResult((status_t)UNKNOWN_ERROR);
    }
    MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    ret = MMI_S_PENDING;
  }

  return ret;
}

/*
 * handles asynchronous events from MMI in AAL state base
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStateBase::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  switch(nEvtCode)
  {
  case MMI_RESP_STOP:
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                 "CHTTPAALStateBase::EventHandler, "
                 "recved stop rsp, closing device");
    m_pMMHTTPDataCache.DrmCleanup();
    break;

  case MMI_RESP_FLUSH:
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                 "CHTTPAALStateBase::EventHandler, "
                 "recved flush rsp ");
    if (nEvtStatus == MMI_S_COMPLETE){
        m_pMMHTTPDataCache.Signal(DASHMMIInterface::AAL_WAKEUP_SIG);
    }
    break;

  case MMI_EVT_QOMX_EXT_SPECIFIC:
    if (nEvtStatus == MMI_S_COMPLETE)
    {
        MMI_ExtSpecificMsgType *pExtSpecificInfo = (MMI_ExtSpecificMsgType *)pEvtData;
        QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                  "EventHandler - ext specific");
        if ( (pExtSpecificInfo != NULL) &&
             (pExtSpecificInfo->nData2 == (OMX_U32)m_pMMHTTPDataCache.mWatermarkStatusExtIndex))
        {
           QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
               "CHTTPAALStateBase::EventHandler, buffering status received in port %lu ", pExtSpecificInfo->nData1);

           if (IS_AUDIO_PORT((OMX_U32)pExtSpecificInfo->nData1))
           {
              OMX_U32 ret = MMI_S_EFAIL;
              MMI_OmxParamCmdType cmd;
              QOMX_BUFFERINGSTATUSTYPE bufferingStatus;
              bufferingStatus.nSize = sizeof(bufferingStatus);
              bufferingStatus.nPortIndex = pExtSpecificInfo->nData1;
              cmd.nParamIndex = m_pMMHTTPDataCache.mWatermarkStatusExtIndex;
              cmd.pParamStruct = &bufferingStatus;
              ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);
              QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                   "CHTTPAALStateBase::EventHandler, buffering status  %d ", (int)bufferingStatus.eCurrentWaterMark);
              // Notify DASHHTTPLive Source about the Events (only incase of Audio)
              if (bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_UNDERRUN)
              {
                 m_pMMHTTPDataCache.mBufferingIndication |= 0x02;
                 if (m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                     m_pMMHTTPDataCache.mHttpLiveSrcObj->isMiddleOfPlayback())
                 {
                     sp<DASHMMIInterface::CPortsValidity> mAudObj = m_pMMHTTPDataCache.getObjectByPort((uint32_t)pExtSpecificInfo->nData1);
                     if (mAudObj != NULL)
                     {
                        mAudObj->setStatus(DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                        m_pMMHTTPDataCache.mHttpLiveSrcObj->AudioNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                      }
                 }
                 else if (m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                          m_pMMHTTPDataCache.canBufferingBeSent((uint32_t)pExtSpecificInfo->nData1, DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START))
                 {
                   m_pMMHTTPDataCache.mHttpLiveSrcObj->AudioNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                 }
              }
              else if(bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_NORMAL)
              {
                  m_pMMHTTPDataCache.mBufferingIndication &= 0x01;
                  if (m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                      m_pMMHTTPDataCache.mHttpLiveSrcObj->isMiddleOfPlayback())
                  {
                     sp<DASHMMIInterface::CPortsValidity> mAudObj = m_pMMHTTPDataCache.getObjectByPort((uint32_t)pExtSpecificInfo->nData1);
                     if (mAudObj != NULL)
                     {
                        mAudObj->setStatus(DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                        m_pMMHTTPDataCache.mHttpLiveSrcObj->AudioNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                     }
                  }
                  else if(m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                          m_pMMHTTPDataCache.canBufferingEndBeSent((uint32_t)pExtSpecificInfo->nData1, DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END))
                  {
                    m_pMMHTTPDataCache.mHttpLiveSrcObj->AudioNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                  }
              }
           }
           else if (IS_VIDEO_PORT((OMX_U32)pExtSpecificInfo->nData1))
           {
              OMX_U32 ret = MMI_S_EFAIL;
              MMI_OmxParamCmdType cmd;
              QOMX_BUFFERINGSTATUSTYPE bufferingStatus;
              bufferingStatus.nSize = sizeof(bufferingStatus);
              bufferingStatus.nPortIndex = pExtSpecificInfo->nData1;
              cmd.nParamIndex = m_pMMHTTPDataCache.mWatermarkStatusExtIndex;
              cmd.pParamStruct = &bufferingStatus;
              ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);
              QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "CHTTPAALStateBase::EventHandler, buffering status  %d ", (int)bufferingStatus.eCurrentWaterMark);
              // Notify DASHHTTPLive Source about the Events (only incase of Audio)
              if (bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_UNDERRUN)
              {
                 m_pMMHTTPDataCache.mBufferingIndication |= 0x01;
                 if (m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                     m_pMMHTTPDataCache.mHttpLiveSrcObj->isMiddleOfPlayback())
                 {
                     sp<DASHMMIInterface::CPortsValidity> mVidObj = m_pMMHTTPDataCache.getObjectByPort((uint32_t)pExtSpecificInfo->nData1);
                     if (mVidObj != NULL)
                     {
                        mVidObj->setStatus(DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                        m_pMMHTTPDataCache.mHttpLiveSrcObj->VideoNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                     }
                  }
                  else if(m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                          m_pMMHTTPDataCache.canBufferingBeSent((uint32_t)pExtSpecificInfo->nData1, DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START))
                  {
                    m_pMMHTTPDataCache.mHttpLiveSrcObj->VideoNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                  }
               }
               else if(bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_NORMAL)
               {
                  m_pMMHTTPDataCache.mBufferingIndication &= 0x02;
                  if (m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                      m_pMMHTTPDataCache.mHttpLiveSrcObj->isMiddleOfPlayback())
                  {
                      sp<DASHMMIInterface::CPortsValidity> mVidObj = m_pMMHTTPDataCache.getObjectByPort((uint32_t)pExtSpecificInfo->nData1);
                      if (mVidObj != NULL)
                      {
                         mVidObj->setStatus(DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                         m_pMMHTTPDataCache.mHttpLiveSrcObj->VideoNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                      }
                  }
                  else if(m_pMMHTTPDataCache.mHttpLiveSrcObj &&
                          m_pMMHTTPDataCache.canBufferingEndBeSent((uint32_t)pExtSpecificInfo->nData1, DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END))
                  {
                       m_pMMHTTPDataCache.mHttpLiveSrcObj->VideoNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                  }
               }
           }
           else if (IS_TEXT_PORT((OMX_U32)pExtSpecificInfo->nData1))
           {
              OMX_U32 ret = MMI_S_EFAIL;
              MMI_OmxParamCmdType cmd;
              QOMX_BUFFERINGSTATUSTYPE bufferingStatus;
              bufferingStatus.nSize = sizeof(bufferingStatus);
              bufferingStatus.nPortIndex = pExtSpecificInfo->nData1;
              cmd.nParamIndex = m_pMMHTTPDataCache.mWatermarkStatusExtIndex;
              cmd.pParamStruct = &bufferingStatus;
              ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);
              QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                  "CHTTPAALStateBase::EventHandler, buffering status   %d ", (int)bufferingStatus.eCurrentWaterMark);
              // Notify DASHHTTPLive Source about the Events (only incase of Audio)
              if((m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)  &&
                 (!m_pMMHTTPDataCache.mHttpLiveSrcObj->isMiddleOfPlayback()))
              {
                 if (bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_UNDERRUN)
                 {
                    m_pMMHTTPDataCache.mHttpLiveSrcObj->TextNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START);
                 }
                 else if(bufferingStatus.eCurrentWaterMark == QOMX_WATERMARK_NORMAL)
                 {
                    m_pMMHTTPDataCache.mHttpLiveSrcObj->TextNotifyCB(NULL,DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END);
                 }
              }
           }
       }

       else if ( (pExtSpecificInfo != NULL) &&
             (pExtSpecificInfo->nData2 == (OMX_U32)m_pMMHTTPDataCache.mQOEDataPlay))
       {
         OMX_U32 ret = MMI_S_EFAIL;
         MMI_OmxParamCmdType cmd;
         QOMX_QOE_DATA_PLAY dataQOEPlay;
         dataQOEPlay.size = sizeof(QOMX_QOE_DATA_PLAY);
         cmd.nParamIndex = m_pMMHTTPDataCache.mQOEDataPlay;
         cmd.pParamStruct = &dataQOEPlay;
         /* Querying to get data related to Play*/
         ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);

         /*Preparing message to be sent to upper layer containing data*/
         sp<AMessage> dataPlay = new AMessage;
         dataPlay->setInt32("what", android::kWhatQOEPlay);
         dataPlay->setInt64("timeofday",dataQOEPlay.timeOfDay);

         if(m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
         {
           /*Notifying DASHHTTPLiveSource with message*/
           m_pMMHTTPDataCache.mHttpLiveSrcObj->QOENotifyCB(dataPlay);
         }
         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
               "CHTTPAALStateBase::EventHandler - MMI_EVT_QOMX_EXT_SPECIFIC dataQOEPlay->timeofDay = %llu ", dataQOEPlay.timeOfDay);
       }
       else if ( (pExtSpecificInfo != NULL) &&
             (pExtSpecificInfo->nData2 == (OMX_U32)m_pMMHTTPDataCache.mQOEDataSwitch))
       {
         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
               "CHTTPAALStateBase::EventHandler - MMI_EVT_QOMX_EXT_SPECIFIC  pExtSpecificInfo->nData2 = %x ", (uint32_t)pExtSpecificInfo->nData2);
         OMX_U32 ret = MMI_S_EFAIL;
         MMI_OmxParamCmdType cmd;
         QOMX_QOE_DATA_SWITCH dataQOESwitch;
         dataQOESwitch.size = sizeof(QOMX_QOE_DATA_SWITCH);
         cmd.nParamIndex = m_pMMHTTPDataCache.mQOEDataSwitch;
         cmd.pParamStruct = &dataQOESwitch;
         ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);

         sp<AMessage> dataSwitch = new AMessage;
         dataSwitch->setInt32("what", kWhatQOESwitch);
         dataSwitch->setInt32("bandwidth",dataQOESwitch.bandwidth);
         dataSwitch->setInt32("rebufct",dataQOESwitch.reBufCount);
         dataSwitch->setInt64("timeofday",dataQOESwitch.timeOfDay);

         if(m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
         {
           /*Notifying DASHHTTPLiveSource with message*/
           m_pMMHTTPDataCache.mHttpLiveSrcObj->QOENotifyCB(dataSwitch);
         }

         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
               "CHTTPAALStateBase::EventHandler - MMI_EVT_QOMX_EXT_SPECIFIC dataQOEPlay->timeofDay = %llu ", dataQOESwitch.timeOfDay);
       }
       else if ( (pExtSpecificInfo != NULL) &&
             (pExtSpecificInfo->nData2 == (OMX_U32)m_pMMHTTPDataCache.mQOEDataStop))
       {
         OMX_U32 ret = MMI_S_EFAIL;
         MMI_OmxParamCmdType cmd;
         QOMX_QOE_DATA_STOP* dataQOEStop = NULL;
         dataQOEStop = (QOMX_QOE_DATA_STOP*)MM_Malloc(sizeof(QOMX_QOE_DATA_STOP));
         if (!dataQOEStop)
         {
           QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "Out of memory\n");
           return;
         }
         memset(dataQOEStop, 0x0,sizeof(QOMX_QOE_DATA_STOP));
         dataQOEStop->size = sizeof(QOMX_QOE_DATA_STOP);
         cmd.nParamIndex = m_pMMHTTPDataCache.mQOEDataStop;
         cmd.pParamStruct = dataQOEStop;

         ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);

         if ((dataQOEStop->size > sizeof(QOMX_QOE_DATA_STOP)) && (ret == MMI_S_COMPLETE))
         {
           int size = dataQOEStop->size;
           MM_Free(dataQOEStop);
           ret = MMI_S_EFAIL;
           dataQOEStop = (QOMX_QOE_DATA_STOP*)MM_Malloc(size);
           if (!dataQOEStop)
           {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "Out of memory\n");
             return;
           }
           memset(dataQOEStop, 0x0, size);
           dataQOEStop->size = size;
           cmd.nParamIndex = m_pMMHTTPDataCache.mQOEDataStop;
           cmd.pParamStruct = dataQOEStop;
           ret = HTTPMMIDeviceCommand(m_pMMHTTPDataCache.GetMMIHandle(), MMI_CMD_GET_STD_OMX_PARAM, &cmd);
           if ((ret == MMI_S_COMPLETE) && (dataQOEStop->size > 0) &&
                   (dataQOEStop->size > sizeof(QOMX_QOE_DATA_STOP)))
           {
             sp<AMessage> dataStop = new AMessage;
             dataStop->setInt32("what", kWhatQOEStop);
             dataStop->setInt32("bandwidth",dataQOEStop->bandwidth);
             dataStop->setInt32("rebufct",dataQOEStop->reBufCount);
             dataStop->setInt64("timeofday",dataQOEStop->timeOfDay);
             if (dataQOEStop->nInfoStopLen > 0)
             {
               if (dataQOEStop->nStopPhraseLen > 0)
               {
                 dataStop->setInt32("sizestopphrase",dataQOEStop->nStopPhraseLen-1);
                 dataStop->setString("stopphrase",(char*)dataQOEStop->infoStop,dataQOEStop->nStopPhraseLen);
                     QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                         "CHTTPAALStateBase::EventHandler - MMI_EVT_QOMX_EXT_SPECIFIC  QOE STOP stopPhrase = %s",
                          dataQOEStop->infoStop);
               }
               if (dataQOEStop->nVideoURLLen > 0)
               {
                 dataStop->setInt32("sizevideo",dataQOEStop->nVideoURLLen - 1);
                 dataStop->setString("videourl",(char*)dataQOEStop->infoStop + dataQOEStop->nStopPhraseLen,
                                      dataQOEStop->nVideoURLLen);
                     QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                         "CHTTPAALStateBase::EventHandler - MMI_EVT_QOMX_EXT_SPECIFIC  QOE STOP videoUrl = %s",
                          dataQOEStop->infoStop+dataQOEStop->nStopPhraseLen);
               }
             }
             if(m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
             {
               m_pMMHTTPDataCache.mHttpLiveSrcObj->QOENotifyCB(dataStop);
             }
          }
        }
         MM_Free(dataQOEStop);
       }
    }
    break;
    case MMI_EVT_RESOURCES_LOST:
    {
       status_t ret = UNKNOWN_ERROR;//nEvtStatus;

       QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "MMI_EVT_RESOURCES_LOST Event Code : %lu, status %lu", nEvtCode,nEvtStatus);

       if (m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
       {
          if ( m_pMMHTTPDataCache.mVideoSource != NULL)
          {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                          "Processing Video Error due to MMI_EVT_RESOURCES_LOST");
             m_pMMHTTPDataCache.mHttpLiveSrcObj->VideoNotifyCB(NULL,ret);
          }
          else if ( m_pMMHTTPDataCache.mAudioSource != NULL)
          {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                          "Processing Audio Error due to MMI_EVT_RESOURCES_LOST");
             m_pMMHTTPDataCache.mHttpLiveSrcObj->AudioNotifyCB(NULL,ret);
          }
          else if ( m_pMMHTTPDataCache.mTextSource != NULL)
          {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                          "Processing Text Error due to MMI_EVT_RESOURCES_LOST");
             m_pMMHTTPDataCache.mHttpLiveSrcObj->TextNotifyCB(NULL,ret);
          }
          else
          {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                          "MMI_EVT_RESOURCES_LOST Port Invalid");
             m_pMMHTTPDataCache.mHttpLiveSrcObj->setFinalResult((status_t)UNKNOWN_ERROR);
          }
       }
       else
       {
         QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                      "Error: MMI_EVT_RESOURCES_LOST");
       }
       break;
    }

    case MMI_RESP_FILL_THIS_BUFFER:
    {
      MMI_BufferCmdType *pFillBuffCmdResp = NULL;
        pFillBuffCmdResp = (MMI_BufferCmdType *) pEvtData;
        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                     "CHTTPAALStatePlaying::EventHandler EventCode : 0x%x port index 0x%x",(int)nEvtCode,(int)pFillBuffCmdResp->nPortIndex);

        int err = m_pMMHTTPDataCache.DrmCheckAndDecrypt(pFillBuffCmdResp);
        m_pMMHTTPDataCache.setAALDRMError(err);

        if (pFillBuffCmdResp->nPortIndex == MMI_HTTP_VIDEO_PORT_INDEX)
        {
          m_pMMHTTPDataCache.m_recentVidStatus = nEvtStatus;
          sp<DASHMMIInterface::CSrcQueueSt> buffer;

          MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALVideoReadLock);
          m_pMMHTTPDataCache.mVidBuffer.dequeue(&buffer);
          MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALVideoReadLock);
          if (buffer != NULL && buffer->mBuffer != NULL)
          {
            m_pMMHTTPDataCache.ProcessFrameNotify(buffer->mBuffer, pFillBuffCmdResp, nEvtStatus);
            buffer->mBuffer = NULL;
            buffer = NULL;
          }
        }
        else if (pFillBuffCmdResp->nPortIndex == MMI_HTTP_AUDIO_PORT_INDEX)
        {
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                        "CHTTPAALStatePlaying::EventHandler EventCode : 0x%x, AudioPort",(unsigned)nEvtCode);

          m_pMMHTTPDataCache.m_recentAudStatus = nEvtStatus;
          sp<DASHMMIInterface::CSrcQueueSt> buffer;
          MM_CriticalSection_Enter( m_pMMHTTPDataCache.m_hHTTPAALAudioReadLock);
          m_pMMHTTPDataCache.mAudBuffer.dequeue(&buffer);
          MM_CriticalSection_Leave( m_pMMHTTPDataCache.m_hHTTPAALAudioReadLock);
          if (buffer != NULL && buffer->mBuffer != NULL)
          {
             m_pMMHTTPDataCache.ProcessFrameNotify(buffer->mBuffer, pFillBuffCmdResp, nEvtStatus);
             buffer->mBuffer = NULL;
             buffer = NULL;
          }
        }
        else if (pFillBuffCmdResp->nPortIndex == MMI_HTTP_OTHER_PORT_INDEX)
        {
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                        "CHTTPAALStatePlaying::EventHandler EventCode : 0x%x, TextPort",(unsigned)nEvtCode);

          m_pMMHTTPDataCache.m_recentTextStatus = nEvtStatus;
          sp<DASHMMIInterface::CSrcQueueSt> buffer;
          MM_CriticalSection_Enter( m_pMMHTTPDataCache.m_hHTTPAALTextReadLock);
          m_pMMHTTPDataCache.mTextBuffer.dequeue(&buffer);
          MM_CriticalSection_Leave( m_pMMHTTPDataCache.m_hHTTPAALTextReadLock);
          if (buffer != NULL && buffer->mBuffer != NULL)
          {
              m_pMMHTTPDataCache.ProcessFrameNotify(buffer->mBuffer, pFillBuffCmdResp, nEvtStatus);
              buffer->mBuffer = NULL;
              buffer = NULL;
          }
        }

      break;
    }

    case MMI_EVT_PORT_CONFIG_CHANGED: //Codec change would be handled as PORT CONFIG CHANGE
    {
      if (MMI_S_COMPLETE == nEvtStatus)
        {
           MMI_PortMsgType *pPort = (MMI_PortMsgType *) pEvtData;

           if (pPort != NULL)
           {
             (void)m_pMMHTTPDataCache.checkAndSetPortValidityForBufferingEvents(pPort->nPortIndex);
             QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                       "CHTTPAALStatePlaying config changed in playing state%d", pPort->nPortIndex);
           }


          MMI_PortCmdType flushCmd;
          OMX_U32 nret = MMI_S_EFAIL;
          int timeout = 0;
  //Disable MMI_EVT_PORT_CONFIG_CHANGED event handling in Playing state for video
  #if 0
          if (pPort && pPort->nPortIndex == MMI_HTTP_VIDEO_PORT_INDEX)
          {
             // Set the Video Discontineuty flag so that read with in MediaSource can return INFO DISCONTINEUTY
            m_pMMHTTPDataCache.ResetDiscontinuity(false,true);
            flushCmd.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
            nret = m_pMMHTTPDataCache.postCmd(m_pMMHTTPDataCache.m_handle, MMI_CMD_FLUSH, &flushCmd);
            int signal = m_pMMHTTPDataCache.Wait(DASHMMIInterface::AAL_WAKEUP_SIG,
                                                        MMI_WAIT_INTERVAL, &timeout);
            if (timeout)
            {
                QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                  "Flush Video port timed out.");
            }
            else
            {
                QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                  "Flush Video port Done");
            }
          }
          else
  #endif
          if(pPort && pPort->nPortIndex == MMI_HTTP_AUDIO_PORT_INDEX)
          {
            /* Port Config changed event is also sent when Audio track gets  removed for
               AV->A->AV usecases,  triger dicontinuity only when there change in audio format */
            OMX_AUDIO_PARAM_PORTFORMATTYPE paramAudioStruct;
            MMI_CustomParamCmdType data;
            OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();
            OMX_U32 ret = MMI_S_EFAIL;

            QOMX_STRUCT_INIT(paramAudioStruct, OMX_AUDIO_PARAM_PORTFORMATTYPE);
            paramAudioStruct.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
            data.nParamIndex = (MMI_IndexType) OMX_IndexParamAudioPortFormat;
            data.pParamStruct = &paramAudioStruct;

            ret = m_pMMHTTPDataCache.postCmd(handle,MMI_CMD_GET_STD_OMX_PARAM,&data);
            if (ret == MMI_S_COMPLETE && OMX_AUDIO_CodingUnused != paramAudioStruct.eEncoding)
            {
              // Set the Audio Discontineuty flag so that read with in MediaSource can return INFO DISCONTINEUTY
              m_pMMHTTPDataCache.ResetDiscontinuity(true,true);
               QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "Audio Discontinuity falg set.");
            }
            else
            {
              QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "Ignoring Audio port config changed event,get AudioPortFormat ret %ld, AudioEncoding %d",
                    (uint32)ret,paramAudioStruct.eEncoding);
            }
          }
        }

      break;
    }

  default:
    break;
  }
}

/*
 * ExitHandler, called when state exits
 *
 * @param[in] none
 *
 * @return true when successful, false otherwise
 */
bool CHTTPAALStateBase::ExitHandler()
{
  QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
               "CHTTPAALStateBase::ExitHandler");
  return true;
}


/**
 * C'tor of CHTTPAALStateIdle
 * @param[in]
 *
 * @return
 */
CHTTPAALStateIdle::CHTTPAALStateIdle(DASHMMIInterface &pDataCache):
                    CHTTPAALStateBase(pDataCache, HTTPAALStateIdle)
{
}

/**
 * D'tor of CHTTPAALStateIdle
 * @param[in]
 *
 * @return
 */
CHTTPAALStateIdle::~CHTTPAALStateIdle()
{
}

/*
 * process commands given by framework in state IDLE
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStateIdle::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                     int32 arg1, int32 arg2, int64 arg3)
{
  OMX_U32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();
  if (DASHMMIInterface::AAL_STOP == cmd)
  {
    //just be sure close the handle
    if (handle)
    {
      HTTPMMIDeviceClose(handle);
      m_pMMHTTPDataCache.SetMMIHandle(NULL);
    }

    ret = MMI_S_COMPLETE;
  } else if (DASHMMIInterface::AAL_PREPARE == cmd) {
    ret = HTTPMMIDeviceCommand(handle, MMI_CMD_LOAD_RESOURCES, NULL);
  }
  return ret;
}

/*
 * handles asynchronous events from MMI in AAL state IDLE
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStateIdle::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  switch(nEvtCode)
  {
  case MMI_RESP_LOAD_RESOURCES:
      if (MMI_S_COMPLETE != nEvtStatus)
      {
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "CHTTPAALStateIdle Load resources Evnt error  = %d ", nEvtStatus);
      }

      m_pMMHTTPDataCache.sendPrepareDoneCallBack(m_pMMHTTPDataCache.MapMMIToAALStatus(nEvtStatus));

      break;

  case MMI_RESP_FILL_THIS_BUFFER:
  case MMI_EVT_PORT_CONFIG_CHANGED:
      break;

    default:
      CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
      break;
  }
}

/**
 * C'tor of CHTTPAALStateConnecting
 * @param[in]
 *
 * @return
 */
CHTTPAALStateConnecting::CHTTPAALStateConnecting(DASHMMIInterface &pDataCache):
                          CHTTPAALStateBase(pDataCache, HTTPAALStateConnecting),
                          m_nNumTracksAvailable(0),
                          m_nNumTracksReady(0),
                          m_nValidPortCount(0)
{
  mStoredSeekCmnd = NULL;
  mStoredPauseCmnd = NULL;
}

/**
 * D'tor of CHTTPAALStateConnecting
 * @param[in]
 *
 * @return
 */
CHTTPAALStateConnecting::~CHTTPAALStateConnecting()
{
  m_nValidPortCount = 0;
  if(mStoredSeekCmnd)
  {
      delete mStoredSeekCmnd;
      mStoredSeekCmnd = NULL;
  }

  if(mStoredPauseCmnd)
  {
      delete mStoredPauseCmnd;
      mStoredPauseCmnd = NULL;
  }
}

/*
 * process commands given by framework in state CONNECTING
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStateConnecting::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                           int32 arg1, int32 arg2, int64 arg3)
{
  OMX_U32 ret = MMI_S_EFAIL;
  if (DASHMMIInterface::AAL_SEEK == cmd)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                   "Seek Called in state CHTTPAALStateConnecting, Store and process later");
    if(mStoredSeekCmnd !=NULL)
    {
      delete mStoredSeekCmnd;
      mStoredSeekCmnd = NULL;
    }

    mStoredSeekCmnd = (struct MMICmd *)malloc(sizeof(struct MMICmd));
    if (mStoredSeekCmnd != NULL)
    {
      mStoredSeekCmnd->cmd = cmd;
      mStoredSeekCmnd->arg1 = arg1;
      mStoredSeekCmnd->arg2 = arg2;
      mStoredSeekCmnd->arg3 = arg3;
      ret = MMI_S_PENDING;
    }
  }
  else if (DASHMMIInterface::AAL_PAUSE == cmd)
  {
    QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                   "Pause Called in state CHTTPAALStateConnecting, Store and process later");
    if(mStoredPauseCmnd != NULL)
    {
      delete mStoredPauseCmnd;
      mStoredPauseCmnd = NULL;
    }

    mStoredPauseCmnd = (struct MMICmd *)malloc(sizeof(struct MMICmd));
    if (mStoredPauseCmnd != NULL)
    {
      mStoredPauseCmnd->cmd = cmd;
      mStoredPauseCmnd->arg1 = arg1;
      mStoredPauseCmnd->arg2 = arg2;
      mStoredPauseCmnd->arg3 = arg3;
      ret = MMI_S_PENDING;
    }
  }

  return ret;
}

/*
 * handles asynchronous events from MMI in AAL state CONNECTING
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStateConnecting::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  switch(nEvtCode)
  {
  case MMI_RESP_START:
    if (MMI_S_COMPLETE == nEvtStatus)
    {
      m_pMMHTTPDataCache.QueryStreamType(OMX_ALL);

      m_nNumTracksAvailable = m_pMMHTTPDataCache.GetNumTracks();
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                   "CHTTPAALStateConnecting start response m_nNumTracksAvailable (%lu)",m_nNumTracksAvailable);
    }
    else
    {
      if (m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
      {
        m_pMMHTTPDataCache.mHttpLiveSrcObj->setFinalResult((status_t)UNKNOWN_ERROR);
      }
    }

    break;
    case MMI_EVT_PORT_CONFIG_CHANGED: //after getting port config changed for all the tracks AAL moves to playing state
      if (MMI_S_COMPLETE == nEvtStatus)
      {
        MMI_PortMsgType *pPort = (MMI_PortMsgType *) pEvtData;
        status_t ret = OK;
        bool bValid = false;

        MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
        if (pPort)
        {
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                       "CHTTPAALStateConnecting config changed on port %lu", pPort->nPortIndex);
          bValid = m_pMMHTTPDataCache.checkAndSetPortValidityForBufferingEvents(pPort->nPortIndex);

          if(bValid)
          {
              m_pMMHTTPDataCache.RegisterDecryptUnit(m_nValidPortCount,pPort->nPortIndex);
              m_nValidPortCount++;
          }
        }
        m_nNumTracksReady++;
        if(pPort && IS_TEXT_PORT(pPort->nPortIndex))
        {
            char* str = (char *)OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_DIMENSIONS;
            m_pMMHTTPDataCache.paramStructSMTPTimedTextDimensions.cParamName = str;
            (void)m_pMMHTTPDataCache.GetOmxIndexByExtensionString(m_pMMHTTPDataCache.paramStructSMTPTimedTextDimensions);

            str = NULL;
            str = (char *)OMX_QUALCOMM_INDEX_PARAM_SMPTE_TIME_TEXT_SUBINFO;
            m_pMMHTTPDataCache.paramStructSMTPTimedTextSubInfo.cParamName = str;
            (void)m_pMMHTTPDataCache.GetOmxIndexByExtensionString(m_pMMHTTPDataCache.paramStructSMTPTimedTextSubInfo);
        }
        if (m_nNumTracksReady >= DASH_MAX_NUM_TRACK)
        {
          int err = m_pMMHTTPDataCache.SetupDRMEnv();
          if (err == -1 && m_pMMHTTPDataCache.getDrmType()== QOMX_MARLIN_DRM)//error happened
          {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH, "Error happened..");
            if (m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
            {
              m_pMMHTTPDataCache.mHttpLiveSrcObj->setFinalResult((status_t)UNKNOWN_ERROR);
            }
            MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
            break;
          }

          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                      "CHTTPAALStateConnecting moving to HTTPAALStatePlaying");
          m_pMMHTTPDataCache.GetIndexForExtensions();
          //all ports ready, change state now..
          m_pMMHTTPDataCache.SetHTTPAALState(HTTPAALStatePlaying);
          if(mStoredSeekCmnd)
          {
              uint32 ret = OK;
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                               "Processing Stored Seek since state is HTTPAALStatePlaying");
              ret = m_pMMHTTPDataCache.ProcessCmd(mStoredSeekCmnd->cmd,mStoredSeekCmnd->arg1,
                                                mStoredSeekCmnd->arg2,mStoredSeekCmnd->arg3);
              if (OK != ret)
              {
                  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                                   "Seek - failed %lu but proceed with read", ret);
              }
              else
              {
                  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                                   "Seek success result = %lu ", ret);
              }
              delete mStoredSeekCmnd;
              mStoredSeekCmnd = NULL;
          }
        }
        MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      }
      else
      {
        QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                   "CHTTPAALStateConnecting Config changed Evnt error  = %d ", m_pMMHTTPDataCache.MapMMIToAALStatus(nEvtStatus));
        if (m_pMMHTTPDataCache.mHttpLiveSrcObj != NULL)
        {
          m_pMMHTTPDataCache.mHttpLiveSrcObj->setFinalResult((status_t)UNKNOWN_ERROR);
        }
      }
      break;

  case MMI_RESP_FILL_THIS_BUFFER:
    break;

  default:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
    break;
  }
}

/**
 * C'tor of CHTTPAALStatePlaying
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePlaying::CHTTPAALStatePlaying(DASHMMIInterface &pDataCache):
                       CHTTPAALStateBase(pDataCache, HTTPAALStatePlaying)
{
}

/**
 * D'tor of CHTTPAALStatePlaying
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePlaying::~CHTTPAALStatePlaying()
{
}

/*
 * process commands given by framework in state PLAYING
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStatePlaying::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                        int32 arg1, int32 arg2, int64 arg3)
{
  uint32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(cmd)
  {
    case DASHMMIInterface::AAL_PAUSE:
     /* Dont move to pause state, just issue pause cmd on dashSource */
      MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      ret = HTTPMMIDeviceCommand(handle, MMI_CMD_PAUSE, NULL);
      MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      break;

    case DASHMMIInterface::AAL_RESUME:
      MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      ret = HTTPMMIDeviceCommand(handle, MMI_CMD_RESUME, NULL);
      MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      break;

    case DASHMMIInterface::AAL_SEEK:
      //arg2 - seekMode
      //arg3 - seekTimeUs
      ret = m_pMMHTTPDataCache.ProcessSeek(arg2, arg3);
    break;

    case DASHMMIInterface::AAL_STOP:
      ret = CHTTPAALStateBase::ProcessCmd(cmd, arg1, arg2, arg3);
      break;

    default:
      break;
  }

  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                 "CHTTPAALStatePlaying::ProcessCmd - seek/pause processed - result = %lu", ret);

  return ret;
}

/*
 * Get specified attribute in state PLAYING
 *
 * @param[in] nKey Attribute key
 * @param[out] nVal Attribute value
 *
 * @return status of the get operation
 */
bool CHTTPAALStatePlaying::GetAttribute
(
 const HTTPAALAttributeKey nKey,
 HTTPAALAttributeValue& nVal
)
{
  bool bOk = true;
  switch (nKey)
  {
  case HTTP_AAL_ATTR_METADATA_AVAILABLE:
    nVal.bBoolVal = true;
    break;
  default:
    QTV_MSG_PRIO1( MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR, "CHTTPAALStatePlaying::GetAttribute Unsupported attribute %d", nKey );
    bOk = false;
    break;
  }
  return bOk;
}

/*
 * handles asynchronous events from MMI in AAL state PLAYING
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStatePlaying::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{

  MMI_BufferCmdType *pFillBuffCmdResp = NULL;
  pFillBuffCmdResp = (MMI_BufferCmdType *) pEvtData;

  switch(nEvtCode)
  {
  case MMI_RESP_PAUSE:
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                     "CHTTPAALStatePlaying pause response status (%lu)",nEvtStatus);
      if (MMI_S_COMPLETE != nEvtStatus)
      {
        // trigger  error
        CHTTPAALStateBase::EventHandler(MMI_EVT_RESOURCES_LOST, nEvtStatus, nPayloadLen, pEvtData);
      }
      break;
    }

    case MMI_RESP_RESUME:
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                     "CHTTPAALStatePlaying resume response status (%lu)",nEvtStatus);
      if (MMI_S_COMPLETE != nEvtStatus)
      {
        // trigger  error
        CHTTPAALStateBase::EventHandler(MMI_EVT_RESOURCES_LOST, nEvtStatus, nPayloadLen, pEvtData);
      }
      break;
    }


  default:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
    break;
  }
  return;
}

/**
 * C'tor of CHTTPAALStatePausing
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePausing::CHTTPAALStatePausing(DASHMMIInterface &pDataCache):
                       CHTTPAALStateBase(pDataCache, HTTPAALStatePausing)
{
}

/**
 * D'tor of CHTTPAALStatePausing
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePausing::~CHTTPAALStatePausing()
{
}

/*
 * process commands given by framework in state PAUSING
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStatePausing::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                        int32 arg1, int32 arg2, int64 arg3)
{
  uint32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(cmd)
  {
  case DASHMMIInterface::AAL_PAUSE:
    ret = MMI_S_PENDING;
    break;

  case DASHMMIInterface::AAL_RESUME:
  case DASHMMIInterface::AAL_SEEK:
    ret = MMI_S_EFAIL;
    break;

  case DASHMMIInterface::AAL_STOP:
    ret = CHTTPAALStateBase::ProcessCmd(cmd, arg1, arg2, arg3);
    break;

  default:
    break;
  }

  return ret;
}

/*
 * Get specified attribute in state PAUSING
 *
 * @param[in] nKey Attribute key
 * @param[out] nVal Attribute value
 *
 * @return status of the get operation
 */
bool CHTTPAALStatePausing::GetAttribute
(
 const HTTPAALAttributeKey nKey,
 HTTPAALAttributeValue& nVal
)
{
  bool bOk = true;
  switch (nKey)
  {
  case HTTP_AAL_ATTR_METADATA_AVAILABLE:
    nVal.bBoolVal = true;
    break;
  default:
    QTV_MSG_PRIO1( MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                   "CHTTPAALStatePausing::GetAttribute Unsupported attribute %d", nKey );
    bOk = false;
    break;
  }
  return bOk;
}

/*
 * handles asynchronous events from MMI in AAL state PAUSING
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStatePausing::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  switch(nEvtCode)
  {
  case MMI_RESP_PAUSE:
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "CHTTPAALStatePausing pause response status (%lu)",nEvtStatus);
      if (MMI_S_COMPLETE != nEvtStatus)
      {
        // trigger  error
        CHTTPAALStateBase::EventHandler(MMI_EVT_RESOURCES_LOST, nEvtStatus, nPayloadLen, pEvtData);
      }
      else
      {
    //set to paused
    MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    m_pMMHTTPDataCache.SetHTTPAALState(HTTPAALStatePaused);
    MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      }
    break;
    }
  default:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
    break;
  }
}

/**
 * C'tor of CHTTPAALStatePaused
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePaused::CHTTPAALStatePaused(DASHMMIInterface &pDataCache):
                      CHTTPAALStateBase(pDataCache, HTTPAALStatePaused)
{
}

/**
 * D'tor of CHTTPAALStatePaused
 * @param[in]
 *
 * @return
 */
CHTTPAALStatePaused::~CHTTPAALStatePaused()
{
}

/*
 * process commands given by framework in state PAUSED
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStatePaused::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                       int32 arg1, int32 arg2, int64 arg3)
{
  uint32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(cmd)
  {
  case DASHMMIInterface::AAL_PAUSE:
    //no-op
    ret = MMI_S_COMPLETE;
    break;

  case DASHMMIInterface::AAL_RESUME:
    MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    ret = HTTPMMIDeviceCommand(handle, MMI_CMD_RESUME, NULL);
    m_pMMHTTPDataCache.SetHTTPAALState(HTTPAALStateResuming);
    MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    break;

  case DASHMMIInterface::AAL_SEEK:
    //arg2 - seekMode
    //arg3 - seekTimeUs
    ret = m_pMMHTTPDataCache.ProcessSeek(arg2, arg3);
    break;

  case DASHMMIInterface::AAL_STOP:
    ret = CHTTPAALStateBase::ProcessCmd(cmd, arg1, arg2, arg3);
    break;

  default:
    break;
  }

  return ret;
}

/*
 * Get specified attribute in state PAUSED
 *
 * @param[in] nKey Attribute key
 * @param[out] nVal Attribute value
 *
 * @return status of the get operation
 */
bool CHTTPAALStatePaused::GetAttribute
(
 const HTTPAALAttributeKey nKey,
 HTTPAALAttributeValue& nVal
)
{
  bool bOk = true;
  switch (nKey)
  {
  case HTTP_AAL_ATTR_METADATA_AVAILABLE:
    nVal.bBoolVal = true;
    break;
  default:
    QTV_MSG_PRIO1( MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                   "CHTTPAALStatePaused::GetAttribute Unsupported attribute %d", nKey );
    bOk = false;
    break;
  }
  return bOk;
}

/*
 * handles asynchronous events from MMI in AAL state PAUSED
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStatePaused::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  switch(nEvtCode)
  {
  default:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
    break;
  }
}

/**
 * C'tor of CHTTPAALStateResuming
 * @param[in]
 *
 * @return
 */
CHTTPAALStateResuming::CHTTPAALStateResuming(DASHMMIInterface &pDataCache):
                        CHTTPAALStateBase(pDataCache, HTTPAALStateResuming)
{
}

/**
 * D'tor of CHTTPAALStateResuming
 * @param[in]
 *
 * @return
 */
CHTTPAALStateResuming::~CHTTPAALStateResuming()
{
}

/*
 * process commands given by framework in state RESUMING
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStateResuming::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                         int32 arg1, int32 arg2, int64 arg3)
{
  uint32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(cmd)
  {
  case DASHMMIInterface::AAL_PAUSE:
  case DASHMMIInterface::AAL_SEEK:
    {
      //we do not accept pause in this state
      ret = MMI_S_EFAIL;
    }
    break;

  case DASHMMIInterface::AAL_RESUME:
    {
      ret = MMI_S_PENDING;
    }
    break;

  case DASHMMIInterface::AAL_STOP:
    ret = CHTTPAALStateBase::ProcessCmd(cmd, arg1, arg2, arg3);
    break;

  default:
    break;
  }

  return ret;
}

/*
 * Get specified attribute in state RESUMING
 *
 * @param[in] nKey Attribute key
 * @param[out] nVal Attribute value
 *
 * @return status of the get operation
 */
bool CHTTPAALStateResuming::GetAttribute
(
 const HTTPAALAttributeKey nKey,
 HTTPAALAttributeValue& nVal
)
{
  bool bOk = true;
  switch (nKey)
  {
  case HTTP_AAL_ATTR_METADATA_AVAILABLE:
    nVal.bBoolVal = true;
    break;
  default:
    QTV_MSG_PRIO1( MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                   "CHTTPAALStateResuming::GetAttribute Unsupported attribute %d", nKey );
    bOk = false;
    break;
  }
  return bOk;
}

/*
 * handles asynchronous events from MMI in AAL state RESUMING
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStateResuming::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                         OMX_IN OMX_U32 nEvtStatus,
                                         OMX_IN OMX_U32 nPayloadLen,
                                         OMX_IN OMX_PTR pEvtData)
{
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(nEvtCode)
  {
  case MMI_RESP_RESUME:
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "CHTTPAALStateResuming resume response status (%lu)",nEvtStatus);
      if (MMI_S_COMPLETE != nEvtStatus)
      {
        // trigger  error
        CHTTPAALStateBase::EventHandler(MMI_EVT_RESOURCES_LOST, nEvtStatus, nPayloadLen, pEvtData);
      }
      else
      {
    //set to playing state
    MM_CriticalSection_Enter(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
    m_pMMHTTPDataCache.SetHTTPAALState(HTTPAALStatePlaying);
    MM_CriticalSection_Leave(m_pMMHTTPDataCache.m_hHTTPAALStateLock);
      }
    break;
    }
  case MMI_RESP_STOP:
    {
    //stop irresptive of what we are doing
    break;
    }

  default:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
      break;
    }
  }
/**
 * C'tor of CHTTPAALStateClosing
 * @param[in]
 *
 * @return
 */
CHTTPAALStateClosing::CHTTPAALStateClosing(DASHMMIInterface &pDataCache):
                                     CHTTPAALStateBase(pDataCache, HTTPAALStateClosing)
{
}


/**
 * D'tor of CHTTPAALStateClosing
 * @param[in]
 *
 * @return
 */
CHTTPAALStateClosing::~CHTTPAALStateClosing()
{
}

/*
 * EntryHandler, called when enters state
 *
 * @param[in] none
 *
 * @return true when successful, false otherwise
 */
bool CHTTPAALStateClosing::EntryHandler()
{
  OMX_U32 nReturn = MMI_S_EFAIL;
  bool bReturn = false;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  if(handle != NULL)
  {
      //call base class entry handler
      if (CHTTPAALStateBase::EntryHandler())
      {
        //issue stop to mmi
        nReturn = HTTPMMIDeviceCommand(handle, MMI_CMD_STOP, NULL);
      }
      if ((MMI_S_COMPLETE == nReturn) || (MMI_S_PENDING == nReturn))
      {
        bReturn = true;
      }
      return bReturn;
  }
  else
  {
    return true;
  }
}

/*
 * process commands given by framework in state CLOSING
 *
 * @param[in] cmd command to be processed
 * @param[in] arg1 valid only if requires args (now seek uses this)
 * @param[in] arg2 valid only if requires args (now seek uses this)
 *
 * @return status of the command processing
 */
uint32 CHTTPAALStateClosing::ProcessCmd(DASHMMIInterface::HTTPAALCmd cmd,
                                        int32 arg1, int32 arg2, int64 arg3)
{
  uint32 ret = MMI_S_EFAIL;
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(cmd)
  {
    case DASHMMIInterface::AAL_PAUSE:
    case DASHMMIInterface::AAL_RESUME:
    case DASHMMIInterface::AAL_SEEK:
      ret = MMI_S_EFAIL;
      break;
    case DASHMMIInterface::AAL_STOP:
      ret = MMI_S_PENDING;
      break;
    default:
      break;
  }

  return ret;
}

/*
 * handles asynchronous events from MMI in AAL state CLOSING
 *
 * @param[in] nEvtCode event code
 * @param[in] nEvtStatus event status
 * @param[in] nPayloadLen length of pEvtData if not null
 * @param[in] pEvtData event payload
 *
 * @return
 */
void CHTTPAALStateClosing::EventHandler(OMX_IN OMX_U32 nEvtCode,
                                     OMX_IN OMX_U32 nEvtStatus,
                                     OMX_IN OMX_U32 nPayloadLen,
                                     OMX_IN OMX_PTR pEvtData)
{
  OMX_HANDLETYPE handle = m_pMMHTTPDataCache.GetMMIHandle();

  switch(nEvtCode)
  {
  case MMI_RESP_STOP:
    m_pMMHTTPDataCache.DrmCleanup();
    // intentional fallthrough
  case MMI_EVT_RESOURCES_LOST:
  case MMI_EVT_FATAL_ERROR:
    //we got response, close the device and set state to IDLE
    //wake up who ever waiting for us
    m_pMMHTTPDataCache.Signal(DASHMMIInterface::AAL_ABORT_SIG);
    break;
  case MMI_EVT_QOMX_EXT_SPECIFIC:
    CHTTPAALStateBase::EventHandler(nEvtCode, nEvtStatus, nPayloadLen, pEvtData);
    break;
  default:
    if (!IS_SUCCESS(nEvtStatus))
    {
      m_pMMHTTPDataCache.Signal(DASHMMIInterface::AAL_ABORT_SIG);
    }
    break;
  }
}

/*
 * ExitHandler, called when state exits
 *
 * @param[in] none
 *
 * @return true when successful, false otherwise
 */
bool CHTTPAALStateClosing::ExitHandler()
{
  CHTTPAALStateBase::ExitHandler();
  return true;
}

/*
 * SignalHandler constructor
 *
 * @param[in]
 *
 * @return
 */
DASHMMIInterface::SignalHandler::SignalHandler()
{
  int32 status = 0;
  uint32 nReturn = MMI_S_EFAIL;

  //create signals
  status = MM_SignalQ_Create(&m_signalQ);
  if (0 != status)
  {
    nReturn = MMI_S_EFAIL;
  }

  //create signals according to their prioirty
  int sig = AAL_ABORT_SIG;
  status = MM_Signal_Create(m_signalQ, (void *)sig,
                            NULL, &m_stopSignal);
  if (0 != status)
  {
    nReturn = MMI_S_EFAIL;
  }

  sig = AAL_WAKEUP_SIG;
  status = MM_Signal_Create(m_signalQ, (void *)sig,
                            NULL, &m_wakeupSignal);
  if (0 != status)
  {
    nReturn = MMI_S_EFAIL;
  }

  sig = AAL_VIDEO_SIG;
  status = MM_Signal_Create(m_signalQ, (void *)sig,
                            NULL, &m_videoSignal);
  if (0 != status)
  {
    nReturn = MMI_S_EFAIL;
  }

  sig = AAL_AUDIO_SIG;
  status = MM_Signal_Create(m_signalQ, (void *)sig,
                            NULL, &m_audioSignal);
  if (0 != status)
  {
    nReturn = MMI_S_EFAIL;
  }

}

/*
 * SignalHandler destructor
 *
 * @param[in]
 *
 * @return
 */
DASHMMIInterface::SignalHandler::~SignalHandler()
{
  //release signals
  if (m_wakeupSignal)
  {
    MM_Signal_Release(m_wakeupSignal);
    m_wakeupSignal = NULL;
  }
  if (m_stopSignal)
  {
    MM_Signal_Release(m_stopSignal);
    m_stopSignal = NULL;
  }

  if (m_videoSignal)
  {
    MM_Signal_Release(m_videoSignal);
    m_videoSignal = NULL;
  }

  if (m_audioSignal)
  {
    MM_Signal_Release(m_audioSignal);
    m_audioSignal = NULL;
  }

  if ( m_signalQ )
  {
    MM_SignalQ_Release(m_signalQ);
    m_signalQ = NULL;
  }
  return;
}

/*
 * SignalHandler Wait, waits for the signals
 *
 * @param[in] time - time to wait
 * @param[out] timeout - whether timeout happened or not
 *
 * @return status of the operation
 */
uint32 DASHMMIInterface::SignalHandler::Wait(HTTPAALSignalSet signal,
                                             int time, int *timeout)
{
  int32 status = 0;
  MM_HANDLE signals[4];
  int index = 0;

  //MM_Signal_Reset(m_stopSignal);
  if (signal & AAL_WAKEUP_SIG)
  {
    signals[index++] = m_wakeupSignal;
  }

  if (signal & AAL_ABORT_SIG)
  {
    signals[index++] = m_stopSignal;
  }

  if (signal & AAL_VIDEO_SIG)
  {
    signals[index++] = m_videoSignal;
  }

  if (signal & AAL_AUDIO_SIG)
  {
    signals[index++] = m_audioSignal;
  }

  int sig = AAL_INVALID_SIG;

  if (time && timeout)
  {
    status = MM_SignalQ_TimedWaitEx(m_signalQ, time, (void **)&sig,
                                    timeout, signals, index);
  }
  else
  {
    status = MM_SignalQ_WaitEx(m_signalQ, (void **)&sig, signals, index);
  }

  if (status != 0)
  {
    sig = AAL_INVALID_SIG;
  }

  return sig;
}

/**
 * sets the signal
 *
 * @param[in] signal - signal to be set
 *
 * @return status of the operation
 */
int32 DASHMMIInterface::SignalHandler::Signal(HTTPAALSignalSet signal)
{
  int32 status = 0;
  switch(signal)
  {
  case AAL_WAKEUP_SIG:
    status = MM_Signal_Set(m_wakeupSignal);
    break;
  case AAL_ABORT_SIG:
    status = MM_Signal_Set(m_stopSignal);
    break;
  case AAL_VIDEO_SIG:
    status = MM_Signal_Set(m_videoSignal);
    break;
  case AAL_AUDIO_SIG:
    status = MM_Signal_Set(m_audioSignal);
    break;
  default:
    break;
  }
  return status;
}


void DASHMMIInterface::SessionSeek::Reset()
{
  m_seekTime = -1;
  for (int32 i=0; i<MAX_TRACKS; i++)
  {
    m_bTrackSeek[i] = true;
  }
  return;
}

OMX_U32 DASHMMIInterface::postCmd(OMX_HANDLETYPE handle, OMX_U32 nCode, void *pData)
{

    return HTTPMMIDeviceCommand(handle, nCode, pData);
}
void DASHMMIInterface::ResetDiscontinuity(bool audio, bool resetTo)
{
    if (audio)
    {
      bAudioDiscontinuity = resetTo;
    }
    else
    {
      bVideoDiscontinuity = resetTo;
    }
}

/** @brief   Count number of tracks in session.
 *
 *  @return numTracks
 */
uint32 DASHMMIInterface::GetNumTracks()
{
  MMI_OmxParamCmdType param;
  param.nParamIndex = OMX_IndexParamNumAvailableStreams;

  //get audio num tracks
  OMX_PARAM_U32TYPE audioU32;
  QOMX_STRUCT_INIT(audioU32, OMX_PARAM_U32TYPE);
  audioU32.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
  param.pParamStruct = &audioU32;
  (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &param);

  //get video num tracks
  OMX_PARAM_U32TYPE videoU32;
  QOMX_STRUCT_INIT(videoU32, OMX_PARAM_U32TYPE);
  videoU32.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
  param.pParamStruct = &videoU32;
  (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &param);

  //get text num tracks
  OMX_PARAM_U32TYPE textU32;
  QOMX_STRUCT_INIT(textU32, OMX_PARAM_U32TYPE);
  textU32.nPortIndex = MMI_HTTP_OTHER_PORT_INDEX;
  param.pParamStruct = &textU32;
  (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &param);

  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                "CMMHTTPDataCache::GetNumTracks %lu",
                (audioU32.nU32 + videoU32.nU32 + textU32.nU32));

  return (audioU32.nU32 + videoU32.nU32 + textU32.nU32);
}


status_t DASHMMIInterface::readFrameAsync(int iTrack, sp<ABuffer> fBuffer)
{
  status_t nReturn = OK;

  if ( (iTrack == 0) && (mAudioSource != NULL)) // read audio
  {
    if (mAudBuffer.getBufCount() > 5) {
       QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "Already requested enough audio frames");
       return nReturn;
    }
    sp<DASHMMIInterface::CSrcQueueSt> mSrcQueueBufData = new DASHMMIInterface::CSrcQueueSt();
    mSrcQueueBufData->mBuffer = fBuffer;
    MM_CriticalSection_Enter(m_hHTTPAALAudioReadLock);
    nReturn = mAudioSource->readFrameAsync(fBuffer,mSrcQueueBufData->bufHdr);
    if (nReturn == OK)
    {
       mAudBuffer.enqueue(mSrcQueueBufData);
    }
    else
    {
       mSrcQueueBufData = NULL;
    }
    MM_CriticalSection_Leave(m_hHTTPAALAudioReadLock);
  }
  else if((iTrack == 1) && (mVideoSource != NULL)) // read video
  {
    if (mVidBuffer.getBufCount() > 5) {
       QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "Already requested enough video frames");

       return nReturn;
    }
    sp<DASHMMIInterface::CSrcQueueSt> mSrcQueueBufData = new DASHMMIInterface::CSrcQueueSt();
    mSrcQueueBufData->mBuffer = fBuffer;
    MM_CriticalSection_Enter(m_hHTTPAALVideoReadLock);
    nReturn = mVideoSource->readFrameAsync(fBuffer,mSrcQueueBufData->bufHdr);
    if (nReturn == OK)
    {
       mVidBuffer.enqueue(mSrcQueueBufData);
    }
    else
    {
       mSrcQueueBufData = NULL;
    }
    MM_CriticalSection_Leave(m_hHTTPAALVideoReadLock);
  }
  else if((iTrack == 2) && (mTextSource != NULL)) // read text
  {
    if (mTextBuffer.getBufCount() > 5) {
       QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                    "Already requested enough text frames");

       return nReturn;
    }
    sp<DASHMMIInterface::CSrcQueueSt> mSrcQueueBufData = new DASHMMIInterface::CSrcQueueSt();
    mSrcQueueBufData->mBuffer = fBuffer;
    MM_CriticalSection_Enter(m_hHTTPAALTextReadLock);
    nReturn = mTextSource->readFrameAsync(fBuffer,mSrcQueueBufData->bufHdr);
    if (nReturn == OK)
    {
       mTextBuffer.enqueue(mSrcQueueBufData);
    }
    else
    {
       mSrcQueueBufData = NULL;
    }
    MM_CriticalSection_Leave(m_hHTTPAALTextReadLock);
  }

  return nReturn;
}


DASHMMIInterface::CSourceBuffer::CSourceBuffer()
{
  count = 0;
}
DASHMMIInterface::CSourceBuffer::~CSourceBuffer()
{
   count = 0;
}

void DASHMMIInterface::CSourceBuffer::enqueue(sp<DASHMMIInterface::CSrcQueueSt> buf)
{
 Mutex::Autolock Lock(mSrcBufLock);
  mSrcBuf.push_back(buf);
  count++;
}

status_t DASHMMIInterface::CSourceBuffer::dequeue(sp<DASHMMIInterface::CSrcQueueSt> *srcQbuf)
{

  Mutex::Autolock Lock(mSrcBufLock);
  if (!mSrcBuf.empty()) {
    *srcQbuf = *mSrcBuf.begin();
    mSrcBuf.erase(mSrcBuf.begin());
    count--;
    return OK;
  }
  *srcQbuf = NULL;
  return WOULD_BLOCK;
}

int32_t DASHMMIInterface::CSourceBuffer::getBufCount()
{
   return count;
}

status_t DASHMMIInterface::ProcessFrameNotify(sp<ABuffer> mABuffer, MMI_BufferCmdType *pMMICmdBuf,OMX_U32 nEvtStatus)
{
        status_t ret = OK;

        OMX_BUFFERHEADERTYPE *pBufHdr = pMMICmdBuf->pBufferHdr;
        ret = MapMMIToAALStatus(nEvtStatus);

        QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                     "DASHMMIInterface::ProcessFrameNotify -> nEventStatus %lu, ret %d",nEvtStatus, ret);

        if (pBufHdr != NULL && mABuffer != NULL)
        {
           if (IS_AUDIO_PORT(pMMICmdBuf->nPortIndex))
           {

               // if audio discontinuity is detected
               if (isAudioDiscontinuity() == true)
               {
                  /* Discontinuity is queued only when there is change in Channe Mode or Sampling rate.
                     ProcessFrameNotify() will be called only when DahSource returns FTB , the previously set
                     Discontinuity flag during Port config changes event is not read until this point,
                     so Decoder needs to be notified of discontinuity and also queue the valid access unit
                     that comes with this call */
                   QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                                "Audio Discontinuity Detected, return  INFO_DISCONTINUITY");
                   if ((mHttpLiveSrcObj != NULL))
                   {
                     mHttpLiveSrcObj->AudioNotifyCB(NULL,INFO_DISCONTINUITY);
                   }
                   ResetDiscontinuity(true,false);  // reset audio discontinuity flag
               }

               if (mAudioSource != NULL)
               {
                  const char *mime = NULL;
                  sp<MetaData> md = mAudioSource->getFormat();
                  if( md !=NULL )
                  {
                     md->findCString(kKeyMIMEType, &mime);
                     if (mime && !std_strnicmp(mime, DASH_MEDIA_MIMETYPE_AUDIO_AAC,
                                                      std_strlen(DASH_MEDIA_MIMETYPE_AUDIO_AAC)))
                     {
                        if ( (pBufHdr->pBuffer[0] == 0xFF ) && ((pBufHdr->pBuffer[1] & 0xF0 ) == 0xF0))
                        {
                            if (pBufHdr->nFilledLen > 7)
                            {
                                pBufHdr->nFilledLen -= 7;
                                std_memmove(pBufHdr->pBuffer, pBufHdr->pBuffer+7, pBufHdr->nFilledLen);
                            }
                        }
                     }
                  }
               }
           }
#if 0
           else if (IS_VIDEO_PORT(pMMICmdBuf->nPortIndex))
           {
               if (isVideoDiscontinuity() == true)
               {
                   QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                                "Video Discontinuity Detected, return  INFO_DISCONTINUITY");

                   ResetDiscontinuity(false,false); // reset video discontinuity flag
                   ret =  INFO_DISCONTINUITY;
               }
            }
#endif
            if (pBufHdr->nFlags & OMX_BUFFERFLAG_EOS)
            {
                  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                                "DASHMMIInterface::DASHMMIInterface - EOS on port %lu",
                                pMMICmdBuf->nPortIndex);
                  ret = ERROR_END_OF_STREAM;
                  if (IS_VIDEO_PORT(pMMICmdBuf->nPortIndex))
                  {
                     m_bVidEos = true;
                  }
                  else if (IS_AUDIO_PORT(pMMICmdBuf->nPortIndex))
                  {
                     m_bAudEos = true;
                  }
                  else if (IS_TEXT_PORT(pMMICmdBuf->nPortIndex))
                  {
                    m_bTextEos = true;
                  }
            }
            else if ((pBufHdr->nFlags & OMX_BUFFERFLAG_DATACORRUPT))
            {
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                            "recv. OMX_BUFFERFLAG_DATACORRUPT");
              ret = ERROR_IO;
            }
            if(!(pBufHdr->nFlags & OMX_BUFFERFLAG_EOS) &&
               !(pBufHdr->nFlags & OMX_BUFFERFLAG_DATACORRUPT) &&
                 pBufHdr->nFilledLen == 0 && (ret != INFO_DISCONTINUITY))
            {
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                            "Flushed Buffer Returned");
              return OK;
            }

            mABuffer->setRange(0, pBufHdr->nFilledLen);
            if (getAALDRMError() == DRM_ERROR_UNKNOWN)
            {
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                          "skipping Enc. frame from rendering");
              return OK;
            }
            else if (getAALDRMError() == DRM_ERROR_CANNOT_HANDLE)
            {
              //report error to upper layer and close the session
              QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
                          "DRM Error cannot be handle...Closing session");
              ret = (status_t)ERROR_DRM_CANNOT_HANDLE;
            }
            if (OK != ret)
            {
               QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                             "DASHMMIMediaSource::read  status %d", ret);
               if ((mHttpLiveSrcObj != NULL) && IS_VIDEO_PORT(pMMICmdBuf->nPortIndex))
               {
                  mHttpLiveSrcObj->VideoNotifyCB(mABuffer,ret);
               }
               else if ((mHttpLiveSrcObj != NULL) && IS_AUDIO_PORT(pMMICmdBuf->nPortIndex))
               {
                  mHttpLiveSrcObj->AudioNotifyCB(mABuffer,ret);
               }
               else if ((mHttpLiveSrcObj != NULL) && IS_TEXT_PORT(pMMICmdBuf->nPortIndex))
               {
                  mHttpLiveSrcObj->TextNotifyCB(mABuffer,ret);
               }
               return ret;
            }

            mABuffer->meta()->setInt64("timeUs",pBufHdr->nTimeStamp);
            mABuffer->meta()->setInt32("conf", ((pBufHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG)!=0));
            if (IS_VIDEO_PORT(pMMICmdBuf->nPortIndex))
            {
               mABuffer->meta()->setInt32("sync", ((pBufHdr->nFlags & OMX_BUFFERFLAG_SYNCFRAME)!=0) );
            }

            if((IS_TEXT_PORT(pMMICmdBuf->nPortIndex)) && (pBufHdr->nFlags & OMX_BUFFERFLAG_EXTRADATA))
            {
               QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                            "ProcessFrameNotify TextPort Extradata");

               OMX_OTHER_EXTRADATATYPE *pExtraDataSampleDimensions = NULL;
               OMX_OTHER_EXTRADATATYPE *pExtraData = NULL;

               uint32 ulAddr = (uint32)( pBufHdr->pBuffer) +  pBufHdr->nFilledLen;
               // Aligned address to DWORD boundary
               ulAddr = (ulAddr + 0x3) & (~0x3);
               pExtraData = (OMX_OTHER_EXTRADATATYPE *)ulAddr;

               while ( pExtraData && pExtraData->eType != OMX_ExtraDataNone)
               {
                  QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                               "ProcessFrameNotify pExtraData->eType(0x%x)",(OMX_EXTRADATATYPE)pExtraData->eType);

                  if((OMX_INDEXTYPE)pExtraData->eType == mSMTPTimedTextDimensionsIndex)
                  {
                     pExtraDataSampleDimensions = pExtraData;
                     QOMX_SUBTITILE_DIMENSIONS *sampleDimensions = (QOMX_SUBTITILE_DIMENSIONS *)pExtraDataSampleDimensions->data;
                     mABuffer->meta()->setInt32("height",sampleDimensions->nHeight);
                     mABuffer->meta()->setInt32("width",sampleDimensions->nWidth);
                     mABuffer->meta()->setInt32("duration",sampleDimensions->nDuration);
                     mABuffer->meta()->setInt32("startoffset",sampleDimensions->nStartOffset);
                     ulAddr = ulAddr + pExtraDataSampleDimensions->nSize;
                     ulAddr = (ulAddr + 0x3) & (~0x3);
                     pExtraData = (OMX_OTHER_EXTRADATATYPE*)ulAddr;
                  }

                  if ((OMX_INDEXTYPE)pExtraData->eType == mSMTPTimedTextSubInfoIndex)
                  {
                     OMX_OTHER_EXTRADATATYPE *pExtraDataSubs = pExtraData;
                     QOMX_SUBTITLE_SUB_INFOTYPE *subInfo = (QOMX_SUBTITLE_SUB_INFOTYPE *)pExtraDataSubs->data;
                     mABuffer->meta()->setInt32("subSc",subInfo->subSampleCount);
                     mABuffer->meta()->setInt32("subSt",subInfo->eSubCodingType);
                     mABuffer->meta()->setInt32("subSz",subInfo->nSubtitleSubInfoSize);
                     mABuffer->meta()->setString("subSi",(const char*)subInfo->cSubtitleSubInfo,subInfo->nSubtitleSubInfoSize);
                     ulAddr = ulAddr + pExtraDataSubs->nSize;
                     ulAddr = (ulAddr + 0x3) & (~0x3);
                     pExtraData = (OMX_OTHER_EXTRADATATYPE*)ulAddr;
                  }
               } //while
            }

            if ((mHttpLiveSrcObj != NULL) && IS_VIDEO_PORT(pMMICmdBuf->nPortIndex))
            {
               mHttpLiveSrcObj->VideoNotifyCB(mABuffer,ret);
            }
            else if ((mHttpLiveSrcObj != NULL) && IS_AUDIO_PORT(pMMICmdBuf->nPortIndex))
            {
               mHttpLiveSrcObj->AudioNotifyCB(mABuffer,ret);
            }
            else if ((mHttpLiveSrcObj != NULL) && IS_TEXT_PORT(pMMICmdBuf->nPortIndex))
            {
               mHttpLiveSrcObj->TextNotifyCB(mABuffer,ret);
            }
        }
        return ret;
}


void DASHMMIInterface::setHttpLiveSourceObj(DashPlayer::DASHHTTPLiveSource *Obj)
{
   mHttpLiveSrcObj = Obj;
   return;
}

void DASHMMIInterface::flush()
{
  if(m_handle)
  {
      MMI_PortCmdType flushCmd;
      flushCmd.nPortIndex = OMX_ALL;

      OMX_U32 nret = MMI_S_EFAIL;
      nret = postCmd(m_handle, MMI_CMD_FLUSH, &flushCmd);
      int timeout = 0;
      if (nret == MMI_S_PENDING)
      {
         int signal = Wait(AAL_WAKEUP_SIG,
                         MMI_WAIT_INTERVAL, &timeout);
         if (timeout)
         {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                           "Flush all ports timed out.");
         }
         else
         {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                          "Flush all ports Done");
         }
      }
  }
}

status_t DASHMMIInterface::seekTo(int64_t seekTimeUs)
{

   uint32 ret = OK;
   DASHMMIMediaInfo::SeekMode seekMode = DASHMMIMediaInfo::SEEK_CLOSEST_SYNC;

   QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
         "DASHMMIInterface::seekTo seektime %ld ms for all tracks ",
         (uint32)(seekTimeUs/1000));
   ret = ProcessCmd(DASHMMIInterface::AAL_SEEK, TRACK_ID_ALL,
                           seekMode, seekTimeUs);

    if (OK != ret)
    {
      if (WOULD_BLOCK == ret)
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                       "DASHMMIInterface::seekTo - Pending Seek - State still is connecting", ret);
      else
          QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                       "DASHMMIInterface::seekTo - failed %lu but proceed with read", ret);
    }
    else
    {
       QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
             "DASHMMIInterface::seekTo success result = %lu ", ret);
    }

   //Claim all buffers if seek is successfully posted to
   //DASH source for processing
   if ((ret == WOULD_BLOCK) || (ret == OK))
   {
     flush();
   }

    return ret;
}

status_t DASHMMIInterface::prepareAsync()
{
    OMX_U32 ret = MMI_S_EFAIL;
    if (m_handle ==  NULL)
    {
      QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
             "calling HTTPMMIDeviceOpen");
      ret = HTTPMMIDeviceOpen(&m_handle);

      if (IS_SUCCESS(ret))
      {
        ret = HTTPMMIRegisterEventHandler(m_handle, EventHandler, this);
      }

      if (IS_SUCCESS(ret))
      {
        //uri
        MMI_OmxParamCmdType uriData;
        OMX_PARAM_CONTENTURITYPE *paramStruct;
        uint32 uriLen = std_strlen(m_uri) + 1;
        paramStruct = (OMX_PARAM_CONTENTURITYPE *)
            MM_Malloc(sizeof(OMX_PARAM_CONTENTURITYPE) + uriLen);

        if (paramStruct == NULL)
        {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                       "Out of memory\n");
            return NO_MEMORY;
        }

        QOMX_STRUCT_INIT(*paramStruct, OMX_PARAM_CONTENTURITYPE);
        paramStruct->nSize += uriLen;
        std_strlcpy((char *)paramStruct->contentURI, m_uri, uriLen);
        uriData.nParamIndex = OMX_IndexParamContentURI;
        uriData.pParamStruct = paramStruct;

        ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &uriData);

        MM_Free(paramStruct);
        paramStruct = NULL;
      }

      if ((IS_SUCCESS(ret)))
      {
        return ProcessCmd(AAL_PREPARE, 0, 0, 0);
      }
    }
    else
    {
      QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_FATAL,
             "m_handle is valid, prepareAsync not possible again", ret);
    }
    return ret;
}

void DASHMMIInterface::sendPrepareDoneCallBack(status_t status)
{
    if (mHttpLiveSrcObj != NULL)
    {
      mHttpLiveSrcObj->PrepareDoneNotifyCB(status);
      mHttpLiveSrcObj->setFinalResult(status);
    }
}

status_t DASHMMIInterface::getParameter(int key, void **data, size_t *size)
{
    OMX_U32 err = MMI_S_EBADPARAM;
    OMX_INDEXTYPE extIndex;
    MMI_GetExtensionCmdType ext;

    if (mGetParamStruct) {
        free(mGetParamStruct);
        mGetParamStruct = NULL;
    }
    if(key == KEY_DASH_ADAPTION_PROPERTIES || key == KEY_DASH_GET_ADAPTION_PROPERTIES) {
        ALOGW("DASHMMIInterface::getParameter OMX_QUALCOMM_INDEX_PARAM_COMPLETE_DASH_ADAPTATION_PROPERTIES");
        ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_COMPLETE_DASH_ADAPTATION_PROPERTIES;
    }
    else if (key == KEY_DASH_MPD_QUERY) {
        ALOGW("DASHMMIInterface::getParameter OMX_QUALCOMM_INDEX_PARAM_DASH_MPD");
        ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_DASH_MPD;
    }
    else if (key == KEY_DASH_QOE_PERIODIC_EVENT)
    {
         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
               "DASHMMIInterface::getParameter - KEY_DASH_QOE_PERIODIC_EVENT = %d ", key);
         OMX_U32 ret = MMI_S_EFAIL;
         MMI_OmxParamCmdType cmd;
         QOMX_QOE_DATA_PERIODIC* dataQOEPeriodic = (QOMX_QOE_DATA_PERIODIC*)MM_Malloc(sizeof(QOMX_QOE_DATA_PERIODIC));
         if (!dataQOEPeriodic)
         {
           QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "Out of memory\n");
           return NO_MEMORY;
         }
         memset(dataQOEPeriodic,0x0,sizeof(QOMX_QOE_DATA_PERIODIC));
         dataQOEPeriodic->size = sizeof(QOMX_QOE_DATA_PERIODIC);
         cmd.nParamIndex = mQOEDataPeriodic;
         cmd.pParamStruct = dataQOEPeriodic;
         ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &cmd);

         if ((dataQOEPeriodic->size > sizeof(QOMX_QOE_DATA_PERIODIC)) && (ret == MMI_S_COMPLETE))
         {
           int size = dataQOEPeriodic->size;
           MM_Free(dataQOEPeriodic);
           ret = MMI_S_EFAIL;
           dataQOEPeriodic = (QOMX_QOE_DATA_PERIODIC*)MM_Malloc(size);
           if (!dataQOEPeriodic)
           {
             QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "Out of memory\n");
             return NO_MEMORY;
           }
           dataQOEPeriodic->size = size;
           cmd.nParamIndex = mQOEDataPeriodic;
           cmd.pParamStruct = dataQOEPeriodic;
           ret = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &cmd);
           if ((ret == MMI_S_COMPLETE))
           {
             dataPeriodic = new AMessage;

             dataPeriodic->setInt32("what", kWhatQOEPeriodic);
             dataPeriodic->setInt32("bandwidth",dataQOEPeriodic->bandwidth);
             dataPeriodic->setInt64("timeofday",dataQOEPeriodic->timeOfDay);
             if (dataQOEPeriodic->nInfoPeriodicLen > 0)
             {
               if (dataQOEPeriodic->nIpAddressLen > 0)
               {
                 dataPeriodic->setInt32("sizeipadd",dataQOEPeriodic->nIpAddressLen-1);
                 dataPeriodic->setString("ipaddress",(char*)dataQOEPeriodic->infoPeriodic,dataQOEPeriodic->nIpAddressLen);
                }
               if (dataQOEPeriodic->nVideoURLLen > 0)
               {
                 dataPeriodic->setInt32("sizevideo",dataQOEPeriodic->nVideoURLLen -1);
                 dataPeriodic->setString("videourl",(char*)(dataQOEPeriodic->infoPeriodic+dataQOEPeriodic->nIpAddressLen),
                                       dataQOEPeriodic->nVideoURLLen);
                }
                 QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_LOW,
                 "DASHMMIInterface::getParameter - KEY_DASH_QOE_PERIODIC_EVENT  ipaddress = %s , videoUrl = %s",
                         dataQOEPeriodic->infoPeriodic,dataQOEPeriodic->infoPeriodic+dataQOEPeriodic->nIpAddressLen);
               }
             }
           else
           {
             QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
               "DASHMMIInterface::getParameter - KEY_DASH_QOE_PERIODIC_EVENT failed(%d)",ret);
           }
           (*data) = (void *)(dataPeriodic.get());
           MM_Free(dataQOEPeriodic);
         }
         return OK;
    }

    ext.pIndex = &extIndex;
    err = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if (err == MMI_S_COMPLETE)
    {
      QOMX_DASHPROPERTYINFO paramSize;
      QOMX_DASHPROPERTYINFO *paramData;
      MMI_OmxParamCmdType param;
      param.nParamIndex = extIndex;

      paramSize.nSize = (sizeof(QOMX_DASHPROPERTYINFO));
      paramSize.nPropertiesSize = 0;
      paramSize.nPortIndex = OMX_ALL;
      param.pParamStruct = (OMX_PTR)(&paramSize);
      (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &param);

      (*size) = paramSize.nPropertiesSize;
      mGetParamStruct = malloc(sizeof(QOMX_DASHPROPERTYINFO) + (*size) + 1);

      if (mGetParamStruct == NULL)
      {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                       "Out of memory\n");
          return NO_MEMORY;
      }

      paramData = (QOMX_DASHPROPERTYINFO *)mGetParamStruct;
      if (paramData)
      {
        paramData->nSize = (sizeof(QOMX_DASHPROPERTYINFO) + (*size) + 1);
        paramData->nPropertiesSize = (*size);
        paramData->nPortIndex = OMX_ALL;
        param.pParamStruct = (OMX_PTR)(paramData);
        (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_STD_OMX_PARAM, &param);

        (*data) = (void *)(paramData->cDashProperties);
        return OK;
      }
    }
    return UNKNOWN_ERROR;
}

status_t DASHMMIInterface::setParameter(int key, void *data, size_t size)
{
    OMX_U32 err = MMI_S_EBADPARAM;
    OMX_INDEXTYPE extIndex;
    MMI_GetExtensionCmdType ext;
    if (key == KEY_DASH_ADAPTION_PROPERTIES || key == KEY_DASH_SET_ADAPTION_PROPERTIES)
    {
    ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_SELECTED_DASH_ADAPTATION_PROPERTIES;
    ext.pIndex = &extIndex;
    err = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
    if (err == MMI_S_COMPLETE)
    {
      MMI_OmxParamCmdType param;
      param.nParamIndex = extIndex;

      QOMX_DASHPROPERTYINFO * paramData;
      void * paramStruct = malloc(sizeof(QOMX_DASHPROPERTYINFO) + (size) + 1);

      if (paramStruct == NULL)
      {
          QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                       "Out of memory\n");
          return NO_MEMORY;
      }

      paramData = (QOMX_DASHPROPERTYINFO *)paramStruct;
      if (paramData)
      {
        paramData->nPropertiesSize = size;
        paramData->nSize = (sizeof(QOMX_DASHPROPERTYINFO) + (size) + 1);
        paramData->nPortIndex = OMX_ALL;
        memcpy((void *)paramData->cDashProperties, data, size + 1);

        param.pParamStruct = (OMX_PTR)paramData;
        (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &param);
        free(paramStruct);
        return OK;
      }
    }
    }
    else if (key == KEY_DASH_QOE_EVENT)
    {
      ext.cParamName = (OMX_STRING)OMX_QUALCOMM_INDEX_PARAM_QOE;
      ext.pIndex = &extIndex;
      err = HTTPMMIDeviceCommand(m_handle, MMI_CMD_GET_EXTENSION_INDEX, &ext);
      if (err == MMI_S_COMPLETE)
      {
        MMI_OmxParamCmdType param;
        param.nParamIndex = extIndex;

        QOMX_QOE_EVENT_REG * paramData;
        void * paramStruct = malloc(sizeof(QOMX_QOE_EVENT_REG) + (size) + 1);

        if (paramStruct == NULL)
        {
            QTV_MSG_PRIO(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_ERROR,
                         "Out of memory\n");
            return NO_MEMORY;
        }

        paramData = (QOMX_QOE_EVENT_REG *)paramStruct;
        memcpy((void *)paramData, data, size);

        param.pParamStruct = (OMX_PTR)paramData;
        (void)HTTPMMIDeviceCommand(m_handle, MMI_CMD_SET_STD_OMX_PARAM, &param);

        bQOENotify =  paramData->bNotify;

        GetIndexForQOEExtensions(bQOENotify);
        free(paramStruct);
        return OK;
      }
    }
    return UNKNOWN_ERROR;
}

DASHMMIInterface::CPortsValidity::CPortsValidity(uint32_t nPort)
{
  setPort(nPort);
  setStatus(0);

  // in the begining, both the ports should be valid as we register for both the ports and
  // DASH Source notifies us buffering for both the tracks even if anye one track is missing
  updatePortValidity(IS_VALID_PORT(nPort));
}

void DASHMMIInterface::resetObjectsOnAllPort()
{
  sp<DASHMMIInterface::CPortsValidity> mObject;

  if (!mAvailableValidPorts.empty())
  {
    List<sp<DASHMMIInterface::CPortsValidity> >::iterator iterator;

    for (iterator = mAvailableValidPorts.begin(); iterator != mAvailableValidPorts.end(); iterator++)
    {
      mObject = *iterator;
      if (mObject != NULL)
      {
        mObject->setStatus(0);
        mObject->updatePortValidity(true);
      }
    }
  }
}

sp<DASHMMIInterface::CPortsValidity> DASHMMIInterface::getObjectByPort(uint32_t mPort)
{

   sp<DASHMMIInterface::CPortsValidity> mObject;

   if (!mAvailableValidPorts.empty() && IS_VALID_PORT(mPort))
   {
     List<sp<DASHMMIInterface::CPortsValidity> >::iterator iterator;

     QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
               "mAvailableValidPorts.size() %d",mAvailableValidPorts.size());

     for (iterator = mAvailableValidPorts.begin(); iterator != mAvailableValidPorts.end(); iterator++)
     {
       mObject = *iterator;
       if (mObject != NULL && mObject->getPort() == mPort)
       {
         QTV_MSG_PRIO1(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                   "mObject->getPort() %d",mObject->getPort());
         return mObject;
       }
     }
   }

   return NULL;
}

void DASHMMIInterface::updatePortObjectValidity(uint32_t mPort, bool bValidity)
{
    if (IS_VALID_PORT(mPort)   &&
        (IS_AUDIO_PORT(mPort) || IS_VIDEO_PORT(mPort)))
    {
       sp<DASHMMIInterface::CPortsValidity> mAudObj = getObjectByPort(MMI_HTTP_AUDIO_PORT_INDEX);
       sp<DASHMMIInterface::CPortsValidity> mVidObj = getObjectByPort(MMI_HTTP_VIDEO_PORT_INDEX);

       if ( (mAudObj != NULL) && (mPort == mAudObj->getPort()))
       {
         mAudObj->updatePortValidity(bValidity);
       }
       else if ((mVidObj != NULL) && (mPort == mVidObj->getPort()))
       {
         mVidObj->updatePortValidity(bValidity);
       }
    }
}

bool DASHMMIInterface::canBufferingBeSent(uint32_t mPort, status_t iBuffStatus)
{
  bool bReturn = false;

  if (IS_VALID_PORT(mPort)   &&
      (IS_AUDIO_PORT(mPort) || IS_VIDEO_PORT(mPort)))
  {
     sp<DASHMMIInterface::CPortsValidity> mAudObj = getObjectByPort(MMI_HTTP_AUDIO_PORT_INDEX);
     sp<DASHMMIInterface::CPortsValidity> mVidObj = getObjectByPort(MMI_HTTP_VIDEO_PORT_INDEX);

     if ( (mAudObj != NULL) && (mPort == mAudObj->getPort()))
     {
       mAudObj->setStatus(iBuffStatus);
     }
     else if ((mVidObj != NULL) && (mPort == mVidObj->getPort()))
     {
       mVidObj->setStatus(iBuffStatus);
     }
     if ( (mAudObj != NULL) && (mVidObj != NULL))
     {
     QTV_MSG_PRIO4(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                      "DASHMMIInterface::canBufferingBeSent mAudObj->isValidPort() (%d), mVidObj->isValidPort() (%d), mAudObj->isBuffering() (%d),mVidObj->isBuffering() (%d)",
              mAudObj->isValidPort(),
              mVidObj->isValidPort(),
              mAudObj->getStatus(),
              mVidObj->getStatus());

        if ( (mAudObj->isValidPort() && mAudObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START) &&
             (mVidObj->isValidPort() && mVidObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START))
        {
           bReturn = true;
        }
        else if ((mPort == mAudObj->getPort()) && !mAudObj->isValidPort())
        {
           bReturn = false;
        }
        else if((mPort == mVidObj->getPort()) && !mVidObj->isValidPort())
        {
            bReturn = false;
        }
        else if (!mAudObj->isValidPort() && ((mVidObj->isValidPort()) &&
                 (mVidObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START)))
        {
           bReturn = true;
        }
        else if (!mVidObj->isValidPort() && ((mAudObj->isValidPort()) &&
                 (mAudObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_START)))
        {
           bReturn = true;
        }
     }
  }

  return bReturn;
}

bool DASHMMIInterface::checkAndSetPortValidityForBufferingEvents(uint32_t nPortIndex)
{
      MMI_CustomParamCmdType data;
      OMX_HANDLETYPE handle = GetMMIHandle();
      OMX_U32 nRet = MMI_S_EFAIL;
      bool bReturn = false;

      if (IS_VALID_PORT(nPortIndex) && IS_AUDIO_PORT(nPortIndex))
      {
          OMX_AUDIO_PARAM_PORTFORMATTYPE paramAudioStruct;
          OMX_AUDIO_CODINGTYPE code= OMX_AUDIO_CodingUnused;
          //audio port def
          QOMX_STRUCT_INIT(paramAudioStruct, OMX_AUDIO_PARAM_PORTFORMATTYPE);
          paramAudioStruct.nPortIndex = MMI_HTTP_AUDIO_PORT_INDEX;
          data.nParamIndex = (MMI_IndexType) OMX_IndexParamAudioPortFormat;
          data.pParamStruct = &paramAudioStruct;

          nRet = postCmd(handle,MMI_CMD_GET_STD_OMX_PARAM,&data);
          if (nRet == MMI_S_COMPLETE)
          {
              QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_MEDIUM,
                            "DASHMMIInterface::checkAndSetPortValidityForBufferingEvents Port : %d, AudioEncoding Type= %d ",
                             nPortIndex,paramAudioStruct.eEncoding);

              if (code == paramAudioStruct.eEncoding)
              {
                  updatePortObjectValidity(MMI_HTTP_AUDIO_PORT_INDEX,false);
                  bReturn = false;
              }
              else
              {
                updatePortObjectValidity(MMI_HTTP_AUDIO_PORT_INDEX,true);
                bReturn = true;
              }
          }
      }
      else if(IS_VALID_PORT(nPortIndex) && IS_VIDEO_PORT(nPortIndex))
      {
          MMI_ParamDomainDefType paramStruct;
          paramStruct.nPortIndex = MMI_HTTP_VIDEO_PORT_INDEX;
          data.nParamIndex = MMI_IndexDomainDef;
          data.pParamStruct = &paramStruct;

          nRet = postCmd(handle,MMI_CMD_GET_CUSTOM_PARAM,&data);
          if (nRet == MMI_S_COMPLETE)
          {
              QTV_MSG_PRIO2(MSG_SSID_APPS_QTV_STREAMING, QTVDIAG_PRIO_HIGH,
                            "DASHMMIInterface::checkAndSetPortValidityForBufferingEvents Port : %d, VideoEncoding Type= %d ",
                             nPortIndex,paramStruct.format.video.eCompressionFormat);
            OMX_VIDEO_CODINGTYPE code= OMX_VIDEO_CodingUnused;
            if(code == paramStruct.format.video.eCompressionFormat)
            {
              updatePortObjectValidity(MMI_HTTP_VIDEO_PORT_INDEX,false);
              bReturn = false;
            }
            else
            {
              updatePortObjectValidity(MMI_HTTP_VIDEO_PORT_INDEX,true);
              bReturn = true;
            }
          }
      }
      else if(IS_VALID_PORT(nPortIndex) && IS_TEXT_PORT(nPortIndex))
      {
          //text port def
          OMX_OTHER_PARAM_PORTFORMATTYPE paramOtherStruct;
          QOMX_STRUCT_INIT(paramOtherStruct, OMX_OTHER_PARAM_PORTFORMATTYPE);
          paramOtherStruct.nPortIndex = MMI_HTTP_OTHER_PORT_INDEX;
          data.nParamIndex = (MMI_IndexType)OMX_IndexParamOtherPortFormat;
          data.pParamStruct = &paramOtherStruct;

          nRet = postCmd(handle,MMI_CMD_GET_STD_OMX_PARAM,&data);
          if (nRet == MMI_S_COMPLETE)
          {
             QOMX_OTHER_CODINGTYPE code= QOMX_OTHER_CodingUnused;
             if (code == (QOMX_OTHER_CODINGTYPE)paramOtherStruct.eFormat)
             {
                bReturn = false;
             }
             else
             {
                bReturn = true;
             }
         }
      }
      return bReturn;
}

bool DASHMMIInterface::canBufferingEndBeSent(uint32_t mPort, status_t iStatus)
{
   bool bReturn = false;

   if (IS_VALID_PORT(mPort)   &&
       (IS_AUDIO_PORT(mPort) || IS_VIDEO_PORT(mPort)))
   {
        sp<DASHMMIInterface::CPortsValidity> mAudObj = getObjectByPort(MMI_HTTP_AUDIO_PORT_INDEX);
        sp<DASHMMIInterface::CPortsValidity> mVidObj = getObjectByPort(MMI_HTTP_VIDEO_PORT_INDEX);
        if ( (mAudObj != NULL) && (mPort == mAudObj->getPort()))
        {
           mAudObj->setStatus(iStatus);
        }
        else if ((mVidObj != NULL) && (mPort == mVidObj->getPort()))
        {
           mVidObj->setStatus(iStatus);
        }
        if ( (mAudObj != NULL) && (mVidObj != NULL))
        {
            LOGE("DASHMMIInterface::canBufferingEndBeSent mAudObj->isValidPort() (%d), mVidObj->isValidPort() (%d), mAudObj->getStatus() (%d),mVidObj->getStatus() (%d)",
                  mAudObj->isValidPort(),
                  mVidObj->isValidPort(),
                  mAudObj->getStatus(),
                  mVidObj->getStatus());
            if ( (mAudObj->isValidPort() && mAudObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END) &&
                 (mVidObj->isValidPort() && mVidObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END))
            {
                bReturn = true;
            }
            else if ((mPort == mAudObj->getPort()) && !mAudObj->isValidPort())
            {
                bReturn = false;
            }
            else if((mPort == mVidObj->getPort()) && !mVidObj->isValidPort())
            {
                bReturn = false;
            }
            else if (!mAudObj->isValidPort() && ((mVidObj->isValidPort()) &&
                     (mVidObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END)))
            {
                bReturn = true;
            }
            else if (!mVidObj->isValidPort() && ((mAudObj->isValidPort()) &&
                     (mAudObj->getStatus() == DashPlayer::DASHHTTPLiveSource::DASH_ERROR_NOTIFY_BUFFERING_END)))
            {
                bReturn = true;
            }
        }
   }

   return bReturn;
}

/**
 * @brief call pause on the current handler
 *
 * @return status_t
 */
status_t DASHMMIInterface::pause()
{
  status_t status = UNKNOWN_ERROR;
  status = ProcessCmd(DASHMMIInterface::AAL_PAUSE);
  return status;
}


/**
 * @brief call resume on the current handler
 *
 * @return status_t
 */
status_t DASHMMIInterface::resume()
{
  status_t status = UNKNOWN_ERROR;
  status = ProcessCmd(DASHMMIInterface::AAL_RESUME);
  return status;
}

}  // namespace android
