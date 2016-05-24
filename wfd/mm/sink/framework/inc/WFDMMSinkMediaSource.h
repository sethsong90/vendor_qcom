#ifndef __WFD_MM_SINK_VIDEO_SOURCE_H__
#define __WFD_MM_SINK_VIDEO_SOURCE_H__
/*==============================================================================
*       WFDMMSinkMediaSource.h
*
*  DESCRIPTION:
*       Class declaration for WFDMM Sink MediaSource. Connects RTP decoder and
*       parser and provides Audio and Video samples to
*       the media framework.
*
*
*  Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
*  Qualcomm Technologies Proprietary and Confidential.
*===============================================================================
*/
/*==============================================================================
                             Edit History
================================================================================
   When            Who           Why
-----------------  ------------  -----------------------------------------------
03/28/2013         SK            InitialDraft
================================================================================
*/

/*==============================================================================
**               Includes and Public Data Declarations
**==============================================================================
*/

/* =============================================================================

                     INCLUDE FILES FOR MODULE

================================================================================
*/
//#include "AEEStd.h"
#include <linux/msm_ion.h>
#include "OMX_Core.h"
#include "MMCriticalSection.h"
#include "WFDMMSinkCommon.h"
#include "filesource.h"
#include "GraphicBuffer.h"
#include "wdsm_mm_interface.h"
#include "RTPStreamPort.h"

typedef bool (*decryptCbType) (int handle, int trackId, int input, int output,
                          int size, char* pIV, int IVSize);

typedef void (*avInfoCbType) (int handle, avInfoType *pInfo);

typedef struct mediaSourceConfig
{
    decryptCbType pFnDecrypt;
    RTPStreamPort *pRTPStreamPort;
    uint32 nLocalIP;
    uint32 nPeerIP;
    unsigned short rtpPort;
    unsigned short rtcpPortLocal;
    unsigned short rtcpPortRemote;
    int    nRtpSock;
    int    nRtcpSock;
    WFD_audio_type eAudioFmt;
    int32  nFrameDropMode;
    bool   bIsTCP;
    bool   bHasVideo;
    bool   bHasAudio;
    bool   bSecure;
}mediaSourceConfigType;

class  WFDMMThreads;
class  RTPStreamPort;
class  SignalQueue;

class WFDMMSinkMediaSource
{
public:
    WFDMMSinkMediaSource(int moduleId,
                         WFDMMSinkHandlerFnType pFnHandler,
                         WFDMMSinkFBDType       pFnFBD,
                         avInfoCbType           pFnAVInfo,
                         int clientData);

    ~WFDMMSinkMediaSource();

    OMX_ERRORTYPE Configure(mediaSourceConfigType *pCfg);

    OMX_ERRORTYPE Deinit();

    OMX_ERRORTYPE setFreeBuffer(
        int trackId,
        OMX_BUFFERHEADERTYPE *pBuffer
    );

    OMX_ERRORTYPE Start();

    OMX_ERRORTYPE Stop();

    void setFlushTimeStamp(uint64 nTS);

    void streamPlay(bool);

    void streamPause(void);

    void receiveIDRTimerNotification(WFDMMIDRTimerStatus pStatus);
private:
    int state(int state, bool get_true_set_false);

    OMX_ERRORTYPE createResources();

    bool createThreadsAndQueues();

    static void VideoThreadEntry(int pThis,
                          unsigned int nSignal);
    void VideoThread(unsigned int nSignal);

    static void AudioThreadEntry(int pThis,
                          unsigned int nSignal);
    void AudioThread(unsigned int nSignal);

    bool configureDataSource();

    bool configureHDCPResources();

    bool configureParser();

    static void fileSourceCallback
    (
        FileSourceCallBackStatus status,
        void *pThis
    );

    void fileSourceCallbackHandler
    (
        FileSourceCallBackStatus status
    );

    int fetchVideoSample
    (
        uint32
    );

    int fetchVideoSampleSecure
    (
        uint32
    );

    int fetchAudioSample
    (
        uint32
    );

    int fetchAudioSampleSecure
    (
        uint32
    );

    bool allocateAudioBuffers();
    bool deallocateAudioBuffers();

    bool AllocateIonBufs();
    OMX_ERRORTYPE deinitialize();
    bool deallocateIonBufs();

    void InitData();
    void SetMediaBaseTime(uint64 nStartTime);

    WFDMMSinkHandlerFnType mpFnHandler;
    WFDMMSinkFBDType       mpFnFBD;
    avInfoCbType           mpFnAVInfoCb;
    uint32 mVideoTrackId;
    uint32 mAudioTrackId;
    uint8 *mpVideoFmtBlk;
    uint32 mnVideoFmtBlkSize;
    uint8 *mpAudioFmtBlk;
    uint32 mnAudioFmtBlkSize;
    uint32 mnAudioMaxBufferSize;
    uint32 mnVideoMaxBufferSize;
    uint32 mnVideoTimescale;
    uint64 mnActualBaseTime;
    SignalQueue *mpAudioQ;
    SignalQueue *mpVideoQ;
    WFDMMThreads *mpVideoThread;
    WFDMMThreads *mpAudioThread;
    RTPStreamPort *mpRTPStreamPort;
    mediaSourceConfigType mCfg;
    MM_HANDLE mhCritSect;
    int      mnVideoFrameDropMode;
    FileSource  *mpFileSource;
    uint64   mnFlushTimeStamp;
    uint32   mnCurrVideoTime;
    int32    meState;
    int32    mnTracks;
    int32    mIonFd;
    int32    mAudioBufFd;
    int32    mAudioBufSize;
    int32    mVideoBufSize;
    ion_user_handle_t mAudioIonHandle;
    int32    mVideoBufFd;
    ion_user_handle_t mVideoIonHandle;
    uint8   *mAudioBufPtr;
    uint8   *mVideoBufPtr;
    uint8   *mVideoHeapPtr;
    int      mnModuleId;
    int      mClientData;
    bool     mbMediaTimeSet;
    bool     mbFlushInProgress;
    bool     mbPaused;
    WFDMMIDRTimerStatus meIDRTimerStatus;
    bool     mbIDRPendingRequest;
    bool     mbCancelPendingIDRRequest;
    bool     mbSinkIDRRequest;
};
#endif /*__WFD_MM_SINK_VIDEO_SOURCE_H__*/
