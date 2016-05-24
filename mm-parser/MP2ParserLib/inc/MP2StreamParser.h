#ifndef _MP2STREAM_PARSER_H
#define _MP2STREAM_PARSER_H
/* =======================================================================
                              MP2StreamParser.h
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

Copyright (c) 2009-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/inc/MP2StreamParser.h#88 $
========================================================================== */
#include "MP2StreamParserStatus.h"
#include "MP2StreamParserDataDefn.h"
#include "MMDebugMsg.h"
#include "parserdatadef.h"
#include "filesourcetypes.h"
#include "filebase.h"

#define DEFAULT_AUDIO_BUFF_SIZE    64000
#define DEFAULT_VIDEO_BUFF_SIZE    316000
#define MPEG2_FILE_READ_CHUNK_SIZE 188000
#define WFD_MIN_TS_PACKETS_PARSED  100
#define SEEK_JUMP_INTERVAL         1000
//#define MPEG2_PARSER_DEBUG

/*
*Callback function used by parser for reading the data.
*Parser does not implement this function, it's the responsibility
*of the APP to implement it.
*/

extern uint32 MP2StreamCallbakGetData (uint64 nOffset, uint32 nNumBytesRequest, unsigned char* pData,
                                       uint32  nMaxSize, uint32  u32UserData );
class MP2StreamParser
{
  /*=======================================================================
  * Public Members                                                        *
  * ======================================================================*/
  public:
                     MP2StreamParser(void* pUData,uint64 fsize,
                         bool blocatecodechdr,bool bHttpStreaming = false,
                         FileSourceFileFormat format= FILE_SOURCE_UNKNOWN);
   virtual           ~MP2StreamParser();
   /*! =======================================================================
   @brief         Returns current offset from m_nCurrOffset(private)
   @return        value of m_nCurrOffset
   ==========================================================================  */
   uint64            GetCurrOffset()
   {
     return m_nCurrOffset;
   }
   /*! =======================================================================
   @brief         Set current value of m_nCurrOffset
   ==========================================================================  */
   void              SetCurrOffset(uint64 ullCurrOffset)
   {
     m_nCurrOffset = ullCurrOffset;
   }
   uint32            GetTotalNumberOfTracks(void);
   uint32            GetTotalNumberOfAudioTracks(void);
   uint32            GetTotalNumberOfVideoTracks(void);
   MP2StreamStatus   StartParsing(void);
   uint32            GetTrackWholeIDList(uint32*);
   MP2StreamStatus   GetTrackType(uint32,track_type*,media_codec_type*);
   MP2StreamStatus   GetCurrentSample(uint32 trackId,uint8* dataBuffer,
                                      uint32 nMaxBufSize, int32* nBytesRead,
                                      float *frameTS, uint32 *pulIsSync = NULL);
   MP2StreamStatus   GetAssembledPESPacket(uint32 trackId,uint8* dataBuffer,
                                           uint32 nMaxBufSize, int32* nBytesRead,
                                           float *frameTS, uint32 *pulisSync = NULL);
   MP2StreamStatus   GetSampleAtFrameBoundary(uint32 trackId,uint8* dataBuffer,
                                              uint32 nMaxBufSize, int32* nBytesRead,float *frameTS);
   MP2StreamStatus   GetProgStreamSample(uint32 trackId,uint8* dataBuffer,
                                         uint32 nMaxBufSize, int32* nBytesRead,float *frameTS);
   double            GetPTSFromCurrentPESPacket();
   uint64            GetSampleOffset(void){return m_nCurrSampleOffset;};
   uint64            GetClipDurationInMsec();
   uint32            GetVideoWidth(uint32);
   uint32            GetVideoHeight(uint32);
   uint32            GetAudioSamplingFrequency(uint32);
   uint8             GetNumberOfAudioChannels(uint32);
   /*!===========================================================================
    @brief      Get Audio/Video/Text stream parameter

    @details    This function is used to get Audio/Video stream parameter i.e.
                codec configuration, profile, level from specific parser.

    @param[in]  ulTrackId           TrackID of media
    @param[in]  ulParamIndex        Parameter Index of the structure to be
                                    filled.It is from the FS_MEDIA_INDEXTYPE
                                    enumeration.
    @param[in]  pParameterStructure Pointer to client allocated structure to
                                    be filled by the underlying parser.

    @return     PARSER_ErrorNone in case of success otherwise Error.
    @note
   ============================================================================*/
   PARSER_ERRORTYPE  GetStreamParameter( uint32 ulTrackId,
                                      uint32 ulParamIndex, void* pParamStruct);
   uint8             GetLayer(uint32 trackid);
   uint8             GetVersion(uint32 trackid);
   media_codec_type  GetAudioCodec(uint32 trackid);
   uint32            GetTrackAverageBitRate(uint32);
   float             GetVideoFrameRate(uint32);
   bool              GetAVCCodecInfo(uint32* nBytesRead, uint8* dataBuf);
   bool              GetNextH264NALUnit(uint32 nOffset, uint8* dataBuf,
                                        uint8* nalUType, uint32* nalLen, int32 nBytesRead=0,
                                        uint32* dataOffset = NULL);
   uint32            GetAACAudioProfile(uint32);
   uint32            GetAACAudioFormat(uint32);
  /*! ======================================================================
   @brief   Get AAC Codec Info

   @detail  Retrieve AAC codec information in PS/TS

   @param[in]
            ulTrackid: Identifies the track to be repositioned.
            psAACInfo: AAC information struct pointer
   @return  TRUE if successful other wise returns FALSE
   @note      None.
   ========================================================================== */
   bool              GetAACAudioInfo(uint32 ulTrackID, aac_audio_info *psAACInfo);
   MP2StreamStatus   Seek(uint32, uint64,
                          uint64, mp2_stream_sample_info*, bool,
                          bool canSyncToNonKeyFrame=false,
                          int  nSyncFramesToSkip = 0);
   /*! ======================================================================
   @brief  Repositions given track to specified time

   @detail    Seeks given track in forward/backward direction to specified time
              in Program Stream

   @param[in]3
   trackid: Identifies the track to be repositioned.
   nReposTime:Target time to seek to
   nCurrPlayTime: Current playback time
   sample_info: Sample Info to be filled in if seek is successful
   canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
   nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

   @return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
   @note      None.
   ========================================================================== */
   MP2StreamStatus   SeekInProgramStream(uint32 ultrackid,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bcanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip);
   /*! ======================================================================
   @brief  Repositions given track to specified time

   @detail    Seeks given track in forward/backward direction to specified time
              in Transport Stream

   @param[in]3
   trackid: Identifies the track to be repositioned.
   nReposTime:Target time to seek to
   nCurrPlayTime: Current playback time
   sample_info: Sample Info to be filled in if seek is successful
   canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
   nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

   @return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
   @note      None.
   ========================================================================== */
   MP2StreamStatus   SeekInTransportStream(uint32 ultrackid,
                        uint64 lluReposTime,
                        uint64 lluCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bcanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip);
   /*! ======================================================================
   @brief  Repositions given track to specified time

   @detail    Seeks given track in backward direction to specified time
              in Transport Stream

   @param[in]3
   trackid: Identifies the track to be repositioned.
   nReposTime:Target time to seek to
   nCurrPlayTime: Current playback time
   sample_info: Sample Info to be filled in if seek is successful
   canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
   nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.
   eTrackType: Track type (Video/Audio)
   eMediaType: (H264/MPG2)
   @return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
   @note      None.
   ========================================================================== */
   MP2StreamStatus   TSSeekBackwards(uint32 ultrackid,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bcanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip,
                        track_type eTrackType,
                        media_codec_type eMediaType);
   /*! ======================================================================
   @brief  Repositions given track to specified time

   @detail    Seeks given track in forward direction to specified time
              in Transport Stream

   @param[in]3
   trackid: Identifies the track to be repositioned.
   nReposTime:Target time to seek to
   nCurrPlayTime: Current playback time
   sample_info: Sample Info to be filled in if seek is successful
   canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
   nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.
   eTrackType: Track type (Video/Audio)
   eMediaType: (H264/MPG2)
   @return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
   @note      None.
   ========================================================================== */
   MP2StreamStatus   TSSeekForward(uint32 ultrackid,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bcanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip,
                        track_type eTrackType,
                        media_codec_type eMediaType);
   /*! ======================================================================
   @brief  Checks if current frame is I Frame or not

   @detail Checks if current frame is I Frame or not
           for MPG2 and H264 codecs
   @param[in]3
   foundFrameType: Variable to return if a valid frame was found or not
                   True means valid frame was found and false means valid
                   frame not found
   eMediaType:     Specifies Codec type (H264/MPG2)
   @return    MP2STREAM_SUCCESS if I Frame is found other wise returns MP2STREAM_FAIL
   @note      None.
   ========================================================================== */
   MP2StreamStatus  isKeyFrame(media_codec_type eMediaType, bool *foundFrameType);
   MP2StreamStatus  SkipNSyncSamples(uint32 trackid, mp2_stream_sample_info* sample_info,
                          int  nSyncFramesToSkip = 0);
   MP2StreamStatus  GetTrackDecoderSpecificInfoContent(uint32 id,uint8*,uint8*);
   virtual uint64   GetLastRetrievedSampleOffset(uint32);
   virtual bool     GetBaseTime(uint32 trackid, double* nBaseTime);
   virtual bool     SetBaseTime(uint32 trackid, double nBaseTime);
   void             SetAudioSeekRefPTS(double audioSeekRefPTS){m_nAudioSeekRefPTS = (double)audioSeekRefPTS;};
   void             SetMediaAbortFlag(void) { m_bMediaAbort = true;};
   MP2StreamStatus  GetFileFormat(FileSourceFileFormat& fileFormat);
   uint32           GetBytesLost(void) {return m_nBytesLost;};
   void             SetEOFFlag(uint64 ullOffset) {m_nFileSize = ullOffset;m_bEOFFound = true;};
   virtual FileSourceStatus SetAudioOutputMode(FileSourceConfigItemEnum);
   virtual FileSourceStatus GetAudioOutputMode(bool*,FileSourceConfigItemEnum);
   virtual FileSourceStatus SetConfiguration(FileSourceConfigItemEnum);
   bool             GetPesPvtData(uint32 trackId, uint8* pPvtData);
   virtual bool     IsDRMProtection();
/*! =======================================================================
   @brief    Provides the closes PTS from a PES packet in the
             backward direction from a given offset.

   @param[in]      nTrackID  The track ID.
   @param[in]      ullAvailOffset  Buffered/Downloaded bytes.
   @param[out]     Derived timestamp
   @return         FILE_SOURCE_SUCCESS if successful in retrieving
                   buffered duration else returns FILE_SOURCE_FAIL.

   @note    It is expected that user has received the successul
            callback for OpenFile() earlier. In case of local file
            playback, value returned is same as track duration.
   ==========================================================================  */
   MP2StreamStatus GetPTSFromLastPES(uint32 trackId, uint64 ullAvailOffset,
       uint64 *pDuration, uint64 *pullEndOffset=NULL);
   virtual bool     IsMPeg1Video(){return m_bMpeg1Video;};
   /*! ======================================================================
   @brief  Get Available TimeStamp from the first PES packet in FWD direction

   @detail    Starts parsing from the position specified for a PES packet with
              non-zero PTS value
   @param[in] trackId Identifies elementary stream to demultiplex from
                   current pes packet
   @param[in] ullStartPos Absolute offset from where to start looking for
                       PES packet
   @param[out] ullStartOffset Start offset of PES packet
   @param[out] ullEndOffset   End offset of PES packet
   @param[out] *ullTimeStamp TS from PES packet

   @return    Void.
   @note      None.
   ========================================================================== */
   void GetPTSFromNextPES(uint32 ulTrackId,uint64 ullStartPos,
       uint64 *ullStartOffset, uint64 *ullEndOffset, uint64 *ullTimeStamp);

  private:
  //Functions internal to MP2 parser
  bool                      isTrackIdInIdStore(uint32);
  MP2StreamStatus           parseTransportStreamPacket(uint32 trackId = 0xff);
  MP2StreamStatus           parseAdaptionFieldControl(uint64);
  MP2StreamStatus           parseProgAssociationTable(uint64);
  MP2StreamStatus           parseAdaptationField(uint64);
  MP2StreamStatus           parseCondAccessTable(uint64);
  MP2StreamStatus           parseTSDescriptionTable(uint64);
  MP2StreamStatus           parseTSStreamSection(uint64);
  MP2StreamStatus           parseProgMapTable(uint64,int nArrayIndex);
  MP2StreamStatus           parseSceneDescrSection(uint64);
  MP2StreamStatus           parseObjDescrSection(uint64);
  MP2StreamStatus           parsePCIPacket(uint64,uint32);
  MP2StreamStatus           parseDSIPacket(uint64,uint32);

  MP2StreamStatus           parsePackHeader(uint64&, bool bParseMultiplePESPkts = true,
                                            uint32 trackId = 0xff,uint8* dataBuffer=NULL,
                                            uint32 nMaxBufSize = 0, int32* nBytesRead = NULL
                                            );

  MP2StreamStatus           parsePESPacket(uint64&,uint32,uint32 trackId = 0xff,uint8* dataBuffer=NULL,
                                           uint32 nMaxBufSize = 0, int32* nBytesRead = NULL);

  MP2StreamStatus           parseElementaryStream(uint64&,uint32,
                                                  track_type strmType,
                                                  uint8* dataBuffer=NULL,
                                                  uint32 nMaxBufSize = 0,
                                                  int32* nBytesRead = NULL);

  MP2StreamStatus           parseSystemTargetHeader(uint32, uint64&);
  MP2StreamStatus           parseProgStreamMap(uint64& nOffset);
  MP2StreamStatus           parseProgStream();
  uint32                    getBytesValue(int,uint8*);
  uint32                    parseProgDescriptors(uint64 nStartOffset, const int nBytes, CADescriptor* pDescData = NULL);
  uint32                    parseProgESDescriptors(uint64 nStartOffset, const int nBytes, ESDescriptor* pESDescData = NULL);
  uint16                    make15BitValue(uint8,uint16);
  uint32                    make22BitValue(uint16 part1,uint8 part2);
  uint16                    make9BitValue(uint8,uint8);
  uint32                    readMpeg2StreamData (uint64 nOffset, uint32 nNumBytesRequest, unsigned char* pData,
                                       uint32  nMaxSize, uint32  u32UserData );
  uint64                    make42BitValue(uint64, uint16);
  uint64                    make33BitValue(uint8 ms3bits,uint16 middle15bits,uint16 ls15bits);
  bool                      isProgramMapPacket(uint16 pid,int* index);
  bool                      isPESPacket(uint8*,uint32*);
  void                      getByteFromBitStream(uint8 *pByte, uint8 *pSrc, int nFromBit, int nBits);
  uint8                     getNumberOfStreamsFromTargetHeader(int,uint64);
  void                      updateStreamInfo(uint8,uint32);
  MP2StreamStatus           updateTrackInfoFromPSI(uint16 pid, uint32 streamType);
  bool                      isInitialParsingDone();
  bool                      getLastPTS();
  bool                      parseVideoMetaData(int32*);
  bool                      parseAudioMetaData();
  bool                      parseLPCMHeader(audio_info*);
  bool                      parseAC3Header(audio_info*);
  bool                      parseAACHeader(aac_audio_info*);
  bool                      reAllocStreamInfo(int);
  bool                      parsePictureHeader(uint64*);
  bool                      isSameStream(uint32* trackId, uint8* newPayload);
  bool                      makeAVCVideoConfig(uint8* pBuf,uint8* pSize);
  bool                      findH264NALTypeForFrame(uint8* nalType);
  bool                      findVC1FrameStartCode(uint8* pucPicType);
  bool                      findPicCodingTypeForFrame(uint8* picType);
  bool                      isFrameStartWithStartCode(uint32* nOffset,uint32 trackId, uint8* buf,
                                                    int32 nBytesRead, start_code_type* startCodeType);
  MP2StreamStatus           MakeAccessUnit(uint32 trackId, uint8* dataBuffer, uint32 totalBytes);
  bool                      isAssembledAtFrameBoundary(uint32 trackId, uint8* buf, uint32* dataLen, uint32 maxBufSize);
  bool                      getPidForTrackId(uint32 trackId, uint16* pid);
  MP2StreamStatus           LocateAudioFrameBoundary(uint8* buf, uint32* frame_len, float* frame_time, int* index, uint32 dataLen);
  bool                      isPSI(uint32 pid);
  bool                      freePAT(void);
  void                      correctTSDiscontinuity(uint32 trackId);
  bool                      freePMT(ProgramMapSection* curProgMapSection);
  float                     getSampleDelta(uint32 trackId);
  float                     getADTSTimestamp(float pesPTS, float curFrameDuration);
  float                     getPSTimestamp(float pesPTS, float curFrameDuration);
  int                       getContinuityCounterJump(uint8 presentCounter);
  MP2StreamStatus           backupInUnderrunBuffer(uint8* dataBuffer, uint32 bytesToCopy, uint32 nPESLen);
  uint32                    restoreFromUnderrunBuffer(uint8* dataBuffer, uint32* pPESLen);
  MP2StreamStatus           scanTSPacketToSeek(uint64* pcr, bool* bPCRFound, bool bForward);
  MP2StreamStatus           parsePTS(uint64 offset,uint8 ptsFlags,uint64* pts,uint64* dts = NULL);
  bool                      isAudioStreamType(uint8 ucStreamType);
  bool                      isVideoStreamType(uint8 ucStreamType);
  MP2StreamStatus           parseAVCDescriptor(uint64 ullStartOffset,uint8 ucBytes);
  MP2StreamStatus           parseCADescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseRegistrationDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseDTSAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseDTSHDAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseDTSHDSubstreamInfo(uint64 ullStartOffset, DTSHDSubstreamStruct* pSubstreamStruct,
                                              uint8 ucLen);
  bool                      parseDTSHeader(audio_info* pDTSInfo);
  void                      updateOffsetToNextPacket(uint64 ullStartOffset, bool bIsBDMVFormat = false,
                                                     bool bForward = true);
  MP2StreamStatus           parseMpeg4AudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseMp2AACAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseDVDLPCMAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           updateTotalTracks(uint32 streamType, uint16 pid);
  MP2StreamStatus           parseLanguageDescriptor(uint64 ullStartOffset, uint8 ucBytes);
  MP2StreamStatus           parseAC3AudioDescriptor(uint64 ullStartOffset, uint8 ucBytes);
// This code is only for ATSC standard, in other standards
// there may or may not be an EAC3 descriptor. Some standards
// use AC3 descriptor even for EAC3 descriptor
#ifdef ATSC_COMPLIANCE
  MP2StreamStatus           parseEAC3AudioDescriptor(uint64 ullStartOffset,uint8 ucBytes);
#endif
  void                      GetClipDurationFromPTS(uint32);
  void                      TSGetClipDurationFromPTS(uint32);
  //Private data members used by MP2 parser

  static double             m_nRefAfterDisc; /* used when discontinuity occurs */
  static int                m_nTimesRefUsed; /* used when discontinuity occurs */

  MP2StreamStatus           m_eParserState;
  uint64                    m_nCurrOffset;
  uint64                    m_nFileSize;
  uint64                    m_nEndPESPktTS;
  uint64                    m_nClipDuration;
  uint64                    m_nClipStartTime;
  void*                     m_pUserData;
  bool                      m_bProgramStream;
  bool                      m_bIsBDMVFormat; /* used to indicate BDMV/M2TS stream */
  bool                      m_bMpeg1Video; /* used to indicate mpeg1 program stream */
  bool                      m_bInitialParsingPending;
  uint8                     m_nstreams;
  uint32                    m_nDataBufferSize;
  uint8*                    m_pDataBuffer;
  uint8*                    m_pH264PESPacketBuf;
  uint8*                    m_pReadBuffer;
  uint64                    m_ullCurrentEnd;
  uint64                    m_ullCurrentStart;
  uint32                    m_nBytesRead;
  double                    m_nTotalADTSTrackDuration;
  double                    m_nTotalProgStreamDuration;
  double                    m_nPrevPESTime;

  uint32                    m_nBytesLost;

  uint64                    m_nCurrSampleOffset;

  uint64                    m_nStartOffset;
  bool                      m_bStartOffsetSet;

  int                       m_nProgNumSelected;
  int                       m_nVideoPIDSelected;
  int                       m_nAudioPIDSelected;
  int                       m_nStreamsSelected;

  uint8                     m_nAudioStreams;
  uint16*                   m_pAudioStreamIds;
  uint8                     m_nAudioIdsArraySize;
  uint8                     m_naudioIndex;

  uint8                     m_nVideoStreams;
  uint16*                   m_pVideoStreamIds;
  uint8                     m_nVideoIdsArraySize;
  uint8                     m_nvideoIndex;
  uint8                     m_nInitialPacksParsed;
  int                       m_nInitialTSPktParsed;
  double                    m_nRefPCR; //first pcr for the TS clip
  bool                      m_bRefPCRSet;
  double                    m_nRefAudioPTS; //first pts for the whole m3u8 or of the 1st segment
  double                    m_nFirstAudioPTS; //first pts for current segment
  double                    m_nAudioSeekRefPTS; //ref pts after seek has been performed
  double                    m_nLastAudioPTS; //last pts used for duration calc
  double                    m_nRefVideoPTS; //first pts for the whole m3u8 or of the 1st segment
  double                    m_nLastVideoPTS; //last pts used for duration calc
  bool                      m_bRefAudioPTSSet;  // flag to indicate whether reference PTS value for audio track is found or not
  bool                      m_bRefVideoPTSSet;  // flag to indicate whether reference PTS value for video track is found or not

  bool                      m_bpartialPESTS;
  bool                      m_bGetLastPTS; //bool currently used for calculating duration for TS
  bool                      m_bHttpStreaming;
  bool                      m_bLocateCodecHdr;
  uint64                    m_availOffset;
  bool                      m_bEOFFound; //bool used for streaming only
  bool                      m_bMediaAbort;
  int                       m_nPrevCC;

  ProgramStreamMap*         m_pProgramStreamMap;
  stream_info*              m_pStream_Info;
  PESPacket                 m_currPESPkt;
  MP2TransportStreamPkt     m_currTSPkt;
  ProgramAssociationSection m_ProgramAssociationSect;
  ConditionalAccessSection  m_CondAccessSection;
  ProgramMapSection*        m_ProgMapSection;
  DescriptionSection        m_DescSection;
  TSStreamSection           m_TSStreamSection;
  pack_header               m_currPackHeader;
  mp2_stream_sample_info    m_sampleInfo;
  pci_pkt*                  m_pFirstVOBUPCIPkt;
  pci_pkt*                  m_pCurrVOBUPCIPkt;
  dsi_pkt*                  m_pFirstVOBUDSIPkt;
  dsi_pkt*                  m_pCurrVOBUDSIPkt;
  avc_codec_info*           m_pAvcCodecInfo;
  partial_frame_data*       m_pPartialFrameData;
  underrun_frame_data       m_UnderrunBuffer; //Our buffer to store collected data in case of underrun during sample
                                              //collection so that we cna restore during next getNextSample call
  FileSourceConfigItemEnum m_hFrameOutputModeEnum;
  FileSourceConfigItemEnum m_hHeaderOutputModeEnum;
  FileSourceConfigItemEnum m_hTSDiscCorrectModeEnum;
  AVCDescriptor*           m_pAVCDescriptor;
  CADescriptor*            m_pCADescriptor;
  RegistrationDescriptor*  m_pRegistrationDescriptor;
  DTSAudioDescriptor*      m_pDTSAudioDescriptor;
  DTSHDAudioDescriptor*    m_pDTSHDAudioDescriptor;
  Mpeg4AudioDescriptor*    m_pMpeg4AudioDescriptor;
  Mp2AACAudioDescriptor*   m_pMp2AACAudioDescriptor;
  DVDLPCMAudioDescriptor*  m_pDVDLPCMAudioDescriptor;
  LanguageDescriptor*      m_pLanguageDescriptor;
  AC3AudioDescriptor*      m_pAC3AudioDescriptor;
  aac_audio_info           m_AACAudioInfo;
  FileSourceFileFormat     m_eFileFormat;
#ifdef ATSC_COMPLIANCE
  EAC3AudioDescriptor*     m_pEAC3AudioDescriptor;
#endif
};
#endif
