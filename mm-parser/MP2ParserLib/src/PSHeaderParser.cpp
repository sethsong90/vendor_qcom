/* =======================================================================
                              PSHeaderParser.cpp
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
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/src/PSHeaderParser.cpp#22 $
========================================================================== */
#include "MP2StreamParser.h"
#include "string.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMMalloc.h"

#include "math.h"
/*! ======================================================================
@brief  Parses program stream pack header.

@detail    Starts parsing pack header from current byte offset.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parsePackHeader(uint64& nOffset,bool bParseMultiplePESPkts,
                                                 uint32 trackId,uint8* dataBuffer,
                                                 uint32 nMaxBufSize, int32* nBytesRead)
{
  MP2StreamStatus retError = MP2STREAM_SUCCESS;
  bool bContinue = true;
  if(!readMpeg2StreamData (nOffset, MPEG2_PACK_HDR_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32)m_pUserData) )
  {
    retError = m_eParserState;
    bContinue = false;
  }
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS,MM_PRIO_HIGH,"parsePackHeader nOffset %llu",nOffset);
#endif

  if(bContinue)
  {
    m_currPackHeader.pack_start_code = MP2_PACK_START_CODE;
    m_currPackHeader.noffset = nOffset;
    m_nInitialPacksParsed++;
    //parse pack header fields from bytes read in
    int localoffset = 4;//we have already read in pack start code

    /*Check if Mpeg1 PS or Mpeg2 PS */

    if((m_pDataBuffer[4] & 0xf0) == 0x20)
    {
      m_bMpeg1Video = true;
    }

    if(m_bMpeg1Video)
    {
      nOffset += MPEG1_PACK_HDR_BYTES;
      uint8 firstpart = (m_pDataBuffer[localoffset] & 0x0E)<<4;//get ms 3 bits
      localoffset++;
      uint16 secondpart = ((m_pDataBuffer[localoffset] << 8) | (m_pDataBuffer[localoffset+1] & 0xFE) )>>1;
      localoffset+=2;
      uint16 thirdpart = ((m_pDataBuffer[localoffset] << 8) | (m_pDataBuffer[localoffset+1] & 0xFE) )>>1;
      localoffset+=2;
      m_currPackHeader.scr_val = (double)make33BitValue(firstpart,secondpart,thirdpart);
      uint16 muxRate1 = ((m_pDataBuffer[localoffset] & 0x7F) << 1);
      muxRate1 = ( (muxRate1 << 8)|(m_pDataBuffer[localoffset+1]) );
      localoffset+=2;
      uint8 muxRate2 = (m_pDataBuffer[localoffset] & 0xFE) >> 1;
      m_currPackHeader.program_mux_rate = make22BitValue(muxRate1,muxRate2);
      localoffset++;
    }
    else
    {
      nOffset += MPEG2_PACK_HDR_BYTES;
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

      //get top 2 bits out of 9 bit scr extension
      uint8 scrextpart1     = (m_pDataBuffer[localoffset++] & 0x03)<<6;
      //get lower 7 bits out of 9 bit scr extension
      uint8 scrextpart2     = (m_pDataBuffer[localoffset++] & 0xFE);
      m_currPackHeader.scr_extension = make9BitValue(scrextpart1,scrextpart2);

      m_currPackHeader.scr_base = (double)make33BitValue(firstpart,secondpart,thirdpart);
      m_currPackHeader.scr_base *= 300;

      m_currPackHeader.scr_val = m_currPackHeader.scr_base + m_currPackHeader.scr_extension;
      m_currPackHeader.scr_val = m_currPackHeader.scr_val /27000.0;    //27 MHz clock

#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                   "parsePackHeader scr_val %f ",m_currPackHeader.scr_val);
#endif
      //get top 16 bits from program mux rate
      uint16 prog_mux_rate_part1 = (((uint16)m_pDataBuffer[localoffset])<<8) | m_pDataBuffer[localoffset+1];
      localoffset += 2;
      //get ls 6 bits from program mux rate
      uint8 prog_mux_rate_part2 = (m_pDataBuffer[localoffset] & 0xFC);
      prog_mux_rate_part2 >>= 2;
      prog_mux_rate_part2 &= 0x3F;
      localoffset++;
      m_currPackHeader.program_mux_rate = ((uint32)prog_mux_rate_part1)<<6;
      m_currPackHeader.program_mux_rate |= prog_mux_rate_part2;


      uint8* stuffing_val = m_pDataBuffer+MPEG2_PACK_HDR_BYTES-1;
      m_currPackHeader.pack_stuffing_length = (*stuffing_val)&(0x07);
      nOffset += m_currPackHeader.pack_stuffing_length;
      retError = MP2STREAM_SUCCESS;
    }
  }
  return retError;
}
/*! ======================================================================
@brief  Inspects Data content from given offset to determine total number of
        streams that exists in given program stream.

@detail    Inspects Data content from given offset to determine total number of
        streams that exists in given program stream

@param[in] N/A

@return    Number of streams from given program stream.
@note      Please note that offset being passed should point after
           target system fixed header..
========================================================================== */
uint8 MP2StreamParser::getNumberOfStreamsFromTargetHeader(int bytesRemaining, uint64 nStartOffset)
{
  uint8 nStreams = 0;
  bool bContinue = true;
  while( (bytesRemaining) > 0 && (bContinue ==true))
  {
    if(!readMpeg2StreamData (nStartOffset, SYS_HDR_STREAM_ID_INFO_BYTES, m_pDataBuffer, m_nDataBufferSize,(uint32)m_pUserData) )
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"getNumberOfStreamsFromTargetHeader READ Failed!!");
      bContinue = false;
      continue;
    }
    nStartOffset += SYS_HDR_STREAM_ID_INFO_BYTES;
    bytesRemaining -= SYS_HDR_STREAM_ID_INFO_BYTES;
    if(m_pDataBuffer[0]& 0x01)//need to check if byte needs to be 0x01 or not
    {
      nStreams++;
    }
  }
  return nStreams;
}

/*! ======================================================================
@brief  Parses program stream system target header.

@detail    Starts parsing system target header from program stream.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parseSystemTargetHeader(uint32 trackId, uint64& nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  int localoffset = 0;
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
               "parseSystemTargetHeader nOffset %llu", nOffset);

  if(!readMpeg2StreamData (nOffset, 2*(sizeof(uint8)),
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32)m_pUserData) )
  {
    retError = m_eParserState;
    bContinue = false;
  }
  uint8 tmpAudioBound = 0;
  uint8 tmpVideoBound = 0;
  uint16 Length    = ((uint16)(m_pDataBuffer[localoffset])<<8) |
                         (uint16)m_pDataBuffer[localoffset+1];

  /* If system header is already parsed, then update Offset field and do not
     parse the system header details again. It will avoid overhead processing.
  */
  if(m_currPackHeader.sys_header)
  {
    nOffset += Length +  sizeof(Length);
    return MP2STREAM_SUCCESS;
  }
  system_header* trg_sys_hdr = (system_header*)MM_Malloc(sizeof(system_header));
  if(!trg_sys_hdr)
  {
    retError = MP2STREAM_OUT_OF_MEMORY;
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
                "parseSystemTargetHeader sys_header allocation failed");
  }
  else
  {
    memset(trg_sys_hdr,0,sizeof(system_header));
    trg_sys_hdr->sys_header_start_code = MP2_SYS_HDR_START_CODE;
    trg_sys_hdr->header_length = ((uint16)(m_pDataBuffer[localoffset])<<8) |
                                  (uint16)m_pDataBuffer[localoffset+1];
    trg_sys_hdr->noffset = nOffset - sizeof(MP2_SYS_HDR_START_CODE);
    nOffset += 2*(sizeof(uint8));
#ifdef MPEG2_PARSER_DEBUG
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"parseSystemTargetHeader size %u",
                 trg_sys_hdr->header_length);
#endif

    if(!readMpeg2StreamData (nOffset, SYS_HDR_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32)m_pUserData) )
    {
      retError = m_eParserState;
      bContinue = false;
    }
    nOffset += SYS_HDR_BYTES;

    //get top 15 bits from rate bound
    uint16 rate_bound_part1 = (((uint16)m_pDataBuffer[localoffset])<<8) |
                                        m_pDataBuffer[localoffset + 1];
    localoffset += 2;
    //knock off marker bit and get the 15 bits of interest.
    rate_bound_part1 <<= 1;
    //get ls 7 bits from rate bound
    uint8 rate_bound_part2 = (m_pDataBuffer[localoffset] & 0xFE);
    localoffset++;
    trg_sys_hdr->rate_bound = make22BitValue(rate_bound_part1,rate_bound_part2);
    if(trg_sys_hdr->rate_bound && (m_nClipDuration == 0) )
    {
      //rate bound is measured in RATE_MEASURED_UNITS bytes/seconds
     long double durval = ceil( ((long double)m_nFileSize/
                         (RATE_MEASURED_UNITS * trg_sys_hdr->rate_bound) ) );
      m_nClipDuration = (uint64)(durval * MPEG2_STREAM_TIME_SCALE);
    }
    if(m_bGetLastPTS)
    {
      GetClipDurationFromPTS(trackId);

    }
    if(m_nClipDuration != m_nEndPESPktTS)
    {
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
        "(m_nClipDuration=%llu) != (m_nEndPESPktTS=%llu)",
         m_nClipDuration,m_nEndPESPktTS);
      m_nClipDuration= m_nEndPESPktTS;
    }
    readMpeg2StreamData (nOffset- SYS_HDR_BYTES, SYS_HDR_BYTES,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32)m_pUserData);

    trg_sys_hdr->audio_bound = (m_pDataBuffer[localoffset] & 0xFC)>>2;

    trg_sys_hdr->fixed_flag  = m_pDataBuffer[localoffset] & 0x02 >> 1;
    trg_sys_hdr->csps_flag   = m_pDataBuffer[localoffset++] & 0x01;

    trg_sys_hdr->sys_audio_lock_flag = (m_pDataBuffer[localoffset] & 0x80) >> 7;
    trg_sys_hdr->sys_video_lock_flag = (m_pDataBuffer[localoffset] & 0x40) >> 6;
    trg_sys_hdr->video_bound         =  m_pDataBuffer[localoffset++] & 0x1F;
    trg_sys_hdr->packet_restriction_flag = (m_pDataBuffer[localoffset++]& 0x80) >> 7;
    int bytesRemaining = trg_sys_hdr->header_length - SYS_HDR_BYTES;

    tmpAudioBound = trg_sys_hdr->audio_bound;
    tmpVideoBound = trg_sys_hdr->video_bound;

    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                 "parseSystemTargetHeader video_bound %d audio_bound %d",
                 tmpVideoBound, tmpAudioBound);

    //Check if we have already parsed the system header
    if( (m_currPackHeader.sys_header == NULL )&& (m_nstreams ==0) )
    {
      //We encountered system target header for the first time
      //allocate memory for storing stream information
      m_nstreams = trg_sys_hdr->video_bound + trg_sys_hdr->audio_bound;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                   "parseSystemTargetHeader #Total Streams %d",m_nstreams);
      if(m_nstreams)
      {
        m_pStream_Info = (stream_info*)
                         MM_Malloc(m_nstreams * sizeof(stream_info));
        if(!m_pStream_Info)
        {
          bContinue = false;
          retError = MP2STREAM_OUT_OF_MEMORY;
        }
        else
        {
          memset(m_pStream_Info,0, sizeof(stream_info)* m_nstreams);
        }
      }
    }
    int i = 0;
    while( (bytesRemaining) > 0 && (bContinue ==true))
    {
      localoffset = 0;
      if(!readMpeg2StreamData (nOffset, SYS_HDR_STREAM_ID_INFO_BYTES,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32)m_pUserData) )
      {
        retError = m_eParserState;
        bContinue = false;
        continue;
      }

      if( (m_pDataBuffer[localoffset]& 0x01)&&
          (m_currPackHeader.sys_header == NULL) )
      {
        nOffset += SYS_HDR_STREAM_ID_INFO_BYTES;
        bytesRemaining -= SYS_HDR_STREAM_ID_INFO_BYTES;
        localoffset++;
        if((m_pStream_Info)&&(m_pStream_Info+i))
        {
          m_pStream_Info[i].stream_media_type = TRACK_TYPE_UNKNOWN;
          m_pStream_Info[i].stream_id  = m_pDataBuffer[localoffset++];
          m_pStream_Info[i].buffer_bound_scale = (m_pDataBuffer[localoffset] & 0x20)>>5;
          m_pStream_Info[i].buffer_size_bound = ((uint16)(m_pDataBuffer[localoffset] & 0x1F))<<8 | m_pDataBuffer[localoffset+1];
          localoffset++;

          if(!m_pStream_Info[i].buffer_bound_scale)
          {
            m_pStream_Info[i].buffer_size =
            m_pStream_Info[i].buffer_size_bound * 128;
          }
          else
          {
            m_pStream_Info[i].buffer_size =
            m_pStream_Info[i].buffer_size_bound * 1024;
          }
        }
        localoffset++;
        i++;
      }//if(m_pDataBuffer[localoffset]& 0x01)
      else
      {
        bytesRemaining--;
        localoffset++;
        nOffset++;
      }
    }//while( (bytesRemaining) > 0 && (bContinue ==true))

    retError = MP2STREAM_SUCCESS;
    if(m_currPackHeader.sys_header == NULL)
    {
      m_currPackHeader.sys_header = (system_header*)
                                    MM_Malloc(sizeof(system_header));
      if(m_currPackHeader.sys_header)
      {
        /* Reset this field to NULL value. It will be updated using common
           function for both PS and TS streams. Audio and Video stream count
           is also updated along with total stream count.
           MP2StreamParser::updateTotalTracks will serve this purpose.
        */
        m_nstreams = 0;
        memcpy(m_currPackHeader.sys_header,trg_sys_hdr,sizeof(system_header));
      }
    }
    MM_Free(trg_sys_hdr);
  }//end of else of if(!trg_sys_hdr)

  return retError;
}

/*! ======================================================================
@brief  Parses program stream system target header.

@detail    Starts parsing system target header from program stream.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parseProgStreamMap(uint64& ullOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  uint32 ulIndex = 0;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"parseProgStreamMap nOffset %llu",
               ullOffset);
#endif

  if(!readMpeg2StreamData (ullOffset, m_currPESPkt.packet_length,m_pDataBuffer,
                           m_nDataBufferSize, (uint32)m_pUserData) )
  {
    retError  = m_eParserState;
    bContinue = false;
  }

  m_pProgramStreamMap = (ProgramStreamMap*)MM_Malloc(sizeof(ProgramStreamMap));
  if (!m_pProgramStreamMap)
  {
    retError = MP2STREAM_OUT_OF_MEMORY;
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
                "parseProgStreamMap m_pProgramStreamMap alloc failed");
  }
  else
  {
    memset(m_pProgramStreamMap, 0, sizeof(ProgramStreamMap));
    m_pProgramStreamMap->current_next_indicator = m_pDataBuffer[ulIndex] &0x80;
    m_pProgramStreamMap->version_no             = m_pDataBuffer[ulIndex++] & 0x1F;
    //Reserved byte
    ulIndex++;
    m_pProgramStreamMap->program_stream_length  =
      ((uint16)(m_pDataBuffer[ulIndex])<<8) | (uint16)m_pDataBuffer[ulIndex+1];
    ulIndex += 2;
#ifdef MPEG2_PARSER_DEBUG
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"parseProgStreamMap length %u",
                 m_pProgramStreamMap->program_stream_length);
#endif
    //Skip program stream descriptors
    ulIndex += m_pProgramStreamMap->program_stream_length;

    m_pProgramStreamMap->elementary_stream_map_len  =
      ((uint16)(m_pDataBuffer[ulIndex])<<8) | (uint16)m_pDataBuffer[ulIndex+1];
    ulIndex += 2;

    uint32 ulBytesRemaining = m_pProgramStreamMap->elementary_stream_map_len;

    while( (ulBytesRemaining) > 0 && (bContinue ==true))
    {
      uint8 stream_type = m_pDataBuffer[ulIndex++];
      ulBytesRemaining--;
      uint8 stream_id   = m_pDataBuffer[ulIndex++];
      ulBytesRemaining--;
      uint16 stream_len =
        ((uint16)(m_pDataBuffer[ulIndex])<<8) | (uint16)m_pDataBuffer[ulIndex+1];
      ulIndex += 2;
      ulBytesRemaining -= 2;
      /* Need to add support for parsing Descriptors*/
      ulBytesRemaining -= stream_len;

      /* Update track properties. */
      updateTotalTracks(stream_type, stream_id);
    }//while( (bytesRemaining) > 0 && (bContinue ==true))
    for (uint32 ulCount = 0; ulCount < m_nstreams; ulCount++)
    {
      if( ((m_pStream_Info[ulCount].stream_id & 0xB8) == 0xB8)         ||
          ((m_pStream_Info[ulCount].stream_id >= AUDIO_STREAM_ID_START) &&
           (m_pStream_Info[ulCount].stream_id <= AUDIO_STREAM_ID_END) ) )
      {
        if(!m_nAudioPIDSelected)
          m_nStreamsSelected++;
        m_nAudioPIDSelected = m_pStream_Info[ulCount].stream_id;
      }
      else if( ((m_pStream_Info[ulCount].stream_id & 0xB9) == 0xB9)    ||
               ((m_pStream_Info[ulCount].stream_id >= VIDEO_STREAM_ID_START) &&
                (m_pStream_Info[ulCount].stream_id <= VIDEO_STREAM_ID_END) ) )
      {
        if(!m_nVideoPIDSelected)
          m_nStreamsSelected++;
        m_nVideoPIDSelected = m_pStream_Info[ulCount].stream_id;
      }
    }

    retError = MP2STREAM_SUCCESS;
  }//end of else of if (!m_pProgramStreamMap)

  ullOffset += m_currPESPkt.packet_length;
  return retError;
}

