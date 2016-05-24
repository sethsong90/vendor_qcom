/* =======================================================================
                              MP2StreamParser.cpp
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

Copyright (c) 2009-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/src/MP2StreamParser.cpp#169 $
========================================================================== */
#include "MP2StreamParser.h"
#include "string.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMMalloc.h"
#include "MP2Stream.h"
#include "H264HeaderParser.h"
#include "MMTimer.h"

#include "math.h"

#include <stdio.h>

double MP2StreamParser::m_nRefAfterDisc = 0;
int    MP2StreamParser::m_nTimesRefUsed = 0;

/*! ======================================================================
@brief    MP2StreamParser constructor

@detail    Instantiates MP2StreamParser to parse MP2 Transport stream.

@param[in]  pUData    APP's UserData.This will be passed back when this parser invokes the callbacks.
@return    N/A
@note      None
========================================================================== */
MP2StreamParser::MP2StreamParser(void* pUData,uint64 fsize,
    bool blocatecodechdr,bool bHttpStreaming,
    FileSourceFileFormat eFormat)
{
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser blocatecodechdr %d",blocatecodechdr);
#endif
  memset(&m_currPESPkt,0,sizeof(PESPacket));
  memset(&m_currTSPkt,0,sizeof(MP2TransportStreamPkt));
  memset(&m_ProgramAssociationSect,0,sizeof(ProgramAssociationSection));
  memset(&m_CondAccessSection,0,sizeof(ConditionalAccessSection));
  memset(&m_DescSection,0,sizeof(DescriptionSection));
  memset(&m_currPackHeader,0,sizeof(pack_header));
  memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
  memset(&m_TSStreamSection,0,sizeof(m_TSStreamSection));
  m_ullCurrentEnd=0;
  m_ullCurrentStart=0;
  m_pAvcCodecInfo = NULL;
  m_ProgMapSection = NULL;
  m_pProgramStreamMap = NULL;
  m_pUserData = pUData;
  m_nCurrOffset = m_nCurrSampleOffset = 0;
  m_bProgramStream = false;
  m_bIsBDMVFormat = false;
  m_nFileSize = fsize;
  m_pDataBuffer = NULL;
  m_nDataBufferSize = 0;
  m_nAudioStreams = m_nVideoStreams = 0;
  m_pAudioStreamIds = m_pVideoStreamIds = NULL;
  m_nAudioIdsArraySize = m_nVideoIdsArraySize = 0;
  m_naudioIndex = m_nvideoIndex = 0;
  m_bInitialParsingPending = true;
  m_nClipDuration = m_nClipStartTime = 0;
  m_nstreams      = 0;
  m_nProgNumSelected = m_nVideoPIDSelected = m_nAudioPIDSelected = 0;
  m_nInitialPacksParsed = m_nInitialTSPktParsed = 0;
  m_nRefPCR = 0;
  m_nRefAudioPTS = m_nRefVideoPTS = m_nFirstAudioPTS = m_nAudioSeekRefPTS = 0;
  m_nLastAudioPTS = m_nLastVideoPTS = 0;
  m_pStream_Info  = NULL;
  m_pFirstVOBUPCIPkt = NULL;
  m_pFirstVOBUDSIPkt = NULL;
  m_pCurrVOBUPCIPkt = NULL;
  m_pCurrVOBUDSIPkt = NULL;
  m_eParserState = MP2STREAM_IDLE;
  m_bRefPCRSet = false;
  m_bGetLastPTS = false;
  m_bpartialPESTS = false;
  m_bRefVideoPTSSet = false;
  m_bRefAudioPTSSet = false;
  m_bHttpStreaming = bHttpStreaming;
  m_nStreamsSelected = 0;
  m_pPartialFrameData = NULL;
  m_availOffset = 0;
  m_nStartOffset = 0;
  m_bStartOffsetSet = false;
  m_nTotalADTSTrackDuration = m_nPrevPESTime = m_nTotalProgStreamDuration = 0;
  m_bLocateCodecHdr = blocatecodechdr;
  m_bMediaAbort = false;
  m_nBytesRead = 0;
  m_nBytesLost = 0;
  memset(&m_UnderrunBuffer, 0, sizeof(m_UnderrunBuffer));
  m_nPrevCC = -1;
  m_bEOFFound = false;
  m_bMpeg1Video = false;
  //by default, parser won't process anything for the data being read for audio AU
  m_hFrameOutputModeEnum = FILE_SOURCE_MEDIA_OUTPUT_BYTES_STREAM;
  m_hHeaderOutputModeEnum = FILE_SOURCE_MEDIA_RETAIN_AUDIO_HEADER;
  m_hTSDiscCorrectModeEnum = FILE_SOURCE_MEDIA_DISABLE_TS_DISCONTINUITY_CORRECTION;
  m_pAVCDescriptor = NULL;
  m_pCADescriptor = NULL;
  m_pRegistrationDescriptor = NULL;
  m_pDTSAudioDescriptor = NULL;
  m_pDTSHDAudioDescriptor = NULL;
  m_pMpeg4AudioDescriptor = NULL;
  m_pMp2AACAudioDescriptor = NULL;
  m_pDVDLPCMAudioDescriptor = NULL;
  m_pLanguageDescriptor = NULL;
  m_pReadBuffer = NULL;
  m_pReadBuffer = (uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);
  if(m_pReadBuffer)
  {
    memset(m_pReadBuffer,0,MPEG2_FILE_READ_CHUNK_SIZE);
  }
  m_pH264PESPacketBuf = NULL;
  m_pAC3AudioDescriptor = NULL;
  m_nEndPESPktTS=0;
  memset(&m_AACAudioInfo, 0, sizeof(aac_audio_info));
  m_eFileFormat = eFormat;
#ifdef ATSC_COMPLIANCE
  m_pEAC3AudioDescriptor = NULL;
#endif
}
/*! ======================================================================
@brief    MP2StreamParser destructor

@detail    MP2StreamParser destructor

@param[in] N/A
@return    N/A
@note      None
========================================================================== */
MP2StreamParser::~MP2StreamParser()
{
  if(m_currPackHeader.sys_header)
  {
    MM_Free(m_currPackHeader.sys_header);
  }
  if(m_pDataBuffer)
  {
    MM_Free(m_pDataBuffer);
  }
  if(m_pReadBuffer)
  {
    MM_Free(m_pReadBuffer);
  }
  if(m_pAudioStreamIds)
  {
    MM_Free(m_pAudioStreamIds);
  }
  if(m_pVideoStreamIds)
  {
    MM_Free(m_pVideoStreamIds);
  }
  if(m_pStream_Info)
  {
    MM_Free(m_pStream_Info);
  }
  if(m_ProgMapSection && m_ProgMapSection->ESDescData)
  {
    if(m_ProgMapSection->ESDescData->DescriptorData)
    {
      MM_Free(m_ProgMapSection->ESDescData->DescriptorData);
    }
    MM_Free(m_ProgMapSection->ESDescData);
  }
  if(m_ProgMapSection && m_ProgMapSection->programDescriptorsData)
  {
    if(m_ProgMapSection->programDescriptorsData->pvt_data_byte)
    {
      MM_Free(m_ProgMapSection->programDescriptorsData->pvt_data_byte);
    }
    MM_Free(m_ProgMapSection->programDescriptorsData);
  }
  if(m_ProgMapSection)
  {
    MM_Free(m_ProgMapSection);
  }
  if(m_ProgramAssociationSect.program_numbers)
  {
    MM_Free(m_ProgramAssociationSect.program_numbers);
  }
  if(m_ProgramAssociationSect.ts_PID)
  {
    MM_Free(m_ProgramAssociationSect.ts_PID);
  }
  if(m_CondAccessSection.DescriptorsData)
  {
    MM_Delete_Array(m_CondAccessSection.DescriptorsData);
  }
  if(m_pFirstVOBUPCIPkt)
  {
    MM_Free(m_pFirstVOBUPCIPkt);
  }
  if(m_pFirstVOBUDSIPkt)
  {
    MM_Free(m_pFirstVOBUDSIPkt);
  }
  if(m_pCurrVOBUPCIPkt)
  {
    MM_Free(m_pCurrVOBUPCIPkt);
  }
  if(m_pCurrVOBUDSIPkt)
  {
    MM_Free(m_pCurrVOBUDSIPkt);
  }
  if(m_pAvcCodecInfo)
  {
    if(m_pAvcCodecInfo->codecInfoBuf)
      MM_Free(m_pAvcCodecInfo->codecInfoBuf);
    MM_Free(m_pAvcCodecInfo);
  }
  if(m_pPartialFrameData)
  {
    MM_Free(m_pPartialFrameData);
  }
  if(m_UnderrunBuffer.pFrameBuf)
  {
    MM_Free(m_UnderrunBuffer.pFrameBuf);
  }
  if(m_pH264PESPacketBuf)
  {
    MM_Free(m_pH264PESPacketBuf);
    m_pH264PESPacketBuf = NULL;
  }
  if(m_pAVCDescriptor)
  {
    MM_Free(m_pAVCDescriptor);
  }
  if(m_pCADescriptor)
  {
    MM_Free(m_pCADescriptor);
  }
  if(m_pRegistrationDescriptor)
  {
    if(m_pRegistrationDescriptor->pucAdditionalInfo)
    {
      MM_Free(m_pRegistrationDescriptor->pucAdditionalInfo);
    }
    MM_Free(m_pRegistrationDescriptor);
  }
  if(m_pDTSAudioDescriptor)
  {
    if(m_pDTSAudioDescriptor->pucAdditionalInfo)
    {
      MM_Free(m_pDTSAudioDescriptor->pucAdditionalInfo);
    }
    MM_Free(m_pDTSAudioDescriptor);
  }
  if(m_pDTSHDAudioDescriptor)
  {
    if(m_pDTSHDAudioDescriptor->pucAdditionalInfo)
    {
      MM_Free(m_pDTSHDAudioDescriptor->pucAdditionalInfo);
    }
    if(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct)
    {
      if(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->pAssetStruct)
      {
        MM_Free(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->pAssetStruct);
      }
      MM_Free(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct);
    }
    if(m_pDTSHDAudioDescriptor->pSubstream0Struct)
    {
      if(m_pDTSHDAudioDescriptor->pSubstream0Struct->pAssetStruct)
      {
        MM_Free(m_pDTSHDAudioDescriptor->pSubstream0Struct->pAssetStruct);
      }
      MM_Free(m_pDTSHDAudioDescriptor->pSubstream0Struct);
    }
    if(m_pDTSHDAudioDescriptor->pSubstream1Struct)
    {
      if(m_pDTSHDAudioDescriptor->pSubstream1Struct->pAssetStruct)
      {
        MM_Free(m_pDTSHDAudioDescriptor->pSubstream1Struct->pAssetStruct);
      }
      MM_Free(m_pDTSHDAudioDescriptor->pSubstream1Struct);
    }
    if(m_pDTSHDAudioDescriptor->pSubstream2Struct)
    {
      if(m_pDTSHDAudioDescriptor->pSubstream2Struct->pAssetStruct)
      {
        MM_Free(m_pDTSHDAudioDescriptor->pSubstream2Struct->pAssetStruct);
      }
      MM_Free(m_pDTSHDAudioDescriptor->pSubstream2Struct);
    }
    if(m_pDTSHDAudioDescriptor->pSubstream3Struct)
    {
      if(m_pDTSHDAudioDescriptor->pSubstream3Struct->pAssetStruct)
      {
        MM_Free(m_pDTSHDAudioDescriptor->pSubstream3Struct->pAssetStruct);
      }
      MM_Free(m_pDTSHDAudioDescriptor->pSubstream3Struct);
    }
    MM_Free(m_pDTSHDAudioDescriptor);
  }
  if(m_pMpeg4AudioDescriptor)
  {
    MM_Free(m_pMpeg4AudioDescriptor);
  }
  if(m_pMp2AACAudioDescriptor)
  {
    MM_Free(m_pMp2AACAudioDescriptor);
  }
  if(m_pDVDLPCMAudioDescriptor)
  {
    MM_Free(m_pDVDLPCMAudioDescriptor);
  }
  if(m_pLanguageDescriptor)
  {
    MM_Free(m_pLanguageDescriptor);
  }
  m_nDataBufferSize = 0;
  if(m_pProgramStreamMap)
  {
    MM_Free(m_pProgramStreamMap);
  }
  if (m_pAC3AudioDescriptor)
  {
    MM_Free(m_pAC3AudioDescriptor);
    m_pAC3AudioDescriptor = NULL;
  }
#ifdef ATSC_COMPLIANCE
  if (m_pEAC3AudioDescriptor)
  {
    MM_Free(m_pEAC3AudioDescriptor);
    m_pEAC3AudioDescriptor = NULL;
  }
#endif
}
/*! ======================================================================
@brief    Returns total number of streams/programs from MP2 Stream

@detail    This function retrieves the total number of tracks/streams/programs
        from current Mpeg2 Transport stream.

@param[in] N/A
@return    Total number of streams/tracks from MP2 Transport stream
@note      It is expected that StartParsing has been called and it returned SUCCESS.
========================================================================== */
uint32 MP2StreamParser::GetTotalNumberOfTracks(void)
{
  return (m_nAudioStreams + m_nVideoStreams);
}
/*! ======================================================================
@brief    Returns total number of audio streams/programs from MP2 Stream

@detail    This function retrieves the total number of audio tracks/streams/programs
        from current Mpeg2 Transport stream.

@param[in] N/A
@return    Total number of audio streams/tracks from MP2 Transport stream
@note      It is expected that StartParsing has been called and it returned SUCCESS.
========================================================================== */
uint32 MP2StreamParser::GetTotalNumberOfAudioTracks(void)
{
  return m_nAudioStreams;
}
/*! ======================================================================
@brief    Returns total number of video streams/programs from MP2 Stream

@detail    This function retrieves the total number of video tracks/streams/programs
        from current Mpeg2 Transport stream.

@param[in] N/A
@return    Total number of video streams/tracks from MP2 Transport stream
@note      It is expected that StartParsing has been called and it returned SUCCESS.
========================================================================== */
uint32 MP2StreamParser::GetTotalNumberOfVideoTracks(void)
{
  return m_nVideoStreams;
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetTrackWholeIDList

DESCRIPTION:
Returns trackId list for all the tracks in given clip.

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:

SIDE EFFECTS:
  None.
=============================================================================*/
uint32 MP2StreamParser::GetTrackWholeIDList(uint32* idList)
{
  if(!idList)
  {
    return 0;
  }
  if(m_pVideoStreamIds)
  {
    for (uint32 i = 0; i < m_nVideoStreams; i++)
    {
      (*idList) = m_pVideoStreamIds[i];
      idList++;
    }
  }
  if(m_pAudioStreamIds)
  {
    for (uint32 i = 0; i < m_nAudioStreams; i++)
    {
      (*idList) = m_pAudioStreamIds[i];
      idList++;
    }
  }
  return GetTotalNumberOfTracks();
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetTrackType

DESCRIPTION:
Returns track type for given track id

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:
 Success if track id is valid otherwise, returns invalid parameter.
SIDE EFFECTS:
  None.
=============================================================================*/
MP2StreamStatus MP2StreamParser::GetTrackType(uint32 id,track_type* type,media_codec_type* codectype)
{
  MP2StreamStatus retError = MP2STREAM_INVALID_PARAM;
  if(type && codectype)
  {
    *codectype = UNKNOWN_AUDIO_VIDEO_CODEC;
    *type = TRACK_TYPE_UNKNOWN;
    for(int i = 0; i < m_nstreams;i++)
    {
      if(id == m_pStream_Info[i].stream_id)
      {
        if(m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO )
        {
          *type = TRACK_TYPE_AUDIO;
          *codectype = m_pStream_Info[i].audio_stream_info.Audio_Codec;
          retError = MP2STREAM_SUCCESS;
        }
        else if(m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO )
        {
          *type = TRACK_TYPE_VIDEO;
          *codectype = m_pStream_Info[i].video_stream_info.Video_Codec;
          retError = MP2STREAM_SUCCESS;
        }
        break;
      }
    }
  }
  return retError;
}
/* =============================================================================
FUNCTION:
  MP2StreamParser::GetClipDurationInMsec

DESCRIPTION:
 Returns clip duration from the current parsed data.

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:

SIDE EFFECTS:
  None.
=============================================================================*/
uint64 MP2StreamParser::GetClipDurationInMsec()
{
  uint64 audioDuration = 0;
  uint64 videoDuration = 0;
  bool bPlayAudio      = false;
  bool bPlayVideo      = false;
  MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
  if (pMP2TStream)
  {
    bPlayAudio = pMP2TStream->isAudioInstance();
    bPlayVideo = pMP2TStream->isVideoInstance();
  }
  if(m_bProgramStream)
    return m_nClipDuration;
  else
  {
    if(m_bInitialParsingPending)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
        "MP2StreamParser::GetClipDurationInMsec not available before parsing");
    }
    else
    {
      //Calculate track durations and take max
      for(int i = 0; i < m_nstreams; i++)
      {
        if(m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO && bPlayAudio)
        {
          audioDuration = (uint64)(m_nLastAudioPTS - m_nRefAudioPTS);
        }
        else if (m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO && bPlayVideo)
        {
          videoDuration = (uint64)(m_nLastVideoPTS - m_nRefVideoPTS);
        }
      }
    }
    return m_nClipDuration = FILESOURCE_MAX(audioDuration,videoDuration);
  }
}
/*! ======================================================================
@brief    MP2StreamParser::GetFileFormat

@detail    Returns the format of the file

@param[in/out] fileFormat    VOB or M2TS
@return  MP2StreamStatus
@note      None
========================================================================== */
MP2StreamStatus MP2StreamParser::GetFileFormat(FileSourceFileFormat& fileFormat)
{
  if(m_bProgramStream)
  {
    fileFormat = FILE_SOURCE_MP2PS;
  }
  else
  {
    fileFormat = FILE_SOURCE_MP2TS;
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"MP2StreamParser::GetFileFormat %d",fileFormat);
  return MP2STREAM_SUCCESS;
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetCurrentSample

DESCRIPTION:
Returns current sample for the given trackId.

INPUT/OUTPUT PARAMETERS:
trackId - Track Id to fetch sample for.
dataBuffer - Buffer into which sample is filled.
nMaxBufSize - Max buffer size
nBytesRead - sample size filled into dataBuffer
frameTS - timestamp of the sample

RETURN VALUE:
MP2StreamStatus - MP2STREAM_SUCCESS if successful

SIDE EFFECTS:
  None.
=============================================================================*/
MP2StreamStatus MP2StreamParser::GetCurrentSample(uint32 trackId,uint8* dataBuffer,
                                                  uint32 nMaxBufSize, int32* nBytesRead,
                                                  float *frameTS, uint32 *pulIsSync)
{
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"MP2StreamParser::GetCurrentSample m_nCurrOffset %llu",m_nCurrOffset);
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;

  /* Check the output mode, get assembled PES packet unless
     FILE_SOURCE_MEDIA_OUTPUT_SINGLE_AUDIO_FRAME is set */

  if((m_hFrameOutputModeEnum == FILE_SOURCE_MEDIA_OUTPUT_SINGLE_AUDIO_FRAME) || (m_bProgramStream))
  {
    retError = GetSampleAtFrameBoundary(trackId,dataBuffer,nMaxBufSize,nBytesRead,frameTS);
  }
  else if(m_hFrameOutputModeEnum == FILE_SOURCE_MEDIA_OUTPUT_BYTES_STREAM)
  {
    retError = GetAssembledPESPacket(trackId, dataBuffer, nMaxBufSize,
                                     nBytesRead,frameTS, pulIsSync);
  }

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::GetCurrentSample returning nBytesRead %ld, m_nCurrOffset %llu",(*nBytesRead), m_nCurrOffset);
#endif
  return retError;
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetAssembledPESPacket

DESCRIPTION:
Returns assembled PES packet for the given trackId.

INPUT/OUTPUT PARAMETERS:
trackId - Track Id to fetch sample for.
dataBuffer - Buffer into which sample is filled.
nMaxBufSize - Max buffer size
nBytesRead - sample size filled into dataBuffer
frameTS - timestamp of the sample

RETURN VALUE:
MP2StreamStatus - MP2STREAM_SUCCESS if successful

SIDE EFFECTS:
  None.
=============================================================================*/
MP2StreamStatus MP2StreamParser::GetAssembledPESPacket(uint32  trackId,
                                                       uint8*  dataBuffer,
                                                       uint32  nMaxBufSize,
                                                       int32*  nBytesRead,
                                                       float*  frameTS,
                                                       uint32* pulIsSync)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;

  uint32 bytesRead = 0;
  uint8 newPayload = 0;
  bool foundStartOfFrame = false;
  uint32 pesLen = 0;
  *nBytesRead = 0;
  uint32 ulBytesLost = 0;
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"GetAssembledPESPacket m_nCurrOffset %llu",m_nCurrOffset);

  if (pulIsSync)
  {
    *pulIsSync = 0;
  }
  if(m_UnderrunBuffer.nDataLen)
  {
    ulBytesLost = m_nBytesLost;
    if(pulIsSync)
      *pulIsSync = m_UnderrunBuffer.nRandomAccess;
    m_UnderrunBuffer.nRandomAccess = 0;
  }
  while(m_nCurrOffset <= m_nFileSize)
  {
    if(isSameStream(&trackId,&newPayload))
    {
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"GetAssembledPESPacket m_nCurrOffset %llu",m_nCurrOffset);
      if(m_UnderrunBuffer.nDataLen)
      {
        bytesRead = restoreFromUnderrunBuffer(dataBuffer,&pesLen);
        *nBytesRead = 0;
        foundStartOfFrame = 1;
        *frameTS = (float)m_UnderrunBuffer.nFrameTime;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetAssembledPESPacket restoring bytesRead %lu",bytesRead);
        retError = MP2STREAM_SUCCESS;
      }

      /* Stop assembling frame if new payload starts. */
      if(foundStartOfFrame && newPayload)
        break;
      retError = parseTransportStreamPacket(trackId);
      if(retError == MP2STREAM_SUCCESS)
      {
        if(newPayload)
        {
          foundStartOfFrame = true;
          *frameTS = (float)m_currPESPkt.pts;
          if (pulIsSync)
          {
            *pulIsSync = m_currTSPkt.adaption_field.random_access_indicator;
            m_currTSPkt.adaption_field.random_access_indicator = 0;
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                         "GetAssembledPESPacket sync frame @ %llu",
                         (uint64)m_currPESPkt.pts);
          }
          pesLen = m_currPESPkt.packet_length;
        }
        else if(!foundStartOfFrame)
        {
          //Partial packet needs to be skipped.
          continue;
        }

        if(bytesRead + m_nBytesRead > nMaxBufSize)
        {
          m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"GetAssembledPESPacket MP2STREAM_INSUFFICIENT_MEMORY");
          break;
        }
        //Calculate any loss if present
        int counterJump = getContinuityCounterJump(m_currTSPkt.continuity_counter);
        ulBytesLost += (uint32)(counterJump * TS_PKT_SIZE);
        if (counterJump)
        {
          MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
            "GetAssembledPESPacket cnt jump: prev counter %d, cur counter %d, cur TS pkt offset %llu",
            m_nPrevCC, m_currTSPkt.continuity_counter, m_currTSPkt.noffset);
        }

        //Copy data into PES assembly buffer
        memcpy(dataBuffer+bytesRead,m_pDataBuffer,m_nBytesRead);

        m_nPrevCC = m_currTSPkt.continuity_counter;
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetAssembledPESPacket bytesRead %lu m_nBytesRead %lu m_nCurrOffset %llu",bytesRead,m_nBytesRead,m_nCurrOffset);
        bytesRead += m_nBytesRead;
        if(pesLen && (bytesRead == pesLen))
        {
          /* Stop assembling frame when pesLen found. */
          break;
        }
      }
      else
      {
        break;
      }
    }
    else
    {
      if(m_eParserState == MP2STREAM_READY)
      {
        updateOffsetToNextPacket(m_nCurrOffset,m_bIsBDMVFormat,true);
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetAssembledPESPacket m_nCurrOffset %llu",m_nCurrOffset);
        if(m_nCurrOffset >= m_nFileSize)
        {
          m_eParserState = MP2STREAM_EOF;
          break;
        }
      }
      // this condition is hit when a seek operation is performed
      // just when the playback is about to end(few ms before the end).
      else if (m_eParserState == MP2STREAM_EOF && (m_nCurrOffset < (m_nFileSize - TS_PKT_SIZE)) )
      {
        updateOffsetToNextPacket(m_nCurrOffset,m_bIsBDMVFormat,true);
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "End of file reached, but seek probaly reset the offset");
        m_eParserState = MP2STREAM_READY;
      }
      else if (m_eParserState == MP2STREAM_EOF && (m_nCurrOffset < (m_nFileSize - TS_PKT_SIZE)) )
      {
        updateOffsetToNextPacket(m_nCurrOffset,m_bIsBDMVFormat,true);
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
          "End of file reached, but seek probaly reset the offset");
        m_eParserState = MP2STREAM_READY;
      }
      else
        break;
    }
  }

  //! update class variable
  m_nBytesLost = ulBytesLost;
  if( (m_eParserState == MP2STREAM_DATA_UNDER_RUN) ||
      (m_eParserState == MP2STREAM_INSUFFICIENT_MEMORY))
  {
    MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_MEDIUM,
    "GetAssembledPESPacket under-run @TS pkt offset %llu, bytes lost %lu, Prev TS Pkt %d",
    m_nCurrOffset, m_nBytesLost, m_nPrevCC);

    retError = backupInUnderrunBuffer(dataBuffer,bytesRead,pesLen);
    //! Restore the sync sample flag in UnderRun Buffer
    if(pulIsSync)
    {
      m_UnderrunBuffer.nRandomAccess = *pulIsSync;
    }
    if(bytesRead)
    {
      m_UnderrunBuffer.nFrameTime = (uint64)m_currPESPkt.pts;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetAssembledPESPacket copying timestamp %llu",m_UnderrunBuffer.nFrameTime);
    }
  }
  else if(m_eParserState == MP2STREAM_EOF)
  {
    retError = MP2STREAM_EOF;
  }

  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
               "GetAssembledPESPacket returning bytesRead %lu *nBytesRead %ld",
               bytesRead,*nBytesRead);
  if(retError == MP2STREAM_SUCCESS)
  {
    //! If PES Packet length is provided, check whether all the data is read
    //! into output buffer or not. If all the data is not available, then
    //! indicate some bytes are lost.
    if((pesLen) && (pesLen != bytesRead) && (!m_nBytesLost))
    {
      m_nBytesLost = 1;
    }
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                 "GetAssembledPESPacket sample @ time %llu, byte lost %lu",
                 (uint64)*frameTS, m_nBytesLost);
    *nBytesRead = bytesRead;
    track_type tType;
    media_codec_type codecType;
    if((GetTrackType(trackId, &tType, &codecType) == MP2STREAM_SUCCESS) &&
       (codecType == AUDIO_CODEC_LPCM) &&
       (m_hHeaderOutputModeEnum == FILE_SOURCE_MEDIA_STRIP_AUDIO_HEADER))
    {
      if(*nBytesRead)
      {
        if(((uint32)*nBytesRead - M2TS_PCM_HDR_LEN) >= 0)
        {
          memmove(dataBuffer, dataBuffer + M2TS_PCM_HDR_LEN,
                ((uint32)*nBytesRead-M2TS_PCM_HDR_LEN));
          *nBytesRead -= M2TS_PCM_HDR_LEN;
        }
      }
    }
  }
  return retError;
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetSampleAtFrameBoundary

DESCRIPTION:
Returns current sample for the given trackId at frame boundary for AAC audio and H264 Video.

INPUT/OUTPUT PARAMETERS:
trackId - Track Id to fetch sample for.
dataBuffer - Buffer into which sample is filled.
nMaxBufSize - Max buffer size
nBytesRead - sample size filled into dataBuffer
frameTS - timestamp of the sample

RETURN VALUE:
MP2StreamStatus - MP2STREAM_SUCCESS if successful

SIDE EFFECTS:
  None.
=============================================================================*/
MP2StreamStatus MP2StreamParser::GetSampleAtFrameBoundary(uint32 trackId,uint8* dataBuffer,
                                                  uint32 nMaxBufSize, int32* pBytesRead,
                                                  float *frameTS)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,
      "MP2StreamParser::GetSampleAtFrameBoundary");
#endif
  uint32 nLocalBytesRead = 0;
  uint8 newPayload = 0;
#ifdef PLATFORM_LTK
  bool endOfFrame = false;
#endif
  bool bRet = false;
  uint32 frame_len = 0;
  float frame_time = 0;
  int index = 0;
  bool bTSFlag = false;
  float timestampFromPES = (float)GetPTSFromCurrentPESPacket();
  //Reset this for every frame
  m_nBytesLost = 0;
  bool bFoundStart = false;

  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;
  retError = GetTrackType(trackId,&ttype,&medtype);

  if( (pBytesRead && dataBuffer) && (retError == MP2STREAM_SUCCESS))
  {
    *pBytesRead = 0;

#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  if(m_bHttpStreaming)
  {
    uint64 availOffset = 0;
    boolean bEnd = false;
    MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
    (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
    m_availOffset = availOffset;
    if(m_nCurrOffset + sizeof(uint32) > availOffset)
    {
      if(!bEnd)
      {
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_MEDIUM,
          "MP2STREAM_DATA_UNDER_RUN m_nCurrOffset %llu availOffset %llu bEnd %d",
                   m_nCurrOffset,availOffset,bEnd);
        m_eParserState = MP2STREAM_DATA_UNDER_RUN;
      }
      else if(bEnd)
      {
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
        "MP2STREAM_EOF m_nCurrOffset %llu availOffset %llu bEnd %d",
                     m_nCurrOffset,availOffset,bEnd);
        m_eParserState = MP2STREAM_EOF;
      }
    }
  }
  #endif
    if((m_eParserState != MP2STREAM_DATA_UNDER_RUN)&&
       (m_eParserState != MP2STREAM_EOF))
    {
      while((m_nCurrOffset < m_nFileSize) )
      {
        *pBytesRead = 0;
        if(!readMpeg2StreamData (m_nCurrOffset,  sizeof(uint32),
                                 m_pDataBuffer, m_nDataBufferSize,
                                 (uint32) m_pUserData) )
        {
          retError = m_eParserState;
          break;
        }
        if(!memcmp(m_pDataBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
        {
          if(m_UnderrunBuffer.nDataLen)
          {
            memcpy(dataBuffer,m_UnderrunBuffer.pFrameBuf,m_UnderrunBuffer.nDataLen);
            nLocalBytesRead = m_UnderrunBuffer.nDataLen;
            m_UnderrunBuffer.nDataLen = 0;
            m_nBytesLost = m_UnderrunBuffer.bytesLost;
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                "MP2StreamParser::GetSampleAtFrameBoundary restored %lu bytes into sample buffer",
                nLocalBytesRead);
          }
          if(m_pPartialFrameData && (m_pPartialFrameData->haveFrameData))
          {
            retError = MakeAccessUnit(trackId, dataBuffer, nLocalBytesRead);
            nLocalBytesRead += m_pPartialFrameData->len;
            memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
          }
          /* Check if the next TS packet has payload of the same stream
             Parse the TS packet further only if either newPayload or m_bpartialPESTS flags are set to TRUE.
             This check is to avoid the parsing of partial frames which occur at the start of the playback.
             Once ES is parsed successfully, m_bpartialPESTS flag will be set to TRUE*/
          if(isSameStream(&trackId,&newPayload) && ( newPayload || m_bpartialPESTS))
          {
            //When there is no data collected or data collected and new payload not found
            //then go ahead with parsing
            if(( ((!nLocalBytesRead) || (!newPayload)) && (ttype == TRACK_TYPE_VIDEO) )
                  || (ttype == TRACK_TYPE_AUDIO) )
            {
              retError = parseTransportStreamPacket(trackId);

              /* We cannot completely rely on "newPayload" flag in TS header.
                 To make our logic more robust, after parsing of TS packet check whether this
                 "m_bpartialPESTS" flag set to true or not. If not, parse next TS packet rather
                 than using the data parsed in the current TS packet. */
              if(!m_bpartialPESTS)
                continue;
              //Make sure we have 1 frame worth of data. Calculate if its not
              //calculated earlier.
              if( (!frame_len) &&
                  (nLocalBytesRead > M2TS_AAC_ADTS_HDR_LEN) &&
                  (medtype == AUDIO_CODEC_AAC))
              {
                retError = LocateAudioFrameBoundary(dataBuffer, &frame_len,
                                                    &frame_time, &index,
                                                    nLocalBytesRead);
              }
              //Make sure that, minimum frame length worth of data is available in the buffer
             //"index" points to audio frame start.
              if( (frame_len && (nLocalBytesRead - index > frame_len) &&
                  (ttype == TRACK_TYPE_AUDIO) && (medtype == AUDIO_CODEC_AAC)) ||
                  (nLocalBytesRead && (m_currTSPkt.pyld_unit_start_indicator) && (ttype == TRACK_TYPE_VIDEO)) ||
                  (nLocalBytesRead && (m_currTSPkt.pyld_unit_start_indicator) &&
                   (ttype == TRACK_TYPE_AUDIO) && (medtype != AUDIO_CODEC_AAC)) )
              {
                //If new payload is starting, reset the offset
                //We will read when we come back for next frame
                m_nCurrOffset -= TS_PKT_SIZE;
              }
              else if((retError == MP2STREAM_SUCCESS))
              {
                if(nLocalBytesRead + m_nBytesRead > nMaxBufSize)
                {
                  m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
                  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                      "GetSampleAtFrameBoundary MP2STREAM_INSUFFICIENT_MEMORY");
                  break;
                }
                retError = MakeAccessUnit(trackId, dataBuffer, nLocalBytesRead);
                if(retError == MP2STREAM_SUCCESS)
                  nLocalBytesRead += m_nBytesRead;
              }
              else
              {
                if(MP2STREAM_DEFAULT_ERROR == retError)
                {
                  //If we did not find sync word in the frame, we should discard it.
                  //Sometimes in live streaming, 1st audio packet can be a partial frame.
                  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                      "LocateAudioFrameBoundary no audio sync word: index %d, nLocalBytesRead %lu",
                               index, nLocalBytesRead);
                  memcpy(dataBuffer, dataBuffer + index, nLocalBytesRead - index);
                  nLocalBytesRead = nLocalBytesRead - index;
                  index = 0;
                  retError = MP2STREAM_SUCCESS;
                }
                //If the error reported is something else, break the while loop.
                else
                break;
              }
            }
          }
          else
          {
            if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
            {
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                  "GetSampleAtFrameBoundary isSameStream MP2STREAM_DATA_UNDER_RUN");
              break;
            }
            //Skip this packet
            m_nCurrOffset += 188;

            /* If single byte also not read into output buffer, then no need
               to proceed further. */
            if(!nLocalBytesRead)
              continue;
          }
          if( (!frame_len) && (nLocalBytesRead > M2TS_AAC_ADTS_HDR_LEN) &&
              (ttype == TRACK_TYPE_AUDIO) && (medtype == AUDIO_CODEC_AAC) )
          {
            retError = LocateAudioFrameBoundary(dataBuffer, &frame_len, &frame_time, &index, nLocalBytesRead);
            if(retError != MP2STREAM_SUCCESS)
            {
              //If we did not find sync word in the frame, we should discard the data till we
              //did not find the frame sync word. Remaining data should be in the buffer.
              //Sometimes in live streaming, 1st audio packet can be a partial frame.
              MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary no audio sync word: index %d, nLocalBytesRead %lu",
                           index, nLocalBytesRead);
              memcpy(dataBuffer, dataBuffer + index, nLocalBytesRead - index);
              nLocalBytesRead = nLocalBytesRead - index;
              index = 0;
              retError = MP2STREAM_SUCCESS;
            }
          }
          /* We need to  make sure that frameLen amount of data is available in
             the output buffer to avoid frame sync word emulation situation. */
          if(((frame_len) && (nLocalBytesRead - index >= frame_len) &&
              (medtype == AUDIO_CODEC_AAC) ) ||
             ((isSameStream(&trackId,&newPayload)) &&
              ((ttype == TRACK_TYPE_VIDEO) ||
               ((ttype == TRACK_TYPE_AUDIO) && (medtype != AUDIO_CODEC_AAC)))) )

          {
            if( ((newPayload) && (nLocalBytesRead) && (ttype == TRACK_TYPE_VIDEO)) ||
                ((ttype == TRACK_TYPE_AUDIO) && (medtype == AUDIO_CODEC_AAC)) ||
                ((newPayload) && (nLocalBytesRead) && (ttype == TRACK_TYPE_AUDIO) && (medtype != AUDIO_CODEC_AAC)))
            {
              if( (!bTSFlag) && ( (ttype == TRACK_TYPE_VIDEO)||
                                  ((ttype == TRACK_TYPE_AUDIO) && (medtype != AUDIO_CODEC_AAC)) ))
              {
                 bTSFlag = true;
                *frameTS = (float)m_currPESPkt.pts;
              }
              bRet = isAssembledAtFrameBoundary(trackId, dataBuffer, &nLocalBytesRead, nMaxBufSize);
              if(bRet)
              {
                if((ttype == TRACK_TYPE_AUDIO) && (medtype == AUDIO_CODEC_AAC))
                {
                  //Do only in case of audio aac
                  *frameTS = getADTSTimestamp(timestampFromPES,frame_time);
                }
                *pBytesRead = nLocalBytesRead;
#ifdef PLATFORM_LTK
                endOfFrame = true;
#endif
                retError = MP2STREAM_SUCCESS;
                break;
              }
              else
              {
#ifdef MPEG2_PARSER_DEBUG
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                    "GetSampleAtFrameBoundary isAssembledAtFrameBoundary returned FALSE");
#endif
                if(m_eParserState == MP2STREAM_DATA_UNDER_RUN || m_eParserState == MP2STREAM_INSUFFICIENT_MEMORY)
                {
                  *pBytesRead = nLocalBytesRead;
                  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                      "GetSampleAtFrameBoundary state is %d", m_eParserState);
                  break;
                }
              }
            }
          }
          else
          {
            if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
            {
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                  "GetSampleAtFrameBoundary isSameStream MP2STREAM_DATA_UNDER_RUN");
              break;
            }
          }
        }
        else
        {
          uint32 codeVal = getBytesValue(sizeof(uint32),m_pDataBuffer);
          uint32 valCode = 0;
          uint64 startOffset = m_nCurrOffset;
          if(codeVal == MP2_PACK_START_CODE)
          {
            retError = parsePackHeader(m_nCurrOffset,true,trackId,dataBuffer+(nLocalBytesRead),nMaxBufSize,pBytesRead);
          }
          else if( isPESPacket(m_pDataBuffer,&valCode))
          {
            retError = parsePESPacket(m_nCurrOffset,valCode,trackId,dataBuffer+(nLocalBytesRead),nMaxBufSize,pBytesRead);
          }
          else if(MP2_SYS_HDR_START_CODE == codeVal)
          {
            m_nCurrOffset += sizeof(MP2_SYS_HDR_START_CODE);
            retError = parseSystemTargetHeader(trackId, m_nCurrOffset);
          }
          else
          {
            m_nCurrOffset++;
          }
          if ((m_currPESPkt.trackid != trackId) || (0 == *pBytesRead))
          {
             continue;
          }

          if(m_currPESPkt.pts_dts_flag)
          {
            if(bFoundStart)
            {
              m_nCurrOffset = startOffset;
              *pBytesRead = nLocalBytesRead;
              break;
            }
            bFoundStart = true;
            *frameTS = (float)m_currPESPkt.pts;
          }
          nLocalBytesRead += *pBytesRead;
        }
        if( (nLocalBytesRead) && (!m_bProgramStream) &&
            ( (m_currPESPkt.packet_length) &&
              (nLocalBytesRead >= m_currPESPkt.packet_length) &&
              (m_currTSPkt.PID == m_nAudioPIDSelected)) &&
              (m_currPESPkt.trackid == trackId) )
        {
          *frameTS = (float)m_currPESPkt.pts;
          *pBytesRead = nLocalBytesRead;
          break;
        }
      }
      if(m_nCurrOffset >= m_nFileSize)
      {
        m_eParserState = MP2STREAM_EOF;
      }
    }
    if( (m_eParserState == MP2STREAM_DATA_UNDER_RUN) ||
        (m_eParserState == MP2STREAM_INSUFFICIENT_MEMORY) )
    {
      if(nLocalBytesRead)
      {
        if(m_UnderrunBuffer.nBufSize < nLocalBytesRead)
        {
          if(m_UnderrunBuffer.pFrameBuf)
            MM_Free(m_UnderrunBuffer.pFrameBuf);
          m_UnderrunBuffer.pFrameBuf = (uint8*)MM_Malloc(nLocalBytesRead + 1);
          if(!m_UnderrunBuffer.pFrameBuf)
          {
            return MP2STREAM_INSUFFICIENT_MEMORY;
          }
          m_UnderrunBuffer.nBufSize = nLocalBytesRead + 1;
          m_UnderrunBuffer.nDataLen = 0;
        }
        memcpy(m_UnderrunBuffer.pFrameBuf,dataBuffer,nLocalBytesRead);
        m_UnderrunBuffer.nDataLen = nLocalBytesRead;
        m_UnderrunBuffer.bytesLost = m_nBytesLost;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "MP2StreamParser::GetSampleAtFrameBoundary copied %lu bytes into underrun buffer",
            nLocalBytesRead);
      }
      retError = m_eParserState;
      m_eParserState = MP2STREAM_READY;
    }
    if(m_eParserState == MP2STREAM_EOF)
    {
      retError = m_eParserState;
    }
#ifdef MPEG2_PARSER_DEBUG
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
        "MP2StreamParser::GetSampleAtFrameBoundary returning pBytesRead %ld, m_nCurrOffset %llu",
        (*pBytesRead), m_nCurrOffset);
#endif
  }
  return retError;
}
/*! ======================================================================
@brief  Starts Parsing the MP2 Transport stream.

@detail    This function starts parsing the MP2 transport stream.
        Upon successful parsing, user can retrieve total number of streams and
        stream specific information.

@param[in] N/A
@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      StartParsing needs to be called before retrieving any elementary stream specific information.
========================================================================== */
MP2StreamStatus MP2StreamParser::StartParsing(void)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  int counter = 0;
  uint32 bytesRead = 0;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"StartParsing");
#endif

  if(m_eParserState == MP2STREAM_IDLE)
  {
    //Read 8 bytes to decide whether we need to
    //parse program stream/transport stream/M2TS stream
    uint8 tempBuff[2* sizeof(uint32)];
    if(readMpeg2StreamData (m_nCurrOffset, 2*sizeof(uint32), tempBuff, 2* sizeof(uint32), (uint32) m_pUserData) )
    {
      uint32 codeval = getBytesValue(sizeof(MP2_PACK_START_CODE),tempBuff);
      //Determine if we need to parse program stream or transport stream.
      if(memcmp(tempBuff,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) == 0)
      {
        m_nCurrOffset = 0;
        m_eParserState = MP2STREAM_INIT;
        m_pDataBuffer = (uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);
        retError = MP2STREAM_SUCCESS;
        if(m_pDataBuffer)
        {
          m_nDataBufferSize = MPEG2_FILE_READ_CHUNK_SIZE;
        }
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing TS_PKT_SYNC(0x47) found..");
     }
     else if(codeval == MP2_PACK_START_CODE)
     {
       m_eParserState = MP2STREAM_INIT;
       m_bProgramStream = true;
       m_bGetLastPTS =true;
       m_pDataBuffer = (uint8*)MM_Malloc(PS_DATA_BUFFER_SIZE);
       retError = MP2STREAM_SUCCESS;
       if(m_pDataBuffer)
       {
         m_nDataBufferSize = PS_DATA_BUFFER_SIZE;
       }
       MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing MP2_PACK_START_CODE found..");
     }
     else if(memcmp(tempBuff+M2TS_PKT_EXTRA_HDR_BYTES,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) == 0)
     {
       m_nCurrOffset = M2TS_PKT_EXTRA_HDR_BYTES;
       m_bIsBDMVFormat = true;
       m_eParserState = MP2STREAM_INIT;
       m_pDataBuffer = (uint8*)MM_Malloc(MPEG2_M2TS_PKT_BYTES * 200);
       if(m_pDataBuffer)
       {
         m_nDataBufferSize = MPEG2_M2TS_PKT_BYTES * 200;
       }
       retError = MP2STREAM_SUCCESS;
       MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing M2TS_PKT_SYNC(0x47) found..");
     }
     else
     {
       bContinue = false;
       retError = MP2STREAM_CORRUPT_DATA;
     }
    }
    else
    {
      bContinue = false;
      retError = m_eParserState;
    }
    if(!bContinue)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"StartParsing detected Corrupted Stream!!!");
    }

    if(bContinue && m_bProgramStream && m_pDataBuffer)
    {
      if(MP2STREAM_SUCCESS != parseProgStream())
      {
        retError = MP2STREAM_PARSE_ERROR;
        bContinue = false;
      }
    }
  }
  if((m_eParserState == MP2STREAM_INIT) || (m_eParserState == MP2STREAM_DATA_UNDER_RUN) )
  {
    while(bContinue                     &&
         (m_nCurrOffset < m_nFileSize)  &&
         (m_pDataBuffer)                &&
         (!m_bProgramStream))
    {
      if(!readMpeg2StreamData (m_nCurrOffset,  TS_PKT_SIZE,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
      {
        retError = m_eParserState;
        bContinue = false;
        break;
      }
      if( memcmp(m_pDataBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"StartParsing TS_PKT_SYNC(0x47) not found");
        retError = MP2STREAM_CORRUPT_DATA;
        bContinue = false;
      }
      if(bContinue)
      {
        if(m_bInitialParsingPending)
        {
          if( isInitialParsingDone())
          {
            bContinue = false;
            m_bInitialParsingPending = false;
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"Done collecting stream INFO");
            retError = MP2STREAM_SUCCESS;
          }
        }
        if((m_bGetLastPTS) && (m_bInitialParsingPending))
        {
          counter++;
#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
          if(m_bHttpStreaming && m_nCurrOffset + TS_PKT_SIZE > m_availOffset)
          {
            uint64 availOffset = 0;
            boolean bEnd = false;
            MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
            (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
            m_availOffset = availOffset;
            if(bEnd)
            {
              m_nFileSize = availOffset;
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing updating filesize to %llu ",m_nFileSize);
            }
          }
#endif
          /* m_bGetLastPTS is set only after m_bEOFFound is TRUE. We would have updated
             m_nFileSize, so it is safe to use it here. */
          if(m_bIsBDMVFormat)
          {
            m_nFileSize = (m_nFileSize/MPEG2_M2TS_PKT_BYTES) * MPEG2_M2TS_PKT_BYTES;
            m_nCurrOffset = m_nFileSize - (counter * MPEG2_M2TS_PKT_BYTES) + M2TS_PKT_EXTRA_HDR_BYTES;
          }
          else
          {
            m_nFileSize = (m_nFileSize/TS_PKT_SIZE) * TS_PKT_SIZE;
            m_nCurrOffset = m_nFileSize - (counter * TS_PKT_SIZE);
          }
        }
        retError = parseTransportStreamPacket();
        if(MP2STREAM_SUCCESS != retError)
        {
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing parseTransportStreamPacket returned %d ",retError);
          bContinue = false;
        }
        m_nInitialTSPktParsed++;
        #ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"StartParsing nTSPktCounter %d ",m_nInitialTSPktParsed);
        #endif
        //Currently we will make only H.264 video access units(look for codec info in these access units)
        if( bContinue && (m_bInitialParsingPending) && (!m_bGetLastPTS) )
        {
          uint8 newPayload = 0;
          uint32 frameLen = 0;
          track_type ttype = TRACK_TYPE_UNKNOWN;
          media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;

          if(m_pVideoStreamIds)
          {
            uint32 trackId = (uint32)m_pVideoStreamIds[0];
            if( (isSameStream(&trackId,&newPayload)) &&
                (MP2STREAM_SUCCESS == GetTrackType(trackId,&ttype,&medtype)) &&
                ((TRACK_TYPE_VIDEO == ttype) && (medtype == VIDEO_CODEC_H264)) )
            {
              if (NULL == m_pH264PESPacketBuf)
              {
                m_pH264PESPacketBuf = (uint8*)MM_Malloc(PS_DATA_BUFFER_SIZE);
              }
              if(!m_pH264PESPacketBuf)
              {
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                            "Insufficient Memory to store PES Packet");
                retError  = MP2STREAM_INSUFFICIENT_MEMORY;
                bContinue = false;
                break;
              }
              if(m_UnderrunBuffer.nDataLen)
              {
                //Restore if we have anything in underrun buffer
                memcpy(m_pH264PESPacketBuf, m_UnderrunBuffer.pFrameBuf,
                       m_UnderrunBuffer.nDataLen);
                bytesRead = m_UnderrunBuffer.nDataLen;
                m_UnderrunBuffer.nDataLen = 0;
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                            "MP2StreamParser::StartParsing restored %lu bytes",
                            bytesRead);
              }
              if(bytesRead + m_nBytesRead > PS_DATA_BUFFER_SIZE)
              {
                m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                            "StartParsing MP2STREAM_INSUFFICIENT_MEMORY");
                break;
              }
              retError = MakeAccessUnit(trackId,m_pH264PESPacketBuf,bytesRead);
              if((retError == MP2STREAM_SUCCESS))
              {
                bytesRead += m_nBytesRead;
              }
              else
              {
                m_nCurrOffset = m_nCurrSampleOffset;
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing MakeAccessUnit returning error %d",retError);
              }
              if( (newPayload) &&  (bytesRead) )
              {
                frameLen = (int32)bytesRead;
                bytesRead = 0;
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Access unit size is %lu",frameLen);

                if((!m_pAvcCodecInfo) || (!m_pAvcCodecInfo->isValid))
                {
                  bool bRet = GetAVCCodecInfo(&frameLen, m_pH264PESPacketBuf);
                  if(bRet)
                  {
                    uint16 height = 0;
                    uint16 width = 0;
                    H264HeaderParser headerParser;
                    start_code_type startCodeType = START_CODE_DEFAULT;
                    uint32 offset = 0;
                    bool bret = isFrameStartWithStartCode(&offset,trackId,m_pAvcCodecInfo->codecInfoBuf,m_pAvcCodecInfo->size,&startCodeType);
                    if(bret)
                    {
                      headerParser.parseParameterSet((const unsigned char *)m_pAvcCodecInfo->codecInfoBuf+offset+startCodeType,m_pAvcCodecInfo->size);
                      headerParser.GetVideoDimensions(height,width);
                      for(int j = 0; j< m_nstreams; j++)
                      {
                        if ( (m_pStream_Info) && (m_pStream_Info + j ) && (m_pStream_Info[j].stream_id == trackId) )
                        {
                          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing found height,width as %d,%d",height,width);
                          m_pStream_Info[j].video_stream_info.Height = height;
                          m_pStream_Info[j].video_stream_info.Width = width;
                        }
                      }
                    }
                  }
                  else
                  {
  #ifdef MPEG2_PARSER_DEBUG
                    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"StartParsing GetAVCCodecInfo RETURNED FALSE");
  #endif
                  }
                }
              }
            }
            else
            {
              //backup buffer at this point
              if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
              {
                if(bytesRead)
                {
                  if( (m_UnderrunBuffer.nBufSize < bytesRead) || (m_UnderrunBuffer.pFrameBuf == NULL) )
                  {
                    if(m_UnderrunBuffer.pFrameBuf)
                      MM_Free(m_UnderrunBuffer.pFrameBuf);
                    m_UnderrunBuffer.pFrameBuf = (uint8*)MM_Malloc(bytesRead + 1);
                    if(!m_UnderrunBuffer.pFrameBuf)
                    {
                      retError = MP2STREAM_INSUFFICIENT_MEMORY;
                      break;
                    }
                    m_UnderrunBuffer.nBufSize = bytesRead + 1;
                    m_UnderrunBuffer.nDataLen = 0;
                  }
                  memcpy(m_UnderrunBuffer.pFrameBuf, m_pH264PESPacketBuf,
                         bytesRead);
                  m_UnderrunBuffer.nDataLen = bytesRead;
                  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                          "StartParsing copied %lu bytes into underrun buffer",
                          bytesRead);
                }
                retError = m_eParserState;
                m_eParserState = MP2STREAM_INIT;
                break;
              }
            }
          }
          else
          {
  #ifdef MPEG2_PARSER_DEBUG
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing m_pVideoStreamIds is NULL");
  #endif
          }
        }
      }
    }
    if(retError == MP2STREAM_SUCCESS || retError == MP2STREAM_EOF)
    {
      m_eParserState = MP2STREAM_READY;
      memset(&m_currPESPkt,0,sizeof(PESPacket));
      memset(&m_currTSPkt,0,sizeof(MP2TransportStreamPkt));
      /*
      *Free up all the DSI/PCI pointers as parsing will start from the
      *begining when playback begins.Since we store end offset for each of these
      *structures, we will end up adding sizes for first few PCI/DSI again which
      *will mess up the reposition logic.
      */
      if(m_pCurrVOBUDSIPkt)
      {
        MM_Free(m_pCurrVOBUDSIPkt);
      }
      if(m_pFirstVOBUDSIPkt)
      {
        MM_Free(m_pFirstVOBUDSIPkt);
      }
      if(m_pCurrVOBUPCIPkt)
      {
        MM_Free(m_pCurrVOBUPCIPkt);
      }
      if(m_pFirstVOBUPCIPkt)
      {
        MM_Free(m_pFirstVOBUPCIPkt);
      }
      m_pCurrVOBUDSIPkt = m_pFirstVOBUDSIPkt = NULL;
      m_pCurrVOBUPCIPkt = m_pFirstVOBUPCIPkt = NULL;
      retError = MP2STREAM_SUCCESS;
    }
    if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
    {
      m_eParserState = MP2STREAM_IDLE;
    }
  }
  if(m_eParserState == MP2STREAM_READY)
  {
    m_nPrevCC = -1;
    retError = MP2STREAM_SUCCESS;
    /* Reset the Sample offset variable Once Parser is moved to ready state*/
    m_nCurrSampleOffset = 0;

    /* Reset m_bpartialPESTS flag to false, before Parser is moved to ready state*/
    m_bpartialPESTS = false;
    uint32 totalStreams = m_nVideoStreams + m_nAudioStreams;
    for(uint32 i = 0; i < totalStreams; i++)
    {
      stream_info *tempStreamInfo = &m_pStream_Info[i];
      if(tempStreamInfo &&
         (TRACK_TYPE_VIDEO == tempStreamInfo->stream_media_type) &&
         (VIDEO_CODEC_H264 == tempStreamInfo->video_stream_info.Video_Codec) &&
         (0 == tempStreamInfo->video_stream_info.Width ||
          0 == tempStreamInfo->video_stream_info.Height) && m_bLocateCodecHdr)
      {
        tempStreamInfo->stream_media_type = TRACK_TYPE_UNKNOWN;
        tempStreamInfo->video_stream_info.Video_Codec = UNKNOWN_AUDIO_VIDEO_CODEC;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"SPS/PPS not available for Video H264 track");
      }
    }
  }
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"StartParsing RETURNING %d", retError);
#endif
  return retError;
}

/*! ======================================================================
@brief  Parses program stream

@detail    Starts parsing program stream for demultiplexing elementary streams

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parseProgStream()
{
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseProgStream");
#endif
  bool bContinue       = true;
  uint8* pDataBuf      = NULL;
  uint32 ulDataBufSize = 0;
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;

#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  if(m_bHttpStreaming && (m_nCurrOffset + TS_PKT_SIZE) >= m_availOffset)
  {
    uint64 availOffset = 0;
    boolean bEnd = false;
    MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
    (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
    m_availOffset = availOffset;
    if(m_nCurrOffset + sizeof(uint32) > availOffset)
    {
      m_eParserState = MP2STREAM_DATA_UNDER_RUN;
      bContinue = false;
    }
  }
#endif
  int32 nBytesRead = 0;
  uint32 valCode = 0;
  uint32 ulH264BufIndex = 0;
  bool   bisPESPacket = false;

  while(bContinue && (m_nCurrOffset < m_nFileSize) && (m_bInitialParsingPending) )
  {
    bisPESPacket = false;
    if((m_bInitialParsingPending) &&
       (m_currPackHeader.sys_header &&
       m_currPackHeader.sys_header->audio_bound == m_nAudioStreams &&
       m_currPackHeader.sys_header->video_bound == m_nVideoStreams))
    {
      if( isInitialParsingDone())
      {
        bContinue = false;
        m_bInitialParsingPending = false;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"Done collecting stream INFO");
        retError = MP2STREAM_SUCCESS;
        break;
      }
    }
    if(!readMpeg2StreamData (m_nCurrOffset,  sizeof(uint32),
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
    {
      retError = m_eParserState;
      bContinue = false;
      break;
    }
    uint32 ulCodeVal = getBytesValue(sizeof(uint32),m_pDataBuffer);
    if(MP2_PACK_START_CODE == ulCodeVal)
    {
      retError = parsePackHeader(m_nCurrOffset, true, 0xFF, pDataBuf,
                                 ulDataBufSize, &nBytesRead);
    }
    else if(MP2_SYS_HDR_START_CODE == ulCodeVal)
    {
      m_nCurrOffset += sizeof(MP2_SYS_HDR_START_CODE);
      retError = parseSystemTargetHeader(0xFF, m_nCurrOffset);
    }
    else if(isPESPacket(m_pDataBuffer,&valCode))
    {
      retError = parsePESPacket(m_nCurrOffset, valCode, 0xFF, pDataBuf,
                                ulDataBufSize, &nBytesRead);
      bisPESPacket = true;
    }
    else
    {
      m_nCurrOffset++;
    }
    if( (bContinue) && (m_bInitialParsingPending) && (nBytesRead))
    {
      uint8 newPayload = 0;
      uint32 frameLen = 0;
      track_type ttype = TRACK_TYPE_UNKNOWN;
      media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;

      if(m_pVideoStreamIds)
      {
        uint32 trackId = (uint32)m_pVideoStreamIds[0];
        stream_info* pStreamInfo = NULL;
        for (uint32 nCount = 0; nCount < m_nstreams; nCount++)
        {
          if ((m_pStream_Info) &&
              (m_pStream_Info + nCount) &&
              (trackId == m_pStream_Info[nCount].stream_id))
          {
            pStreamInfo = &m_pStream_Info[nCount];
            break;
          }
        }

        if(
           (MP2STREAM_SUCCESS == GetTrackType(trackId,&ttype,&medtype)) &&
           ((TRACK_TYPE_VIDEO == ttype) && (medtype == VIDEO_CODEC_H264) &&
            (pStreamInfo) && (pStreamInfo->bParsed == false)) )
        {
          if (NULL == m_pH264PESPacketBuf)
          {
            m_pH264PESPacketBuf = (uint8*)MM_Malloc(PS_DATA_BUFFER_SIZE);
          }
          if(!m_pH264PESPacketBuf)
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
              "Insufficient Memory to store PES Packet");
            retError  = MP2STREAM_INSUFFICIENT_MEMORY;
            bContinue = false;
            break;
          }
          if(ulH264BufIndex + nBytesRead > PS_DATA_BUFFER_SIZE)
          {
            m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
              "StartParsing MP2STREAM_INSUFFICIENT_MEMORY");
            break;
          }
          memcpy(m_pH264PESPacketBuf + ulH264BufIndex, m_pDataBuffer,
                 nBytesRead);
          ulH264BufIndex += nBytesRead;
          if( (m_currPESPkt.pts_dts_flag) &&  (ulH264BufIndex) )
          {
            frameLen = (int32)ulH264BufIndex;
            ulH264BufIndex = 0;
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                "Access unit size is %lu",frameLen);

            if((!m_pAvcCodecInfo) || (!m_pAvcCodecInfo->isValid))
            {
              bool bRet = GetAVCCodecInfo(&frameLen, m_pH264PESPacketBuf);
              if(bRet)
              {
                uint16 height = 0;
                uint16 width = 0;
                H264HeaderParser headerParser;
                start_code_type startCodeType = START_CODE_DEFAULT;
                uint32 offset = 0;
                bool bret = isFrameStartWithStartCode(&offset,trackId,
                    m_pAvcCodecInfo->codecInfoBuf,
                    m_pAvcCodecInfo->size,&startCodeType);
                if(bret)
                {
                  headerParser.parseParameterSet(
                      (const unsigned char *)m_pAvcCodecInfo->codecInfoBuf
                      +offset+startCodeType,m_pAvcCodecInfo->size);
                  headerParser.GetVideoDimensions(height,width);
                  for(int j = 0; j< m_nstreams; j++)
                  {
                    if ( (m_pStream_Info + j ) &&
                        (m_pStream_Info[j].stream_id == trackId) )
                    {
                      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                          "StartParsing found height,width as %d,%d",height,width);
                      m_pStream_Info[j].video_stream_info.Height = height;
                      m_pStream_Info[j].video_stream_info.Width = width;
                      m_pStream_Info[j].bParsed = true;
                      break;
                    }
                  }
                }
              }
              else
              {
#ifdef MPEG2_PARSER_DEBUG
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
                    "StartParsing GetAVCCodecInfo RETURNED FALSE");
#endif
              }
            }
          }
        }
      }
      else
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
            "StartParsing m_pVideoStreamIds is NULL");
#endif
      }
    }
  }
  if(!m_bInitialParsingPending)
  {
    m_nCurrOffset = 0;
  }
  if (pDataBuf)
  {
    MM_Free(pDataBuf);
  }

  return retError;
}


/*! ======================================================================
@brief  Inspects Data content to determine if it's a PES packet start code

@detail    Inspects Data content to determine if it's a PES packet start code

@param[in] N/A

@return    True if Data represents PES packet otherwise returns false.
@note      None.
========================================================================== */
bool MP2StreamParser::isPESPacket(uint8* Data,uint32* val)
{
  bool bRet = false;
  if(Data && val)
  {
    //read 4 bytes value including PES packet start code
    *val = getBytesValue(sizeof(uint32),Data);
    uint32 startCode = getBytesValue(3*sizeof(uint8),Data);
    if(startCode == PES_PKT_START_CODE)
    {
      //mask off PES start code (first 24 bits)and get just the stream id
      *val = (*val) & 0x000000FF;

      if( (*val ==  PROG_STREAM_MAP_ID)                                               ||
          (*val == PRIVATE_STREAM1_ID)                                                ||
          (*val == PADDING_STREAM_ID)                                                 ||
          (*val == PRIVATE_STREAM2_ID)                                                ||
          ( (*val >= AUDIO_STREAM_ID_START) && (*val <= AUDIO_STREAM_ID_END) )        ||
          ( (*val >= VIDEO_STREAM_ID_START) && (*val <= VIDEO_STREAM_ID_END) )        ||
          ( (*val >= RES_DATA_STREAM_START_ID) && (*val <= RES_DATA_STREAM_END_ID ) ) ||
          (*val == ECM_STREAM_ID)                                                     ||
          (*val == EMM_STREAM_ID)                                                     ||
          (*val == DSMCC_STREAM_ID)                                                   ||
          (*val == ISO_OEC_13522_STREAM_ID)                                           ||
          (*val == ANCILLARY_STREAM_ID )                                              ||
          (*val == SL_PACKETIZED_STREAM_ID)                                           ||
          (*val == FLEX_MUX_STREAM_ID )                                               ||
          (*val == H222_TYPE_A_STREAM_ID)                                             ||
          (*val == H222_TYPE_B_STREAM_ID)                                             ||
          (*val == H222_TYPE_C_STREAM_ID)                                             ||
          (*val == H222_TYPE_D_STREAM_ID)                                             ||
          (*val == H222_TYPE_E_STREAM_ID)                                             ||
          (*val == PROG_STREAM_DIRECTORY_ID)   )
      {
        bRet = true;
      }
    }
  }
  return bRet;
}
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
MP2StreamStatus MP2StreamParser::isKeyFrame(media_codec_type eMediaType, bool *bFoundFrameType)
{
  MP2StreamStatus eRetError=MP2STREAM_DEFAULT_ERROR;
  uint8 ucPicType = 0;
  if(eMediaType == VIDEO_CODEC_H264)
  {
    *bFoundFrameType = findH264NALTypeForFrame(&ucPicType);
    if( (*bFoundFrameType) && (ucPicType == NAL_UNIT_TYPE_IDR) )
    {
      eRetError = MP2STREAM_SUCCESS;
    }
  }
  else if(eMediaType == VIDEO_CODEC_MPEG2)
  {
    *bFoundFrameType = findPicCodingTypeForFrame(&ucPicType);
    if( (*bFoundFrameType) && (ucPicType == MPEG2_I_FRAME_TYPE) )
    {
      eRetError = MP2STREAM_SUCCESS;
    }
  }
  else if(eMediaType == VIDEO_CODEC_VC1)
  {
    *bFoundFrameType = findVC1FrameStartCode(&ucPicType);
    if( (*bFoundFrameType) && (ucPicType == VC1_I_FRAME_TYPE) )
    {
      eRetError = MP2STREAM_SUCCESS;
    }
  }
  return eRetError;
}
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
MP2StreamStatus MP2StreamParser::TSSeekForward(uint32 ulTrackId,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bCanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip,
                        track_type eTrackType,
                        media_codec_type eMediaType)
{
  MP2StreamStatus eRetError = MP2STREAM_SUCCESS;
  MP2StreamStatus eRetStatus = MP2STREAM_DEFAULT_ERROR;
  uint64 prevOffset = m_nCurrOffset;
  uint8 newPayload = 0;
  bool foundFrameType = false;
  bool bSeekSuccess=false;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "MP2StreamParser::Forward Seek called on Transport Stream");
  // Seek in Forward Direction
  while((m_nCurrOffset < m_nFileSize))
  {
    if(isSameStream(&ulTrackId, &newPayload))
    {
      if(newPayload || (!foundFrameType))
      {
        eRetStatus = parseTransportStreamPacket(ulTrackId);
        if(eRetStatus == MP2STREAM_SUCCESS)
        {
          if(newPayload)
          {
            sample_info->noffset = m_currTSPkt.noffset;
            foundFrameType = false;
          }
          sample_info->ntime = (float)m_currPESPkt.pts;
          if(m_currPESPkt.pts >= ullReposTime)
          {
            //We are adding this sleep so that we don't continuously
            //loop for too long when IDR frameis far away from the
            //seek point.
            MM_Timer_Sleep(5);
            if(TRACK_TYPE_VIDEO == eTrackType)
            {
              eRetError = isKeyFrame(eMediaType, &foundFrameType);
              if ((MP2STREAM_SUCCESS == eRetError) && (foundFrameType))
              {
                memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
                bSeekSuccess=true;
                break;
              }
            }
#if 0
            // This logic is keeping lot of strress on TS Parser to seek to
            // closest entry for audio track. There are some clips which does
            // not have valid data for large amount of time (eg silence in b/w)
            // Parser is kind of struck in loop in such cases (CR#639568).
            // This logic is added to seek to nearest to video timestamp to
            // avoid AV sync problems. But now decided to remove this.
            // Keeping it in case required if required in future.
            else if(TRACK_TYPE_AUDIO == eTrackType)
            {
              // Check if difference in time stamps is greater than
              //SEEK_TIMESTAMP_GAP_TOLERANCE_MS
              //If yes then issue a seek in appropriate direction
              //If no then return as seek success.
              if(((double)ullReposTime - m_currPESPkt.pts) >
                  SEEK_TIMESTAMP_GAP_TOLERANCE_MS)
              {
                SeekInTransportStream(ulTrackId,ullReposTime,
                    (uint64)m_currPESPkt.pts,sample_info,bForward,
                    bCanSyncToNonKeyFrame,nSyncFramesToSkip);
              }
              else if((m_currPESPkt.pts -(double)ullReposTime) >
                  SEEK_TIMESTAMP_GAP_TOLERANCE_MS)
              {
                SeekInTransportStream(ulTrackId,ullReposTime,
                    (uint64)m_currPESPkt.pts,sample_info,!bForward,
                    bCanSyncToNonKeyFrame,nSyncFramesToSkip);
              }
              else
              {
                bSeekSuccess=true;
                eRetError = MP2STREAM_SUCCESS;
                break;
              }
            }
#endif
            else
            {
              bSeekSuccess = true;
              eRetError    = MP2STREAM_SUCCESS;
              break;
            }
          }
        }
        else
        {
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,
              "MP2StreamParser::Seek returning status %d", eRetStatus);
          return eRetStatus;
        }
      }
      else
      {
        updateOffsetToNextPacket(m_nCurrOffset,m_bIsBDMVFormat,bForward);
      }
    }
    else
    {
      if( (m_eParserState == MP2STREAM_DATA_UNDER_RUN) ||
          ((m_eParserState == MP2STREAM_EOF) && ((m_nCurrOffset + TS_PKT_SIZE)>m_nFileSize)) )
      {
        eRetStatus = m_eParserState;
        m_eParserState = MP2STREAM_READY;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,
            "MP2StreamParser::Seek returning status %d", eRetStatus);
        return eRetStatus;
      }
      else
      {
        updateOffsetToNextPacket(m_nCurrOffset,m_bIsBDMVFormat,bForward);
      }
    }
  }
  if((eRetError == MP2STREAM_SUCCESS) && (bSeekSuccess==true))
  {
    MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
        "MP2StreamParser:: Seek matched m_currPESPkt.pts %f,ullReposTime %llu, ulTrackId %lu",
        m_currPESPkt.pts,ullReposTime,ulTrackId);
    if(m_pPartialFrameData)
      memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
    m_nCurrOffset = sample_info->noffset;
    m_nTotalADTSTrackDuration = 0;
    // If seek has completed successfully it is no point returning EOF
    // This can happen due to code in databits.cpp which sets EOF,
    // that code is necessary for streaming cases.
    if(m_eParserState == MP2STREAM_EOF)
    {
      m_eParserState = MP2STREAM_READY;
    }
  }
  else if((eRetError == MP2STREAM_SUCCESS) && (bSeekSuccess==false))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
        "MP2StreamParser:: Seek Unsuccessful");
  }
  return eRetError;
}
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
MP2StreamStatus MP2StreamParser::TSSeekBackwards(uint32 ultrackid,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bCanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip,
                        track_type eTrackType,
                        media_codec_type eMediaType)
{
  MP2StreamStatus eRetError = MP2STREAM_SUCCESS;
  uint64 prevOffset = m_nCurrOffset;
  uint8 newPayload = 0;
  bool foundFrameType = false;
  bool bAtDesiredPesPacket=false;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "MP2StreamParser::Backwards Seek called on Transport Stream");

  while (m_nCurrOffset != 0)
  {
    uint32 valCode =0;
    uint32 ulBytesRead = readMpeg2StreamData (m_nCurrOffset, m_nDataBufferSize,
                                              m_pDataBuffer, m_nDataBufferSize,
                                             (uint32) m_pUserData);
    uint32 ulLocalOffset=0;
    // 4 bytes are needed to check isPesPacket
    while ((ulLocalOffset < ulBytesRead - 4) &&
           //defensive check for very small files
           (m_nCurrOffset < m_nFileSize))
    {
      if( (isPESPacket(&m_pDataBuffer[ulLocalOffset],&valCode)) &&
          (( (valCode >= AUDIO_STREAM_ID_START)   &&
              (valCode <= AUDIO_STREAM_ID_END) )        ||
             ((valCode >= VIDEO_STREAM_ID_START)  &&
              (valCode <= VIDEO_STREAM_ID_END)) )               &&
          (isTrackIdInIdStore(ultrackid)))
      {
        //optimize to parsePES packet for the required ultrackid
        //or do it by default when no ultrackid is mentioned
        //during initial parsing
        int32 nBytesRead = 0;
        uint64 ullTempOffset = m_nCurrOffset;
        eRetError = parsePESPacket(ullTempOffset,valCode,
            ultrackid,m_pDataBuffer,m_nDataBufferSize,
            &nBytesRead);

        track_type eTrackType = TRACK_TYPE_UNKNOWN;
        media_codec_type eMediaType = UNKNOWN_AUDIO_VIDEO_CODEC;

        GetTrackType(ultrackid,&eTrackType,&eMediaType);

        if((uint64)m_currPESPkt.pts < ullReposTime)
        {
          if( ((eTrackType == TRACK_TYPE_VIDEO) &&
                (((m_currPESPkt.trackid >= VIDEO_STREAM_ID_START) &&
                  (m_currPESPkt.trackid <= VIDEO_STREAM_ID_END)))) ||
              ((eTrackType == TRACK_TYPE_AUDIO) &&
                ((m_currPESPkt.trackid >= AUDIO_STREAM_ID_START) &&
                 (m_currPESPkt.trackid <= AUDIO_STREAM_ID_END)))
            )
          {
            bAtDesiredPesPacket = true;
            MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "MP2StreamParser::Seek Found PES packet with m_currPESPkt.pts=%f m_currPESPkt.packet_length=%lu",
                      m_currPESPkt.pts,
                      m_currPESPkt.packet_length);
            break;
          }
          else
          {
            m_nCurrOffset++;
            ulLocalOffset++;
          }
        }
        else
        {
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                    "MP2StreamParser::Seek NOT Found PES packet with m_currPESPkt.pts=%f m_currPESPkt.packet_length=%lu",
                    m_currPESPkt.pts,
                    m_currPESPkt.packet_length);
          // if timestamp is greater then go back
          break;
        }
        m_nBytesRead = nBytesRead;
      }
      else
      {
        m_nCurrOffset++;
        ulLocalOffset++;
      }
    }
    if (bAtDesiredPesPacket == true)
    {
      break;
    }
    else if(m_nCurrOffset > 2*MPEG2_FILE_READ_CHUNK_SIZE)
    {
      m_nCurrOffset -= 2*MPEG2_FILE_READ_CHUNK_SIZE;
    }
    else
    {
      m_nCurrOffset = 0;
    }
  }
  // Now we have reached the PES packet which has a TS less
  // than ullReposTime, now search for the next I Frame
  //
  // Round of offset to TS packets.
  if (m_bIsBDMVFormat)
  {
    m_nCurrOffset = ((m_nCurrOffset / MPEG2_M2TS_PKT_BYTES) *
                     MPEG2_M2TS_PKT_BYTES) + FOURCC_SIGNATURE_BYTES;
  }
  else
  {
    m_nCurrOffset = (m_nCurrOffset / TS_PKT_SIZE) * TS_PKT_SIZE;
  }
  // Do a seek forward in case of audio even if m_nCurrOffset == 0
  // as there can be hundreds of audio samples in a quantum of
  // 188KB so without doing this accuracy can take a big hit.
  if(m_nCurrOffset || (eTrackType == TRACK_TYPE_AUDIO))
  {
    eRetError = TSSeekForward(ultrackid,
              ullReposTime,
              ullCurrPlayTime,
              sample_info,
              !bForward,
              bCanSyncToNonKeyFrame,
              nSyncFramesToSkip,
              eTrackType,
              eMediaType);
  }
  else
  {
    sample_info->noffset = m_nCurrOffset;
    sample_info->ntime = 0;
  }
  return eRetError;
}
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
MP2StreamStatus MP2StreamParser::SeekInTransportStream(uint32 ulTrackId,
                        uint64 ullReposTime,
                        uint64 ullCurrPlayTime,
                        mp2_stream_sample_info* sample_info,
                        bool bForward,
                        bool bCanSyncToNonKeyFrame,
                        int  nSyncFramesToSkip)
{
  MP2StreamStatus eRetError = MP2STREAM_FAIL;
  bool bOKToSeek = true;
  MP2StreamStatus eRetStatus = MP2STREAM_DEFAULT_ERROR;

#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  uint64 availOffset = 0;
  boolean bEnd = false;
#endif

  bool bContinue = true;
  uint16 pidToSeek = 0;
  uint8 newPayload = 0;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "MP2StreamParser::Seek called on Transport Stream");

  bContinue = getPidForTrackId(ulTrackId, &pidToSeek);
  if(bContinue && (ullCurrPlayTime <= ullReposTime))
  {
    bForward = true;
  }
  else
  {
    bForward = false;
  }

#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  if(m_bHttpStreaming && m_nCurrOffset + TS_PKT_SIZE > m_availOffset)
  {
    MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
    (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
    m_availOffset = availOffset;
    if((m_nCurrOffset + TS_PKT_SIZE > availOffset) && (!bEnd))
    {
      sample_info->noffset = m_nCurrOffset;
      sample_info->ntime = (float)GetPTSFromCurrentPESPacket();
      eRetError = m_eParserState = MP2STREAM_DATA_UNDER_RUN;
      bContinue = false;
    }
  }
#endif

  bool foundFrameType = false;
  uint8 picType = 0;
  track_type eTrackType = TRACK_TYPE_UNKNOWN;
  media_codec_type eMediaType = UNKNOWN_AUDIO_VIDEO_CODEC;
  memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
  uint64 ullPCR = 0;
  bool bPCRFound = false;

  if(MP2STREAM_SUCCESS == GetTrackType(ulTrackId,&eTrackType,&eMediaType))
  {
#ifdef USE_PCR_TO_SEEK
    /* If reference PCR is set, then we can seek faster through PCR */
    while(m_bRefPCRSet && (m_nCurrOffset <= m_nFileSize) && bContinue)
    {
      if(m_eParserState == MP2STREAM_EOF)
      {
        m_eParserState = MP2STREAM_SUCCESS;
      }
      /* Scan to find PCR seek point and use it to move offsets */
      eRetStatus = scanTSPacketToSeek(&ullPCR, &bPCRFound, bForward);
      if(bForward && (bPCRFound && ((ullPCR) >= ullReposTime)) )
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "MP2StreamParser::scanTSPacketToSeek found ullPCR %llu",
            ullPCR);
        break;
      }
      if((!bForward) && (bPCRFound &&((ullPCR) < ullReposTime)) )
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "MP2StreamParser::scanTSPacketToSeek found ullPCR %llu",
            ullPCR);
        break;
      }
    }
#else
    // Calculate approximate offset for desired seek position based
    // on the current offset, desired time and current play time.
    // Rationale is that number of bytes per time quantum will be
    // approximately similar for each time quantum.

    // For DASH case seek linearly from the beginning
    if(m_eFileFormat == FILE_SOURCE_DASH_MP2TS)
    {
      m_nCurrOffset = 0;
    }
    else if(ullCurrPlayTime)
    {
      float seekRatio = (float)ullReposTime / (float)ullCurrPlayTime ;
      m_nCurrOffset = uint64(float((float)m_nCurrOffset * seekRatio));
    }
    //Next best effort, try calcualting based on filesize
    //and clip duration if we have filesize already.
    else if (m_bEOFFound)
    {
      m_nCurrOffset = (uint64) (((float)((float)ullReposTime/
              (float) m_nClipDuration ))*
              ((float) ((float)m_nFileSize )));
    }
    //Next best effort, start searching from the beginning.
    else
    {
      m_nCurrOffset = 0;
    }

    if (m_nCurrOffset > m_availOffset)
    {
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
          "Bad offset calculation m_nCurrOffset=%llu m_availOffset=%llu",
          m_nCurrOffset, m_availOffset);
      while(m_nCurrOffset > m_availOffset)
      {
        m_nCurrOffset = (4*m_nCurrOffset)/5;
      }
    }
    // Round of approximation to packet size
    if (m_bIsBDMVFormat)
    {
      m_nCurrOffset = ((m_nCurrOffset / MPEG2_M2TS_PKT_BYTES) *
                       MPEG2_M2TS_PKT_BYTES) + FOURCC_SIGNATURE_BYTES;
    }
    else
    {
      m_nCurrOffset = (m_nCurrOffset / TS_PKT_SIZE) * TS_PKT_SIZE;
    }
#endif
    if(bForward)
    {
      if(bContinue)
      {
        eRetError = TSSeekForward(ulTrackId,
              ullReposTime,
              ullCurrPlayTime,
              sample_info,
              bForward,
              bCanSyncToNonKeyFrame,
              nSyncFramesToSkip,
              eTrackType,
              eMediaType);
      }
    }
    else
    {
      if(bContinue)
      {
        while(eRetError != MP2STREAM_SUCCESS)
        {
          eRetError = TSSeekBackwards(ulTrackId,
                        ullReposTime,
                        ullCurrPlayTime,
                        sample_info,
                        bForward,
                        bCanSyncToNonKeyFrame,
                        nSyncFramesToSkip,
                        eTrackType,
                        eMediaType);
          if (eRetError != MP2STREAM_SUCCESS)
          {
            // If seek failed then try to seek further back by 1 sec
            if(ullReposTime >= SEEK_JUMP_INTERVAL)
            {
              ullReposTime = ullReposTime - SEEK_JUMP_INTERVAL;
            }
            else
            {
              // if all has failed just seek to the beginning
              // of the clip.
              ullReposTime=0;
            }
          }
        }
      }
    }
  }
  else
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,
        "MP2StreamParser::GetTrackType returning status %d",
        eRetStatus);
  }

  return  eRetError;
}

/*! ======================================================================
@brief  Repositions given track to specified time

@detail    Seeks given track in forward/backward direction to specified time
           in Program Stream

@param[in]3
 ullTrackId: Identifies the track to be repositioned.
 ullReposTime:Target time to seek to
 ullCurrPlayTime: Current playback time
 sample_info: Sample Info to be filled in if seek is successful
 bCanSyncToNonKeyFrame: When true, video can be repositioned to non key frame
 nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

@return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::SeekInProgramStream(uint32 ulTrackId,
                                      uint64 ullReposTime,
                                      uint64 ullnCurrPlayTime,
                                      mp2_stream_sample_info* pSampleInfo,
                                      bool bForward,
                                      bool bCanSyncToNonKeyFrame,
                                      int  nSyncFramesToSkip)
{
  MP2StreamStatus eRetError = MP2STREAM_FAIL;
  bool bOKToSeek = true;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
    "MP2StreamParser::Seek called on Program Stream");
  if(!m_pCurrVOBUPCIPkt)
  {
    bOKToSeek = false;
  }
  else if(m_pCurrVOBUPCIPkt->end_pts <=ullReposTime)
  {
    bForward = true;
  }
  else
  {
    bForward = false;
  }

  if( (m_eParserState == MP2STREAM_READY)         &&
      (bOKToSeek)                                 &&
      (m_pFirstVOBUDSIPkt)                        &&
      (m_pFirstVOBUDSIPkt->next_vobu_offset_valid) )
  {
    bool bScanFwd = true;
    pci_pkt temp_pci;
    memset(&temp_pci,0,sizeof(pci_pkt));
    uint64 offsettogo = 0;
    uint64 offsettostart = 0;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,
      "MP2StreamParser::SeekInProgramStream");
    if(!bForward)
    {
      //User initiated the rewind
      uint64 dist_from_curr_time =ullnCurrPlayTime -ullReposTime;
      uint64 dist_from_beg       =ullReposTime - m_pFirstVOBUDSIPkt->vob_v_s_ptm;
      if(
          (dist_from_beg <= dist_from_curr_time)                            ||
          (m_pCurrVOBUDSIPkt && (!m_pCurrVOBUDSIPkt->prv_vobu_offset_valid)) )
      {
        offsettogo = 0;
        bScanFwd = true;
      }
      else
      {
        if(m_pCurrVOBUDSIPkt && m_pCurrVOBUDSIPkt->prv_vobu_offset_valid)
        {
          offsettogo = m_pCurrVOBUDSIPkt->noffset;
          bScanFwd = false;
        }
      }
    }
    else
    {
      //User initiated the fast forward
      offsettogo = m_pCurrVOBUDSIPkt->noffset;
      bScanFwd = true;
    }
    offsettostart = offsettogo;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"offsettostart %llu",
        offsettostart);

    while( (m_pCurrVOBUDSIPkt)                                &&
           (m_pCurrVOBUPCIPkt)                                &&
           (MP2STREAM_SUCCESS == parsePackHeader(offsettostart,
                                                 true,ulTrackId))
         )
    {
      m_eParserState = MP2STREAM_SEEKING;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
          "m_pCurrVOBUPCIPkt->start_pts %lu",
          m_pCurrVOBUPCIPkt->start_pts);
      if(bForward)
      {
        if( (m_pCurrVOBUPCIPkt->start_pts >=ullReposTime)||
            (m_pCurrVOBUPCIPkt->end_pts >=ullReposTime) )
        {

          eRetError = MP2STREAM_SUCCESS;
          pSampleInfo->bsync = 1;
          if(m_pCurrVOBUPCIPkt->start_pts >=ullReposTime)
          {
            pSampleInfo->ntime = (float)m_pCurrVOBUPCIPkt->start_pts;
          }
          if(m_pCurrVOBUPCIPkt->end_pts >=ullReposTime)
          {
            pSampleInfo->ntime = (float)m_pCurrVOBUPCIPkt->end_pts;
          }
          pSampleInfo->noffset = m_pCurrVOBUPCIPkt->noffset;
          m_nCurrOffset = pSampleInfo->noffset;
          MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
              "Matched pSampleInfo->ntime %fullReposTime %llu offset %llu",
              pSampleInfo->ntime,ullReposTime,m_nCurrOffset);
          break;
        }
      }//if(bForward)
      else
      {
        if(!bScanFwd)
        {
          if( (m_pCurrVOBUPCIPkt->start_pts <=ullReposTime)||
              (m_pCurrVOBUPCIPkt->end_pts <=ullReposTime) )
          {
            eRetError = MP2STREAM_SUCCESS;
            pSampleInfo->bsync = 1;
            if(m_pCurrVOBUPCIPkt->start_pts <=ullReposTime)
            {
              pSampleInfo->ntime = (float)m_pCurrVOBUPCIPkt->start_pts;
            }
            if(m_pCurrVOBUPCIPkt->end_pts <=ullReposTime)
            {
              pSampleInfo->ntime = (float)m_pCurrVOBUPCIPkt->end_pts;
            }
            pSampleInfo->noffset = m_pCurrVOBUPCIPkt->noffset;
            m_nCurrOffset = pSampleInfo->noffset;
            MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                "Matched pSampleInfo->ntime %fullReposTime %llu offset %llu",
                pSampleInfo->ntime,ullReposTime,m_nCurrOffset);
            break;
          }
        }//if(!bScanFwd)
        else
        {
          if( (temp_pci.end_pts)&&
                                  ((m_pCurrVOBUPCIPkt->start_pts >=ullReposTime)||
                                   (m_pCurrVOBUPCIPkt->end_pts >=ullReposTime)) )
          {
            eRetError = MP2STREAM_SUCCESS;
            pSampleInfo->bsync = 1;
            if(temp_pci.start_pts <= ullReposTime)
            {
              pSampleInfo->ntime = (float)temp_pci.start_pts;
            }
            if(temp_pci.end_pts <=ullReposTime)
            {
              pSampleInfo->ntime = (float)temp_pci.end_pts;
            }
            pSampleInfo->noffset = temp_pci.noffset;
            m_nCurrOffset = pSampleInfo->noffset;
            MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                "Matched pSampleInfo->ntime %fullReposTime %llu offset %llu",
                pSampleInfo->ntime, ullReposTime, m_nCurrOffset);
            break;
          }
          else
          {
            memcpy(&temp_pci,m_pCurrVOBUPCIPkt,sizeof(pci_pkt));
          }
        }//end of else of if(!bScanFwd)
      }//end of else of if(bForward)
      if( (m_pCurrVOBUDSIPkt) && (m_pCurrVOBUDSIPkt->next_vobu_offset_valid) && (bScanFwd) )
      {
        offsettogo = offsettogo + (m_pCurrVOBUDSIPkt->next_vobu_offset * MAX_PS_PACK_SIZE);
        offsettostart = offsettogo;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "bScanFwd:updated offsettogo %llu",offsettogo);
      }
      if( (m_pCurrVOBUDSIPkt) && (m_pCurrVOBUDSIPkt->prv_vobu_offset_valid) && (!bScanFwd) )
      {
        offsettogo = m_pCurrVOBUDSIPkt->noffset -
          (m_pCurrVOBUDSIPkt->prv_vobu_offset * MAX_PS_PACK_SIZE);
        offsettostart = offsettogo;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "!bScanFwd: updated offsettogo %llu",offsettogo);
      }
    }
    //Irrespective of seek being fail/successful,
    //change the state to continue playback
    m_eParserState = MP2STREAM_READY;
  }
  else
  {
    track_type eTrackType = TRACK_TYPE_UNKNOWN;
    media_codec_type eMediaType = UNKNOWN_AUDIO_VIDEO_CODEC;
    if((MP2STREAM_SUCCESS == GetTrackType(ulTrackId,&eTrackType,&eMediaType))&&
       (m_nClipDuration))
    {
      // Make approximation on offset to speed up seek
      m_nCurrOffset = (uint64) (((float)((float)ullReposTime /
              (float) m_nClipDuration )) *
          ((float) ((float)m_nFileSize )));
    }
    if(!readMpeg2StreamData (m_nCurrOffset,m_nDataBufferSize,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData))
    {
      eRetError = m_eParserState;
    }
    else
    {
      uint32 ulLocalOffset=0;
      while(m_nCurrOffset < m_nFileSize)
      {
        uint32 ulCodeVal = getBytesValue(sizeof(uint32),
            &m_pDataBuffer[ulLocalOffset]);
        if(ulCodeVal == MP2_PACK_START_CODE)
        {
          break;
        }
        else
        {
          ulLocalOffset++;
          m_nCurrOffset++;
        }
      }
      if(!readMpeg2StreamData (m_nCurrOffset,
            MPEG2_FILE_READ_CHUNK_SIZE,m_pDataBuffer,
            m_nDataBufferSize,(uint32) m_pUserData))
      {
        eRetError = m_eParserState;
      }
      else
      {
        float frameTS=0.0;
        int32 nBytesRead=0;
        bool bfoundFrameType=false;

        eRetError = GetSampleAtFrameBoundary(ulTrackId,m_pDataBuffer,
            PS_DATA_BUFFER_SIZE,&nBytesRead,&frameTS);
        pSampleInfo->noffset = m_nCurrOffset;
        pSampleInfo->ntime = (float)GetPTSFromCurrentPESPacket();
        if(eRetError == MP2STREAM_SUCCESS)
        {
          if (TRACK_TYPE_VIDEO == eTrackType)
          {
            m_nBytesRead = (uint32)nBytesRead;
            eRetError = isKeyFrame(eMediaType, &bfoundFrameType);
          }
          uint64 ullStartOffset = m_nCurrOffset;
          while(((uint64)pSampleInfo->ntime < ullReposTime) ||
              ((TRACK_TYPE_VIDEO == eTrackType) &&
               (eRetError != MP2STREAM_SUCCESS)&&
               (eRetError != MP2STREAM_EOF)))
          {
            /* Keep Current frame start offset value in local variable.
               Following function will update "m_nCurrOffset" value to start of
               next media sample. If this is sync frame we are looking for, then
               by using local variable we will seek. */
            ullStartOffset = m_nCurrOffset;
            eRetError = GetSampleAtFrameBoundary(ulTrackId,m_pDataBuffer,
                PS_DATA_BUFFER_SIZE,&nBytesRead,&frameTS);
            if(MP2STREAM_EOF == eRetError)
            {
              break;
            }
            if (TRACK_TYPE_VIDEO == eTrackType)
            {
              m_nBytesRead = (uint32)nBytesRead;
              eRetError = isKeyFrame(eMediaType, &bfoundFrameType);
            }
            //! Always update with Start values
            pSampleInfo->noffset = ullStartOffset;
            pSampleInfo->ntime   = (float)GetPTSFromCurrentPESPacket();
          }//while(((uint64)pSampleInfo->ntime < ullReposTime) ||
          if(MP2STREAM_EOF != eRetError)
          {
            eRetError = MP2STREAM_SUCCESS;
            //! Update current offset value to start of sync sample
            m_nCurrOffset = ullStartOffset;
          }//if(MP2STREAM_EOF != eRetError)
        }//if(eRetError == MP2STREAM_SUCCESS)
      }//else
      return eRetError;
    }
  }
  return  eRetError;
}
/*! ======================================================================
@brief  Repositions given track to specified time

@detail    Seeks given track in forward/backward direction to specified time

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
MP2StreamStatus MP2StreamParser::Seek(uint32 ulTrackid,
                                      uint64 ullReposTime,
                                      uint64 ullCurrPlayTime,
                                      mp2_stream_sample_info* sample_info,
                                      bool bForward,
                                      bool bCanSyncToNonKeyFrame,
                                      int  nSyncFramesToSkip)
{
  MP2StreamStatus eRetError = MP2STREAM_FAIL;

  if(m_eParserState == MP2STREAM_EOF)
  {
    m_eParserState = MP2STREAM_READY;
  }
#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
    uint64 availOffset = 0;
    boolean bEnd = false;
#endif

  if(sample_info)
  {
    memset(sample_info,0,sizeof(mp2_stream_sample_info));
  }
  else
  {
    return eRetError;
  }
  if(ullReposTime <= m_nClipStartTime)
  {
    //Start from the begining, reset the offset and sampleinfo and return
    eRetError = MP2STREAM_SUCCESS;
    sample_info->bsync = 1;
    sample_info->ntime = (float)m_nClipStartTime;
    m_nCurrOffset = sample_info->noffset = m_nStartOffset;
    m_eParserState = MP2STREAM_READY;
    m_nTotalADTSTrackDuration = 0;
    memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
    if(m_pPartialFrameData)
      memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
    memset(&m_UnderrunBuffer, 0, sizeof(m_UnderrunBuffer));
    MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
        "Matched sample_info->ntime %f ullReposTime %llu offset %llu ",
        sample_info->ntime,ullReposTime,m_nCurrOffset);
    return eRetError;
  }

  if(m_bProgramStream)
  {
    // Seek in Program Stream
    eRetError = SeekInProgramStream(ulTrackid,
                        ullReposTime,
                        ullCurrPlayTime,
                        sample_info,
                        bForward,
                        bCanSyncToNonKeyFrame,
                        nSyncFramesToSkip);
  }
  else
  {
    // Seek in Transport Stream
    eRetError = SeekInTransportStream(ulTrackid,
                          ullReposTime,
                          ullCurrPlayTime,
                          sample_info,
                          bForward,
                          bCanSyncToNonKeyFrame,
                          nSyncFramesToSkip);
  }
  return eRetError;
}
/*! ======================================================================
@brief  Checks id store to see if given id exists in the store

@detail    Checks id store to see if given id exists in the store

@param[in] N/A

@return    true if id exists in id store else returns false
@note      None.
========================================================================== */
bool  MP2StreamParser::isTrackIdInIdStore(uint32 id)
{
  bool bRet = false;
  for(int i = 0; m_pAudioStreamIds && i< m_nAudioStreams;i++)
  {
    if(m_pAudioStreamIds[i] == id)
    {
      bRet = true;
      break;
    }
  }
  for(int i = 0; m_pVideoStreamIds && i< m_nVideoStreams;i++)
  {
    if(m_pVideoStreamIds[i] == id)
    {
      bRet = true;
      break;
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Updates Stream Info read from system target header with private stream

@detail    If system target header contains private streamid, update it's type with AUDIO.
        If given stream-id is not specified in system target header, then this function is a NOOP.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
void MP2StreamParser::updateStreamInfo(uint8 streamid,uint32 valCode)
{
  bool bFound = false;
  for(int i = 0; i < m_nstreams; i++)
  {
    //check if we already stored this stream-id in our table
    if(  (streamid == m_pStream_Info[i].stream_id)
         &&
            ((m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO)||
            (m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO))
      )
    {
      bFound = true;
      break;
    }
  }
  if(!bFound)
  {
    for(int i = 0; i < m_nstreams; i++)
    {
      //find an unused entry and update the information
      if( m_pStream_Info[i].stream_media_type == TRACK_TYPE_UNKNOWN)
      {
        //Update the stream info with given stream-id
        m_pStream_Info[i].stream_id = streamid;

        //Now check if valCode belongs to audio/video
        if( ((valCode >= AUDIO_STREAM_ID_START) && (valCode <= AUDIO_STREAM_ID_END)) ||
            ((valCode >= RES_DATA_STREAM_START_ID) && (valCode <= RES_DATA_STREAM_END_ID)) ||
            (valCode == PRIVATE_STREAM1_ID) )
        {
          m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
        }
        else if( (valCode >= VIDEO_STREAM_ID_START) && (valCode <= VIDEO_STREAM_ID_END) )
        {
          m_pStream_Info[i].stream_media_type = TRACK_TYPE_VIDEO;
        }
        else
        {
          //SOMETHING IS WRONG, WE SHOULD HAVE NEVER COME HERE
          // TO DO: Temp fix for the compilation error in AMSS
          //assert(0);
        }
        break;
      }
    }
  }
}
/*! ======================================================================
@brief  Updates Track Info using information from stream type present in PMT

@detail Assigns PID as track id and fills track info with info from PMT

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::updateTrackInfoFromPSI(uint16 pid, uint32 streamType)
{
  bool bFound = false;
  bool bUpdate = false;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;
  MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
  bool bPlayAudio = pMP2TStream->isAudioInstance();
  bool bPlayVideo = pMP2TStream->isVideoInstance();

  for(int i = 0; i < m_nstreams; i++)
  {
    //check if we already stored this stream-id in our table
    if(  (pid == m_pStream_Info[i].stream_id) &&
         ( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO)||
           (m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO) )
      )
    {
      bFound = true;
      break;
    }
  }
  if(!bFound)
  {
    for(int i = 0; i < m_nstreams && !bUpdate; i++)
    {
      //find an unused entry and update the information
      if( m_pStream_Info[i].stream_media_type == TRACK_TYPE_UNKNOWN)
      {
        //Update the stream info with given pid
        m_pStream_Info[i].stream_id = pid;

        //Now check if streamType belongs to audio/video
        switch(streamType)
        {
          case VC1_VIDEO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_VIDEO;
            m_pStream_Info[i].video_stream_info.Video_Codec = VIDEO_CODEC_VC1;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayVideo;
            bUpdate = true;
            break;
          case MPEG2_VIDEO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_VIDEO;
            m_pStream_Info[i].video_stream_info.Video_Codec = VIDEO_CODEC_MPEG2;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayVideo;
            bUpdate = true;
            break;
          case AVC_VIDEO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_VIDEO;
            m_pStream_Info[i].video_stream_info.Video_Codec = VIDEO_CODEC_H264;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayVideo;
            bUpdate = true;
            break;
          case AAC_ADTS_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_AAC;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
          case LPCM_AUDIO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_LPCM;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
          case AC3_AUDIO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_AC3;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
#ifdef ATSC_COMPLIANCE
          case EAC3_AUDIO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_EAC3;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
#endif
          case HDMV_DTS_STREAM_TYPE:
          case DTS_HD_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_DTS;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
          case MPEG1_AUDIO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_MP3;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
          case MPEG2_AUDIO_STREAM_TYPE:
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
            m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_MPEG2;
            m_pStream_Info[i].bParsed = m_bLocateCodecHdr && !bPlayAudio;
            bUpdate = true;
            break;
          default:
            break;
        }
      }
    }
  }
  return retStatus;
}
/*! ======================================================================
@brief  MP2StreamParser::reAllocStreamInfo

@detail    Some time, system target header may have wrong information regarding audio and video bound.
        Thus, when audio/video stream is encountered and such information is missing from
        system target header, we need to do re-alloc to store all the stream information.

@param[in] N/A

@return    True if rea-lloc is successful otherwise, returns false.
@note      None.
========================================================================== */
bool MP2StreamParser::reAllocStreamInfo(int count)
{
  bool bRet = false;
  if(count > 0)
  {
    stream_info* tmp = 0;
    if(m_pStream_Info)
    {
      tmp = (stream_info*)MM_Realloc(m_pStream_Info,count * sizeof(stream_info));
    }
    else
    {
      tmp = (stream_info*)MM_Malloc(count * sizeof(stream_info));
    }
    if(tmp)
    {
      memset(tmp+(count-1),0,sizeof(stream_info));
      m_pStream_Info =  tmp;
      bRet = true;
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Determines if initial parsing is done so that playback can begin

@detail    Determines if initial parsing is done so that playback can begin

@param[in] N/A

@return    True if it is determined that initial parsing is done
           otherwise returns false.
@note      None.
========================================================================== */
bool MP2StreamParser::isInitialParsingDone()
{
  bool bRet = false;
  uint8 count = 0;
  bool bContinue = true;
  boolean bEnd = false;
  bool bNeedCodecInfo = false;

#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  if((m_bHttpStreaming) && m_nCurrOffset + TS_PKT_SIZE >= m_availOffset &&
     (!m_bEOFFound))
  {
    uint64 availOffset = 0;
    MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
   (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
    m_availOffset = availOffset;
    if(m_nCurrOffset > availOffset)
    {
      m_eParserState = MP2STREAM_DATA_UNDER_RUN;
      bContinue = false;
    }
  }
#endif

  if(( ((!m_bProgramStream) && (m_ProgMapSection) && (m_nstreams == m_nStreamsSelected) && (m_nStreamsSelected) )||
      (m_bProgramStream) ||
         (m_nCurrOffset >= m_nFileSize) ) && bContinue)
  {
    for(int i = 0; i < m_nstreams; i++)
    {
      if(m_pStream_Info[i].bParsed)
      {
        if( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO) &&
          (m_pStream_Info[i].video_stream_info.Video_Codec == VIDEO_CODEC_H264) )
        {
          if(m_bLocateCodecHdr)
          {
            bNeedCodecInfo = true;
          }
        }
        count++;
      }
    }
    if(m_bProgramStream)
    {
      //Total streams should be nonzero
      if(count == m_nstreams && m_nstreams &&
        ( ( (bNeedCodecInfo) && (m_pAvcCodecInfo) && (m_pAvcCodecInfo->isValid)) ||
        (!bNeedCodecInfo) ) )
      {
        bRet = true;
      }
      else
      {
        m_nInitialPacksParsed = 0;
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"isInitialParsingDone resetting m_nInitialPacksParsed %d",m_nInitialPacksParsed);
#endif
      }
    }
    else
    {
      if((count > 0) &&
          (m_nCurrOffset > WFD_MIN_TS_PACKETS_PARSED*TS_PKT_SIZE) &&
          (FILE_SOURCE_WFD_MP2TS == m_eFileFormat))
      {
        bRet = true;
        if(m_bRefVideoPTSSet && !m_bRefAudioPTSSet)
        {
          m_bRefAudioPTSSet = true;
          m_nRefAudioPTS = m_nRefVideoPTS;
        }
        else if (m_bRefAudioPTSSet && !m_bRefVideoPTSSet)
        {
          m_bRefVideoPTSSet = true;
          m_nRefVideoPTS = m_nRefAudioPTS;
        }
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
            "Returning Early from isInitialParsingDone");
      }

      if( (count == m_nstreams) &&
          ( ( (bNeedCodecInfo) && (m_pAvcCodecInfo) && (m_pAvcCodecInfo->isValid)) ||
              (!bNeedCodecInfo) ) )
      {
        if(((!m_bHttpStreaming) || (m_bHttpStreaming && m_bEOFFound)) &&
            (m_eFileFormat != FILE_SOURCE_DASH_MP2TS))
        {
          m_bGetLastPTS = true;
          if(getLastPTS())
          {
            bRet = true;
          }
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"isInitialParsingDone DONE");
          bRet = true;
        }
      }
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Determines if backwards parsing is done so that playback can begin

@detail    This is done currently to find the last PTS and get duration

@param[in] N/A

@return    True if it is determined that initial parsing is done
           otherwise returns false.
@note      None.
========================================================================== */
bool MP2StreamParser::getLastPTS()
{
  bool bRet = false;
  int count = 0;
  for(int i = 0; i < m_nstreams; i++)
  {
    if( ((m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) && (m_nLastAudioPTS)) ||
        ((m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO) && (m_nLastVideoPTS)))
    {
      count++;
    }
  }
  if(count == m_nstreams)
  {
    bRet = true;
    m_bGetLastPTS = false;
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::GetAACAudioProfile

DESCRIPTION:
  Maps given track-id to track index.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 MP2StreamParser::GetAACAudioProfile(uint32 id)
{
  uint32 audioObject = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if((m_pStream_Info) && (m_pStream_Info[i].stream_id == id))
    {
      audioObject = m_pStream_Info[i].audio_stream_info.AudioObjectType;
    }
  }
  return audioObject;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::GetAACAudioFormat

DESCRIPTION:
  Maps given track-id to track index.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 MP2StreamParser::GetAACAudioFormat(uint32 id)
{
  aac_format_type audioFormat = AAC_FORMAT_UNKNOWN;
  for(int i = 0; i< m_nstreams; i++)
  {
    if((m_pStream_Info) && (m_pStream_Info[i].stream_id == id))
    {
      if(m_pStream_Info[i].audio_stream_info.Audio_Codec == AUDIO_CODEC_AAC)
        audioFormat = AAC_FORMAT_ADTS;
    }
  }
  return audioFormat;
}

/* ======================================================================
FUNCTION:
  MP2StreamParser::GetAACAudioInfo

DESCRIPTION:
  Get AAC codec audio header information

INPUT/OUTPUT PARAMETERS:
  ulTrackID:   Track ID
  psAudioIfno: AAC audio information strucuture

RETURN VALUE:
  TRUE if success otherwise FAIL

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::GetAACAudioInfo(uint32 ulTrackID,aac_audio_info *psAudioInfo)
{
  bool bStatus = false;
  if( NULL != psAudioInfo)
  {
    for(int i = 0; i< m_nstreams; i++)
    {
      if((m_pStream_Info) && (m_pStream_Info[i].stream_id == ulTrackID ) &&
         ( AUDIO_CODEC_AAC == m_pStream_Info[i].audio_stream_info.Audio_Codec ) )
      {
        psAudioInfo->AudioObjectType = \
          m_pStream_Info[i].audio_stream_info.AudioObjectType;
        psAudioInfo->SamplingFrequency = \
          m_pStream_Info[i].audio_stream_info.SamplingFrequency;
        psAudioInfo->NumberOfChannels = \
          m_pStream_Info[i].audio_stream_info.NumberOfChannels;
        psAudioInfo->ucCRCPresent = \
          m_pStream_Info[i].audio_stream_info.ucProtection;
        bStatus = true;
      }
    }
  }
  return bStatus;
}
/*! ======================================================================
@brief  Parses MP2 Transport stream packet and finds if it has new payload
        or has continueing payload.

@detail    This function starts parsing the MP2 transport stream.
        Returns if this packet has start of new payload or has continueing
        payload.

@param[in] N/A
@return    true or false
========================================================================== */
bool MP2StreamParser::isSameStream(uint32* trackId, uint8* newPayload)
{
  MP2StreamStatus retError = MP2STREAM_SUCCESS;
  bool bContinue = true;
  uint8 tempBuffer[TS_PKT_SIZE];
  uint8 val = 0;
  int localOffset = 0;
  uint32 adpt_field = 0;
  uint8 adaption_field_length = 0;
  uint16 pid, currentPid = 0;
  uint64 availOffset = m_availOffset;
  uint64 ullLowerBoundOffset = 0;
  bool nRet = false;
  boolean bEnd = false;
  MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;

if(!trackId)
{
  return false;
}
#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
  if(m_bHttpStreaming && (m_nCurrOffset + TS_PKT_SIZE) > m_availOffset)
  {
    (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
    m_availOffset = availOffset;
    if(m_nCurrOffset + TS_PKT_SIZE > availOffset)
    {
      if(!bEnd)
      {
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_MEDIUM,
        "MP2STREAM_DATA_UNDER_RUN m_nCurrOffset %llu availOffset %llu bEnd %d",
        m_nCurrOffset,availOffset,bEnd);
        m_eParserState = MP2STREAM_DATA_UNDER_RUN;
      }
      else
        m_eParserState = MP2STREAM_EOF;
      bContinue = false;
    }
  }
  else
#endif
  {
    if(!m_bHttpStreaming)
    availOffset = m_nFileSize;
  }

  while(bContinue && (m_nCurrOffset < availOffset))
  {
    if(readMpeg2StreamData (m_nCurrOffset,  (TS_INIT_BYTES+1),
                             tempBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
    {
      if(memcmp(tempBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
            "isSamePayload Sync byte(0x47) not found!! m_nCurrOffset=%llu",
            m_nCurrOffset);
        retError = MP2STREAM_CORRUPT_DATA;
        bContinue = false;
      }
      if(bContinue)
      {
        getByteFromBitStream(&val,&tempBuffer[1],1,1);
        if(newPayload)
        {
          *newPayload = val;
          pid = (tempBuffer[1] & 0x1F)<< 8;
          pid |=  tempBuffer[2];
          getByteFromBitStream(&val,&tempBuffer[3],2,2);
          adpt_field = val;

          getByteFromBitStream(&val,&tempBuffer[localOffset+TS_PKT_HDR_BYTES],0,8);
          adaption_field_length = val;

          bContinue = getPidForTrackId(*trackId,&currentPid);

          if( bContinue && (pid == currentPid) &&
             ((pid > TS_GENERAL_PURPOSE_PID_START) &&
             (pid < TS_GENERAL_PURPOSE_PID_END) ) )
          {
            nRet = true;
            bContinue = false;
          }
          else if(isPSI(pid))
          {
            retError = parseTransportStreamPacket(*trackId);
            if(retError == MP2STREAM_SUCCESS)
            {
              bContinue = true;
            }
          }
          else
          {
            nRet = false;
            bContinue = false;
          }
        }
      }
    }
    /* If Current Offset is less than available offset, but data read is
       failed, then it will be WFD use case. RTP might have overwritten data
       in ring buffer. Instead of reporting underrun in such case, increase
       buffer offset to lower bound offset of RTP data buffer and try read */
    else if((m_nCurrOffset + 5 * TS_PKT_SIZE) < m_availOffset)
    {
      (void)pMP2TStream->GetBufferLowerBound(&ullLowerBoundOffset,&bEnd);

      // If ullLowerBoundOffset is not at TS Packet boundary
      if(ullLowerBoundOffset % TS_PKT_SIZE)
      {
        ullLowerBoundOffset += TS_PKT_SIZE;
      }

      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                   "isSamePayload data overwritten, \
                    increas m_ncurroffset %llu to next TS Pkt lowerBoundOffset %llu",
                   m_nCurrOffset,ullLowerBoundOffset);
      // Correct current offset value to nearest TS Packet start
      m_nCurrOffset = (ullLowerBoundOffset / TS_PKT_SIZE) * TS_PKT_SIZE;
    }
    else
    {
      retError = m_eParserState;
      bContinue = false;
    }
  }
  if((m_nCurrOffset >= availOffset))
  {
    if(m_bHttpStreaming && !bEnd)
    {
      m_eParserState = MP2STREAM_DATA_UNDER_RUN;
    }
    else if( (!m_bHttpStreaming) && ((!m_bGetLastPTS) || bEnd))
    {
      m_eParserState = MP2STREAM_EOF;
    }
  }
  return nRet;
}
/* =============================================================================
FUNCTION:
 MP2StreamParser::GetLastRetrievedSampleOffset

DESCRIPTION:
Returns the absolute file offset of the last successful retrieved sample.

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:

SIDE EFFECTS:
  None.
=============================================================================*/
uint64 MP2StreamParser::GetLastRetrievedSampleOffset(uint32 /*id*/)
{
  return m_nCurrOffset;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::GetBaseTime

DESCRIPTION:
  gets base time of the clip.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::GetBaseTime(uint32 trackid, double* nBaseTime)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"GetBaseTime");
  *nBaseTime = 0;
  bool nRet = false;
  for(int i = 0; i< m_nstreams; i++)
  {
    if((m_pStream_Info) && (m_pStream_Info[i].stream_id == trackid))
    {
      if((m_pStream_Info[i].stream_media_type == TRACK_TYPE_VIDEO) &&
         (m_bRefVideoPTSSet))
      {
        nRet = true;
        *nBaseTime = m_nRefVideoPTS;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetBaseTime for video %f",
                     m_nRefVideoPTS);
      }
      else if((m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
              (m_bRefAudioPTSSet))
      {
        nRet = true;
        *nBaseTime = m_nRefAudioPTS;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetBaseTime for audio %f",
                     m_nRefAudioPTS);
      }
    }
  }
  return nRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::SetBaseTime

DESCRIPTION:
  sets base time of the clip.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::SetBaseTime(uint32 trackid,double nBaseTime)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"SetBaseTime");
  bool bRet = false;
  for(uint32 ulIndex = 0; ulIndex < m_nstreams; ulIndex++)
  {
    if((m_pStream_Info) && (m_pStream_Info[ulIndex].stream_id == trackid))
    {
      if(m_pStream_Info[ulIndex].stream_media_type == TRACK_TYPE_VIDEO)
      {
        bRet = true;
        m_nRefVideoPTS = (double)nBaseTime;
      }
      else if(m_pStream_Info[ulIndex].stream_media_type == TRACK_TYPE_AUDIO)
      {
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                     "SetBaseTime m_nRefAudioPTS %f nBaseTime %f",
                     m_nRefAudioPTS, nBaseTime);
        m_nRefAudioPTS = (double)nBaseTime;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                     "SetBaseTime m_nRefAudioPTS %f", m_nRefAudioPTS);
        bRet = true;
      }
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Returns video frame rate for given track id

@detail    Returns PTS from current PES packet parsed by mpeg2 parser

@param[in] N/A

@return    Frame Rate fps for given track
@note      None.
========================================================================== */
bool MP2StreamParser::GetNextH264NALUnit(uint32 nOffset, uint8* dataBuffer,
                                         uint8* nalUType, uint32* nalLen,
                                         int32 nBytesRead, uint32* dataOffset)
{
  bool bRet = false;
  uint32 trackid = 0;
  uint32 startOffset = 0;
  start_code_type startCodeType = START_CODE_DEFAULT;

#ifdef MPEG2_PARSER_DEBUG
MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"MP2StreamParser::GetNextH264NalUnit");
#endif

  while(dataBuffer && nBytesRead && ((int32)nOffset + 4 < nBytesRead) && dataOffset)
  {
    if(m_pVideoStreamIds)
    {
      trackid = m_pVideoStreamIds[0];
    }
    //With this change dataOffset only gets value from data present before NAL units and not
    //from other nal units present before
    int inOffset = nOffset;
    bRet = isFrameStartWithStartCode(&nOffset, trackid, dataBuffer, nBytesRead, &startCodeType);
    if(bRet)
    {
      *dataOffset = nOffset - inOffset;
      nOffset += startCodeType;
      *nalUType = (dataBuffer[nOffset] & 0x1F);
      startOffset = nOffset;
      bRet = isFrameStartWithStartCode(&nOffset, trackid, dataBuffer, nBytesRead, &startCodeType);
      if(bRet)
      {
        *nalLen = (nOffset - startOffset);
        break;
      }
    }
    else
    {
      break;
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::findH264NALTypeForFrame

DESCRIPTION:
  Returns type of frame.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::findH264NALTypeForFrame(uint8* nalType)
{
  bool bRet = false;
  uint32 index = 0;
  uint8 nalUType = 0;
  uint32 nalLen = 0;
  uint32 nBytesRead = m_nBytesRead;
  uint8* dataBuf = m_pDataBuffer;
  uint32 dataOffset = 0;

  while(index < nBytesRead)
  {
    bRet = GetNextH264NALUnit(index, dataBuf, &nalUType, &nalLen, nBytesRead, &dataOffset);

    if(nalUType == NAL_UNIT_TYPE_IDR)
    {
      *nalType = NAL_UNIT_TYPE_IDR;
      bRet = true;
      break;
    }
    else if(nalUType == NAL_UNIT_TYPE_NON_IDR)
    {
      *nalType = NAL_UNIT_TYPE_NON_IDR;
      bRet = true;
      break;
    }

    if(bRet)
    {
      index += nalLen;
    }
    else
    {
      //We might not have found the full NAL Unit, we can only use nalUType
      //to check frame type if available
      break;
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::findVC1FrameStartCode

DESCRIPTION:
  Returns true if type of frame is found else false

INPUT/OUTPUT PARAMETERS:
  Output  pucPicType- Type of frame in VC1

RETURN VALUE:
  Returns true if type of frame is found else false

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::findVC1FrameStartCode(uint8* pucPicType)
{
  uint32 ulLocalOffset = 0;
  uint32 nBytesRead = m_nBytesRead;
  uint32 ulStartCodeVal;
  bool bRet = false;


  while(ulLocalOffset < nBytesRead)
  {
    // Get 3 bytes for VIDEO_START_CODE_PREFIX
    ulStartCodeVal = getBytesValue(3,&m_pDataBuffer[ulLocalOffset]);
    //make sure there is video prefix start code
    if(ulStartCodeVal == VIDEO_START_CODE_PREFIX)
    {
      ulLocalOffset+= 3;
      if((m_pDataBuffer[ulLocalOffset] == VC1_SEQ_START_CODE))
      {
        ulLocalOffset++;
        *pucPicType = VC1_I_FRAME_TYPE;
        bRet = true;
        break;
      }
      else if((m_pDataBuffer[ulLocalOffset] == VC1_FRAME_START_CODE))
      {
        ulLocalOffset++;
        *pucPicType = getBytesValue(1,&m_pDataBuffer[ulLocalOffset]);
        //Get last four bits
        *pucPicType = *pucPicType & 0x0F;
        bRet = true;
        break;
      }
       else
      {
        ulLocalOffset += 1;
      }
    }
    else
    {
      ulLocalOffset += 1;
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::findPicCodingTypeForFrame

DESCRIPTION:
  Returns type of frame.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::findPicCodingTypeForFrame(uint8* picType)
{
  uint32 localOffset = 0;
  uint8 val = 0;
  uint32 nBytesRead = m_nBytesRead;
  uint32 startcodeval = getBytesValue(3,m_pDataBuffer);
  bool bRet = false;

  while(localOffset < nBytesRead)
  {
    startcodeval = getBytesValue(3,&m_pDataBuffer[localOffset]);
    //make sure there is video prefix start code
    if(startcodeval == VIDEO_START_CODE_PREFIX)
    {
      localOffset+= 3;
      if(m_pDataBuffer[localOffset] == PICTURE_START_CODE)
      {
        localOffset++;
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],10,3);
        *picType = val;
        bRet = true;
        break;
      }
      else
      {
        localOffset += 1;
      }
    }
    else
    {
      localOffset += 1;
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::isFrameStartWithStartCode

DESCRIPTION:
  Returns if our current frame in buffer starts with start code

INPUT/OUTPUT PARAMETERS:
  nOffset[in/out]: Offset from which to look for start code in buffer
  trackId[in]: trackId of track to which frame belongs to
  buf[in]: buffer in which to look for frame
  nBytesRead[in]: length of frame
  startCodeType[out]: Type of start code


RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::isFrameStartWithStartCode(uint32* nOffset,uint32 trackId, uint8* buf, int32 nBytesRead, start_code_type* startCodeType)
{
  bool bRet = false;
  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;
  uint32 startcodeval_24bit = 0;
  uint32 startcodeval_32bit = 0;

  if( (MP2STREAM_SUCCESS== GetTrackType(trackId,&ttype,&medtype)) &&
      ((TRACK_TYPE_VIDEO == ttype) && (medtype == VIDEO_CODEC_H264)) )
  {
    while((int32)*nOffset < nBytesRead)
    {
      startcodeval_24bit = getBytesValue(3,&buf[*nOffset]);
      startcodeval_32bit = getBytesValue(4,&buf[*nOffset]);

      if(startcodeval_24bit == AVC_START_CODE_PREFIX_24BIT)
      {
        *startCodeType = AVC_START_CODE_24BIT;
        bRet = true;
        break;
      }
      else if(startcodeval_32bit == AVC_START_CODE_PREFIX_32BIT)
      {
        *startCodeType = AVC_START_CODE_32BIT;
        bRet = true;
        break;
      }
      else
      {
        (*nOffset)++;
      }
    }
  }
  else
  {
    bRet = true;
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::isFrameStartWithStartCode Not yet implemented");
  }

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::isFrameStartWithStartCode returning %d", bRet);
#endif
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::isAssembledAtFrameBoundary

DESCRIPTION:
  Returns if frames are at frame boundary

INPUT/OUTPUT PARAMETERS:
  trackId[in]: TrackID of the track to which frame belongs to
  buf[in]: Buffer in which to look for frame
  dataLen[out]: Length until new frame starts if not at frame boundary

RETURN VALUE:
 TRUE if assembled at Frame boundary
 FALSE otherwise

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::isAssembledAtFrameBoundary(uint32 trackId, uint8* buf,
                                                 uint32* dataLen, uint32 maxBufSize)
{
  bool bRet = false;
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  int counter = 0;
  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;
  uint8 newPayload = 0;
  if(!dataLen)
  {
    return false;
  }

  retError = GetTrackType(trackId, &ttype, &medtype);

  if(ttype == TRACK_TYPE_VIDEO)
  {
      while(buf)
      {
        uint32 index = 0;
        start_code_type startCodeType = START_CODE_DEFAULT;

        if(isSameStream(&trackId,&newPayload))
        {
          retError = parseTransportStreamPacket(trackId);
          if( (retError == MP2STREAM_SUCCESS) && (m_currTSPkt.PID == m_nVideoPIDSelected))
          {
            if(isFrameStartWithStartCode(&index, trackId, m_pDataBuffer, m_nBytesRead, &startCodeType))
            {
              if( (!index) && (!counter) )
              {
                m_nCurrOffset -= TS_PKT_SIZE;
                bRet = true;
                break;
              }
              else
              {
                if(maxBufSize < *dataLen + index)
                {
                  bRet = false;
                  m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
                  MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,"isAssembledAtFrameBoundary returning %d, dataLen %lu, maxBufSize %lu ",
                               retError, *dataLen + index, maxBufSize);
                  break;
                }
                int counterJump = getContinuityCounterJump(m_currTSPkt.continuity_counter);
                if(counterJump)
                {
                  m_nBytesLost += (uint32)(counterJump * TS_PKT_SIZE);
                  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"Packet loss at TS packet# %llu for track %lu",(m_currTSPkt.noffset/TS_PKT_SIZE),trackId);
                }
                m_nPrevCC = m_currTSPkt.continuity_counter;
                //Copy until startCode into prev frame
                memcpy(buf+(*dataLen),m_pDataBuffer,index);
                *dataLen += index;

                //Copy new frame data into member
                if(!m_pPartialFrameData)
                  m_pPartialFrameData = (partial_frame_data*)MM_Malloc(sizeof(partial_frame_data));
                if(m_pPartialFrameData)
                {
                  memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
                  m_pPartialFrameData->haveFrameData = true;
                  memcpy(m_pPartialFrameData->frameBuf,m_pDataBuffer+index,m_nBytesRead-index);
                  m_pPartialFrameData->len = (m_nBytesRead - index);
                  m_pPartialFrameData->dataTSPkt = m_currTSPkt;
                }

                bRet = true;
                break;
              }
            }
            else
            {
              int counterJump = getContinuityCounterJump(m_currTSPkt.continuity_counter);
              if(counterJump)
              {
                m_nBytesLost += (uint32)(counterJump * TS_PKT_SIZE);
                MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"Packet loss at TS packet# %llu for track %lu",(m_currTSPkt.noffset/188),trackId);
              }
              m_nPrevCC = m_currTSPkt.continuity_counter;
              counter++;
              memcpy(buf+(*dataLen),m_pDataBuffer,index);
              *dataLen += index;
            }
          }
          else
          {
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"isAssembledAtFrameBoundary returning %d", retError);
            break;
          }
        }
        else
        {
          if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"GetCurrentSample isSameStream MP2STREAM_DATA_UNDER_RUN");
            break;
          }
          //Skip this packet
          m_nCurrOffset += 188;
        }
      }
    }
    else
    {
    if( medtype == AUDIO_CODEC_AAC )
    {
      uint32 frame_len = 0;
      uint32 frame_len1 = 0;
      float frame_time = 0;
      float frame_time1 = 0;
      int index = 0;
      if(AAC_FORMAT_ADTS == GetAACAudioFormat(trackId))
      {
        retError = LocateAudioFrameBoundary(buf, &frame_len, &frame_time, &index, *dataLen);
        if(index)
        {
          uint8* tempBuf = (uint8*)MM_Malloc((*dataLen)-index);
          if(tempBuf)
          {
            // Freeing tempbuf both successfull and unsuccessfull
            if((*dataLen > (uint32)index))
            {
              memcpy(tempBuf,buf+index,(*dataLen)-index);
              memcpy(buf,tempBuf,(*dataLen)-index);
              *dataLen = (*dataLen)-index;
#ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                  "LocateAudioFrameBoundary discarded some data %lu",
                  *dataLen);
#endif
            }
            // to avoid resource leak in case dataLen is 0 in earlier condition.
            MM_Free(tempBuf);
          }
        }
        if( (frame_len > (*dataLen)) && (retError == MP2STREAM_SUCCESS) )
        {
          while(buf)
          {
            if(isSameStream(&trackId,&newPayload))
            {
              if(MP2STREAM_SUCCESS == parseTransportStreamPacket(trackId))
              {
                index = 0;
                uint8* tempBuf = m_pDataBuffer;
                /* If Partial frame data is available, then copy data into partial FrameBuf and do
                   further processing on that partial FrameBuf rather than on m_pDataBuffer*/
                if(m_pPartialFrameData && m_pPartialFrameData->haveFrameData)
                {
                  memcpy(m_pPartialFrameData->frameBuf+ m_pPartialFrameData->len, m_pDataBuffer, m_nBytesRead);
                  m_nBytesRead += m_pPartialFrameData->len;
                  // Update tempBuf with partial FrameBuf which has all the data
                  tempBuf = m_pPartialFrameData->frameBuf;
                  m_pPartialFrameData->haveFrameData = false;
                  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary partial data of %lu is copied",
                               m_pPartialFrameData->len);
                }
                if(m_currTSPkt.PID == m_nAudioPIDSelected)
                {
                  retError = LocateAudioFrameBoundary(tempBuf, &frame_len1, &frame_time1, &index, m_nBytesRead) ;
                  if(retError &&
                     ( (retError == MP2STREAM_SUCCESS) || (retError == MP2STREAM_EOF) ) )
                  {
                    if(maxBufSize < *dataLen + index)
                    {
                      bRet = false;
                      m_eParserState = MP2STREAM_INSUFFICIENT_MEMORY;
                      MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,"isAssembledAtFrameBoundary returning %d, dataLen %lu, maxBufSize %lu ",
                                   retError, *dataLen + index, maxBufSize);
                      break;
                    }
                    /* If frame length calculation is incorrect or proper frame sync word is not found for next
                       frame (frame sync word emulation), in that case copy the data required to fill current
                       frame and copy the remaining data into partialFrameBuf which can be discarded as part
                       of next "getNextMediaSample" request if frame sync word is not found. */
                    if(!index && m_nBytesRead + *dataLen > frame_len)
                    {
                      index = frame_len - *dataLen;
#ifdef MPEG2_PARSER_DEBUG
                      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"frame length calc went wrongly!! ");
                      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"frame length correction happened dataLen = %d ", index);
#endif
                    }
                    memcpy(buf+(*dataLen),tempBuf,index);
                    *dataLen += index;

                    if(!m_pPartialFrameData)
                      m_pPartialFrameData = (partial_frame_data*)MM_Malloc(sizeof(partial_frame_data));
                    if(m_pPartialFrameData)
                    {
                      /* If tempBuf is pointing to partial FrameBuf only, then we will copy data
                         into another buffer to avoid data losing.*/
                      if(tempBuf == m_pPartialFrameData->frameBuf)
                      {
                        memcpy(m_pDataBuffer, m_pPartialFrameData->frameBuf, m_nBytesRead);
                      }
                      memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
                      m_pPartialFrameData->haveFrameData = true;
                      memcpy(m_pPartialFrameData->frameBuf,m_pDataBuffer+index,m_nBytesRead-index);
                      m_pPartialFrameData->len = (m_nBytesRead - index);
                      m_pPartialFrameData->dataTSPkt = m_currTSPkt;
                    }
                    if(*dataLen == frame_len)
                    {
                      bRet = true;
                      break;
                    }
                  }
                }
              }
              else
              {
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"isAssembledAtFrameBoundary returning %d", retError);
                break;
              }
            }
            else
            {
              if(m_eParserState == MP2STREAM_DATA_UNDER_RUN)
              {
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"GetCurrentSample isSameStream MP2STREAM_DATA_UNDER_RUN");
                break;
              }
              else if(m_eParserState == MP2STREAM_EOF)
              {
                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"GetCurrentSample isSameStream MP2STREAM_EOF");
                break;
              }
              //Skip this packet
              m_nCurrOffset += 188;
            }
          }
        }
        else
        {
          if(!m_pPartialFrameData)
            m_pPartialFrameData = (partial_frame_data*)MM_Malloc(sizeof(partial_frame_data));
          if(m_pPartialFrameData)
          {
            memset(m_pPartialFrameData,0,sizeof(partial_frame_data));
            m_pPartialFrameData->haveFrameData = true;
            memcpy(m_pPartialFrameData->frameBuf,buf+frame_len,((*dataLen)-frame_len));
            m_pPartialFrameData->len = ((*dataLen) - frame_len);
            m_pPartialFrameData->dataTSPkt = m_currTSPkt;
          }
          *dataLen = frame_len;
          bRet = true;
        }
      }
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"isAssembledAtFrameBoundary not AAC ADTS");
      bRet = true;
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  MP2StreamParser::getPidForTrackId

DESCRIPTION:
  Returns the PID for the current track

INPUT/OUTPUT PARAMETERS:
  trackId[in]: TrackID of the track for which PID should be output
  pid[out]: PID for the given track id

RETURN VALUE:
 TRUE if PID found
 FALSE otherwise

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::getPidForTrackId(uint32 trackId, uint16* pid)
{
  bool bRet = false;
  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;

  retError = GetTrackType(trackId, &ttype, &medtype);
  if((retError == MP2STREAM_SUCCESS) && (pid))
  {
    if(ttype == TRACK_TYPE_AUDIO)
    {
      *pid = m_nAudioPIDSelected;
      bRet = true;
    }
    else if(ttype == TRACK_TYPE_VIDEO)
    {
      *pid = m_nVideoPIDSelected;
      bRet = true;
    }
  }
  return bRet;
}/*! ======================================================================
@brief  Finds the audio frame boundary

@detail    Seeks given track in forward/backward direction to specified time

@param[in]
 trackid: Identifies the track to be repositioned.
 nReposTime:Target time to seek to
 nCurrPlayTime: Current playback time
 sample_info: Sample Info to be filled in if seek is successful
 canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
 nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

@return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::LocateAudioFrameBoundary(uint8* buf, uint32* frame_len, float* frame_time, int* index, uint32 dataLen)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  uint16 uData = 0;
  uint64 frameLength = 0;

  if( (!buf)|| (!frame_len) || (!frame_time) || (!index))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary MP2STREAM_INVALID_PARAM");
    retError = MP2STREAM_INVALID_PARAM;
    return retError;
  }

  if(m_nCurrOffset == m_nFileSize)
  {
    retError = MP2STREAM_EOF;
  }
  while( (index) && ((uint32)((*index)+5) < dataLen))
  {
    uData = (buf[(*index)+1] << 8) + buf[(*index)+0];
    // Verify sync word and layer field.
    if (ADTS_HEADER_MASK_RESULT == (uData & ADTS_HEADER_MASK))
    {
      /* These two additional checks will ensure that frame sync word validation more robust*/
      const uint8 audio_object = ((buf[(*index)+2] >> 6) & 0x03)+ 1;
      if(audio_object > AAC_MAX_AUDIO_OBJECT)/*only 5 possible values*/
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary audio_object>AAC_MAX_AUDIO_OBJECT");
        (*index)++;
        continue;
      }

      const uint8 channelConfiguration = ((buf [(*index)+2] << 2) & 0x04)| ((buf [(*index)+3] >> 6) & 0x03);
      if(channelConfiguration > AAC_MAX_CHANNELS) /*only 48 possible values*/
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary channelConfiguration>AAC_MAX_CHANNELS");
       (*index)++;
        continue;
      }
      frameLength = (static_cast<uint64> (buf[(*index)+3] & 0x03) << 11)
                   | (static_cast<uint64> (buf[(*index)+4]) << 3)
                   | (static_cast<uint64> (buf[(*index)+5] & 0xE0) >> 5);

      if (frameLength)
      {
        // parser framework handles max frame length of 32 bits
        *frame_len = (uint32)(frameLength & 0xFFFF);

        if(*frame_len > 1500)
        {
          (*index)++;
          MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,"LocateAudioFrameBoundary frame_len is %lu, index is %d, dataLen is %lu",
                       *frame_len, *index, dataLen);
          *frame_len = 0;
          continue;
        }

        const uint8 samplingFrequencyIndex = ((buf[(*index)+2] >> 2) & 0x0F);

        if(AAC_SAMPLING_FREQUENCY_TABLE[samplingFrequencyIndex])
          *frame_time = ((float)AAC_SAMPLES_PER_DATA_BLOCK * 1000)/
                                    (float)AAC_SAMPLING_FREQUENCY_TABLE[samplingFrequencyIndex];

        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_LOW,"frameLength %lu, frame_time %f", *frame_len, *frame_time);
        retError = MP2STREAM_SUCCESS;
        break;
      }
      else
      {
        (*index)++;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"LocateAudioFrameBoundary MP2STREAM_CORRUPT_DATA");
        retError =  MP2STREAM_CORRUPT_DATA;
      }
    }
    else
    {
      (*index)++;
    }
  }
  if(!(*frame_len))
  {
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"framesync word not found: bytesparsed: %d, bytesAvailable %lu",
                 *index, dataLen);
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::getPSTimestamp()

DESCRIPTION:
  Output timestamp for Program stream sample when not present

INPUT/OUTPUT PARAMETERS:
  pesPTS[in]: Timestamp from the pes packet
  curFrameDuration[in]: current frame duration

RETURN VALUE:
  Returns timestamp

SIDE EFFECTS:
  None.
===========================================================================*/
float MP2StreamParser::getPSTimestamp(float pesPTS, float curFrameDuration)
{

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"MP2StreamParser::getTimestamp");

  float calculatedPTS = (float)m_nTotalProgStreamDuration;

  m_nTotalProgStreamDuration += curFrameDuration;

  if(pesPTS && (pesPTS != calculatedPTS) && (pesPTS != m_nPrevPESTime) && (m_nPrevPESTime) )
  {
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"corrected PS timestamp from %f to %f", calculatedPTS, pesPTS);
    calculatedPTS = pesPTS;
    if(!m_nAudioSeekRefPTS)//Not Seek, during playback
    {
      m_nTotalProgStreamDuration = (pesPTS) + curFrameDuration;
    }
  }
  m_nPrevPESTime = pesPTS;

  return calculatedPTS;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::getADTSTimestamp()

DESCRIPTION:
  Out timestamp for the AAC ADTS sample

INPUT/OUTPUT PARAMETERS:
  pesPTS[in]: Timestamp from the pes packet
  curFrameDuration[in]: current frame duration

RETURN VALUE:
  Returns timestamp

SIDE EFFECTS:
  None.
===========================================================================*/
float MP2StreamParser::getADTSTimestamp(float pesPTS, float curFrameDuration)
{

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"MP2StreamParser::getADTSTimestamp");

  float calculatedPTS = (float)m_nTotalADTSTrackDuration;

  m_nTotalADTSTrackDuration += curFrameDuration;

  //Getting Zero based timestamps
  if(!m_nAudioSeekRefPTS)
  {
    calculatedPTS = (float)((m_nFirstAudioPTS - m_nRefAudioPTS) + calculatedPTS);
  }
  else
  {
    calculatedPTS = (float)(m_nAudioSeekRefPTS + calculatedPTS);
  }

  if(!pesPTS && m_nPrevPESTime)
  {
    // Case where there is a missing TS in PES
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "TS missing in PES Packet");
  }
  else if((pesPTS != calculatedPTS) && (pesPTS != m_nPrevPESTime) &&
          (m_nPrevPESTime) )
  {
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"corrected ADTS TS from %f to %f", calculatedPTS, pesPTS);
    calculatedPTS = pesPTS;
    if(!m_nAudioSeekRefPTS)//Not Seek, during playback
    {
      if(m_nFirstAudioPTS == m_nRefAudioPTS)//No switching has occurred
      {
        m_nTotalADTSTrackDuration = (pesPTS) + curFrameDuration;
      }
      else
      {
        //Get ADTSDuration played so far(after switch)
        m_nTotalADTSTrackDuration = ((pesPTS + m_nRefAudioPTS) - m_nFirstAudioPTS) + curFrameDuration;
      }
    }
    else
    {
      //Seek scenario
      m_nTotalADTSTrackDuration = (pesPTS - m_nAudioSeekRefPTS) + curFrameDuration;
    }
  }
  m_nPrevPESTime = pesPTS;

  return calculatedPTS;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::CorrectTSDiscontinuity()

DESCRIPTION:
  Correct if timestamps have discontinuity

INPUT/OUTPUT PARAMETERS:
  trackId[in]: TrackID for which we are correcting for discontinuity

RETURN VALUE:
  VOID

SIDE EFFECTS:
  None.
===========================================================================*/
void MP2StreamParser::correctTSDiscontinuity(uint32 trackId)
{
  MP2StreamStatus retError = MP2STREAM_INVALID_PARAM;
  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;

  float currDelta = getSampleDelta(trackId);
  retError = GetTrackType(trackId,&ttype,&medtype);

  if(retError == MP2STREAM_SUCCESS)
  {
    if( ( (m_sampleInfo.delta) && (currDelta) &&
          ( ((currDelta) > (PES_PTS_DELTA_DISCONTINUITY * m_sampleInfo.delta)) ||//if delta is 100 times more than prev
            ((currDelta) < (m_sampleInfo.delta/PES_PTS_DELTA_DISCONTINUITY)) ) ) ||
            (m_currTSPkt.adaption_field.discontinuity_indicator))
    {
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"Discontinuity found for track %d",(int)trackId);

        double time = 0;

        /* We use a static member variable to set the same basetime across all selected tracks
           to maintain AV sync */

        if(!MP2StreamParser::m_nRefAfterDisc)
        {
          time = m_currPESPkt.pts - (m_sampleInfo.ntime + m_sampleInfo.delta);
          MP2StreamParser::m_nRefAfterDisc = time;
        }
        else
        {
          time = MP2StreamParser::m_nRefAfterDisc;
        }
        MP2StreamParser::m_nTimesRefUsed++;

        //Reset static member when baseTime is set for all selected tracks
        if(MP2StreamParser::m_nTimesRefUsed == m_nStreamsSelected)
        {
          MP2StreamParser::m_nRefAfterDisc = 0;
          MP2StreamParser::m_nTimesRefUsed = 0;
        }

        //Set our references to new value
        m_nTotalADTSTrackDuration = m_sampleInfo.ntime + m_sampleInfo.delta;
        m_nRefAudioPTS = time;
        m_nFirstAudioPTS = time;
        m_nRefVideoPTS = time;
        m_sampleInfo.ntime = 0;
        m_sampleInfo.delta = 0;

        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"Setting BaseTime %d for track %d due to Discontinuity",(int)time,(int)trackId);
      }
    }
    else
    {
      /* No Discontinuity found */
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"NO Discontinuity");
    }
  }
}

/*===========================================================================
FUNCTION:
  MP2StreamParser::getSampleDelta()

DESCRIPTION:
  Gets the delta between frames

INPUT/OUTPUT PARAMETERS:
  trackId[in]: TrackID for which will find delta

RETURN VALUE:
  Returns Delta in uint32

SIDE EFFECTS:
  None.
===========================================================================*/
float MP2StreamParser::getSampleDelta(uint32 trackId)
{
  double currPTS = 0;
  float  currDelta = 0;
  double baseTime = 0;

  if(GetBaseTime(trackId,&baseTime))
  {
    currPTS = m_currPESPkt.pts - baseTime;
    if(m_sampleInfo.ntime)
    {
      currDelta = fabs((float)currPTS - (float)m_sampleInfo.ntime);
    }
  }

  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"getSampleDelta currDelta %f",currDelta);
  return currDelta;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::getContinuityCounterJump()

DESCRIPTION:
  Gets the jump in continuity counter if any

INPUT/OUTPUT PARAMETERS:
  presentCounter[in]: counter value of current TS pkt

RETURN VALUE:
  Returns jump in counter in int

SIDE EFFECTS:
  None.
===========================================================================*/
int MP2StreamParser::getContinuityCounterJump(uint8 presentCounter)
{
  int counterJump = -1;

  if( (m_nPrevCC > MAX_CONTINUITY_COUNTER_VALUE) ||
      (presentCounter > MAX_CONTINUITY_COUNTER_VALUE) )
  {
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"getContinuityCounterJump ERROR prevCounter %d, presentCounter %d",m_nPrevCC,presentCounter);
  }
  else
  {
    if(m_nPrevCC == -1)
    {
      //We come here for the 1st TS pkt
      counterJump = 0;
    }
    else
    {
      /* TS packet parsed, increment to next to check for discontinuities */
      int prevC = m_nPrevCC + 1;

      /* No TS packets lost if counter increments by 1 */
      counterJump = presentCounter - prevC;

      if(counterJump < 0) /* support wrap around 0-15 */
      {
        counterJump += 16;
      }
    }
  }
  return counterJump;
}
/* ======================================================================
FUNCTION:
  SetAudioOutputMode

DESCRIPTION:
  Called by user to set output mode specified by henum

INPUT/OUTPUT PARAMETERS:
  henum-Output mode

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in setting output mode else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
FileSourceStatus MP2StreamParser::SetAudioOutputMode(FileSourceConfigItemEnum henum)
{
  FileSourceStatus status = FILE_SOURCE_FAIL;
  /*
  * FILE_SOURCE_MEDIA_OUTPUT_BYTES_STREAM means there won't be any processing done on data read from AU.
  */
  if((henum == FILE_SOURCE_MEDIA_OUTPUT_BYTES_STREAM)||
     (henum == FILE_SOURCE_MEDIA_OUTPUT_SINGLE_AUDIO_FRAME))
  {
    m_hFrameOutputModeEnum = henum;
    status = FILE_SOURCE_SUCCESS;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "SetAudioOutputMode AudioOutputMode %d",henum);
  }
  else if( (henum == FILE_SOURCE_MEDIA_STRIP_AUDIO_HEADER)||
           (henum == FILE_SOURCE_MEDIA_RETAIN_AUDIO_HEADER) )
  {
    m_hHeaderOutputModeEnum = henum;
    status = FILE_SOURCE_SUCCESS;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "SetAudioOutputMode HeaderOutputMode %d",henum);
  }
  else
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL, "SetAudioOutputMode failed for configItem %d",henum);
  }
  return status;
}
/* ======================================================================
FUNCTION:
  GetAudioOutputMode

DESCRIPTION:
  Called by user to check what audio output mode is set

INPUT/OUTPUT PARAMETERS:
  bret - FILE_SOURCE_SUCCESS or FILE_SOURCE_FAIL
  henum - FileSourceConfigItemEnum

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in retrieving output mode else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
FileSourceStatus MP2StreamParser::GetAudioOutputMode(bool* bret, FileSourceConfigItemEnum henum)
{
  FileSourceStatus status = FILE_SOURCE_FAIL;
  //if our mode(frame/header) is same as henum,
  if(bret && ( (henum == m_hFrameOutputModeEnum)||
               (henum == m_hHeaderOutputModeEnum) ) )
  {
    status = FILE_SOURCE_SUCCESS;
    *bret = true;
  }
  else
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM, "GetAudioOutputMode configItem %d not set...",henum);
  }
  return status;
}
/* ======================================================================
FUNCTION:
  SetConfiguration

DESCRIPTION:
  Called by user to set output mode specified by henum

INPUT/OUTPUT PARAMETERS:
  henum-Output mode

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in setting output mode else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
FileSourceStatus MP2StreamParser::SetConfiguration(FileSourceConfigItemEnum henum)
{
  FileSourceStatus status = FILE_SOURCE_FAIL;
  if( (henum == FILE_SOURCE_MEDIA_ENABLE_TS_DISCONTINUITY_CORRECTION) ||
      (henum == FILE_SOURCE_MEDIA_DISABLE_TS_DISCONTINUITY_CORRECTION) )
  {
    // Do TimeStamp continuity correction only for TS clips
    if (!m_bProgramStream)
    {
      m_hTSDiscCorrectModeEnum = henum;
    }
    status = FILE_SOURCE_SUCCESS;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "SetConfiguration OutputMode %d",henum);
  }
  else
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL, "SetConfiguration failed for configItem %d",henum);
  }
  return status;
}
/* ======================================================================
FUNCTION:
  backupInUnderrunBuffer

DESCRIPTION:
  Called by user to check what audio audio output mode is set

INPUT/OUTPUT PARAMETERS:
  dataBuffer - buffer to backup data from
  bytesToCopy - number of bytes backed up

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in retrieving output mode else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
MP2StreamStatus MP2StreamParser::backupInUnderrunBuffer(uint8* dataBuffer, uint32 bytesToCopy, uint32 nPESLen)
{
  MP2StreamStatus retStatus = MP2STREAM_DATA_UNDER_RUN;

  if(bytesToCopy)
  {
    if(m_UnderrunBuffer.nBufSize < bytesToCopy)
    {
      if(m_UnderrunBuffer.pFrameBuf)
        MM_Free(m_UnderrunBuffer.pFrameBuf);
      m_UnderrunBuffer.pFrameBuf = (uint8*)MM_Malloc(bytesToCopy + 1);
      if(!m_UnderrunBuffer.pFrameBuf)
      {
        return MP2STREAM_INSUFFICIENT_MEMORY;
      }
      m_UnderrunBuffer.nBufSize = bytesToCopy + 1;
      m_UnderrunBuffer.nDataLen = 0;
    }
    memcpy(m_UnderrunBuffer.pFrameBuf,dataBuffer,bytesToCopy);
    m_UnderrunBuffer.nDataLen = bytesToCopy;
    m_UnderrunBuffer.bytesLost = m_nBytesLost;
    m_UnderrunBuffer.nPESLen = nPESLen;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"Copied %lu bytes into underrun buffer",bytesToCopy);
  }
  m_eParserState = MP2STREAM_READY;
  return retStatus;
}
/* ======================================================================
FUNCTION:
  restoreFromUnderrunBuffer

DESCRIPTION:
  Restores previously assembled data from underrun buffer if any

INPUT/OUTPUT PARAMETERS:
  dataBuffer - buffer into which data is restored

RETURN VALUE:
 bytesRestored - number of bytes restored

SIDE EFFECTS:
  None.
======================================================================*/
uint32 MP2StreamParser::restoreFromUnderrunBuffer(uint8* dataBuffer, uint32* pPESLen)
{
  uint32 bytesRestored = 0;

  memcpy(dataBuffer,m_UnderrunBuffer.pFrameBuf,m_UnderrunBuffer.nDataLen);
  bytesRestored = m_UnderrunBuffer.nDataLen;
  m_UnderrunBuffer.nDataLen = 0;
  m_nBytesLost = m_UnderrunBuffer.bytesLost;
  *pPESLen = m_UnderrunBuffer.nPESLen;
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"Restored %lu bytes into sample buffer",bytesRestored);

  return bytesRestored;
}
/* ======================================================================
FUNCTION:
  GetPesPvtData

DESCRIPTION:
  Return PES pvt data

INPUT/OUTPUT PARAMETERS:
  trackId - track id to get pes pvt data for
  extraData - extra_data_struct

RETURN VALUE:
  infoBuf - buffer into which pes pvt data is copied

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::GetPesPvtData(uint32 trackId, uint8* pPvtData)
{
  bool bRet = false;
  if( ( NULL != pPvtData ) &&
      ( m_currPESPkt.tsPID == trackId ) &&
      ( m_currPESPkt.pes_extn_flag) &&
      ( m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag))
  {
      memcpy( pPvtData,m_currPESPkt.pes_extn_hdr.pes_pvt_data,
              PES_EXTN_PVT_DATA_LEN );
      //Reset pes pvt data buffer. pes pvt data buffer will have
      //stale data if in a stream following sequence followed:
      //encryption-clear-encryption
      memset(m_currPESPkt.pes_extn_hdr.pes_pvt_data, 0, PES_EXTN_PVT_DATA_LEN);
      bRet = true;
  }
  return bRet;
}

/* ======================================================================
FUNCTION:
  IsDRMProtection

DESCRIPTION:
  This API tells if clip is DRM protected or not

INPUT/OUTPUT PARAMETERS:
  None
RETURN VALUE:
  TRUE- Clip DRM protected
  FALSE- Clip is not protected by any DRM scheme

SIDE EFFECTS:
  None.
======================================================================*/
bool MP2StreamParser::IsDRMProtection()
{
  bool bDRMProtected = false;
  // if PES PVT DATA FLAG present in PES Header, then PES PVT DATA field
  // (16Bytes)will carry HDCP2.0/2.1 counter bits and marker bits
  if( m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag )
  {
    bDRMProtected = true;
  }
  return bDRMProtected;
}
/*! ======================================================================
@brief  Repositions given track after skipping given number of key frames

@detail    Seeks given track in forward/backward direction by given num
           of key frames.

@param[in]
 trackid: Identifies the track to be repositioned.
 sample_info: Sample Info to be filled in if seek is successful
 nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

@return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::SkipNSyncSamples(uint32 trackid,
                                      mp2_stream_sample_info* sample_info,
                                      int  nSyncFramesToSkip)
{
  MP2StreamStatus retStatus = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = false;
  uint8 val = 0;
  uint8 newPayload = 0;
  uint8 tempBuffer[TS_PKT_SIZE];
  uint32 nAdptField = 0;
  uint8 nAdaptionFieldLen = 0;
  uint16 pid =0xFFFF, currentPid = 0;
  bool nRet = false;
  uint64 availOffset = m_availOffset;
  int nSyncFramesSkipped = 0;
  track_type ttype = TRACK_TYPE_UNKNOWN;
  media_codec_type medtype = UNKNOWN_AUDIO_VIDEO_CODEC;
  uint64 pts = 0;

  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample nSyncFramesToSkip %d",nSyncFramesToSkip);

  retStatus = GetTrackType(trackid,&ttype,&medtype);

  if((retStatus == MP2STREAM_SUCCESS) && sample_info)
  {
    bContinue = true;
  }

  if(m_bProgramStream)
  {
    if(nSyncFramesToSkip < 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample don't support for RW in PS");
      return MP2STREAM_FAIL;
    }
    while(bContinue && (m_nCurrOffset < m_nFileSize))
    {
      int localOffset = 0;
      if(!readMpeg2StreamData (m_nCurrOffset,  m_nDataBufferSize,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
      {
        retStatus = m_eParserState;
        bContinue = false;
      }
      uint32 startCodeVal = getBytesValue( sizeof (uint32),m_pDataBuffer);
      uint32 val = 0;
      uint64 startOffset = m_nCurrOffset;
      if (startCodeVal == MP2_PACK_START_CODE)
      {
        /* We dont need PACKs, we will parse to skip it */
        localOffset += 4;
        localOffset += 9;
        int nStuffingLen = m_pDataBuffer[localOffset] & (0x07);
        localOffset++;
        localOffset += nStuffingLen;
        if(!readMpeg2StreamData (m_nCurrOffset+localOffset, sizeof(MP2_SYS_HDR_START_CODE),
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData) )
        {
          retStatus = m_eParserState;
          bContinue = false;
        }
        else
        {
          uint32 startCodeVal = getBytesValue(sizeof(uint32),m_pDataBuffer);
          if( startCodeVal == MP2_SYS_HDR_START_CODE)
          {
            localOffset += sizeof(MP2_SYS_HDR_START_CODE);
            int nHdrLength = ((uint16)(m_pDataBuffer[localOffset])<<8) | (uint16)m_pDataBuffer[localOffset+1];
            localOffset += nHdrLength;
          }
          m_nCurrOffset += localOffset;
        }
      }
      else if ( isPESPacket(m_pDataBuffer,&val))
      {
        if(!readMpeg2StreamData (m_nCurrOffset,  PES_PKT_START_CODE_STREAM_ID_SIZE,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          retStatus = m_eParserState;
          bContinue = false;
        }
        localOffset += 4;
        uint32 nPeslength = ((uint32)(m_pDataBuffer[localOffset])<<8) | (uint32)m_pDataBuffer[localOffset+1];
        localOffset += 2;
        m_nCurrOffset += PES_PKT_START_CODE_STREAM_ID_SIZE;

        if(val == PADDING_STREAM_ID)
        {
          m_nCurrOffset += nPeslength;
        }
        else
        {
          if(!readMpeg2StreamData (m_nCurrOffset,  PES_PKT_FIXED_HDR_BYTES,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
          {
            retStatus = m_eParserState;
            bContinue = false;
          }
          localOffset = 0;
          int nOptionalHdrLen = 0;
          int nDataAlignIndicator = (m_pDataBuffer[localOffset++] & 0x04)>>2;
          uint8 nPTSDTSFlag = (m_pDataBuffer[localOffset++] & 0xC0) >> 6;
          int nPesHdrLen = m_pDataBuffer[localOffset++];
          m_nCurrOffset += PES_PKT_FIXED_HDR_BYTES;
          nPeslength -= PES_PKT_FIXED_HDR_BYTES;
          if(nPTSDTSFlag == PES_PKT_PTS_DTS_FLAG)
          {
            /* We will pass PES_PKT_PTS_FLAG as flag since we don't need
               to parse the DTS flag */
            retStatus = parsePTS(m_nCurrOffset,PES_PKT_PTS_FLAG, &pts);
            m_nCurrOffset += PES_PKT_PTS_DTS_BYTES;
            nPesHdrLen -= PES_PKT_PTS_DTS_BYTES;
            nPeslength -= PES_PKT_PTS_DTS_BYTES;
          }
          else if(nPTSDTSFlag == PES_PKT_PTS_FLAG)
          {
            retStatus = parsePTS(m_nCurrOffset,PES_PKT_PTS_FLAG, &pts);
            m_nCurrOffset += PES_PKT_PTS_BYTES;
            nPesHdrLen -= PES_PKT_PTS_BYTES;
            nPeslength -= PES_PKT_PTS_BYTES;
          }
          if(nPesHdrLen)
          {
            m_nCurrOffset+= nPesHdrLen;
          }

          if( (nDataAlignIndicator) && (val == trackid) &&
               ( ((m_currPESPkt.pts_dts_flag & 0x03) == 0x03) || ((m_currPESPkt.pts_dts_flag & 0x02) == 0x02)) )
          {
            if(readMpeg2StreamData(m_nCurrOffset, nPeslength,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
            {
              m_nBytesRead = nPeslength;
              uint8 picType = 0;
              bool foundFrameType = false;
              if( (ttype == TRACK_TYPE_VIDEO) && (medtype == VIDEO_CODEC_MPEG2) )
              {
                foundFrameType = findPicCodingTypeForFrame(&picType);
                if( (foundFrameType) && (picType == MPEG2_I_FRAME_TYPE) )
                {
                  nSyncFramesSkipped++;
                  if(nSyncFramesSkipped == nSyncFramesToSkip)
                  {
                    m_nCurrOffset = startOffset;
                    sample_info->bsync = 1;
                    sample_info->noffset = m_nCurrOffset;
                    sample_info->ntime = (float)pts;
                    retStatus = MP2STREAM_SUCCESS;
                    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample returning pts %llu",pts);
                    break;
                  }
                  m_nCurrOffset += nPeslength;
                }
                else
                {
                  m_nCurrOffset += nPeslength;
                }
              }
              else if( (ttype == TRACK_TYPE_VIDEO) && (medtype == VIDEO_CODEC_H264) )
              {
                foundFrameType = findH264NALTypeForFrame(&picType);
                if( (foundFrameType) && (picType == NAL_UNIT_TYPE_IDR) )
                {
                  nSyncFramesSkipped++;
                  if(nSyncFramesSkipped == nSyncFramesToSkip)
                  {
                    m_nCurrOffset = startOffset;
                    sample_info->bsync = 1;
                    sample_info->noffset = m_nCurrOffset;
                    sample_info->ntime = (float)pts;
                    retStatus = MP2STREAM_SUCCESS;
                    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample returning pts %llu",pts);
                    break;
                  }
                  m_nCurrOffset += nPeslength;
                }
                else
                {
                  m_nCurrOffset += nPeslength;
                }
              }
            }
            else
            {
              retStatus = m_eParserState;
              bContinue = false;
            }
          }
          else
          {
            m_nCurrOffset += nPeslength;
          }
        }
      }
      else
      {
        m_nCurrOffset++;
      }
    }
  }
  else
  {
    bool bForward = true;
    bool bUpdateOffset = false;
    if(nSyncFramesToSkip < 0)
    {
      bForward = false;
    }
    while (m_nCurrOffset && (m_nCurrOffset < m_nFileSize))
    {
      uint64 startOffset = m_nCurrOffset;
      int localOffset = 0;
      int nHeaderBytes = 0;
      if(readMpeg2StreamData (m_nCurrOffset,  (TS_INIT_BYTES+1),
                               tempBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
      {
        if(memcmp(tempBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"SkipNSyncSamples Sync byte(0x47) not found!!");
          retStatus = MP2STREAM_CORRUPT_DATA;
          bContinue = false;
        }
        if(bContinue)
        {
          getByteFromBitStream(&val,&tempBuffer[1],1,1);
          newPayload = val;
          if(newPayload)
          {
            pid = (tempBuffer[1] & 0x1F)<< 8;
            pid |=  tempBuffer[2];
            getByteFromBitStream(&val,&tempBuffer[3],2,2);
            nAdptField = val;

            getByteFromBitStream(&val,&tempBuffer[localOffset+TS_PKT_HDR_BYTES],0,8);
            nAdaptionFieldLen = val;

            bContinue = getPidForTrackId(trackid,&currentPid);

            if( bContinue && (pid == currentPid) )
            {
              localOffset += TS_PKT_HDR_BYTES;
              nHeaderBytes += TS_PKT_HDR_BYTES;
              if(nAdptField == TS_ADAPTION_FILED_DATA_PRSENT)
              {
                localOffset = localOffset + 1 + nAdaptionFieldLen;
                nHeaderBytes = localOffset + 1 + nAdaptionFieldLen;
              }
              m_nCurrOffset += localOffset;
              if(readMpeg2StreamData(m_nCurrOffset, TS_PKT_SIZE-localOffset,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
              {
                uint32 valCode = 0;
                if( (localOffset < TS_PKT_SIZE) &&
                    isPESPacket( m_pDataBuffer,&valCode) )
                {
                  if(valCode == trackid)
                  {
                    localOffset = 8;
                    nHeaderBytes += 8;
                    int pes_hdr_data_length = m_pDataBuffer[localOffset];
                    retStatus = parsePTS(m_nCurrOffset+localOffset+1,PES_PKT_PTS_FLAG, &pts);
                    localOffset += pes_hdr_data_length + 1; //1 byte for pes_hdr_data_length
                    nHeaderBytes += pes_hdr_data_length + 1;
                    memmove(m_pDataBuffer,m_pDataBuffer+localOffset,(TS_PKT_SIZE-nHeaderBytes));
                    m_nBytesRead = TS_PKT_SIZE-nHeaderBytes;
                    if(localOffset < TS_PKT_SIZE)
                    {
                      uint8 picType = 0;
                      bool foundFrameType = false;
                      if( (ttype == TRACK_TYPE_VIDEO) && (medtype == VIDEO_CODEC_MPEG2) )
                      {
                        foundFrameType = findPicCodingTypeForFrame(&picType);
                        if( (foundFrameType) && (picType == MPEG2_I_FRAME_TYPE) )
                        {
                          if(bForward)
                            nSyncFramesSkipped++;
                          else
                            nSyncFramesSkipped--;
                          if(nSyncFramesSkipped == nSyncFramesToSkip)
                          {
                            memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
                            m_nCurrOffset = startOffset;
                            sample_info->bsync = 1;
                            sample_info->noffset = m_nCurrOffset;
                            sample_info->ntime = (float)pts;
                            retStatus = MP2STREAM_SUCCESS;
                            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample returning pts %llu",pts);
                            break;
                          }
                          bUpdateOffset = true;
                        }
                        else
                        {
                          bUpdateOffset = true;
                        }
                      }
                      else if( (ttype == TRACK_TYPE_VIDEO) && (medtype == VIDEO_CODEC_H264) )
                      {
                        foundFrameType = findH264NALTypeForFrame(&picType);
                        if( (foundFrameType) && (picType == NAL_UNIT_TYPE_IDR) )
                        {
                          if(bForward)
                            nSyncFramesSkipped++;
                          else
                            nSyncFramesSkipped--;
                          if(nSyncFramesSkipped == nSyncFramesToSkip)
                          {
                            memset(&m_sampleInfo,0,sizeof(mp2_stream_sample_info));
                            m_nCurrOffset = startOffset;
                            sample_info->bsync = 1;
                            sample_info->noffset = m_nCurrOffset;
                            sample_info->ntime = (float)pts;
                            retStatus = MP2STREAM_SUCCESS;
                            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"MP2StreamParser::SkipNSyncSample returning pts %llu",pts);
                            break;
                          }
                          bUpdateOffset = true;
                        }
                        else
                        {
                          bUpdateOffset = true;
                        }
                      }
                    }
                  }
                  else
                  {
                    bUpdateOffset = true;
                  }
                }
                else
                {
                  bUpdateOffset = true;
                }
              }
              else
              {
                retStatus = m_eParserState;
                bContinue = false;
              }
            }
            else
            {
              bUpdateOffset = true;
            }
          }
          else if(isPSI(pid))
          {
            retStatus = parseTransportStreamPacket(trackid);
            if(retStatus == MP2STREAM_SUCCESS)
            {
              bContinue = true;
            }
            else
            {
              break;
            }
          }
          else
          {
            bUpdateOffset = true;
          }
        }
        else
        {
          break;
        }
      }
      else
      {
        retStatus = m_eParserState;
        bContinue = false;
      }
      if(bUpdateOffset)
      {
        if(bForward)
        {
          m_nCurrOffset = startOffset + TS_PKT_SIZE;
        }
        else
        {
          if(startOffset >= TS_PKT_SIZE)
            m_nCurrOffset = startOffset - TS_PKT_SIZE;
          else
          {
            m_nCurrOffset = 0;
          }
        }
      }
    }
  }
  return retStatus;
}
/*! ======================================================================
@brief  Repositions given track to specified time

@detail    Seeks given track in forward/backward direction to specified time

@param[in]
 trackid: Identifies the track to be repositioned.
 nReposTime:Target time to seek to
 nCurrPlayTime: Current playback time
 sample_info: Sample Info to be filled in if seek is successful
 canSyncToNonKeyFrame: When true, video can be repositioned to non key frame
 nSyncFramesToSkip:Number of sync samples to skip. Valid only if > or < 0.

@return    MP2STREAM_SUCCESS if successful other wise returns MP2STREAM_FAIL
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parsePTS(uint64 offset,uint8 ptsFlags,uint64* pts,uint64* dts)
{
  int localOffset = 0;
  uint8 tempBuffer[PES_PKT_PTS_DTS_BYTES+1];
  MP2StreamStatus retStatus = MP2STREAM_DEFAULT_ERROR;
  if(pts)
  {
    if(!readMpeg2StreamData (offset, PES_PKT_PTS_DTS_BYTES,
                                       tempBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
    {
      retStatus = m_eParserState;
    }
    else
    {
      uint8 firstpart =  (tempBuffer[localOffset] & 0x0E) << 4;
      localOffset++;
      uint16 secondpart = ( ( (uint16)(tempBuffer[localOffset]) ) << 8)  | tempBuffer[localOffset+1];
      secondpart = (secondpart & 0xFFFE)>>1;
      localOffset+=2;
      uint16 thirdpart = ( ( (uint16)(tempBuffer[localOffset]) ) << 8)  | tempBuffer[localOffset+1];
      thirdpart = (thirdpart & 0xFFFE)>>1;
      localOffset+=2;
      *pts = make33BitValue(firstpart,secondpart,thirdpart);
      *pts = (uint64)((*pts)/90.0);//90 KHz clock

      if(!m_bProgramStream)
      {
        if(*pts > m_nRefAudioPTS)
          *pts -= (uint64)m_nRefAudioPTS;
        else
          *pts = 0;
      }

      if((ptsFlags == PES_PKT_PTS_DTS_FLAG) && (dts))
      {
        uint8 firstpart =  (tempBuffer[localOffset++] & 0x0E) << 4;
        uint16 secondpart = ( ( (uint16)(tempBuffer[localOffset]) ) << 8)  | tempBuffer[localOffset+1];
        secondpart = (secondpart & 0xFFFE)>>1;
        uint16 thirdpart = ( ( (uint16)(tempBuffer[localOffset]) ) << 8)  | tempBuffer[localOffset+1];
        thirdpart = (thirdpart & 0xFFFE)>>1;
        localOffset+=2;
        *dts = make33BitValue(firstpart,secondpart,thirdpart);
        *dts = (uint64)((*dts)/90.0);//90 KHz clock
      }
      retStatus = MP2STREAM_SUCCESS;
    }
  }
  else
  {
    retStatus = MP2STREAM_INVALID_PARAM;
  }
  return retStatus;
}
/*! ======================================================================
@brief  Updates the current offset to the beginning of next packet

@detail Updates the current offset to the beginning of next TS or
        M2TS packet

@param[in]
 ullStartOffset:  Start Offset from which we update
 bIsBDMVFormat:   Flag to indicate if clip is M2TS format
 m_bGetLastPTS:   Flag to indicate if we are trying to get last PTS

@return    none
@note      None.
========================================================================== */
void MP2StreamParser::updateOffsetToNextPacket(uint64 ullStartOffset, bool bIsBDMVFormat,
                                               bool bForward)
{
  if(bIsBDMVFormat)
  {
    if(bForward)
    {
      m_nCurrOffset = ullStartOffset + MPEG2_M2TS_PKT_BYTES;
    }
    else if(ullStartOffset > MPEG2_M2TS_PKT_BYTES)
    {
      m_nCurrOffset = ullStartOffset - MPEG2_M2TS_PKT_BYTES;
    }
  }
  else
  {
    if(bForward)
    {
      m_nCurrOffset = ullStartOffset + MPEG2_TS_PKT_BYTES;
    }
    else if(ullStartOffset > MPEG2_TS_PKT_BYTES)
    {
      m_nCurrOffset = ullStartOffset - MPEG2_TS_PKT_BYTES;
    }
    else
    {
      m_nCurrOffset = 0;
    }
  }
  m_nCurrOffset = m_nCurrOffset + (m_nCurrOffset % 2);
}
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
PARSER_ERRORTYPE MP2StreamParser::GetStreamParameter( uint32 ulTrackId,
                                              uint32 ulParamIndex,
                                              void* pParamStruct)
{
  PARSER_ERRORTYPE eError = PARSER_ErrorInvalidParam;
  if(pParamStruct)
  {
    switch (ulParamIndex)
    {
      case FS_IndexParamAudioMP3:
      {
        FS_AUDIO_PARAM_MPGTYPE *pMPGInfo = (FS_AUDIO_PARAM_MPGTYPE*)pParamStruct;

        pMPGInfo->ulSamplingFreq    = GetAudioSamplingFrequency(ulTrackId);
        pMPGInfo->ulBitRate         = GetTrackAverageBitRate(ulTrackId);
        pMPGInfo->usChannels        = GetNumberOfAudioChannels(ulTrackId);
        pMPGInfo->ucLayer           = GetLayer(ulTrackId);
        pMPGInfo->ucVersion         = GetVersion(ulTrackId);
        eError = PARSER_ErrorNone;
        break;
      }
      default :
        eError = PARSER_ErrorInvalidParam;
        break;
    }
  }// if(pParamStruct)
  return eError;
}
