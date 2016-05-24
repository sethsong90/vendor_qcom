#ifndef _FLAC_PARSER_H
#define _FLAC_PARSER_H
/* =======================================================================
                              FlacParser.h
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

  Copyright(c) 2009-2013 by Qualcomm Technologies, Inc. All Rights Reserved
  Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/FlacParserLib/main/latest/inc/FlacParser.h#9 $
========================================================================== */
#include "FlacParserConstants.h"
#include "FlacParserStatus.h"
#include "FlacParserDataDefn.h"
#include "MMDebugMsg.h"
#include "filebase.h"

#ifdef FEATURE_FILESOURCE_FLAC_PARSER
typedef uint32 (*DataReadCallBack)(uint64 nOffset,
                                   uint32 nNumBytesRequest,
                                   unsigned char* pData,
                                   uint32  nMaxSize,
                                   uint32  u32UserData );
class FlacParser
{
  /*=======================================================================
  * Public Members                                                        *
  * ======================================================================*/
  public:
                            FlacParser(void* pUData,uint64 fsize,DataReadCallBack );
                            ~FlacParser();
   FlacParserStatus         StartParsing(uint64&);
   FlacParserStatus         StartParsing(uint64&, bool bForceSeek);
   uint32                   GetTrackWholeIDList(uint32*);
   uint8*                   GetCodecHeader(uint32);
   uint32                   GetCodecHeaderSize(uint32);
   uint32                   GetFlacMaxBufferSize(uint32);
   uint64                   GetClipDurationInMsec();
   uint32                   GetAudioSamplingFrequency(uint32);
   uint32                   GetAudioBitsPerSample(uint32);
   uint8                    GetNumberOfAudioChannels(uint32);
   uint32                   GetTrackAverageBitRate(uint32);

   FlacParserStatus         GetCurrentSample(uint32 trackId,uint8* dataBuffer,
                                             uint32 nMaxBufSize, int32* nBytesRead);
   FlacParserStatus         GetCurrentSampleTimeStamp(uint32,uint64*);

   FlacParserStatus         GetCurrentSample(uint32 trackId,uint8* dataBuffer,
                                              uint32 nMaxBufSize,
                                              int32* nBytesRead,
                                              bool bGetFrameBoundary);
   FlacParserStatus         GetFlacStreamInfo(uint32 id,flac_metadata_streaminfo*);
   /*
   * Parameter details:
   * uint32 Track Id
   * uint64 Requested reposition time
   * uint32 Current Playback Time
   * flac_stream_sample_info* SampleInfo filled in if seek is successful
   * bool Specifies if need to do fwd or rwd
   */

   FlacParserStatus         Seek(uint32, uint64, uint64, flac_stream_sample_info*, bool);
   uint32                   GetTotalNumberOfAudioTracks(void){return m_nAudioStreams;}
   bool                     IsMetaDataParsingDone();
   void                     GetClipMetaData(int,uint8*,uint32*);
   void                     GetClipMetaDataIndex(uint8*,int,int*);

  private:

  //Functions/data types internal to FLAC parser
   FlacParserStatus         FindNextFrameOffset(uint8* pBuffer, uint32 nSize, uint32 *nOffset);
   FlacParserStatus         DecodeFrameHeader(uint8* pBuffer, uint32 nSize);
   void                     ReadUTF8_uint64(uint8 *pBuf,
                                             uint64 *nOutVal,
                                             uint8 *nBytes);
   void                     ReadUTF8_uint32(uint8 *pBuf,
                                           uint32 *nOutVal,
                                           uint8 *nBytes);
   uint8                    Calculate_CRC8(uint8 *pData,
                                           uint32 nLen);
  void                      GenerateSeekTable();
  bool                      ParseStreamInfoMetaBlock(uint64&,uint32);
  bool                      ParseSeekTableMetaBlock(uint64&,uint32);
  bool                      ParsePictureMetaBlock(uint64&,uint32);
  bool                      SkipMetaBlock(uint64&,uint32);
  void                      ParseCommentHdr(uint32,uint32);
  uint64                    m_nCurrOffset;
  uint64                    m_nCurrentTimeStamp;
  uint64                    m_nFileSize;
  uint64                    m_nClipDuration;
  uint32                    m_nCodecHeaderSize;
  uint32                    m_nFlacDataBufferSize;
  uint32                    m_nMetaData;
  uint8                     m_nAudioStreams;
  uint8*                    m_pCodecHeader;
  uint8*                    m_pDataBuffer;
  void*                     m_pUserData;
  bool                      m_bFlacMetaDataParsed;
  uint32                    m_nSeekInfoArraySize;
  uint32                    m_nSeekInfoValidEntries;
  uint64*                   m_pSeekInfoArray;
  DataReadCallBack          m_pReadCallback;
  FlacParserStatus          m_eParserStatus;
  flac_metadata_streaminfo* m_pStreamInfoMetaBlock;
  flac_metadata_seektable*  m_pSeekTableMetaBlock;
  flac_metadata_picture*    m_pPictureMetaBlock;
  flac_frame_header*        m_pCurrentFrameHdr;
  flac_meta_data*           m_pMetaData;
};
#endif//#ifndef _FLAC_PARSER_H
#endif //#ifdef FEATURE_FILESOURCE_FLAC_PARSER

