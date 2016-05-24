#ifndef _MKAV_PARSER_H
#define _MKAV_PARSER_H
/* =======================================================================
                              MKAVParser.h
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

Copyright (c) 2011-2013 Qualcomm Technologies Inc, All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MKAVParserLib/main/latest/inc/mkavparser.h#31 $
========================================================================== */
#include "mkavparserstatus.h"
#include "mkavparserdatadefn.h"
#include "MMDebugMsg.h"
#include "parserinternaldefs.h"
#include "filesourcetypes.h"
#include "ztl.h"
#ifdef FEATURE_FILESOURCE_MKV_PARSER
//#define MKAV_PARSER_DEBUG

/*
*Callback function used by parser for reading the data.
*Parser does not implement this function, it's the responsibility
*of the APP to implement it.
*/

extern uint32 MKAVFileCallbakGetData (uint64 nOffset,
                                      uint32 nNumBytesRequest,
                                      unsigned char* pData,
                                      uint32  nMaxSize,
                                      uint32  u32UserData );
extern bool   MKAVCheckAvailableData(uint64*,bool*,uint32);

typedef struct _tag_info_type
{
  uint8* pTagName;
  uint32 ulTagNameLen;
  uint8* pTagLang;
  uint32 ulTagLangLen;
  uint8* pTagString;
  uint32 ulTagStringLen;
  uint32 ulTagBinValue;
  bool   bTagDefault;
}tag_info_type;

class MKAVParser
{
  /*=======================================================================
  * Public Members                                                        *
  * ======================================================================*/
  public:
                            MKAVParser(void*,uint64 fsize,bool,bool);
                            ~MKAVParser();
   MKAV_API_STATUS          StartParsing(void);
   MKAV_API_STATUS          ParseByteStream(void);
   MKAV_API_STATUS          GetCurrentSample(uint32 trackId,uint8* pucDataBuf,
                                         uint32 ullBufSize, int32* plBytesRead,
                                         mkav_stream_sample_info* psampleInfo);

  /*! ======================================================================
  @brief  Repositions given track to specified time

  @detail  Seeks given track in forward/backward direction to specified time

  @param[in]
   trackid: Identifies the track to be repositioned.
   nReposTime:Target time to seek to
   nCurrPlayTime: Current playback time
   pSampleInfo: Sample Info to be filled in if seek is successful
   canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
   nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

  @return    MKAVPARSER_SUCCESS if successful other wise returns MKAVPARSER_FAIL
  @note      None.
  ========================================================================== */

   MKAV_API_STATUS          Seek(uint32, uint64, uint64,
                                 mkav_stream_sample_info*,
                                 bool, bool canSyncToNonKeyFrame=false,
                                 int  nSyncFramesToSkip = 0);
   uint8                    randomAccessDenied();
   uint32                   GetTrackWholeIDList(uint32*);
   uint8*                   GetCodecHeader(uint32);
   uint32                   GetCodecHeaderSize(uint32);

   uint64                   GetClipDurationInMsec();
   uint32                   GetVideoWidth(uint32);
   uint32                   GetVideoHeight(uint32);
   bool                     GetAudioTrackProperties(uint32,
                                                    mkav_audio_info*);
   uint32                   GetTotalNumberOfTracks(void){return m_nstreams;}
   uint32                   GetTrackBufferSize(uint32);
   mkav_media_codec_type    GetTrackCodecType(uint32);
   mkav_track_type          GetTrackType(uint32);
   float                    GetVideoFrameRate(uint32);
   bool                     bIsWebm() {return m_bIsWebm;}

  tag_info_type*            GetClipMetaData(uint32 ulIndex);
  FileSourceStatus          SetAudioOutputMode(FileSourceConfigItemEnum Enum);
  FileSourceStatus          GetAudioOutputMode(bool* bRet,
                                               FileSourceConfigItemEnum Enum);
/* ============================================================================
  @brief  getBufferedDuration.

  @details    This function is used to calculate the playback time based on the
              given Offset value.

  @param[in]      ulTrackId           Track Id.
  @param[in]      sllAvailBytes       Available offset.
  @param[in/out]  pullBufferedTime    Playback Time.

  @return  "TRUE" if successful in calculating the approximate playback time
           else returns "FALSE".
  @note       None.
=============================================================================*/
  bool                      getBufferedDuration(uint32  ulTrackId,
                                                int64   sllOffset,
                                                uint64* pullBufferedTime);
/* ============================================================================
  @brief  GetOffsetForTime.

  @details    This function is used to calculate the approximate offset value
              based on the given Playback timestamp value.

  @param[in]      ullPBTime           Given Playback Time.
  @param[in/out]  pullFileOffset      Parameter to store o/p Offset Value.
  @param[in]      ulTrackId           Track Id.
  @param[in]      ullCurPosTimeStamp  Current Playback Time.
  @param[in]      reposTime           Reposition Time.

  @return  "TRUE" if successful in calculating the approximate offset value
           else returns "FALSE".
  @note       None.
=============================================================================*/
  bool                      GetOffsetForTime(uint64  ullPBTime,
                                             uint64* pullFileOffset,
                                             uint32  /*ulTrackID*/,
                                             uint64  /*currentPosTimeStamp*/,
                                             uint64& /*reposTime*/);
 private:
  MKAV_PARSER_STATE         m_eParserState;
  MKAV_PARSER_STATE         m_eParserPrvState;

  uint64                    m_nCurrOffset;
  uint64                    m_nFileSize;
  uint64                    m_nSegmentPosn;//absolute file offset where segment data starts
  uint64                    m_nClipDuration;
  uint64                    m_nCurrCluster;
  uint64                    m_nSizeReq;//valid only in case of http to handle under run..

  uint8                     m_nCodecHdrToSend;
  uint8                     m_nstreams;
  uint8                     m_nAudioStreams;
  uint8                     m_nVideoStreams;

  uint8*                    m_pTempBuffer;
  uint8*                    m_pDataBuffer;

  uint32                    m_nMetaData;
  uint32                    m_nCodecHdrSizes[MKAV_VORBIS_CODEC_HDRS];
  uint32                    m_nTrackEntry;
  uint32                    m_nTempBuffSize;

  void*                     m_pUserData;
  bool                      m_bPlayAudio;
  bool                      m_bHttpPlay;
  bool                      m_bEndOfData;
  bool                      m_bIsWebm;

  ebml_doc_hdr*             m_pEBMlDocHdr;
  mkav_track_entry_info*    m_pTrackEntry;
  seekheadinfo*             m_pSeekHeadInfo;
  all_clusters_info*        m_pAllClustersInfo;
  cluster_info*             m_pCurrCluster;
  segment_element_info*     m_pSegmentElementInfo;
  segment_info*             m_pSegmentInfo;
  mkav_stream_sample_info*  m_pSampleInfo;
  mkav_vfw_info*            m_pVFWHeader;
  all_cues_info*            m_pAllCuesInfo;

  ZArray<tag_info_type *> TagInfoArray;
  uint32 m_nTagInfoCount;

  //Current output mode
  FileSourceConfigItemEnum  m_eFrameOutputModeEnum;
  FileSourceConfigItemEnum  m_eHeaderOutputModeEnum;

  bool                      EnsureDataCanBeRead(uint64, uint64);
  bool                      IsMetaDataParsingDone();
  bool                      MapFileOffsetToCluster(uint64  ullOffset,
                                                   uint32* pulClusterNo,
                                                   uint8*  pucHdrSize);

  void                      MapMKAVCodecID(uint8*,uint8,mkav_track_entry_info*);
  void                      PrepareAVCCodecInfo(mkav_avc1_info*);
  void                      FreeUpSegmentInfoMemory(segment_info*);
  void                      ResetCurrentClusterInfo();
  void                      InitData();
  void                      UpdateClustersInfoFromSEEKHeads();
  void                      FreeClustersInfo();

  uint32                    GetDataFromSource(uint64, uint32,
                                              unsigned char*, uint32);
  uint64                    DoWeNeedToParseMoreSeekHead();

  MKAV_API_STATUS           ParseEBMLDocHeader(uint64,uint64);
  MKAV_API_STATUS           ParseSegmentElement(uint64, uint64,
                                                bool bupdateoffset=false);
  MKAV_API_STATUS           ParseSegmentInfo(uint64,uint64);
  MKAV_API_STATUS           ParseTracksElement(uint64,uint64);
  MKAV_API_STATUS           ParseSeekHeadElement(uint64,uint64);
  MKAV_API_STATUS           ParseSeekElement(uint64,uint64,seekhead*);
  MKAV_API_STATUS           ParseTrackEntryElement(uint64,uint64);
  MKAV_API_STATUS           ParseClusterElement(uint64,uint64,
                                                cluster_info* pcluster= NULL,
                                                uint8 nhdrSize=0);
  MKAV_API_STATUS           ParseBlockGroupElement(uint64,uint64,cluster_info*);
  MKAV_API_STATUS           ParseBlockElement(uint64,uint64,cluster_info*);
  MKAV_API_STATUS           ParseVideoInfo(uint8 *pDataBuffer, uint32 ullSize,
                                           mkav_video_info* pVideoinfo);
  MKAV_API_STATUS           ParseAudioInfo(uint8 *pDataBuffer,uint64 ullAtomSize,
                                           mkav_audio_info* pAudioInfo);
  MKAV_API_STATUS           ParseContentEncodeInfo(uint8 *pDataBuffer,
                                                uint64 ullElementSize,
                                                mkav_encode_info* pEncodeInfo);

  MKAV_API_STATUS           ParseCuesInfo(uint64,uint64);
  MKAV_API_STATUS           ParseCuePoint(uint64,uint64,cue_point_info*);
  MKAV_API_STATUS           ParseCueTrackPosnInfo(
                                          uint8 *pDataBuffer,
                                          uint64 ullElementSize,
                                          cue_track_posn_info* pCurrTrPosInfo);
  MKAV_API_STATUS           ParseCueRefInfo(uint8 *pDataBuffer,
                                            uint64 ullElementSize,
                                            cue_ref_info* pCueRefInfo);

  MKAV_API_STATUS           ParseTagsElement(uint64,uint64);
  MKAV_API_STATUS           ParseTagElement(uint64,uint64);
  MKAV_API_STATUS           ReadFrames(uint32 ulTrackId,
                                       uint8 *pucDataBuf,
                                       uint32 *pulBufferSize,
                                       mkav_stream_sample_info *pSampleInfo,
                                       bool   &bIsFrameAvailable);
  void                      UpdateSampleProperties(uint32 ulTrackId,
                                          uint32 ulFrameDurinMS,
                                          uint32 ulBufferSize,
                                          mkav_stream_sample_info *pSampleInfo,
                                          blockinfo       *pBlockInfo,
                                          bool   &rbIsFrameAvailable);
  MKAV_API_STATUS           ParseNextClusterHeader(uint64 &ullClusterOffset,
                                                   uint64 &ullClusterSize);
  bool                      GetOffsetFromSeekHead(uint32 ulElementId,
                                                  uint64 &ullOffset);
  MKAV_API_STATUS           CalcFrameSizes(uint8   ucNumFrames,
                                           uint8   ucFlags,
                                           uint64  ullOffset,
                                           uint32  ulDataRead,
                                           uint64  ullElemSize,
                                           uint32& rulIndex,
                                           uint32* pulFramesSize);
  MKAV_API_STATUS           UpdateCodecDetails();

  /*! ======================================================================
  @brief  Repositions given track to ZERO

  @detail  Update track position to start of the file

  @param[in]
   pSampleInfo: Sample Info to be filled in
   trackid:     Identifies the track to be repositioned.

  @return    MKAV_API_SUCCESS if successful other wise returns MKAV_API_FAIL
  @note      None.
  ========================================================================== */
  MKAV_API_STATUS           SeekToZERO(mkav_stream_sample_info *pSampleInfo,
                                       uint32 ulTrackID);

  /*! ======================================================================
  @brief  Updates the Sample Info properties

  @detail  Updates the sample info properties with given seek point

  @param[in]
   ulTrackID:     Identifies the track to be repositioned.
   pSampleInfo:   Sample Info to be updated
   pCuePointInfo: Cue point info structure

  @return    MKAV_API_SUCCESS if successful other wise returns MKAV_API_FAIL
  @note      None.
========================================================================== */
  MKAV_API_STATUS           UpdateSeekSampleProperties(
                                       uint32 ulTrackID,
                                       mkav_stream_sample_info *pSampleInfo,
                                       cue_point_info* pCuePointInfo);

  /*! =========================================================================
  @brief  Prepares codec config data for HEVC codec

  @detail  Reads codec private data atom from hevc track and prepares codec
           config data in the form of sequence of NALs

  @param[in]
   pucCodecPvt:     Codec private data pointer.
   ulCodecSize:     Private data size
   pHEVCInfo  :     Structure to keep all NALUnit data

  @return    Total size for codec config data
  @note      None.
  ========================================================================== */
  uint32                 PrepareHEVCCodecInfo(uint8* pucCodecPvt,
                                              uint32 ulCodecSize,
                                              uint32* pulNALULenMinusOne,
                                              uint8*  pHEVCInfo = NULL);
};
#endif//#ifndef _MKAV_PARSER_H
#endif //#ifdef FEATURE_FILESOURCE_MKV_PARSER

