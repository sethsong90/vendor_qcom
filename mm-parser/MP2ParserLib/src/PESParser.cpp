/* =======================================================================
                              PESParser.cpp
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
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/src/PESParser.cpp#97 $
========================================================================== */
#include "MP2StreamParser.h"
#include "string.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMMalloc.h"
#include "math.h"
#include "H264HeaderParser.h"
#include "parserinternaldefs.h"

/*! ======================================================================
@brief  Get Available TimeStamp from the last PES packet

@detail    Starts parsing PCI packet encountered in TS or PS

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */

void MP2StreamParser::GetClipDurationFromPTS(uint32 trackId)
{
  bool bParseMultiplePESPkts = true;
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  if(!m_bGetLastPTS)
  {
    return;
  }
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"GetClipDurationFromPTS");
  uint32 ulChunkCount=1;
  uint64 ullOffset = 0;
  uint32 valCode =0;
  int32* nBytesRead=NULL;
  uint8 *ucTempBuf=(uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);

  // We will start scanning for PTS value from
  // the end of the file and break whenever we
  // find the last PTS in the file
  while(ulChunkCount*MPEG2_FILE_READ_CHUNK_SIZE < m_nFileSize)
  {
    if((ullOffset == 0) || (ullOffset >= MPEG2_FILE_READ_CHUNK_SIZE))
    {
      if (!readMpeg2StreamData(m_nFileSize-(ulChunkCount*MPEG2_FILE_READ_CHUNK_SIZE),
            MPEG2_FILE_READ_CHUNK_SIZE,ucTempBuf, MPEG2_FILE_READ_CHUNK_SIZE,
            (uint32)m_pUserData) )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
            "GetClipDurationFromPTS File Read Failure");
        break;
      }
      else
      {
        ullOffset = MPEG2_FILE_READ_CHUNK_SIZE-1;
      }
    }
    uint32 codeVal = getBytesValue(sizeof(uint32),&ucTempBuf[ullOffset]);
    while(codeVal != MP2_PACK_START_CODE && ullOffset > 0)
    {
      ullOffset--;
      codeVal = getBytesValue(sizeof(uint32),&ucTempBuf[ullOffset]);
    }
    if(ullOffset==0)
    {
      //No Start code in this chunk
      ulChunkCount++;
    }
    else
    {
      uint64 ullAbsOffset = m_nFileSize - ulChunkCount*MPEG2_FILE_READ_CHUNK_SIZE+ullOffset;
      retError = parsePackHeader(ullAbsOffset,false,
                                 0/*Track ID does not matter.*/,
                                 ucTempBuf,
                                 MPEG2_FILE_READ_CHUNK_SIZE,
                                 nBytesRead);
      if(MP2STREAM_SUCCESS == retError)
      {
        //look ahead to see if there is system header ahead
        if(!readMpeg2StreamData (ullAbsOffset, sizeof(MP2_SYS_HDR_START_CODE),
                                m_pDataBuffer, m_nDataBufferSize,
                                (uint32)m_pUserData) )
        {
          retError = m_eParserState;
          bContinue = false;
        }
        if(bContinue)
        {
          uint32 codeVal = getBytesValue(sizeof(uint32),m_pDataBuffer);
          if( codeVal == MP2_SYS_HDR_START_CODE)
          {
#ifdef MPEG2_PARSER_DEBUG
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                "SystemTargetHeader ullOffset %d",ullOffset);
#endif
            ullAbsOffset += sizeof(MP2_SYS_HDR_START_CODE);
            retError = parseSystemTargetHeader(trackId, ullAbsOffset);
            if(retError == MP2STREAM_SUCCESS)
            {
              //Now read ahead to see if there is a PES packet ahead
              if(!readMpeg2StreamData (ullAbsOffset, sizeof(uint32),
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
              }
            }
          }//if( codeVal == MP2_SYS_HDR_START_CODE)

          uint32 valCode = 0;
          while( isPESPacket(m_pDataBuffer,&valCode) &&
                 (retError == MP2STREAM_SUCCESS) )
          {
            //There is a PES packet,parse it before moving onto next pack
            retError = parsePESPacket(ullAbsOffset,valCode,trackId,
                ucTempBuf,MPEG2_FILE_READ_CHUNK_SIZE,nBytesRead);

            //Make sure we are nor reading beyond file size
            if(ullAbsOffset >= m_nFileSize)
            {
              retError = MP2STREAM_EOF;
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                  "parsePackHeader EOF detected....");
              break;
            }
          }//while( isPESPacket(m_pDataBuffer,&valCode)&&
        }//if(bContinue)
      }
      else
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
            "parsePackHeader Failed !");
      }
      if ( 0!= m_currPESPkt.pts)
      {
        double nBaseTime;
        // Found the last PES packet with a TS
        m_nEndPESPktTS = m_currPESPkt.pts;
        // Adjust with BaseTime
        GetBaseTime(trackId, &nBaseTime);
        m_nEndPESPktTS -= nBaseTime;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "GetClipDurationFromPTS m_nEndPESPktTS=%llu",
            m_nEndPESPktTS);
        break;
      }
      else
      {
        // NO TS in this PES packet, go back to previous PES packet
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "GetClipDurationFromPTS Error Calculated m_nEndPESPktTS=%llu",
            m_nEndPESPktTS);
        ullOffset = ullOffset - 1;
      }
    }
  }
  m_bGetLastPTS = false;
  if(ucTempBuf!=NULL)
  {
    MM_Free(ucTempBuf);
    ucTempBuf = NULL;
  }
  return;
}
/*! ======================================================================
@brief  Get Available TimeStamp from the last PES packet in a TS stream

@detail    Starts parsing from the end of file for a PES packet with
           non-zero PTS value

@param[in] Track ID

@return    Void.
@note      None.
========================================================================== */
void MP2StreamParser::TSGetClipDurationFromPTS(uint32 trackId)
{
  bool bParseMultiplePESPkts = true;
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  if(!m_bGetLastPTS)
  {
    return;
  }
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"GetClipDurationFromPTS");
  uint32 ulChunkCount=0;
  uint64 ullOffset = 0;
  uint32 valCode =0;
  int32* nBytesRead=NULL;
  uint8 *ucTempBuf=(uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);

  // We will start scanning for PTS value from
  // the end of the file and break whenever we
  // find the last PTS in the file
  while(ulChunkCount*MPEG2_FILE_READ_CHUNK_SIZE < m_nFileSize)
  {
    if((ullOffset == 4) || (ullOffset >= MPEG2_FILE_READ_CHUNK_SIZE))
    {
      uint32 ulBytesRead = readMpeg2StreamData(m_nFileSize-
          ((ulChunkCount+1)*MPEG2_FILE_READ_CHUNK_SIZE)+4,
            MPEG2_FILE_READ_CHUNK_SIZE,ucTempBuf, m_nDataBufferSize,
            (uint32)m_pUserData);
      if (!ulBytesRead)
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,
            "GetClipDurationFromPTS File Read Failure m_nFileSize=%llu", m_nFileSize);
        break;
      }
      else
      {
        ullOffset = ulBytesRead-1;
      }
    }
    uint32 val = 0;
    while(!isPESPacket(&ucTempBuf[ullOffset],&val) && ullOffset > 4)
    {
      ullOffset--;
    }
    if(ullOffset==4)
    {
      //No Start code in this chunk
      ulChunkCount++;
    }
    else
    {
      uint64 ullAbsOffset = m_nFileSize -
        (ulChunkCount+1)*MPEG2_FILE_READ_CHUNK_SIZE+ullOffset;
      retError = parsePESPacket(ullAbsOffset,val,trackId,
          ucTempBuf,MPEG2_FILE_READ_CHUNK_SIZE,nBytesRead);

      if ( 0!= m_currPESPkt.pts)
      {
        double nBaseTime;
        // Found the last PES packet with a TS
        m_nEndPESPktTS = m_currPESPkt.pts;
        // Adjust with BaseTime
        GetBaseTime(trackId, &nBaseTime);
        m_nEndPESPktTS -= nBaseTime;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "GetClipDurationFromPTS m_nEndPESPktTS=%llu",
            m_nEndPESPktTS);
        m_bGetLastPTS = false;
        break;
      }
      else
      {
        // NO TS in this PES packet, go back to previous PES packet
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "GetClipDurationFromPTS Error Calculated m_nEndPESPktTS=%llu",
            m_nEndPESPktTS);
        ullOffset = ullOffset - 1;
      }
      //Make sure we are nor reading beyond file size
      if(ullAbsOffset >= m_nFileSize)
      {
        retError = MP2STREAM_EOF;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
            "EOF detected....");
        break;
      }
    }
  }

  if(ucTempBuf!=NULL)
  {
    MM_Free(ucTempBuf);
    ucTempBuf = NULL;
  }
  return;
}
/*! ======================================================================
@brief  Get Available TimeStamp from the first PES packet in FWD direction

@detail    Starts parsing from the position specified for a PES packet with
           non-zero PTS value
@param[in] trackId Identifies elementary stream to demultiplex from
                   current pes packet
@param[in] ullStartPos Absolute offset from where to start looking for
                       PES packet
@param[out] pullStartOffset Start offset of PES packet
@param[out] pullEndOffset   End offset of PES packet
@param[out] *pullTimeStamp TS from PES packet

@return    Void.
@note      None.
========================================================================== */
void MP2StreamParser::GetPTSFromNextPES(uint32 ulTrackId,
    uint64 ullStartPos,uint64 *pullStartOffset, uint64 *pullEndOffset,
    uint64 *pullTimeStamp)
{
   MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "Shantanu GetPTSFromLastPES trackId=%lu ",ulTrackId);
  uint64 ullAbsOffset=0;
  uint64 ullOffset=0;
  uint32 ulTempTrackid=0xFFFFFFFF;
  uint8 *pucTempBuf=(uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);
  int32* pslBytesRead=NULL;
  if(pucTempBuf)
  {
    uint32 ulBytesRead = readMpeg2StreamData(ullStartPos,
        MPEG2_FILE_READ_CHUNK_SIZE,pucTempBuf, MPEG2_FILE_READ_CHUNK_SIZE,
              (uint32)m_pUserData);
    if (!ulBytesRead)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
        "GetPTSFromLastPES File Read Failure ullAvailOffset");
    }
    else
    {
      ullOffset = ulBytesRead;
    }
    while(ullAbsOffset < ullOffset)
    {
      uint32 val = 0;
      while( (!isPESPacket(&pucTempBuf[ullAbsOffset],&val))  &&
             (ullAbsOffset<ullOffset))
      {
        if((pucTempBuf[ullAbsOffset] == 0x47)&& (!m_bProgramStream))
        {
          // Check for Payload Unit Start Indicator
          if(pucTempBuf[ullAbsOffset+1] & 0x40)
          {
            ulTempTrackid =  (pucTempBuf[ullAbsOffset+1] & 0x1F)<< 8;
            ulTempTrackid |=  pucTempBuf[ullAbsOffset+2];
          }
        }
        ullAbsOffset++;
      }
      if(ullAbsOffset < ullOffset)
      {
        uint64 ullTempOffset = ullAbsOffset+ullStartPos;
        parsePESPacket(ullTempOffset,val,ulTrackId,
            pucTempBuf,MPEG2_FILE_READ_CHUNK_SIZE,pslBytesRead);
        if(m_bProgramStream)
        {
          ulTempTrackid = (uint32)m_currPESPkt.trackid;
        }
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
            "m_currPESPkt.pts=%f ulTempTrackid=%lu",
            m_currPESPkt.pts, ulTempTrackid);
        if ( (0!= m_currPESPkt.pts) &&
             (ulTrackId == ulTempTrackid))
        {
          // Found the last PES packet with a TS
          if(pullTimeStamp)
          {
            *pullTimeStamp = uint64(m_currPESPkt.pts);
          }
          if(pullStartOffset)
          {
            *pullStartOffset = m_currPESPkt.noffset;
          }
          if(pullEndOffset)
          {
            *pullEndOffset = m_currPESPkt.noffset+m_currPESPkt.packet_length;
          }
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
              "GetPTSFromLastPES m_currPESPkt.pts=%f m_currPESPkt.nOffset=%llu",
              m_currPESPkt.pts, m_currPESPkt.noffset);
          break;
        }
        else
        {
          ullAbsOffset++;
        }
      }
      else
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
            "Exhausted all bytes without finding PES for track!");
        GetPTSFromNextPES(ulTrackId,
            ullStartPos+MPEG2_FILE_READ_CHUNK_SIZE,pullStartOffset,
            pullEndOffset, pullTimeStamp);
      }
    }
  }
  else
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                "Malloc for temp buffer failed");
  }
}

/*! ======================================================================
@brief  Get Available TimeStamp from the last PES packet in a TS stream

@detail    Starts parsing from the end of file for a PES packet with
           non-zero PTS value
@param[in] trackId Identifies elementary stream to demultiplex from
                   current pes packet
@param[in] ullAvailOffset Bytes of data available to parse
@param[out] *ullTimeStamp TS from Last PES packet

@return    Void.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::GetPTSFromLastPES(uint32 ultrackId,
    uint64 ullAvailOffset, uint64 *pullTimeStamp, uint64 *pullEndOffset)
{
  MP2StreamStatus eRetError = MP2STREAM_FAIL;
  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
      "GetPTSFromLastPES trackId=%lu ullAvailOffset=%llu",
      ultrackId, ullAvailOffset);
  uint64 ullOffset = 0;
  uint32 valCode =0;
  int32* psiBytesRead=NULL;
  uint32 ulTempTrackid=0xFFFFFFFF;
  uint8 *pucTempBuf=(uint8*)MM_Malloc(MPEG2_FILE_READ_CHUNK_SIZE);
  if(pucTempBuf)
  {
    bool bFound=false;
    if(pullTimeStamp)
    *pullTimeStamp = 0;

    // We will start scanning for PTS value from
    // the end of the file and break whenever we
    // find the last PTS in the file
    uint64 ullAbsOffset = ullAvailOffset;
    while(ullAbsOffset > 0)
    {
      if((ullOffset == 0) || (ullOffset >= MPEG2_FILE_READ_CHUNK_SIZE))
      {
        if(ullAbsOffset > MPEG2_FILE_READ_CHUNK_SIZE)
        {
          ullAbsOffset = ullAbsOffset - MPEG2_FILE_READ_CHUNK_SIZE;
        }
        else
        {
          ullAbsOffset = 0;
        }
        uint32 ulBytesRead = readMpeg2StreamData(ullAbsOffset,
              MPEG2_FILE_READ_CHUNK_SIZE,pucTempBuf, m_nDataBufferSize,
              (uint32)m_pUserData);
        if (!ulBytesRead)
        {
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,
              "GetPTSFromLastPES File Read Failure ullAvailOffset=%llu",
              ullAvailOffset);
          break;
        }
        else
        {
          ullOffset = ulBytesRead;
        }
      }
      while( (!isPESPacket(&pucTempBuf[ullOffset-1],&valCode)) &&
             (ullOffset > 0))
      {
        ullOffset--;
      }
      if(!m_bProgramStream)
      {
        // Get PID for Transport stream
        uint64 ullTempOffset=ullOffset;
        while (pucTempBuf[ullTempOffset] != 0x47)
        {
          ullTempOffset--;
        }
        // In TS packet header 10th bit is
        // payload_unit_start_indicator
        if(pucTempBuf[ullTempOffset+1] & 0x40)
        {
          // TS packet PID = 13 bits from bit 12 to bit 24
          ulTempTrackid =  (pucTempBuf[ullTempOffset+1] & 0x1F)<< 8;
          ulTempTrackid |=  pucTempBuf[ullTempOffset+2];
        }
      }
      if(ullOffset!=0)
      {
        uint64 ullTempOffset = ullAbsOffset+ullOffset-1;
        eRetError = parsePESPacket(ullTempOffset,valCode,ultrackId,
            pucTempBuf,MPEG2_FILE_READ_CHUNK_SIZE,psiBytesRead);
        if(m_bProgramStream)
        {
          // Get Track ID for Program stream
          ulTempTrackid = m_currPESPkt.trackid;
        }

        if ( (0!= m_currPESPkt.pts) &&
            (ultrackId == ulTempTrackid))
        {
          // Found the last PES packet with a TS
          if(pullTimeStamp)
          {
            *pullTimeStamp = uint64(m_currPESPkt.pts);
          }
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
              "GetPTSFromLastPES m_currPESPkt.pts=%f m_currPESPkt.nOffset=%llu",
              m_currPESPkt.pts, m_currPESPkt.noffset);
          break;
        }
        else
        {
          // NO TS in this PES packet, go back to previous PES packet
          ullOffset = ullOffset - 1;
        }
      }
    }
    if(pucTempBuf!=NULL)
    {
      MM_Free(pucTempBuf);
      pucTempBuf = NULL;
    }
  }
  return eRetError;
}
/*! ======================================================================
@brief  Parses PES packet encountered in TS or PS

@detail    Starts parsing PES packet encountered in TS or PS

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parsePESPacket(uint64& nOffset,uint32 valCode,
                                                uint32 trackId,uint8* dataBuffer,
                                                uint32 nMaxBufSize, int32* nBytesRead)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  memset(&m_currPESPkt,0,sizeof(PESPacket));
  int localoffset = 0;
  bool bOk = true;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"parsePESPacket nOffset %llu",nOffset);
#endif

  if(!readMpeg2StreamData (nOffset, PES_PKT_START_CODE_STREAM_ID_SIZE,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32)m_pUserData) )
  {
    retError = m_eParserState;
    bOk = false;
  }

  if(bOk)
  {
    m_currPESPkt.noffset = nOffset;
    //Update the overall offset as we have read in PES_PKT_START_CODE_STREAM_ID_SIZE bytes
    nOffset += PES_PKT_START_CODE_STREAM_ID_SIZE;

    m_currPESPkt.start_code_prefix = PES_PKT_START_CODE;
    //PES_PKT_START_CODE consists of 24 bits, 3 bytes, so update localoffset.
    localoffset += 3;

    //Next byte is stream-id, which is passed in, so update localoffset.
    m_currPESPkt.trackid = (uint8)valCode;
    localoffset++;

    //Next 2 bytes give pes packet length
    m_currPESPkt.packet_length = ((uint32)(m_pDataBuffer[localoffset])<<8) | (uint32)m_pDataBuffer[localoffset+1];
  #ifdef MPEG2_PARSER_DEBUG
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parsePESPacket Length %lu",m_currPESPkt.packet_length);
  #endif
    localoffset += sizeof(uint16);

    switch(valCode)
    {
      case PROG_STREAM_MAP_ID:
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parsePESPacket encountered PSM packet");
        if (m_pProgramStreamMap)
        {
          nOffset += m_currPESPkt.packet_length;
        }
        else
        {
          retError = parseProgStreamMap(nOffset);
        }
        break;
      }
      case PADDING_STREAM_ID:
      case ECM_STREAM_ID:
      case EMM_STREAM_ID:
      case PROG_STREAM_DIRECTORY_ID:
      case DSMCC_STREAM_ID:
      case H222_TYPE_E_STREAM_ID:
      {
        retError = MP2STREAM_SUCCESS;
        nOffset += m_currPESPkt.packet_length;
        if( m_eParserState == MP2STREAM_SEEKING)
        {
          retError = MP2STREAM_SKIP_PES_PKT;
        }
        break;
      }
      case PRIVATE_STREAM2_ID:
      {
        //Read in next byte to get substream-id
        if(!readMpeg2StreamData (nOffset, sizeof(PCI_PKT_SUBSTREAM_ID),
                                 m_pDataBuffer, m_nDataBufferSize,
                                 (uint32)m_pUserData) )
        {
          retError = m_eParserState;
        }
        if(PCI_PKT_SUBSTREAM_ID == m_pDataBuffer[0])
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parsePESPacket encountered PCI packet");
          retError = parsePCIPacket(nOffset+sizeof(PCI_PKT_SUBSTREAM_ID),m_currPESPkt.packet_length);
        }
        else if(DSI_PKT_SUBSTREAM_ID == m_pDataBuffer[0])
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parsePESPacket encountered DSI packet");
          retError = parseDSIPacket(nOffset+sizeof(PCI_PKT_SUBSTREAM_ID),m_currPESPkt.packet_length);
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parsePESPacket encountered non PCI/DSI packet");
          if(!m_bProgramStream)
          {
            retError = MP2STREAM_SUCCESS;
            nOffset = m_currPESPkt.noffset + TS_PKT_SIZE ;
          }
        }
        if(m_bProgramStream)
        {
          nOffset += m_currPESPkt.packet_length;
        }
        break;
      }
      default:
      {
        if( m_eParserState == MP2STREAM_SEEKING)
        {
          nOffset += m_currPESPkt.packet_length;
          retError = MP2STREAM_SKIP_PES_PKT;
          break;
        }
         if( ((valCode >= AUDIO_STREAM_ID_START) && (valCode <= AUDIO_STREAM_ID_END)) ||
             ((valCode >= RES_DATA_STREAM_START_ID) && (valCode <= RES_DATA_STREAM_END_ID)) ||
             (valCode == PRIVATE_STREAM1_ID) )
         {
           #ifdef MPEG2_PARSER_DEBUG
             MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AUDIO STREAM ID %d",m_currPESPkt.trackid);
           #endif
           if(!nBytesRead)
           {
             int32 nRead = 0;
             nBytesRead = &nRead;
           }
           retError = parseElementaryStream(nOffset,trackId,TRACK_TYPE_AUDIO,dataBuffer,nMaxBufSize,nBytesRead);
           if(retError != MP2STREAM_SUCCESS)
           {
             return retError;
           }
           if(m_bInitialParsingPending)
           {
             (void)parseAudioMetaData();
           }
         }
         else if( (valCode >= VIDEO_STREAM_ID_START) && (valCode <= VIDEO_STREAM_ID_END))
         {
           #ifdef MPEG2_PARSER_DEBUG
             MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"VIDEO STREAM ID %d",m_currPESPkt.trackid);
           #endif
           if(!nBytesRead)
           {
             int32 nRead = 0;
             nBytesRead = &nRead;
           }
           retError = parseElementaryStream(nOffset,trackId,TRACK_TYPE_VIDEO,dataBuffer,nMaxBufSize,nBytesRead);
           if(retError != MP2STREAM_SUCCESS)
           {
             return retError;
           }
           if(m_bInitialParsingPending)
           {
             (void)parseVideoMetaData(nBytesRead);
           }
         }
         else
         {
           if(m_bProgramStream)
             nOffset += m_currPESPkt.packet_length;
           else
             retError = MP2STREAM_SUCCESS;
         }
        break;
      }
    }
  }
  return retError;
}
/*! ======================================================================
@brief  Parses PCI packet encountered in TS or PS

@detail    Starts parsing PCI packet encountered in TS or PS

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus  MP2StreamParser::parsePCIPacket(uint64 nOffset,uint32 nLength)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  pci_pkt* tmpPciPkt = m_pCurrVOBUPCIPkt;

  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"parsePCIPacket nOffset %llu",nOffset);
  if(!m_pFirstVOBUPCIPkt)
  {
    m_pFirstVOBUPCIPkt = (pci_pkt*)MM_Malloc(sizeof(pci_pkt));
    tmpPciPkt = m_pFirstVOBUPCIPkt;
  }
  else if(!m_pCurrVOBUPCIPkt)
  {
    m_pCurrVOBUPCIPkt = (pci_pkt*)MM_Malloc(sizeof(pci_pkt));
    tmpPciPkt = m_pCurrVOBUPCIPkt;
  }
  if(tmpPciPkt)
  {
    memset(tmpPciPkt,0,sizeof(pci_pkt));
    if(!readMpeg2StreamData (nOffset, PCI_PKT_INFO_BYTES,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData) )
    {
      retError = m_eParserState;
    }
    else
    {
      tmpPciPkt->noffset = m_currPESPkt.noffset;
      int readindex = 0;
      nOffset += PCI_PKT_INFO_BYTES;

      //Read block number
      tmpPciPkt->blockno = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      //Read APS and skip 2 reserved bytes
      tmpPciPkt->flags_aps = (uint16)getBytesValue(sizeof(uint16),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      //read bit-mask for prohibited user options
      tmpPciPkt->bitmask_puo = (uint16)getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      //read in start and end time
      tmpPciPkt->start_pts = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      tmpPciPkt->start_pts /= 90;
      readindex += sizeof(uint32);

      tmpPciPkt->end_pts = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      tmpPciPkt->end_pts /= 90;
      readindex += sizeof(uint32);

      //skip next 8 bytes to get international standard recording code
      readindex += (2*sizeof(uint32));

      memcpy(&tmpPciPkt->recording_code,m_pDataBuffer+readindex,(sizeof(uint8)*32));
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"parsePCIPacket current VOBU:start PTS %lu end PTS %lu",tmpPciPkt->start_pts,tmpPciPkt->end_pts);
      retError = MP2STREAM_SUCCESS;
      if(!m_pCurrVOBUPCIPkt)
      {
        m_pCurrVOBUPCIPkt = (pci_pkt*)MM_Malloc(sizeof(pci_pkt));
        if(m_pCurrVOBUPCIPkt != NULL)
        {
          memset(m_pCurrVOBUPCIPkt,0,sizeof(pci_pkt));
          memcpy(m_pCurrVOBUPCIPkt,m_pFirstVOBUPCIPkt,sizeof(pci_pkt));
        }
        else
        {
          retError = MP2STREAM_OUT_OF_MEMORY;
        }
      }
    }
  }
  else
  {
    retError = MP2STREAM_OUT_OF_MEMORY;
  }
  return retError;
}
/*! ======================================================================
@brief  Parses DSI packet encountered in TS or PS

@detail    Starts parsing DSI packet encountered in TS or PS

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus  MP2StreamParser::parseDSIPacket(uint64 nOffset,uint32 nLength)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"parseDSIPacket nOffset %llu",nOffset);
  dsi_pkt* tmpDsiPkt = m_pCurrVOBUDSIPkt;
  if(!m_pFirstVOBUDSIPkt)
  {
    m_pFirstVOBUDSIPkt = (dsi_pkt*)MM_Malloc(sizeof(dsi_pkt));
    tmpDsiPkt = m_pFirstVOBUDSIPkt;
  }
  else if(!m_pCurrVOBUDSIPkt)
  {
    m_pCurrVOBUDSIPkt = (dsi_pkt*)MM_Malloc(sizeof(dsi_pkt));
    tmpDsiPkt = m_pCurrVOBUDSIPkt;
  }
  if(tmpDsiPkt)
  {
    memset(tmpDsiPkt,0,sizeof(dsi_pkt));
    tmpDsiPkt->noffset = m_currPackHeader.noffset;
    uint32 nBytesToRead = nLength;
    if(nLength < DSI_PKT_INFO_BYTES)
    {
      nBytesToRead = DSI_PKT_INFO_BYTES;
    }
    if(!readMpeg2StreamData (nOffset, nBytesToRead,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData) )
    {
      retError = m_eParserState;
    }
    else
    {
      //Skip SCR bytes and start parsing
      int readindex = 4;

      //Update the offset with total number of bytes read
      nOffset += DSI_PKT_INFO_BYTES;

      //Read block number
      tmpDsiPkt->blockno = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      tmpDsiPkt->vobu_ea = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      tmpDsiPkt->first_ref_frame_end_block = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      tmpDsiPkt->second_ref_frame_end_block = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      tmpDsiPkt->third_ref_frame_end_block = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      readindex += sizeof(uint32);

      tmpDsiPkt->vobu_vob_idn = (uint16)getBytesValue(sizeof(uint16),m_pDataBuffer+readindex);
      readindex += sizeof(uint16);

      //Go to start and end PTM offset pointer
      readindex = DSI_PKT_START_END_PTM_OFFSET;

      tmpDsiPkt->vob_v_s_ptm = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      tmpDsiPkt->vob_v_s_ptm /= 90;
      m_nClipStartTime = tmpDsiPkt->vob_v_s_ptm;
      readindex += sizeof(uint32);

      tmpDsiPkt->vob_v_e_ptm = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      tmpDsiPkt->vob_v_e_ptm /= 90;

      if (tmpDsiPkt->vob_v_e_ptm < m_nEndPESPktTS)
      {
        m_nClipDuration = m_nEndPESPktTS;
      }
      else if (tmpDsiPkt->vob_v_e_ptm >= m_nClipDuration)
      {
        m_nClipDuration = tmpDsiPkt->vob_v_e_ptm;
      }
      readindex += sizeof(uint32);

      //Go to next video vobu offset pointer
      readindex = NEXT_VOBU_OFFSET_INDEX;

      tmpDsiPkt->next_vobu_offset = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      if( (tmpDsiPkt->next_vobu_offset & 0x80000000) &&
          (NO_VIDEO_VOBU_CODE != tmpDsiPkt->next_vobu_offset) )
      {
        tmpDsiPkt->next_vobu_offset &= 0x0EFFFFFF;
        tmpDsiPkt->end_offset_curr_vobu += tmpDsiPkt->next_vobu_offset * MAX_PS_PACK_SIZE;
        tmpDsiPkt->next_vobu_offset_valid = true;
      }
      readindex += sizeof(uint32);

      //Go to previous video vobu offset pointer
      readindex = PRV_VOBU_OFFSET_INDEX;
      tmpDsiPkt->prv_vobu_offset = getBytesValue(sizeof(uint32),m_pDataBuffer+readindex);
      if( (tmpDsiPkt->prv_vobu_offset & 0x80000000) &&
          (NO_VIDEO_VOBU_CODE != tmpDsiPkt->prv_vobu_offset) )
      {
        tmpDsiPkt->prv_vobu_offset_valid = true;
        tmpDsiPkt->prv_vobu_offset &= 0x0EFFFFFF;
      }
      readindex += sizeof(uint32);
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,"parseDSIPacket DSI_GI VOB:Start V PTS %lu End V PTS %lu tmpDsiPkt->end_offset_curr_vobu %llu",tmpDsiPkt->vob_v_s_ptm,tmpDsiPkt->vob_v_e_ptm,tmpDsiPkt->end_offset_curr_vobu);
      MM_MSG_PRIO5(MM_FILE_OPS, MM_PRIO_HIGH,
        "parseDSIPacket current VOBU# %d EndAddr %lu #1Ref Frame %lu #2Ref Frame %lu #3Ref Frame %lu",
                      tmpDsiPkt->vobu_vob_idn,tmpDsiPkt->vobu_ea,
                      tmpDsiPkt->first_ref_frame_end_block,
                      tmpDsiPkt->second_ref_frame_end_block,
                      tmpDsiPkt->third_ref_frame_end_block);
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"parseDSIPacket:Next/Prv VOBU valid %d/%d",tmpDsiPkt->next_vobu_offset_valid,tmpDsiPkt->prv_vobu_offset_valid);
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"parseDSIPacket:Offset to Next/Prv VOBU with Video: %llu, %llu",tmpDsiPkt->next_vobu_offset,tmpDsiPkt->prv_vobu_offset);
#endif
      retError = MP2STREAM_SUCCESS;
      if(!m_pCurrVOBUDSIPkt)
      {
        m_pCurrVOBUDSIPkt = (dsi_pkt*)MM_Malloc(sizeof(dsi_pkt));
        if(m_pCurrVOBUDSIPkt != NULL)
        {
          memset(m_pCurrVOBUDSIPkt,0,sizeof(dsi_pkt));
          memcpy(m_pCurrVOBUDSIPkt,m_pFirstVOBUDSIPkt,sizeof(dsi_pkt));
        }
        else
        {
          retError = MP2STREAM_OUT_OF_MEMORY;
        }
      }
    }
  }
  else
  {
    retError = MP2STREAM_OUT_OF_MEMORY;
  }
  return retError;
}
/*! ======================================================================
@brief  Parses elementary stream from current PES packet.

@detail    Demultiplexes elementary stream identified by 'id' from current PES packet.

@param[in/out] nOffset Points to the offset in current PES packet.
@param[in] id          Identifies elementary stream to demultiplex from current pes packet

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parseElementaryStream(uint64& nOffset,
                                                       uint32 id,
                                                       track_type StrmType,
                                                       uint8* dataBuffer,
                                                       uint32 nMaxBufSize,
                                                       int32* pnBytesRead)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  int localoffset = 0;
  uint16 total_variable_bytes_consumed = 0;
  uint8 nBytesConsumedInPESExtensionHdr = 0;

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,"parseElementaryStream nOffset %llu",nOffset);
#endif

  //! Reset PES Private data flag. If PES Packet is encrypted, then this flag
  //! will be set to true.
  m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag = false;

  if(m_bProgramStream && m_bMpeg1Video)
  {
    if(!readMpeg2StreamData (nOffset, 8*sizeof(uint8),
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData) )
    {
      retError = m_eParserState;
      bContinue = false;
    }
    else
    {
      if((m_pDataBuffer[localoffset] & 0xc0) == 0x40)
      {
        //STD buffer size field present, not used, skipping
        nOffset += PES_PKT_STD_BUFFER_SIZE;
        total_variable_bytes_consumed += PES_PKT_STD_BUFFER_SIZE;
      }
      if( (m_pDataBuffer[localoffset] & 0xF0) == 0x20)
      {
        //PTS present
        m_currPESPkt.pts_dts_flag = 2;
        uint8 firstpart =  (m_pDataBuffer[localoffset++] & 0x0E) << 4;
        uint16 secondpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
        secondpart = (secondpart & 0xFFFE)>>1;
        localoffset+=2;
        uint16 thirdpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
        thirdpart = (thirdpart & 0xFFFE)>>1;
        localoffset+=2;
        m_currPESPkt.pts = (double)make33BitValue(firstpart,secondpart,thirdpart);
        m_currPESPkt.pts = (double)(m_currPESPkt.pts)/90.0;//90 KHz clock

        nOffset += PES_PKT_PTS_BYTES;
        total_variable_bytes_consumed+=PES_PKT_PTS_BYTES;
      }
      else if( (m_pDataBuffer[localoffset] & 0xF0) == 0x30)
      {
        //PTS & DTS present
        m_currPESPkt.pts_dts_flag = 3;
        uint8 firstpart =  (m_pDataBuffer[localoffset++] & 0x0E) << 4;
        uint16 secondpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
        secondpart = (secondpart & 0xFFFE)>>1;
        localoffset+=2;
        uint16 thirdpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
        thirdpart = (thirdpart & 0xFFFE)>>1;
        localoffset+=2;
        m_currPESPkt.pts = (double)make33BitValue(firstpart,secondpart,thirdpart);
        m_currPESPkt.pts = (double)(m_currPESPkt.pts)/90.0;//90 KHz clock

        //Not parsing DTS since it is not currently used.
        nOffset += PES_PKT_PTS_DTS_BYTES;
        total_variable_bytes_consumed+=PES_PKT_PTS_DTS_BYTES;
      }
      int32 pes_pkt_data_bytes = m_currPESPkt.packet_length - total_variable_bytes_consumed;
      if(dataBuffer)
      {
        if( (int32)nMaxBufSize >= pes_pkt_data_bytes)
        {
          if(!readMpeg2StreamData (nOffset, pes_pkt_data_bytes,
                                   dataBuffer, nMaxBufSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            return retError;
          }
        }
        else
        {
          return MP2STREAM_OUT_OF_MEMORY;
        }
      }
      else
      {
        if(!readMpeg2StreamData (nOffset, pes_pkt_data_bytes,
                                 m_pDataBuffer, m_nDataBufferSize,
                                 (uint32)m_pUserData) )
        {
          retError = m_eParserState;
          return retError;
        }
      }
      if(pnBytesRead)
      {
        *pnBytesRead = pes_pkt_data_bytes;
      }
      nOffset+= pes_pkt_data_bytes;
      retError = MP2STREAM_SUCCESS;
    }
  }
  else
  {

    //Read PES_PKT_FIXED_HDR_BYTES from current offset
    if(!readMpeg2StreamData (nOffset, PES_PKT_FIXED_HDR_BYTES,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData) )
    {
      retError = m_eParserState;
      bContinue = false;
    }
    else
    {
      nOffset += PES_PKT_FIXED_HDR_BYTES;
      /* TS Packet PID may get updated dynamically, retain Video/Audio PID
         values as part of PES Packet structure. This PID is useful especially
         if content is encrypted. */
      m_currPESPkt.tsPID = m_currTSPkt.PID;
      //Parse the bytes read to proceed further
      m_currPESPkt.scrambling_control = (m_pDataBuffer[localoffset] & 0x30)>>4;
      m_currPESPkt.pes_priority = (m_pDataBuffer[localoffset] & 0x08)>>3;
      m_currPESPkt.data_align_indicator = (m_pDataBuffer[localoffset] & 0x04)>>2;
      m_currPESPkt.copyright = (m_pDataBuffer[localoffset] & 0x02)>>1;
      m_currPESPkt.original_copy = m_pDataBuffer[localoffset] & 0x01;
      localoffset++;

      m_currPESPkt.pts_dts_flag = (m_pDataBuffer[localoffset] & 0xD0)>>6;
      m_currPESPkt.escr_flag = (m_pDataBuffer[localoffset] & 0x20)>>5;
      m_currPESPkt.es_rate_flag = (m_pDataBuffer[localoffset] & 0x10)>>4;
      m_currPESPkt.dsm_trick_mode_flag = (m_pDataBuffer[localoffset] & 0x08)>>3;
      m_currPESPkt.add_copy_info_flag = (m_pDataBuffer[localoffset] & 0x04)>>2;
      m_currPESPkt.pes_crc_flag = (m_pDataBuffer[localoffset] & 0x02)>>1;
      m_currPESPkt.pes_extn_flag = m_pDataBuffer[localoffset] & 0x01;
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
        "parseElementaryStream():TS-PID %d pes_extn_flag= %u",
        m_currPESPkt.tsPID, m_currPESPkt.pes_extn_flag);
      localoffset++;
      m_currPESPkt.pes_hdr_data_length = m_pDataBuffer[localoffset++];
      localoffset++;

      //Bytes consumed here onwards as a part of PES packet are variable, so keep counting bytes consumed.
      while( (bContinue) &&
             ((uint32) (localoffset + total_variable_bytes_consumed) <
              TS_PES_PKT_TOTAL_HEADER_SIZE))
      {
        if( (m_currPESPkt.pts_dts_flag & 0x03) == 0x03)
        {
          localoffset = 0;
          //parse PTS/DTS from current PES packet
          if(!readMpeg2StreamData (nOffset, PES_PKT_PTS_DTS_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_PTS_DTS_BYTES;
          total_variable_bytes_consumed += PES_PKT_PTS_DTS_BYTES;

          //30--32
          uint8 firstpart =  (m_pDataBuffer[localoffset++] & 0x0E) << 4;
          //29--15
          uint16 secondpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          secondpart = (secondpart & 0xFFFE)>>1;
          localoffset+=2;
          //0--14
          uint16 thirdpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          thirdpart = (thirdpart & 0xFFFE)>>1;
          localoffset+=2;

          m_currPESPkt.pts = (double)make33BitValue(firstpart,secondpart,thirdpart);
          m_currPESPkt.pts = (double)(m_currPESPkt.pts)/90.0;//90 KHz clock

          if((m_currTSPkt.PID == m_nAudioPIDSelected) ||
             (m_currPESPkt.trackid == m_nAudioPIDSelected))
          {
            /** First Frame TS may be ZERO. m_bpartialPESTS flag will be
             *  set to TRUE, once at least one PES packet is completely
             *  parsed. */
            if((!m_nRefAudioPTS) && !m_bRefAudioPTSSet && (!m_bGetLastPTS))
            {
              m_nRefAudioPTS = m_nFirstAudioPTS = m_currPESPkt.pts;
              m_bRefAudioPTSSet = true;
            }
            if((m_bGetLastPTS) && (!m_nLastAudioPTS))
            {
              m_nLastVideoPTS = m_nLastAudioPTS = m_currPESPkt.pts;
            }
            if(!m_bInitialParsingPending)//During playback
            {
              /* Correct if discontinuity present */
              if(m_hTSDiscCorrectModeEnum ==
                 FILE_SOURCE_MEDIA_ENABLE_TS_DISCONTINUITY_CORRECTION)
              {
                correctTSDiscontinuity(id);
              }
              if(m_currPESPkt.pts >= m_nRefAudioPTS)
                m_currPESPkt.pts = m_currPESPkt.pts - m_nRefAudioPTS;
              else
                m_currPESPkt.pts = 0;
            }
          }
          else if((m_currTSPkt.PID == m_nVideoPIDSelected) ||
                  (m_currPESPkt.trackid == m_nVideoPIDSelected))
          {
            /** First Frame TS may be ZERO. m_bpartialPESTS flag will be
             *  set to TRUE, once at least one PES packet is completely
             *  parsed. */
            if((!m_nRefVideoPTS) && !m_bRefVideoPTSSet && (!m_bGetLastPTS))
            {
              m_nRefVideoPTS = m_currPESPkt.pts;
              m_bRefVideoPTSSet = true;
            }
            if((m_bGetLastPTS) && (!m_nLastVideoPTS))
            {
              m_nLastVideoPTS = m_nLastAudioPTS = m_currPESPkt.pts;
            }
            if(!m_bInitialParsingPending)//During playback
            {
              /* Correct if discontinuity present */
              if(m_hTSDiscCorrectModeEnum ==
                 FILE_SOURCE_MEDIA_ENABLE_TS_DISCONTINUITY_CORRECTION)
              {
                correctTSDiscontinuity(id);
              }
              if(m_currPESPkt.pts >= m_nRefVideoPTS)
                m_currPESPkt.pts = m_currPESPkt.pts - m_nRefVideoPTS;
              else
                m_currPESPkt.pts = 0;
            }
          }

          if(!m_bInitialParsingPending)
          {
            //We will update our member sample info structure
            if((float)m_currPESPkt.pts != m_sampleInfo.ntime)
            {
              if(m_sampleInfo.ntime)
                m_sampleInfo.delta = fabs((float(m_currPESPkt.pts) - float(m_sampleInfo.ntime)));
              m_sampleInfo.ntime = (float)m_currPESPkt.pts;
              m_sampleInfo.noffset = m_currPESPkt.noffset;
            }
          }
          //30--32
          firstpart =  (m_pDataBuffer[localoffset++] & 0x0E) << 4;
          //29--15
          secondpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          secondpart = (secondpart & 0xFFFE)>>1;
          localoffset+=2;
          //0--14
          thirdpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          thirdpart = (thirdpart & 0xFFFE)>>1;
          localoffset+=2;

          m_currPESPkt.dts = (double)make33BitValue(firstpart,secondpart,thirdpart);
          m_currPESPkt.dts = (double)(m_currPESPkt.dts)/90.0;//90 KHz clock
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream PTS %f DTS %f",m_currPESPkt.pts,m_currPESPkt.dts);
        }
        else if( (m_currPESPkt.pts_dts_flag & 0x02) == 0x02)
        {
          localoffset = 0;
          //parse PTS from current PES packet
          if(!readMpeg2StreamData (nOffset, PES_PKT_PTS_BYTES ,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_PTS_BYTES;
          total_variable_bytes_consumed += PES_PKT_PTS_BYTES;

          //30--32
          uint8 firstpart =  (m_pDataBuffer[localoffset] & 0x0E) << 4;
          localoffset++;
          //29--15
          uint16 secondpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          secondpart = (secondpart & 0xFFFE)>>1;
          localoffset+=2;
          //0--14
          uint16 thirdpart = ( ( (uint16)(m_pDataBuffer[localoffset]) ) << 8)  | m_pDataBuffer[localoffset+1];
          thirdpart = (thirdpart & 0xFFFE)>>1;
          localoffset+=2;

          m_currPESPkt.pts = (double)make33BitValue(firstpart,secondpart,thirdpart);
          m_currPESPkt.pts = (m_currPESPkt.pts)/90.0;//90 KHz clock

          if((m_currTSPkt.PID == m_nAudioPIDSelected) ||
             (m_currPESPkt.trackid == m_nAudioPIDSelected))
          {
            /** First Frame TS may be ZERO. m_bpartialPESTS flag will be
             *  set to TRUE, once at least one PES packet is completely
             *  parsed. */
            if((!m_nRefAudioPTS) && !m_bRefAudioPTSSet && (!m_bGetLastPTS))
            {
              m_nRefAudioPTS = m_nFirstAudioPTS = m_currPESPkt.pts;
              m_bRefAudioPTSSet = true;
            }
            if((m_bGetLastPTS) && (!m_nLastAudioPTS))
            {
              m_nLastVideoPTS = m_nLastAudioPTS = m_currPESPkt.pts;
            }
            if(!m_bInitialParsingPending)//During playback
            {
              /* Correct if discontinuity present */
              if(m_hTSDiscCorrectModeEnum ==
                 FILE_SOURCE_MEDIA_ENABLE_TS_DISCONTINUITY_CORRECTION)
              {
                correctTSDiscontinuity(id);
              }
              if(m_currPESPkt.pts >= m_nRefAudioPTS)
                m_currPESPkt.pts = m_currPESPkt.pts - m_nRefAudioPTS;
              else
                m_currPESPkt.pts = 0;
            }
          }
          else if((m_currTSPkt.PID == m_nVideoPIDSelected) ||
                  (m_currPESPkt.trackid == m_nVideoPIDSelected))
          {
            /** First Frame TS may be ZERO. m_bpartialPESTS flag will be
             *  set to TRUE, once at least one PES packet is completely
             *  parsed. */
            if((!m_nRefVideoPTS) && !m_bRefVideoPTSSet && (!m_bGetLastPTS))
            {
              m_nRefVideoPTS = m_currPESPkt.pts;
              m_bRefVideoPTSSet = true;
            }
            if((m_bGetLastPTS) && (!m_nLastVideoPTS))
            {
              m_nLastVideoPTS = m_nLastAudioPTS = m_currPESPkt.pts;
            }
            if(!m_bInitialParsingPending)//During playback
            {
              /* Correct if discontinuity present */
              if(m_hTSDiscCorrectModeEnum ==
                 FILE_SOURCE_MEDIA_ENABLE_TS_DISCONTINUITY_CORRECTION)
              {
                correctTSDiscontinuity(id);
              }
              if(m_currPESPkt.pts >= m_nRefVideoPTS)
                m_currPESPkt.pts = m_currPESPkt.pts - m_nRefVideoPTS;
              else
                m_currPESPkt.pts = 0;
            }
          }

          if(!m_bInitialParsingPending)
          {
            //We will update our member sample info structure
            if((float)m_currPESPkt.pts != m_sampleInfo.ntime)
            {
              if(m_sampleInfo.ntime)
                m_sampleInfo.delta = fabs((float(m_currPESPkt.pts) - float(m_sampleInfo.ntime)));
              m_sampleInfo.ntime = (float)m_currPESPkt.pts;
              m_sampleInfo.noffset = m_currPESPkt.noffset;
            }
          }

    #ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream PTS %f NO DTS",m_currPESPkt.pts);
    #endif
        }
        if(m_currPESPkt.escr_flag && bContinue)
        {
          //parse escr
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_PKT_ESCR_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_ESCR_BYTES;
          total_variable_bytes_consumed += PES_PKT_ESCR_BYTES;
          uint8 firstpart = (m_pDataBuffer[localoffset] & 0x38)<<2;//get ms 3 bits

          //get top 2 bits out of middle 15 bits
          uint8 middle15bitspart1 = (m_pDataBuffer[localoffset] & 0x03)<<6;
          localoffset++;

          //get remaining 13 bits out of middle 15 bits
          uint16 middle15bitspart2 = (((uint16)(m_pDataBuffer[localoffset]))<<8) | (m_pDataBuffer[localoffset+1]& 0xF8);
          localoffset++;
          uint16 secondpart = make15BitValue(middle15bitspart1,middle15bitspart2);

          //get top 2 bits out of lower 15 bits
          uint8 lower15bitspart1 = (m_pDataBuffer[localoffset] & 0x03)<<6;
          localoffset++;

          //get remaining 13 bits out of middle 15 bits
          uint16 lower15bitspart2 = (((uint16)(m_pDataBuffer[localoffset]))<<8) | (m_pDataBuffer[localoffset+1]& 0xF8);
          localoffset++;
          uint16 thirdpart = make15BitValue(lower15bitspart1,lower15bitspart2);

          //get top 2 bits out of 9 bit escr extension
          uint8 escrextpart1     = (m_pDataBuffer[localoffset++] & 0x03)<<6;
          //get lower 7 bits out of 9 bit escr extension
          uint8 escrextpart2     = (m_pDataBuffer[localoffset++] & 0xFE);

          m_currPESPkt.escr_extn = make9BitValue(escrextpart1,escrextpart2);
          m_currPESPkt.escr_base  = (double)make33BitValue(firstpart,secondpart,thirdpart);

          m_currPESPkt.escr_base *= 300;
          m_currPESPkt.escr_val = m_currPESPkt.escr_base + m_currPESPkt.escr_extn;
          m_currPESPkt.escr_val = m_currPESPkt.escr_val /27000.0;    //27 MHz clock
    #ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream escr_val %f ",m_currPESPkt.escr_val);
    #endif
        }
        if(m_currPESPkt.es_rate_flag && bContinue)
        {
          //parse es_rate
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_PKT_ES_RATE_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_ES_RATE_BYTES;
          total_variable_bytes_consumed += PES_PKT_ES_RATE_BYTES;

          //get top 15 bits from rate bound
          uint16 es_rate_part1 = (((uint16)m_pDataBuffer[localoffset])<<8) | m_pDataBuffer[localoffset+1];
          localoffset += 2;
          //knock off marker bit and get the 15 bits of interest.
          es_rate_part1 <<= 1;
          //get ls 7 bits from rate bound
          uint8 es_rate_part2 = (m_pDataBuffer[localoffset] & 0xFE);
          localoffset++;
          m_currPESPkt.es_rate = make22BitValue(es_rate_part1 ,es_rate_part2);
        }
        if(m_currPESPkt.dsm_trick_mode_flag && bContinue)
        {
          //parse trick mode info
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_PKT_DSM_TRICK_MODE_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_DSM_TRICK_MODE_BYTES;
          total_variable_bytes_consumed += PES_PKT_DSM_TRICK_MODE_BYTES;
          m_currPESPkt.trick_mode_control = (m_pDataBuffer[localoffset]& 0xE0)>>5;
          switch(m_currPESPkt.trick_mode_control)
          {
            case TRICK_MODE_CONTROL_FAST_FORWARD:
            {
              m_currPESPkt.field_id = (m_pDataBuffer[localoffset]& 0x18)>>3;
              m_currPESPkt.intra_slice_refresh = (m_pDataBuffer[localoffset]& 0x04)>>2;
              m_currPESPkt.frequency_truncation = m_pDataBuffer[localoffset]& 0x01;
            }
            break;
            case TRICK_MODE_CONTROL_SLOW_MOTION:
            {
              m_currPESPkt.rep_cntrol = m_pDataBuffer[localoffset]& 0x1F;
            }
            break;
            case TRICK_MODE_CONTROL_FREEZE_FRAME:
            {
              m_currPESPkt.field_id = (m_pDataBuffer[localoffset]& 0x18)>>3;
            }
            break;
            case TRICK_MODE_CONTROL_FAST_REVERSE:
            {
             m_currPESPkt.field_id = (m_pDataBuffer[localoffset]& 0x18)>>3;
             m_currPESPkt.intra_slice_refresh = (m_pDataBuffer[localoffset]& 0x04)>>2;
             m_currPESPkt.frequency_truncation = m_pDataBuffer[localoffset]& 0x01;
            }
            break;
            case TRICK_MODE_CONTROL_SLOW_REVERSE:
            {
              m_currPESPkt.rep_cntrol = m_pDataBuffer[localoffset]& 0x1F;
            }
            break;
            default:
              break;
          }
        }
        if(m_currPESPkt.add_copy_info_flag && bContinue)
        {
          //parse additional copy info
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_PKT_COPY_INFO_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_COPY_INFO_BYTES;
          total_variable_bytes_consumed += PES_PKT_COPY_INFO_BYTES;
          m_currPESPkt.add_copy = m_pDataBuffer[localoffset]& 0x7F;
        }
        if(m_currPESPkt.pes_crc_flag && bContinue)
        {
          //parse pes crs information
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_PKT_PES_CRC_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          nOffset += PES_PKT_PES_CRC_BYTES;
          total_variable_bytes_consumed += PES_PKT_PES_CRC_BYTES;
          m_currPESPkt.prv_pkt_pes_crc = (((uint16)m_pDataBuffer[localoffset]) << 8) | m_pDataBuffer[localoffset+1];
        }
        if(m_currPESPkt.pes_extn_flag && bContinue)
        {
          //parse pes extension information
          localoffset = 0;
          if(!readMpeg2StreamData (nOffset, PES_EXTN_FIXED_HDR_BYTES,
                                   m_pDataBuffer, m_nDataBufferSize,
                                   (uint32)m_pUserData) )
          {
            retError = m_eParserState;
            bContinue = false;
            break;
          }
          else
          {
            nOffset += PES_EXTN_FIXED_HDR_BYTES;
            total_variable_bytes_consumed+= PES_EXTN_FIXED_HDR_BYTES;
            nBytesConsumedInPESExtensionHdr += PES_EXTN_FIXED_HDR_BYTES;
            //parse pes extension fixed header

            m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag    = (m_pDataBuffer[localoffset] & 0x80)?1:0;
            m_currPESPkt.pes_extn_hdr.pes_extn_pack_hdr_flag    = (m_pDataBuffer[localoffset] & 0x40)?1:0;
            m_currPESPkt.pes_extn_hdr.pes_extn_pkt_seq_cnt_flag = (m_pDataBuffer[localoffset] & 0x20)?1:0;
            m_currPESPkt.pes_extn_hdr.pes_extn_std_buffer_flag  = (m_pDataBuffer[localoffset] & 0x10)?1:0;
            m_currPESPkt.pes_extn_hdr.pes_extn_flag2            = (m_pDataBuffer[localoffset] & 0x01)?1:0;
            MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
              "parseElementaryStream():TS-PID %u pes_extn_pvt_data_flag =%d",
              m_currPESPkt.tsPID,
              m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag );

            if(m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag && bContinue)
            {
              localoffset = 0;
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
             "parseElementaryStream():TS-PID %u pes_extn_pvt_data_flag is TRUE",
              m_currPESPkt.tsPID);

              if(!readMpeg2StreamData (nOffset, PES_EXTN_PVT_DATA_BYTES,
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              else
              {
                nOffset += PES_EXTN_PVT_DATA_BYTES;
                total_variable_bytes_consumed+= PES_EXTN_PVT_DATA_BYTES;
                nBytesConsumedInPESExtensionHdr += PES_EXTN_PVT_DATA_BYTES;

                localoffset = 0;
                memcpy(m_currPESPkt.pes_extn_hdr.pes_pvt_data, m_pDataBuffer,
                       PES_EXTN_PVT_DATA_BYTES);
              }
            }
            if(m_currPESPkt.pes_extn_hdr.pes_extn_pack_hdr_flag && bContinue)
            {
              localoffset = 0;
    #ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseElementaryStream pes_extn_pack_hdr_flag is TRUE");
    #endif
              if(!readMpeg2StreamData (nOffset, PES_EXTN_PACK_FIELD_LEN_BYTES,
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              else
              {
                total_variable_bytes_consumed+= PES_EXTN_PACK_FIELD_LEN_BYTES;
                nOffset += PES_EXTN_PACK_FIELD_LEN_BYTES;
                nBytesConsumedInPESExtensionHdr += PES_EXTN_PACK_FIELD_LEN_BYTES;
                localoffset = 0;
                memcpy(&m_currPESPkt.pes_extn_hdr.pack_field_length,m_pDataBuffer,1);
                if(m_bProgramStream)
                retError = parsePackHeader(nOffset);
                if(retError != MP2STREAM_SUCCESS)
                {
                   return retError;
                }
              }
            }
            if(m_currPESPkt.pes_extn_hdr.pes_extn_pkt_seq_cnt_flag && bContinue)
            {
    #ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream pes_extn_pkt_seq_cnt_flag is TRUE");
    #endif
              if(!readMpeg2StreamData (nOffset, PES_EXTN_PKT_SEQ_COUNTER_BYTES,
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              else
              {
                localoffset = 0;
                nOffset += PES_EXTN_PKT_SEQ_COUNTER_BYTES;
                total_variable_bytes_consumed+= PES_EXTN_PKT_SEQ_COUNTER_BYTES;
                nBytesConsumedInPESExtensionHdr += PES_EXTN_PKT_SEQ_COUNTER_BYTES;
                m_currPESPkt.pes_extn_hdr.prog_seq_cnt = m_pDataBuffer[localoffset] & 0x7F;
                localoffset++;
                m_currPESPkt.pes_extn_hdr.mpeg1_mpeg2_iden = m_pDataBuffer[localoffset]&0x40;
                m_currPESPkt.pes_extn_hdr.original_stuff_length = m_pDataBuffer[localoffset]&0x3F;
                localoffset++;
    #ifdef MPEG2_PARSER_DEBUG
                MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                               "parseElementaryStream prog_seq_cnt %d mpeg1_mpeg2_iden %d original_stuff_length %d",
                               m_currPESPkt.pes_extn_hdr.prog_seq_cnt,
                               m_currPESPkt.pes_extn_hdr.mpeg1_mpeg2_iden,
                               m_currPESPkt.pes_extn_hdr.original_stuff_length);
    #endif
              }
            }
            if(m_currPESPkt.pes_extn_hdr.pes_extn_std_buffer_flag && bContinue)
            {
    #ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream pes_extn_std_buffer_flag is TRUE");
    #endif
              if(!readMpeg2StreamData (nOffset, PES_EXTN_P_STD_BUFFER_BYTES,
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              else
              {
                localoffset = 0;
                nOffset += PES_EXTN_P_STD_BUFFER_BYTES;
                total_variable_bytes_consumed+= PES_EXTN_P_STD_BUFFER_BYTES;
                nBytesConsumedInPESExtensionHdr += PES_EXTN_P_STD_BUFFER_BYTES;
                m_currPESPkt.pes_extn_hdr.p_std_buffer_scale = m_pDataBuffer[localoffset] & 0x20;
                m_currPESPkt.pes_extn_hdr.p_std_buffer_size  = ((uint16)(m_pDataBuffer[localoffset] & 0x1F))<<8;
                m_currPESPkt.pes_extn_hdr.p_std_buffer_size |= m_pDataBuffer[localoffset+1];

                if(!m_currPESPkt.pes_extn_hdr.p_std_buffer_scale)
                {
                  m_currPESPkt.pes_extn_hdr.p_std_buffer_size *= 128;
                }
                else
                {
                  m_currPESPkt.pes_extn_hdr.p_std_buffer_size *= 1024;
                }
    #ifdef MPEG2_PARSER_DEBUG
                MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                               "parseElementaryStream p_std_buffer_scale %d p_std_buffer_size %d",
                               m_currPESPkt.pes_extn_hdr.p_std_buffer_scale, m_currPESPkt.pes_extn_hdr.p_std_buffer_size);
    #endif
              }
            }
            if(m_currPESPkt.pes_extn_hdr.pes_extn_flag2 && bContinue)
            {
#ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseElementaryStream pes_extn_flag2 is TRUE");
#endif
              if(!readMpeg2StreamData (nOffset, PES_EXTN_FLAG2_BYTES,
                                       m_pDataBuffer, m_nDataBufferSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              else
              {
                localoffset = 0;
                nOffset += PES_EXTN_FLAG2_BYTES;
                total_variable_bytes_consumed+= PES_EXTN_FLAG2_BYTES;
                nBytesConsumedInPESExtensionHdr += PES_EXTN_FLAG2_BYTES;
                m_currPESPkt.pes_extn_hdr.pes_extn_field_length = m_pDataBuffer[localoffset]& 0x7F;
              }
            }//if(m_currPESPkt.pes_extn_hdr.pes_extn_flag2 && bContinue)
          }
        }//if(m_currPESPkt.pes_extn_flag && bContinue)

        /*
        * m_currPESPkt.pes_hdr_data_length gives
        * total number of bytes occupied by optional fields and
        * any stuffing bytes contained in this PES pkt header.
        * Check if we have any non zero stuffing bytes to skip.
        */
        if( (m_currPESPkt.pes_hdr_data_length - total_variable_bytes_consumed) > 0)
        {
          /*
          * There are few non zero stuffing bytes.
          * Update variable bytes consumed and nOffset.
          */
          if( (m_bProgramStream) ||
              ( (!m_bProgramStream) &&
                (m_currPESPkt.pes_hdr_data_length < ( (TS_PKT_SIZE - TS_PKT_HDR_BYTES - PES_PKT_START_CODE_STREAM_ID_SIZE) - PES_PKT_FIXED_HDR_BYTES - total_variable_bytes_consumed) )))
          {
            int nPaddingBytes = m_currPESPkt.pes_hdr_data_length - total_variable_bytes_consumed;
            total_variable_bytes_consumed += nPaddingBytes;
            nOffset += nPaddingBytes;
          }
        }
    #ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                       "parseElementaryStream m_currPESPkt.pes_hdr_data_length %d total_variable_bytes_consumed %d ",
                       m_currPESPkt.pes_hdr_data_length,total_variable_bytes_consumed);
    #endif

        int32 pes_pkt_data_bytes = 0;
        if(m_bProgramStream)
        {
          pes_pkt_data_bytes = m_currPESPkt.packet_length - PES_PKT_FIXED_HDR_BYTES - total_variable_bytes_consumed;
        }
        else
        {
          if(m_currTSPkt.adaption_field_control != TS_ADAPTION_FILED_DATA_PRSENT)
          {
            pes_pkt_data_bytes = TS_PES_PKT_TOTAL_HEADER_SIZE - total_variable_bytes_consumed ;
          }
          else
          {
            if( (uint32)(total_variable_bytes_consumed + 1 +
                m_currTSPkt.adaption_field.adaption_field_length) < TS_PES_PKT_TOTAL_HEADER_SIZE)
            {
            pes_pkt_data_bytes = TS_PES_PKT_TOTAL_HEADER_SIZE - total_variable_bytes_consumed - 1 - m_currTSPkt.adaption_field.adaption_field_length ;
            total_variable_bytes_consumed += (1 + m_currTSPkt.adaption_field.adaption_field_length);
            }
            else
            {
              bContinue = false;
              retError = MP2STREAM_SUCCESS;
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"no pes_pkt_data_bytes here");
            }
          }
        }
    #ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                        "parseElementaryStream m_currPESPkt.packet_length %lu m_currPESPkt.pes_hdr_data_length %d pes_pkt_data_bytes %ld",
                        m_currPESPkt.packet_length,m_currPESPkt.pes_hdr_data_length,pes_pkt_data_bytes);
    #endif
        if(pes_pkt_data_bytes )
        {
          //read in elementary data stream now
          if(dataBuffer && ( (int32)nMaxBufSize >= pes_pkt_data_bytes) )
          {
            bool bSkip = false;
            track_type ttype;
            memset(&ttype,0,sizeof(track_type));
            media_codec_type medcodtype;
            memset(&medcodtype,0,sizeof(media_codec_type));
            (void)GetTrackType(id,&ttype,&medcodtype);
            if ((StrmType == TRACK_TYPE_AUDIO) &&
                (AUDIO_CODEC_AC3  == medcodtype) )
            {
              //Read substream-id
              if(!readMpeg2StreamData (nOffset, sizeof(uint8),
                                       dataBuffer, nMaxBufSize,
                                       (uint32)m_pUserData) )
              {
                retError = m_eParserState;
                bContinue = false;
                break;
              }
              if(( (dataBuffer[0] < 0x80) || (dataBuffer[0] > 0x87) ) )
              {
                bSkip = true;
              }
              else
              {
                track_type ttype;
                memset(&ttype,0,sizeof(track_type));
                media_codec_type medcodtype;
                memset(&medcodtype,0,sizeof(media_codec_type));
                //if( (StrmType == AUDIO) && (GetTrackType(id,&ttype,&medcodtype)==MP2STREAM_SUCCESS) )
                {
                  //if(medcodtype == AUDIO_CODEC_AC3)
                  nOffset += AC3_SUBSTREAM_SYNC_INFO_BYTES;
                  pes_pkt_data_bytes -= AC3_SUBSTREAM_SYNC_INFO_BYTES;
                }
              }
            }
            if(!readMpeg2StreamData (nOffset, pes_pkt_data_bytes,
                                     dataBuffer, nMaxBufSize,
                                     (uint32)m_pUserData) )
            {
              retError = m_eParserState;
              bContinue = false;
              break;
            }
            else
            {
              if(pnBytesRead)
              {
                if( bSkip )
                {
                  *pnBytesRead = 0;
                }
                else
                {
                  *pnBytesRead = pes_pkt_data_bytes;
                }
              }
            }
          }
          else if((int32)m_nDataBufferSize >= pes_pkt_data_bytes)
          {
            if(!readMpeg2StreamData (nOffset, pes_pkt_data_bytes,
                                     m_pDataBuffer, m_nDataBufferSize,
                                     (uint32)m_pUserData) )
            {
              retError = m_eParserState;
              bContinue = false;
              break;
            }
            if(pnBytesRead)
            {
              *pnBytesRead = pes_pkt_data_bytes;
            }
          }
        }
        nOffset += pes_pkt_data_bytes;
        if(!m_bProgramStream)
          localoffset += pes_pkt_data_bytes;

        if(bContinue)
        {
           retError = MP2STREAM_SUCCESS;
        }
        if(m_bProgramStream)
        {
          bContinue = false;
        }
      }
    }
    /* mark the flag as true, if ES parsing is successful at least once */
    if(MP2STREAM_SUCCESS == retError)
    {
      if(m_currPESPkt.packet_length > (uint32)(m_currPESPkt.pes_hdr_data_length + PES_PKT_FIXED_HDR_BYTES))
        m_currPESPkt.packet_length = m_currPESPkt.packet_length - (m_currPESPkt.pes_hdr_data_length + PES_PKT_FIXED_HDR_BYTES);
      m_bpartialPESTS = true;
    }
  }
  return retError;
}
/*! ======================================================================
@brief  Parses Picture Header data from currently parsed PES packet.

@detail

@param[in] N/A

@return
@note
========================================================================== */
bool MP2StreamParser::parsePictureHeader(uint64* nOffset)
{
  uint64 localOffset = *nOffset;
  bool nRet = true;
  uint8 val = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if((m_pStream_Info->stream_id == m_currTSPkt.PID) ||
        (m_pStream_Info->stream_id == m_currPESPkt.trackid ))

    {
      getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,10);
      (*nOffset)++;
      m_pStream_Info[i].video_stream_info.PictureData.temporal_reference = val;
      getByteFromBitStream(&val,&m_pDataBuffer[localOffset],10,3);
      m_pStream_Info[i].video_stream_info.PictureData.picture_coding_type = val;
      getByteFromBitStream(&val,&m_pDataBuffer[localOffset],13,16);
      m_pStream_Info[i].video_stream_info.PictureData.vbv_delay = val;
      if((m_pStream_Info[i].video_stream_info.PictureData.picture_coding_type == 2) ||
         (m_pStream_Info[i].video_stream_info.PictureData.picture_coding_type == 3))
      {
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],13,1);
        m_pStream_Info[i].video_stream_info.PictureData.full_pel_forward_vector = val;
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],14,3);
        m_pStream_Info[i].video_stream_info.PictureData.forward_f_code = val;
      }
      if((m_pStream_Info[i].video_stream_info.PictureData.picture_coding_type == 3))
      {
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],17,1);
        m_pStream_Info[i].video_stream_info.PictureData.full_pel_backward_vector = val;
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],18,3);
        m_pStream_Info[i].video_stream_info.PictureData.backward_f_code = val;
      }
    }
  }
  return nRet;
}
/*! ======================================================================
@brief  Parses video meta data from currently parsed PES packet
        to retrieve resolution, aspect ratio etc.

@detail    Parses video meta data from currently parsed PES packet
        to retrieve resolution, aspect ratio etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseVideoMetaData(int32* nBytesRead)
{
  bool bRet = false;
  uint8 TempBuffer[4];
  int localoffset = 0;
  uint8 val = 0;
  bool bContinue = true;

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"parseVideoMetaData");
#endif

  if(!m_bProgramStream)
  {
    if(m_ProgMapSection && (m_ProgMapSection->ESDescData))
    {
      for(int i=0; i < m_ProgMapSection->nESDesc ; i++)
      {
        if(m_bInitialParsingPending)
        {
          if(m_ProgMapSection->ESDescData[i].stream_type == AVC_VIDEO_STREAM_TYPE)
          {
            for(int j = 0; j< m_nstreams; j++)
            {
              if(m_pStream_Info)
              {
                if(m_pStream_Info[j].stream_id == m_currTSPkt.PID)
                {
                  m_pStream_Info[j].video_stream_info.Video_Codec = VIDEO_CODEC_H264;
                  m_pStream_Info[j].bParsed = true;
                  bRet = true;
                  break;
                }
              }
            }
            bContinue = false;
          }
        }
        else
        {
          bRet = true;
          bContinue = false;
          break;
        }
      }
    }
  }
  else
  {
    if (NULL == m_currPackHeader.sys_header)
    {
      bContinue = false;
    }
    else if(!m_pProgramStreamMap && m_nVideoStreams == 0)
    {
      uint32 ulCodeVal = getBytesValue(sizeof(uint32),m_pDataBuffer);

      if ((FULL_SEQUENCE_HEADER_CODE == ulCodeVal ) ||
          (FULL_USER_DATA_START_CODE == ulCodeVal ) ||
          (FULL_GROUP_START_CODE == ulCodeVal ) ||
          (FULL_SEQUENCE_ERROR_CODE == ulCodeVal ) ||
          (FULL_EXTENSION_START_CODE == ulCodeVal ) ||
          (FULL_SEQUENCE_END_CODE == ulCodeVal ) )
      {
        /* Update track properties. */
        updateTotalTracks(MPEG2_VIDEO_STREAM_TYPE, m_currPESPkt.trackid);
        if(!m_nVideoPIDSelected)
          m_nStreamsSelected++;
        m_nVideoPIDSelected = m_currPESPkt.trackid;
      }
      else if ((AVC_4BYTE_START_CODE == ulCodeVal ) ||
               (AVC_3BYTE_START_CODE == ulCodeVal ))
      {
        /* Update track properties. */
        updateTotalTracks(AVC_VIDEO_STREAM_TYPE, m_currPESPkt.trackid);
        if(!m_nVideoPIDSelected)
          m_nStreamsSelected++;
        m_nVideoPIDSelected = m_currPESPkt.trackid;
      }
    }
  }
  if(bContinue)
  {
    while((bContinue) && (nBytesRead) && (localoffset < *nBytesRead))
    {
      //make sure there is video prefix start code
      memcpy(TempBuffer,m_pDataBuffer+localoffset,3);
      uint32 startcodeval = getBytesValue(3,TempBuffer);
      //make sure there is video prefix start code
      if(startcodeval == VIDEO_START_CODE_PREFIX)
      {
        localoffset+= 3;
        if(m_pDataBuffer[localoffset] == USER_DATA_START_CODE)
        {
          localoffset++;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"USER_DATA_START_CODE found");
          memcpy(TempBuffer,m_pDataBuffer+localoffset,4);
          uint32 startcodeval = getBytesValue(4,TempBuffer);
          if(startcodeval == AFD_START_CODE)
          {
            localoffset += 4;
            getByteFromBitStream(&val,&m_pDataBuffer[localoffset],0,1);
            if(val != 0)
            {
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"AFD corrupted");
            }
            else
            {
              getByteFromBitStream(&val,&m_pDataBuffer[localoffset],1,1);
              if(val == 1)
              {
                getByteFromBitStream(&val,&m_pDataBuffer[localoffset],11,4);
              }
            }
          }
        }
        else if(m_pDataBuffer[localoffset] == PICTURE_START_CODE)
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"PICTURE_START_CODE found");
          localoffset++;
          uint64 nOffset = (uint64)localoffset;
          if(!parsePictureHeader(&nOffset))
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"parsePictureHeader failed");
          }
        }
        else if(m_pDataBuffer[localoffset] == GROUP_START_CODE)
        {
          localoffset++;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"GROUP_START_CODE found");
        }
        else if(m_pDataBuffer[localoffset] == SEQUENCE_HEADER_CODE)
        {
          //skip bytes to get the resolution.
          localoffset++;

          //next 3 bytes contains (12 bits)width and (12 bits)height
          uint16 width1 = ((uint16)m_pDataBuffer[localoffset])<<4;
          uint16 width2 = ((uint16)(m_pDataBuffer[localoffset+1] & 0xF0))>>4;
          uint16 width = width1 | width2;
          uint16 height1 = ((uint16)(m_pDataBuffer[localoffset+1] & 0x0F))<<8;
          uint16 height  = height1 | m_pDataBuffer[localoffset+2];
          localoffset+= 3;

          //next byte gives aspect ratio and frame rate
          uint8 aspect_ratio = (m_pDataBuffer[localoffset]& 0xF0)>>4;
          uint8 frame_rate   = (m_pDataBuffer[localoffset]& 0x0F);
          localoffset++;

          //18 bits nominal bit-rate
          uint32 nominal_bitrate = (((uint32)m_pDataBuffer[localoffset])<<8) | (m_pDataBuffer[localoffset+1]);
          nominal_bitrate <<= 2;
          localoffset+=2;
          //get remaining 2 bits for nominal bit-rate
          nominal_bitrate |= ((m_pDataBuffer[localoffset] & 0xC0) >> 6);

          //10 bits vbv buffer size
          uint16 vbv_buffer_size  = ((uint16)(m_pDataBuffer[localoffset] & 0x3F))<<4;
          localoffset++;
          vbv_buffer_size |= ((m_pDataBuffer[localoffset] & 0xF0)>>4);
          vbv_buffer_size += 0;

          uint16 usPID = m_currTSPkt.PID;
          /* If PSM (Program Stream Map) is not present in Program Streams,
             then parser not update total tracks and other useful information.
             So by using system target header info, update track properties and
             other required info. */
          if (m_bProgramStream && m_currPackHeader.sys_header)
          {
            usPID = m_currPESPkt.trackid;
            uint32 ulTotalTracks = m_currPackHeader.sys_header->audio_bound +
                                   m_currPackHeader.sys_header->video_bound;
            if ((m_pStream_Info) &&
                (m_nVideoStreams < m_currPackHeader.sys_header->video_bound))
            {
              uint32 ulCount = 0;
              m_nstreams = (uint8)ulTotalTracks;
              //Check the structure for uninitialized entry
              while((m_pStream_Info[ulCount++].stream_media_type !=
                     TRACK_TYPE_UNKNOWN) &&
                    (ulCount < m_nstreams));
              m_pStream_Info[ulCount-1].stream_id = usPID;
              m_pStream_Info[ulCount-1].stream_media_type = TRACK_TYPE_VIDEO;
              if (!m_pVideoStreamIds)
              {
                m_pVideoStreamIds = (uint16*)MM_Malloc(m_nstreams *
                                                       sizeof(uint16));
              }
              if (m_pVideoStreamIds)
              {
                m_pVideoStreamIds[m_nVideoStreams] = usPID;
              }
              m_nVideoStreams++;
              if (!m_nVideoPIDSelected)
              {
                m_nVideoPIDSelected = usPID;
              }
            }
          }

          //Store parsed video meta-data in stream info
          for(int i = 0; i< m_nstreams; i++)
          {
            if( m_pStream_Info && (m_pStream_Info +i) )
            {
              if(m_pStream_Info[i].stream_id == usPID)
              {
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Aspect Ratio : %d(REFER to ENUM VALUES)",aspect_ratio);
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Bit-Rate     : %lu",nominal_bitrate);
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Frame Rate   : %d(REFER to ENUM VALUES)",frame_rate);
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Height       : %d",height);
                MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"Width        : %d",width);

                m_pStream_Info[i].video_stream_info.Aspect_Ratio = (video_aspect_ratio)aspect_ratio;
                m_pStream_Info[i].bitRate = nominal_bitrate;
                m_pStream_Info[i].video_stream_info.Frame_Rate = (video_frame_rate)frame_rate;
                m_pStream_Info[i].video_stream_info.Height = height;
                m_pStream_Info[i].video_stream_info.Width = width;
                m_pStream_Info[i].video_stream_info.Video_Codec = VIDEO_CODEC_MPEG2;
                m_pStream_Info[i].bParsed = true;
                break;
              }
            }
          }
        }
        else
        {
          localoffset++;
        }
      }
      else
      {
        localoffset += 1;
      }
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Parses audio meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@detail    Parses audio meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseAudioMetaData()
{
  bool bRet = false;
  int localoffset = 0;
  int no_frame_headers = 0;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"parseAudioMetaData");
#endif
  bool bESStreamTypeSet = false;
  uint16 usPID = m_currTSPkt.PID;
  //! Check whether memory allocated successfully or not
  if(NULL == m_pStream_Info)
  {
    return bRet;
  }
  /* If PSM (Program Stream Map) is not present in Program Streams, then parser
     not update total tracks and other useful information. So by using system
     target header info, update track properties and other required info. */
  if (m_bProgramStream && m_currPackHeader.sys_header)
  {
    usPID = m_currPESPkt.trackid;
    uint32 ulTotalTracks = m_currPackHeader.sys_header->audio_bound +
                           m_currPackHeader.sys_header->video_bound;
    if (m_pStream_Info &&
        m_nAudioStreams < m_currPackHeader.sys_header->audio_bound)
    {
      uint32 ulCount = 0;
      m_nstreams = (uint8)ulTotalTracks;
      //Check the structure for uninitialized entry
      while((m_pStream_Info[ulCount++].stream_media_type !=
             TRACK_TYPE_UNKNOWN) &&
            (ulCount < m_nstreams));
      m_pStream_Info[ulCount-1].stream_id = usPID;
      m_pStream_Info[ulCount-1].stream_media_type = TRACK_TYPE_AUDIO;
      if (!m_pAudioStreamIds)
      {
        m_pAudioStreamIds = (uint16*)MM_Malloc(m_nstreams * sizeof(uint16));
      }
      if(m_pAudioStreamIds)
      {
        m_pAudioStreamIds[m_nAudioStreams] = usPID;
      }
      m_nAudioStreams++;
      if (!m_nAudioPIDSelected)
      {
        m_nAudioPIDSelected = usPID;
      }
    }
  }
  if(m_ProgMapSection && (m_ProgMapSection->ESDescData))
  {
    for(int i=0; i < m_ProgMapSection->nESDesc ; i++)
    {
      if (m_currPESPkt.tsPID != m_ProgMapSection->ESDescData[i].elementary_pid)
      {
        continue;
      }
      //! If PES Packet is encrypted, do not look for audio track properties
      else if (m_currPESPkt.pes_extn_hdr.pes_extn_pvt_data_flag)
      {
        bRet = true;
        m_pStream_Info[i].bParsed = true;
      }
      else if(m_ProgMapSection->ESDescData[i].stream_type == AAC_ADTS_STREAM_TYPE)
      {
        bESStreamTypeSet = true;
        //check if we have sync in place
        if( (m_pDataBuffer[0] != 0xFF) && ( (m_pDataBuffer[1] & 0xF0) != 0xF0 ) )
        {
          //No ADTS SYNC
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseAudioMetaData AAC_ADTS_SYNC_WORD not detected...");
          //Make sure current TS pkt is marked as carrying payload unit start indicator
          //If yes, scan current PES packet fully to locate the SYNC...
          if(m_currTSPkt.pyld_unit_start_indicator)
          {
            uint32 nstartreadoffset = (uint32)m_currPESPkt.noffset+ PES_PKT_START_CODE_STREAM_ID_SIZE;
            //for elementary streams, fixed header is PES_PKT_FIXED_HDR_BYTES bytes
            nstartreadoffset+= PES_PKT_FIXED_HDR_BYTES;
            //add the length of optional fields including padding..
            nstartreadoffset+= m_currPESPkt.pes_hdr_data_length;
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"data_align_indicator is set, nstartreadoffset %ld...",nstartreadoffset);
            uint32 nbytestoscan = (uint32)(TS_PKT_SIZE - (m_currPESPkt.noffset - m_currTSPkt.noffset));
            if(!readMpeg2StreamData (nstartreadoffset, nbytestoscan,m_pDataBuffer, m_nDataBufferSize,(uint32)m_pUserData) )
            {
              //We should never come here as we already verify that whole TS packet
              //carrying this PES is available.
              MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"could not read %ld from offset %ld",m_currPESPkt.packet_length,nstartreadoffset);
            }
            else
            {
              for(uint32 i = 0; (nbytestoscan && (i < (nbytestoscan-1)));i++)
              {
                if( (m_pDataBuffer[i] == 0xFF) && ( (m_pDataBuffer[i+1] & 0xF0) == 0xF0 ) )
                {
                  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"sync word located at %ld...",(nstartreadoffset+i));
                  memmove(m_pDataBuffer,m_pDataBuffer+i,(nbytestoscan-i));
                  break;
                }//if( (m_pDataBuffer[i] == 0xFF) && ( (m_pDataBuffer[i+1] & 0xF0) == 0xF0 ) )
              }//for(int i = 0; (nbytestoscan && (i < (nbytestoscan-1)));i++)
            }
          }
        }//if( (m_pDataBuffer[0] != 0xFF) && ( (m_pDataBuffer[1] & 0xF0) != 0xF0 ) )
        if( (m_pDataBuffer[0] == 0xFF) && ( (m_pDataBuffer[1] & 0xF0) == 0xF0 ) )
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseAudioMetaData AAC_ADTS_SYNC_WORD");
          //ADTS Frame starts from here
          uint8 ucProtectionBit = (m_pDataBuffer[1] & 0x01);
          uint8 audioObjectType = ((m_pDataBuffer[2] >> 6) & 0x03)+1;
          uint8 samplingFrequencyIndex = ((m_pDataBuffer[2] >> 2) & 0x0F);
          uint8 channelConfiguration   = ((m_pDataBuffer[2] << 2) & 0x04) | ((m_pDataBuffer[3] >> 6) & 0x03);

          for(int i = 0; i< m_nstreams; i++)
          {
            if(m_pStream_Info && (m_pStream_Info+i) )
            {
              if(m_pStream_Info[i].stream_id == m_currTSPkt.PID)
              {
                m_pStream_Info[i].audio_stream_info.ucProtection =
                  (ucProtectionBit == 0)?1:0;
                m_pStream_Info[i].audio_stream_info.AudioObjectType =
                  audioObjectType;
                m_pStream_Info[i].audio_stream_info.Audio_Codec =
                  AUDIO_CODEC_AAC;
                m_pStream_Info[i].audio_stream_info.NumberOfChannels =
                  channelConfiguration;

                uint32 tableSize = sizeof(AAC_SAMPLING_FREQUENCY_TABLE)/
                  sizeof(unsigned long);
                if(m_pStream_Info[i].audio_stream_info.SamplingFrequency
                    < tableSize)
                {
                  m_pStream_Info[i].audio_stream_info.SamplingFrequency =
                    AAC_SAMPLING_FREQUENCY_TABLE[samplingFrequencyIndex];
                }
                bRet = true;
                m_pStream_Info[i].bParsed = true;
                break;
              }
            }
          }//for(int i = 0; i< m_nstreams; i++)
        }
      }
      else if((m_ProgMapSection->ESDescData[i].stream_type == MPEG1_AUDIO_STREAM_TYPE)||
        (m_ProgMapSection->ESDescData[i].stream_type == MPEG2_AUDIO_STREAM_TYPE))
      {
        bESStreamTypeSet = true;
        if((0xFF == m_pDataBuffer[0]) && (0xE0 == (m_pDataBuffer[1] & 0xE0)))
        {
          bRet = true;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseAudioMetaData MP3_SYNC_WORD");
          int version = (mp3_ver_enum_type)
                  ((m_pDataBuffer[MP3HDR_VERSION_OFS] & MP3HDR_VERSION_M) >> MP3HDR_VERSION_SHIFT);
          m_pStream_Info[i].audio_stream_info.ucVersion = (uint8)version;
          if (!(MP3_VER_25 == version ||
                MP3_VER_2 == version ||
                MP3_VER_1 == version))
          {
            bRet = false;
          }
          int layer = (mp3_layer_enum_type)
                  ((m_pDataBuffer[MP3HDR_LAYER_OFS] & MP3HDR_LAYER_M) >> MP3HDR_LAYER_SHIFT);

          m_pStream_Info[i].audio_stream_info.ucLayer = (uint8)layer;
          uint8 ucBitRateIndex = ((m_pDataBuffer[MP3HDR_BITRATE_OFS] & MP3HDR_BITRATE_M) >> MP3HDR_BITRATE_SHIFT);
          m_pStream_Info[i].audio_stream_info.Bitrate = 1000 *
                MP3_BITRATE[(version == MP3_VER_1 ? 0 : 1)]
                [layer][ucBitRateIndex];
          m_pStream_Info[i].bitRate = m_pStream_Info[i].audio_stream_info.Bitrate;
          // Check if MPEG layer of input data  is supported
          media_codec_type audioCodec = UNKNOWN_AUDIO_VIDEO_CODEC;
          if (MP3_LAYER_3 == layer)
          {
            audioCodec = AUDIO_CODEC_MP3;
          }
          else if(MP3_LAYER_2 == layer)
          {
            audioCodec = AUDIO_CODEC_MPEG2;
          }
          else
          {
            bRet = false;
            m_naudioIndex --;
            m_nstreams--;
            m_nAudioStreams--;
            m_nStreamsSelected--;
          }
          uint8 sample_index =  ((m_pDataBuffer[MP3HDR_SAMPLERATE_OFS] &
                            MP3HDR_SAMPLERATE_M) >> MP3HDR_SAMPLERATE_SHIFT);

          for(int i = 0; i< m_nstreams; i++)
          {
            if(bRet && (m_pStream_Info[i].stream_id == m_currTSPkt.PID))
            {
              m_pStream_Info[i].audio_stream_info.SamplingFrequency =
                     MP3_SAMPLING_RATE[version][sample_index];
              uint8 ucChannel =
                     ((m_pDataBuffer[MP3HDR_CHANNEL_OFS] & MP3HDR_CHANNEL_M) >>
                      MP3HDR_CHANNEL_SHIFT);
              switch(ucChannel)
              {
                case MP3_CHANNEL_STEREO:
                case MP3_CHANNEL_JOINT_STEREO:
                case MP3_CHANNEL_DUAL:
                {
                  m_pStream_Info[i].audio_stream_info.NumberOfChannels = 2;
                }
                break;
                case MP3_CHANNEL_SINGLE:
                {
                  m_pStream_Info[i].audio_stream_info.NumberOfChannels = 1;
                }
                break;
                default:
                break;
              }
              m_pStream_Info[i].audio_stream_info.Audio_Codec = audioCodec;
              bRet = true;
              m_pStream_Info[i].bParsed = true;
              break;
            }
          }
        }
      }
      else if(m_ProgMapSection->ESDescData[i].stream_type == LPCM_AUDIO_STREAM_TYPE)
      {
        bESStreamTypeSet = true;
        audio_info pcmAudioInfo;
        bRet = parseLPCMHeader(&pcmAudioInfo);
        if(bRet)
        {
          for(int i = 0; i< m_nstreams; i++)
          {
            if(m_pStream_Info && (m_pStream_Info+i) )
            {
              if( (m_pStream_Info[i].stream_id == m_currTSPkt.PID) &&
                  (!m_pStream_Info[i].bParsed) )
              {
                m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_LPCM;
                m_pStream_Info[i].audio_stream_info.NumberOfFrameHeaders = pcmAudioInfo.NumberOfFrameHeaders;
                m_pStream_Info[i].audio_stream_info.SamplingFrequency = pcmAudioInfo.SamplingFrequency;
                m_pStream_Info[i].audio_stream_info.NumberOfChannels = pcmAudioInfo.NumberOfChannels;
                m_pStream_Info[i].bParsed = true;
                bRet = true;
                break;
              }
            }
          }//for(int i = 0; i< m_nstreams; i++)
        }
      }
      else if(m_ProgMapSection->ESDescData[i].stream_type == AC3_AUDIO_STREAM_TYPE)
      {
        audio_info ac3AudioInfo;
        memset(&ac3AudioInfo, 0, sizeof(audio_info));
        bRet = parseAC3Header(&ac3AudioInfo);
        for(int i = 0; i< m_nstreams; i++)
        {
          if(m_pStream_Info && (m_pStream_Info+i) )
          {
            if( (m_pStream_Info[i].stream_id == m_currTSPkt.PID) &&
                (!m_pStream_Info[i].bParsed) )
            {
              m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_AC3;
              m_pStream_Info[i].audio_stream_info.NumberOfFrameHeaders = ac3AudioInfo.NumberOfFrameHeaders;
              m_pStream_Info[i].audio_stream_info.SamplingFrequency = ac3AudioInfo.SamplingFrequency;
              m_pStream_Info[i].bitRate = ac3AudioInfo.Bitrate;
              m_pStream_Info[i].audio_stream_info.NumberOfChannels = ac3AudioInfo.NumberOfChannels;
              bRet = true;
              m_pStream_Info[i].bParsed = true;
              break;
            }
          }
        }
      }
      else if(m_ProgMapSection->ESDescData[i].stream_type == PES_PVT_STREAM_TYPE)
      {
        //Check if registration descriptor has been parsed to identify stream type
        if(m_pRegistrationDescriptor)
        {
          for(int i = 0; i< m_nstreams; i++)
          {
            if(m_pStream_Info && (m_pStream_Info+i) )
            {
              if( (m_pStream_Info[i].stream_id == m_currTSPkt.PID) &&
                  (!m_pStream_Info[i].bParsed) )
              {
                if (m_pRegistrationDescriptor->ullFormatIdentifier == TS_REGIS_FORMATID_AC3)
                {
                    m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_AC3;
                    m_pStream_Info[i].stream_media_type = TRACK_TYPE_AUDIO;
                }
                if( (m_pRegistrationDescriptor->ullFormatIdentifier == TS_REGIS_FORMATID_DTS1) ||
                    (m_pRegistrationDescriptor->ullFormatIdentifier == TS_REGIS_FORMATID_DTS2) ||
                    (m_pRegistrationDescriptor->ullFormatIdentifier == TS_REGIS_FORMATID_DTS3) )
                {
                  m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_DTS;
                  if(m_pDTSAudioDescriptor)
                  {
                    if(m_pDTSAudioDescriptor->ucSampleRateCode < 15)
                    {
                      m_pStream_Info[i].audio_stream_info.SamplingFrequency =
                        TS_DTS_FSCODE_RATE[m_pDTSAudioDescriptor->ucSampleRateCode];
                    }
                    if(m_pDTSAudioDescriptor->ucBitrateCode < 21)
                    {
                      m_pStream_Info[i].audio_stream_info.Bitrate =
                        TS_DTS_BIT_RATE[m_pDTSAudioDescriptor->ucBitrateCode];
                    }
                    if(m_pDTSAudioDescriptor->ucSurroundMode < 10)
                    {
                      m_pStream_Info[i].audio_stream_info.NumberOfChannels =
                        (uint8)TS_DTS_CHANNELS[m_pDTSAudioDescriptor->ucSurroundMode];
                    }
                    bRet = true;
                    m_pStream_Info[i].bParsed = true;
                    break;
                  }
                }
                else if(m_pRegistrationDescriptor->ullFormatIdentifier == TS_REGIS_FORMATID_DTSH)
                {
                  m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_DTS;
                  if( (m_pDTSHDAudioDescriptor) && (m_pDTSHDAudioDescriptor->ucSubstreamCoreFlag) )
                  {
                    if(m_pStream_Info[i].audio_stream_info.SamplingFrequency < 16)
                    {
                      m_pStream_Info[i].audio_stream_info.SamplingFrequency =
                        TS_DTSHD_FSCODE_RATE[m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->ucSamplingFrequency];
                    }
                    m_pStream_Info[i].audio_stream_info.NumberOfChannels =
                      m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->ucChannelCount;
                    bRet = true;
                    m_pStream_Info[i].bParsed = true;
                    break;
                  }
                }
              }
            }
          }
        }
      }
      else if( (m_ProgMapSection->ESDescData[i].stream_type == HDMV_DTS_STREAM_TYPE) ||
               (m_ProgMapSection->ESDescData[i].stream_type == DTS_HD_STREAM_TYPE) )
      {
        audio_info dtsAudioInfo;
        bRet = parseDTSHeader(&dtsAudioInfo);
        if(bRet)
        {
          for(int i = 0; i< m_nstreams; i++)
          {
            if(m_pStream_Info && (m_pStream_Info+i) )
            {
              if( (m_pStream_Info[i].stream_id == m_currTSPkt.PID) &&
                  (!m_pStream_Info[i].bParsed) )
              {
                m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_DTS;
                m_pStream_Info[i].audio_stream_info.SamplingFrequency = dtsAudioInfo.SamplingFrequency;
                m_pStream_Info[i].bitRate = dtsAudioInfo.Bitrate;
                m_pStream_Info[i].audio_stream_info.NumberOfChannels = dtsAudioInfo.NumberOfChannels;
                bRet = true;
                m_pStream_Info[i].bParsed = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  if(m_bProgramStream)
  {
    bool   bContinue = true;
    uint32 ulIndex   = 0;
    audio_info *pAudioStreamInfo = NULL;
    for(; ulIndex < m_nstreams; ulIndex++)
    {
      if(m_pStream_Info && (m_pStream_Info+ulIndex) )
      {
        if(m_pStream_Info[ulIndex].stream_id == usPID)
        {
          pAudioStreamInfo = &m_pStream_Info[ulIndex].audio_stream_info;
          break;
        }
      }
    }//for loop
    if (!pAudioStreamInfo)
    {
      return false;
    }

    if (UNKNOWN_AUDIO_VIDEO_CODEC == pAudioStreamInfo->Audio_Codec &&
        (usPID >= AUDIO_STREAM_ID_START) &&
        (usPID <= AUDIO_STREAM_ID_END))
    {
      aac_audio_info sAACFrameInfo;
      bRet = parseAACHeader(&sAACFrameInfo);
      if (false == bRet)
      {
        pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_MP3;
      }
      else
      {
        /*Copy first frame properties into class variable and compare them with
          second frame properties, they should match. Else we can treat
          bit-stream as MPG compliant. */
        if(m_AACAudioInfo.SamplingFrequency == 0)
        {
          memcpy(&m_AACAudioInfo, &sAACFrameInfo, sizeof(aac_audio_info));
        }
        else if((m_AACAudioInfo.SamplingFrequency !=
                 sAACFrameInfo.SamplingFrequency) ||
                (m_AACAudioInfo.AudioObjectType   !=
                 sAACFrameInfo.AudioObjectType) )
        {
          pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_MP3;
        }
        else
        {
          pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_AAC;
        }
      }
    }
    if( AUDIO_CODEC_AAC == pAudioStreamInfo->Audio_Codec)
    {
      aac_audio_info sAACFrameInfo;
      bool bIsAAC = parseAACHeader(&sAACFrameInfo);;

      if(true == bIsAAC)
      {
        pAudioStreamInfo->AudioObjectType =
          sAACFrameInfo.AudioObjectType;
        pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_AAC;
        pAudioStreamInfo->NumberOfChannels =
          sAACFrameInfo.NumberOfChannels;
        pAudioStreamInfo->SamplingFrequency =
          sAACFrameInfo.SamplingFrequency;

        bRet      = true;
        bContinue = false;
        m_pStream_Info[ulIndex].bParsed = true;
      }
    }
    else if(LPCM_AUDIO_STREAM_TYPE == pAudioStreamInfo->Audio_Codec)
    {
      audio_info pcmAudioInfo;
      memset(&pcmAudioInfo, 0, sizeof(audio_info));
      bRet = parseLPCMHeader(&pcmAudioInfo);
      if((bRet) && (!m_pStream_Info[ulIndex].bParsed))
      {
        pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_LPCM;
        pAudioStreamInfo->SamplingFrequency =
          pcmAudioInfo.SamplingFrequency;
        pAudioStreamInfo->NumberOfChannels  =
          pcmAudioInfo.NumberOfChannels;
        pAudioStreamInfo->NumberOfFrameHeaders =
                       pcmAudioInfo.NumberOfFrameHeaders;
        m_pStream_Info[ulIndex].bParsed = true;
        bContinue = false;
      }
    }
    if((bContinue &&
       (0xFF == m_pDataBuffer[0]) && (0xE0 == (m_pDataBuffer[1] & 0xE0))) ||
      (AUDIO_CODEC_MP3   == pAudioStreamInfo->Audio_Codec ||
      (AUDIO_CODEC_MPEG2 == pAudioStreamInfo->Audio_Codec)))
    {
      bRet = true;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
          "parseAudioMetaData MPEG AUDIO SYNC WORD");

      int version = (mp3_ver_enum_type)
              ((m_pDataBuffer[MP3HDR_VERSION_OFS] & MP3HDR_VERSION_M) >> MP3HDR_VERSION_SHIFT);

      if (!(MP3_VER_25 == version ||
            MP3_VER_2 == version ||
            MP3_VER_1 == version))
      {
        bRet = false;
      }
      int layer = (mp3_layer_enum_type)
              ((m_pDataBuffer[MP3HDR_LAYER_OFS] & MP3HDR_LAYER_M) >> MP3HDR_LAYER_SHIFT);

      uint8 sample_index =  ((m_pDataBuffer[MP3HDR_SAMPLERATE_OFS] &
                              MP3HDR_SAMPLERATE_M) >> MP3HDR_SAMPLERATE_SHIFT);

      if(bRet)
      {
        pAudioStreamInfo->SamplingFrequency = MP3_SAMPLING_RATE[version][sample_index];
        pAudioStreamInfo->NumberOfChannels =((m_pDataBuffer[3] & 0x30) >> 4);
        if (MP3_LAYER_3 == layer)
        {
          pAudioStreamInfo->Audio_Codec = AUDIO_CODEC_MP3;
        }
        else if(MP3_LAYER_2 == layer)
        {
          pAudioStreamInfo->Audio_Codec  = AUDIO_CODEC_MPEG2;
        }
        bRet = true;
        m_pStream_Info[ulIndex].bParsed = true;
        bContinue = false;
      }
    }
    //Audio Sub-stream header for AC3 does not exist for TS
    if( (m_pDataBuffer[localoffset] >= AC3_AUDIO_SUBSTREAM_ID_BEG) &&
        (m_pDataBuffer[localoffset] <= AC3_AUDIO_SUBSTREAM_ID_END) )
    {
      //Advance the offset as we have identified the sub-stream id
      localoffset++;
      //next byte gives number of frame headers
      no_frame_headers = m_pDataBuffer[localoffset];
      localoffset++;
      //next 2 bytes represents first access unit pointer
      uint16 first_access_unit_ptr = (((uint16)m_pDataBuffer[localoffset])<<8)||m_pDataBuffer[localoffset+1];
      localoffset+= 2;

      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 no_frame_headers %d",no_frame_headers);
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 first_access_unit_ptr %d",first_access_unit_ptr);
    }
  }
  uint16 word  = (((uint16)m_pDataBuffer[localoffset])<<8) | m_pDataBuffer[localoffset+1];

    //Parse within first AC3 audio access unit to get
    //sampling frequency and number of channels
    //make sure next 2 bytes represent AC3 SYNC WORD

  if((!bESStreamTypeSet)&&(word == AC3_SYNC_WORD))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseAudioMetaData Detected AC3 AUDIO");
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 sync word %x",word);
    //Sync word is followed by 16 bits CRC, so increment the offset by 4.
    localoffset+= 4;

    //Next byte gives sampling frequency and bit-rate
    int fscod = (m_pDataBuffer[localoffset] & 0xC0)>>6;
    int frmsizecod = m_pDataBuffer[localoffset] & 0x3F;
    localoffset++;

    uint32 samplingfreq = 48000;
    uint32 bit_rate     = 0;
    switch(fscod)
    {
      case SAMPLE_RATE_48_KHZ:
      {
        samplingfreq = 48000;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 48 KHZ");
        break;
      }
      case SAMPLE_RATE_44_1_KHZ:
      {
        samplingfreq = 44100;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 44.1 KHZ");
        break;
      }
      case SAMPLE_RATE_32_KHZ:
      {
        samplingfreq = 32000;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 32 KHZ");
        break;
      }
      default:
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 USING DEFAULT SAMPLING FREQUENCY 48KHZ");
        break;
      }
    }//switch(fscod)
    if( (frmsizecod >= 0x00) && (frmsizecod <= 0x25) )
    {
      bit_rate = FRAME_SIZE_CODE_TABLE[frmsizecod].nominal_bit_rate;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 bit-rate %lu",bit_rate);
    }
    //Get the bit-stream identification, bit-stream Mode and number of channels
    uint8 bsid =  (m_pDataBuffer[localoffset] & 0xF1)>>3;
    uint8 bsmod = m_pDataBuffer[localoffset] & 0x07;
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 bsid %d bsmod %d",bsid,bsmod);
    localoffset++;
    uint8 acmod = (m_pDataBuffer[localoffset] & 0xE0)>>5;
    uint8 nChannels = 2;
    if( acmod <= 0x07 )
    {
      nChannels = CHANNELS_CONFIG[acmod].nfchans;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 #Channels %d",nChannels);
    }
    //Store parsed audio meta-data in stream info
    for(int i = 0; i< m_nstreams; i++)
    {
      if(m_pStream_Info && (m_pStream_Info+i) )
      {
        if( (m_pStream_Info[i].stream_id == usPID) &&
            (!m_pStream_Info[i].bParsed) )
        {
          m_pStream_Info[i].audio_stream_info.Audio_Codec = AUDIO_CODEC_AC3;
          m_pStream_Info[i].audio_stream_info.NumberOfFrameHeaders = no_frame_headers;
          m_pStream_Info[i].audio_stream_info.SamplingFrequency = samplingfreq;
          m_pStream_Info[i].bitRate = bit_rate;
          m_pStream_Info[i].audio_stream_info.NumberOfChannels = nChannels;
          bRet = true;
          m_pStream_Info[i].bParsed = true;
          break;
        }
      }
    }//for(int i = 0; i< m_nstreams; i++)
  }//if(word == AC3_SYNC_WORD)
  else
  {
    if(isTrackIdInIdStore(usPID))
    {
      for(int i = 0; i< m_nstreams; i++)
      {
        //If we have already updated m_naudioIndex,m_nstreams and m_nAudioStreams
        //even for unknown AUDIO, decrement them.
        if(m_pStream_Info && (m_pStream_Info+i) )
        {
          if((m_pStream_Info[i].stream_id == usPID) &&
            (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
            (m_pStream_Info[i].audio_stream_info.Audio_Codec == UNKNOWN_AUDIO_VIDEO_CODEC))
          {
            m_pStream_Info[i].stream_media_type = TRACK_TYPE_UNKNOWN;
            m_naudioIndex --;
            m_nstreams--;
            m_nAudioStreams--;
            break;
          }
        }
      }
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Parses LPCM meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@detail    Parses LPCM meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseLPCMHeader(audio_info* audioInfo)
{
  bool bRet = true;
  uint8 readIndex = 0;
  uint8 val = 0;

  audioInfo->Audio_Codec = AUDIO_CODEC_LPCM;

  //skipping 1 byte of substreamID
  readIndex++;

  audioInfo->NumberOfFrameHeaders = getBytesValue(sizeof(uint8),m_pDataBuffer+readIndex);
  readIndex++;

  //skipping one byte
  readIndex = readIndex + (1* sizeof(uint8));

  (void)getByteFromBitStream(&val,&m_pDataBuffer[readIndex],2,3);

  if(val < MAX_PCM_SAMPLING_FREQ_INDEX_VALUES)
  {
    audioInfo->SamplingFrequency = PCM_SAMPLING_FREQUENCY_TABLE[val];
  }
  else
  {
    bRet = false;
  }

  (void)getByteFromBitStream(&val,&m_pDataBuffer[readIndex],5,3);
  audioInfo->NumberOfChannels = val+1;

  return bRet;
}
/*! ======================================================================
@brief  Parses AC3 meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@detail    Parses AC3 meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseAC3Header(audio_info* audioInfo)
{
  int localoffset= 0;
  bool bRet = true;
  //Sync word is followed by 16 bits CRC, so increment the offset by 4.
  //Store parsed audio meta-data in stream info
  localoffset+=2;
  int fscod = (m_pDataBuffer[localoffset] & 0xC0)>>6;
  int frmsizecod = m_pDataBuffer[localoffset] & 0x3F;
  localoffset++;

  uint32 samplingfreq = 48000;
  uint32 bit_rate     = 0;
  switch(fscod)
  {
    case SAMPLE_RATE_48_KHZ:
    {
      samplingfreq = 48000;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 48 KHZ");
      break;
    }
    case SAMPLE_RATE_44_1_KHZ:
    {
      samplingfreq = 44100;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 44.1 KHZ");
      break;
    }
    case SAMPLE_RATE_32_KHZ:
    {
      samplingfreq = 32000;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 AUDIO 32 KHZ");
      break;
    }
    default:
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 USING DEFAULT SAMPLING FREQUENCY 48KHZ");
      break;
    }
  }//switch(fscod)
  audioInfo->SamplingFrequency = samplingfreq;
  if( (frmsizecod >= 0x00) && (frmsizecod <= 0x25) )
  {
    bit_rate = FRAME_SIZE_CODE_TABLE[frmsizecod].nominal_bit_rate;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 bit-rate %lu",bit_rate);
  }
  audioInfo->Bitrate = bit_rate;
  //Get the bit-stream identification, bit-stream Mode and number of channels
  uint8 bsid =  (m_pDataBuffer[localoffset] & 0xF1)>>3;
  uint8 bsmod = m_pDataBuffer[localoffset] & 0x07;
  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 bsid %d bsmod %d",bsid,bsmod);
  localoffset++;
  uint8 acmod = (m_pDataBuffer[localoffset] & 0xE0)>>5;
  uint8 nChannels = 2;
  if( acmod <= 0x07 )
  {
    nChannels = CHANNELS_CONFIG[acmod].nfchans;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"AC3 #Channels %d",nChannels);
  }
  audioInfo->NumberOfChannels = nChannels;
  return bRet;
}
/*! ======================================================================
@brief  Returns Width from video resolution reported by clip.

@detail    streamid identifies the video stream id

@param[in] N/A

@return    Video horizontal resolution if successful otherwise returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::GetVideoWidth(uint32 streamid)
{
  uint32 width = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == streamid)
    {
      width  = m_pStream_Info[i].video_stream_info.Width;
      break;
    }
  }
  return width;
}
/*! ======================================================================
@brief  Returns Height from video resolution reported by clip.

@detail    streamid identifies the video stream id

@param[in] N/A

@return    Video vertical resolution if successful otherwise returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::GetVideoHeight(uint32 streamid)
{
  uint32 height = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == streamid)
    {
      height  = m_pStream_Info[i].video_stream_info.Height;
      break;
    }
  }
  return height;
}
/*! ======================================================================
@brief  Returns audio sampling frequency for given audio track
@detail    trackd identifies the audio stream id

@param[in] N/A

@return    audio sampling frequency if successful otherwise returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::GetAudioSamplingFrequency(uint32 trackid)
{
  uint32 freq = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      freq  = m_pStream_Info[i].audio_stream_info.SamplingFrequency;
      break;
    }
  }
  return freq;
}
/*! ======================================================================
@brief  Returns number of audio channels for given audio track

@detail    trackid identifies the audio stream id

@param[in] N/A

@return    number of channels if successful otherwise returns 0
@note      None.
========================================================================== */
uint8  MP2StreamParser::GetNumberOfAudioChannels(uint32 trackid)
{
  uint8 nchannels = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      nchannels  = m_pStream_Info[i].audio_stream_info.NumberOfChannels;
      break;
    }
  }
  return nchannels;
}
/*! ======================================================================
@brief  Returns the layer info from a MPG audio track

@detail    trackid identifies the audio stream id

@param[in] N/A

@return    Layer Info if successful otherwise returns 0
@note      None.
========================================================================== */
uint8  MP2StreamParser::GetLayer(uint32 trackid)
{
  uint8 ucLayer = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      ucLayer  = m_pStream_Info[i].audio_stream_info.ucLayer;
      break;
    }
  }
  return ucLayer;
}
/*! ======================================================================
@brief  Returns the layer info from a MPG audio track

@detail    trackid identifies the audio stream id

@param[in] N/A

@return    Layer Info if successful otherwise returns 0
@note      None.
========================================================================== */
uint8  MP2StreamParser::GetVersion(uint32 trackid)
{
  uint8 ucVersion = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      ucVersion  = m_pStream_Info[i].audio_stream_info.ucVersion;
      break;
    }
  }
  return ucVersion;
}
/*! ======================================================================
@brief  Returns the bit-rate for given track

@detail    trackid identifies the track

@param[in] N/A

@return    bit-rate if successful otherwise returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::GetTrackAverageBitRate(uint32 trackid)
{
  uint32 bitrate = 0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      bitrate =   m_pStream_Info[i].bitRate;
      break;
    }
  }
  return bitrate;
}
/*! ======================================================================
@brief  Returns PTS from current PES packet parsed by mpeg2 parser

@detail    Returns PTS from current PES packet parsed by mpeg2 parser

@param[in] N/A

@return    PTS value from parsed PES packet
@note      None.
========================================================================== */
double MP2StreamParser::GetPTSFromCurrentPESPacket()
{
  return m_currPESPkt.pts;
}
/*! ======================================================================
@brief  Returns video frame rate for given track id

@detail    Returns PTS from current PES packet parsed by mpeg2 parser

@param[in] N/A

@return    Frame Rate fps for given track
@note      None.
========================================================================== */
float  MP2StreamParser::GetVideoFrameRate(uint32 trackid)
{
  float frate = 0.0;
  for(int i = 0; i< m_nstreams; i++)
  {
    if(m_pStream_Info[i].stream_id == trackid)
    {
      switch(m_pStream_Info[i].video_stream_info.Frame_Rate)
      {
        case FRAME_RATE_25_FPS:
          frate = 25.0;
          break;
        case FRAME_RATE_29_97_FPS:
          frate = (float)29.97;
          break;
        case FRAME_RATE_30_FPS:
          frate = (float)30;
          break;
      }
      break;
    }
  }
  return frate;
}
/*! ======================================================================
@brief  Returns video frame rate for given track id

@detail    Returns if codec info was found for AVC video

@param[in] nBytesRead: length of buffer in which to look for codec info
           dataBuffer: buffer in which to look

@return    TRUE or FALSE
@note      None.
========================================================================== */
bool  MP2StreamParser::GetAVCCodecInfo(uint32* nBytesRead, uint8* dataBuffer)
{
  uint32 index   = 0;
  uint8 nalUType = 0;
  uint32 spsLen  = 0;
  uint32 ppsLen  = 0;
  bool bRet      = false;
  int spsIndex   = 0;
  uint32 dataOffset =0;

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"MP2StreamParser::GetAVCCodecInfo");
#endif

  if(!m_pAvcCodecInfo)
  {
    m_pAvcCodecInfo = (avc_codec_info*)MM_Malloc(sizeof(avc_codec_info));
    if(m_pAvcCodecInfo)
    {
      memset(m_pAvcCodecInfo,0,sizeof(avc_codec_info));
    }
  }

  if(m_pAvcCodecInfo)
  {
    while((index < *nBytesRead))
    {
      if(GetNextH264NALUnit(index, dataBuffer, &nalUType, &spsLen, *nBytesRead, &dataOffset))
      {
        if(nalUType == NAL_UNIT_TYPE_SPS)
        {
          spsIndex = index;
          index += spsLen;
          if( (GetNextH264NALUnit(index, dataBuffer, &nalUType, &ppsLen, *nBytesRead, &dataOffset)) &&
              (nalUType == NAL_UNIT_TYPE_PPS))
          {
            if(!m_pAvcCodecInfo->codecInfoBuf)
            {
              m_pAvcCodecInfo->codecInfoBuf = (uint8*)MM_Malloc(spsLen+ppsLen);
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"GetAVCCodecInfo allocating %d", (spsLen+ppsLen));
            }
            else
            {
              m_pAvcCodecInfo->codecInfoBuf = (uint8*)MM_Realloc(m_pAvcCodecInfo->codecInfoBuf, spsLen+ppsLen);
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"GetAVCCodecInfo realloc %d", (spsLen+ppsLen));
            }
            if(m_pAvcCodecInfo->codecInfoBuf)
            {
              m_pAvcCodecInfo->size = spsLen+ppsLen;
              memcpy(m_pAvcCodecInfo->codecInfoBuf,dataBuffer+spsIndex,spsLen+ppsLen);
              bRet = true;
              m_pAvcCodecInfo->isValid = true;
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"GetAVCCodecInfo found AVC codec info");
              break;
            }
          }
          else
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetNextNALUnit returned false");
            break;
          }
        }
        else
        {
          index += dataOffset;
          index += spsLen;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"GetNextNALUnit not SPS, SKIP IT");
        }
      }
      else
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"GetNextNALUnit returned false");
#endif
        break;
      }
    }
  }
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"GetAVCCodecInfo returning %d", bRet);
#endif
  return bRet;
}
/*! ======================================================================
@brief  Parses DTS meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@detail    Parses DTS meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseDTSHeader(audio_info* pDTSAudioInfo)
{
  bool bRet = false;
  uint8 ucReadIndex = 0;
  uint8 ucVal = 0;

  if(pDTSAudioInfo)
  {
    if(!memcmp(m_pDataBuffer, (void*)DTS_SYNCWORD_CORE, FOURCC_SIGNATURE_BYTES))
    {
      pDTSAudioInfo->Audio_Codec = AUDIO_CODEC_DTS;
      ucReadIndex += 4; //skip 4 bytes of sync word

      ucReadIndex++;
      (void)getByteFromBitStream(&ucVal,&m_pDataBuffer[ucReadIndex],6,14);
      //frame size = 1+ val;
      ucReadIndex+=2;

      (void)getByteFromBitStream(&ucVal,&m_pDataBuffer[ucReadIndex],4,6);
      pDTSAudioInfo->NumberOfChannels = ucVal;
      ucReadIndex++;

      (void)getByteFromBitStream(&ucVal,&m_pDataBuffer[ucReadIndex],2,4);
      pDTSAudioInfo->SamplingFrequency = DTS_FSCODE_RATE[ucVal];
      //ucReadIndex++;

      (void)getByteFromBitStream(&ucVal,&m_pDataBuffer[ucReadIndex],6,5);
      pDTSAudioInfo->Bitrate = DTS_BIT_RATE[ucVal];
      bRet = true;
    }
  }
  return bRet;
}
/*! ======================================================================
@brief  Parses AAC meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@detail    Parses AAC meta data from currently parsed PES packet
        to retrieve sampling frequency, number of channels etc.

@param[in] N/A

@return    true if meta data is parsed successfully otherwise returns false
@note      None.
========================================================================== */
bool MP2StreamParser::parseAACHeader(aac_audio_info *AACInfo)
{
  bool bIsAAC = false;
  //ADTS Frame starts from here
  uint8 audioObjectType = ((m_pDataBuffer[2] >> 6) & 0x03)+1;
  uint8 samplingFrequencyIndex = ((m_pDataBuffer[2] >> 2) & 0x0F);
  uint8 channelConfiguration   = ((m_pDataBuffer[2] << 2) & 0x04) |
                                  ((m_pDataBuffer[3] >> 6) & 0x03);
  uint16 uData = (m_pDataBuffer[1] << 8) + m_pDataBuffer[0];

  // Verify sync word and layer field.
  if ((ADTS_HEADER_MASK_RESULT != (uData & ADTS_HEADER_MASK)) )
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                "AAC sync word/layer verification failed...");
  }
  else
  {
    // Extract frame length from the frame header
    uint64 frameLength
      = (static_cast<uint64> (m_pDataBuffer [3] & 0x03) << 11)
        | (static_cast<uint64> (m_pDataBuffer [4]) << 3)
        | (static_cast<uint64> (m_pDataBuffer [5] & 0xE0) >> 5);

    uint32 ulSampleFreq = AAC_SAMPLING_FREQUENCY_TABLE [samplingFrequencyIndex];

    // Frame Length field shoud non zero
    // Sampling Frequency should always proper value
    if ((0 == frameLength) || (samplingFrequencyIndex >= 12) ||
        (ulSampleFreq == 0) )
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                  "BitStream is not AAC complaint");
    }
    else
    {
      bIsAAC = true;
    }
    if(true == bIsAAC)
    {
      AACInfo->AudioObjectType = audioObjectType;
      AACInfo->NumberOfChannels = channelConfiguration;
      AACInfo->SamplingFrequency = ulSampleFreq;
    }
  }

  return bIsAAC;
}

