/* =======================================================================
                              TSHeaderParser.cpp
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
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/src/TSHeaderParser.cpp#84 $
========================================================================== */
#include "MP2StreamParser.h"
#include "string.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMMalloc.h"
#include "filebase.h"

#include "math.h"
/*! ======================================================================
@brief  Parses each transport stream packet.

@detail    Starts parsing transport stream packets from current byte offset.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */
MP2StreamStatus MP2StreamParser::parseTransportStreamPacket(uint32 trackId)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bContinue = true;
  m_nBytesRead = 0;
  //MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseTransportStreamPacket");

  if(!readMpeg2StreamData (m_nCurrOffset,  TS_PKT_HDR_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bContinue = false;
  }
  if(bContinue)
  {
    if(memcmp(m_pDataBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTransportStreamPacket Sync byte(0x47) not found!!");
      retError = MP2STREAM_CORRUPT_DATA;
      bContinue = false;
    }

    retError = MP2STREAM_SUCCESS;
    m_currTSPkt.sync_byte = TS_PKT_SYNC_BYTE;
    uint8 val = 0;

    getByteFromBitStream(&val,&m_pDataBuffer[1],0,1);
    m_currTSPkt.t_error_indicator = val;

    getByteFromBitStream(&val,&m_pDataBuffer[1],1,1);
    m_currTSPkt.pyld_unit_start_indicator = val;

    getByteFromBitStream(&val,&m_pDataBuffer[1],2,1);
    m_currTSPkt.transport_priority = val;

    m_currTSPkt.PID = (m_pDataBuffer[1] & 0x1F)<< 8;
    m_currTSPkt.PID |=  m_pDataBuffer[2];

    getByteFromBitStream(&val,&m_pDataBuffer[3],0,2);
    m_currTSPkt.transport_scrambling_control = val;

    getByteFromBitStream(&val,&m_pDataBuffer[3],2,2);
    m_currTSPkt.adaption_field_control = val;

    getByteFromBitStream(&val,&m_pDataBuffer[3],4,4);
    m_currTSPkt.continuity_counter = val;

    m_currTSPkt.noffset = m_nCurrOffset;

    int nProgMatchedIndex = 0;
    uint64 startOffset = m_nCurrOffset;
    m_currTSPkt.adaption_field.adaption_field_length = 0;
    if( (m_bInitialParsingPending) ||
        (m_currTSPkt.PID == m_nVideoPIDSelected) || (m_currTSPkt.PID == m_nAudioPIDSelected) ||
        (isPSI(m_currTSPkt.PID)) )
    {
      //Check if transport packet contains adaption field
      if( (m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_PRESENT_NO_PYLD)||
        (m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT) )
      {
        //Parse the adaption field
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
		     "MP2StreamParser PID %u contains adaption field",m_currTSPkt.PID);
#endif
        retError = parseAdaptationField(m_nCurrOffset + TS_PKT_HDR_BYTES);
      }
      if( (m_currTSPkt.PID == TS_PROG_ASSOCIATION_TBL_PID) &&
          (!m_bGetLastPTS) )
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
		             "MP2StreamParser encountered PAT PID %u",m_currTSPkt.PID);
#endif
        if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
        {
          m_nCurrOffset = m_nCurrOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length;
        }
        retError = parseProgAssociationTable(m_nCurrOffset + TS_PKT_HDR_BYTES);
        m_nCurrOffset += m_ProgramAssociationSect.nBytesConsumed;
      }
      else if( (m_currTSPkt.PID ==  TS_CONDITIONAL_ACCESS_TBL_PID) && (!m_bGetLastPTS) )
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
		             "MP2StreamParser encountered CAT PID %u",m_currTSPkt.PID);
#endif
        retError = parseCondAccessTable(m_nCurrOffset + TS_PKT_HDR_BYTES);
      }
      else if( (m_currTSPkt.PID == TS_DESC_TBL_PID) && (!m_bGetLastPTS) )
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
		           "MP2StreamParser encountered DESCT PID %u",m_currTSPkt.PID);
#endif
        retError = parseTSDescriptionTable(m_nCurrOffset + TS_PKT_HDR_BYTES);
      }
      else if( (isProgramMapPacket(m_currTSPkt.PID,&nProgMatchedIndex))
          && (!m_bGetLastPTS) )
      {
        if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
        {
          m_nCurrOffset = m_nCurrOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length;
        }
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "MP2StreamParser encountered PMT PID %u",m_currTSPkt.PID);
#endif
        if(m_currTSPkt.pyld_unit_start_indicator)
        {
          retError = parseProgMapTable(m_nCurrOffset + TS_PKT_HDR_BYTES,nProgMatchedIndex);
        }
        if(m_ProgMapSection)
        {
          m_nCurrOffset += m_ProgMapSection->nBytesConsumed;
        }
      }
      else if ( ( (m_currTSPkt.PID >= TS_RESERVED_PID_START) && (m_currTSPkt.PID <= TS_RESERVED_PID_END) ) ||
              ( (m_currTSPkt.PID >= TS_RESERVED_FUTURE_PID_START) && (m_currTSPkt.PID <= TS_RESERVED_FUTURE_PID_END) )
              )
      {
        //These PIDs are reserved, skip unknown
        if(m_currTSPkt.PID == TS_RESERVED_PID_START)
        {
          if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
          {
            m_nCurrOffset = m_nCurrOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length;
          }
#ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                       "MP2StreamParser encountered STREAM SECTION PID %d",
                       m_currTSPkt.PID);
#endif
          retError = parseTSStreamSection(m_nCurrOffset + TS_PKT_HDR_BYTES);
        }
      }
      else if( ((m_currTSPkt.PID > TS_GENERAL_PURPOSE_PID_START) &&
        (m_currTSPkt.PID < TS_GENERAL_PURPOSE_PID_END) ) ||
        (m_currTSPkt.pyld_unit_start_indicator == 1))
      {
        //These are the PIDs that contain programs
        //New PES packet starts when pyld_unit_start_indicator = 1
        if( (m_currTSPkt.pyld_unit_start_indicator == 1) &&
          ( (m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_ABSENT_PYLD_ONLY) || (m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT) ) )
        {
          uint32 valCode = 0;
          m_nCurrOffset += 4*sizeof(TS_PKT_HDR_BYTES);
          if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
          {
            m_nCurrOffset = m_nCurrOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length;
          }
          if(readMpeg2StreamData (m_nCurrOffset,  sizeof(uint32),
            m_pDataBuffer, m_nDataBufferSize,
            (uint32) m_pUserData) )
          {
            if( isPESPacket(m_pDataBuffer,&valCode) &&
              (retError == MP2STREAM_SUCCESS) )
            {
              //optimize to parsePES packet for the required trackID or do it by default when no trackID is mentioned during initial parsing
              if( ((isTrackIdInIdStore(trackId)) ) || (m_bInitialParsingPending))
              {
                int32 nBytesRead = 0;

                MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
                            "parseTransportStreamPacket isPESPacket TRUE");
                retError = parsePESPacket(m_nCurrOffset,valCode,trackId,NULL,0,&nBytesRead);
                m_nBytesRead = nBytesRead;
              }
            }
            else
            {
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                          "parseTransportStreamPacket isPESPacket failed %llu",
                          m_nCurrOffset);
              m_nCurrOffset -= 4*(sizeof(TS_PKT_HDR_BYTES));
              m_nCurrOffset += TS_PKT_SIZE;
            }
          }
        }
        else
        {
          //Previously started PES could be continueing here.
          if( ( m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_ABSENT_PYLD_ONLY) ||
            (m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT) )
          {
            //Check CC, if incremented by 1, this has continuous data of the same packet
            uint64 startOffset = m_nCurrOffset;
            uint64 endOffset = startOffset + 188;
            m_nCurrOffset += TS_PKT_HDR_BYTES;
            if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
            {
              m_nCurrOffset = m_nCurrOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length;
            }
            if(endOffset > m_nCurrOffset)
            {
              if(!readMpeg2StreamData (m_nCurrOffset, (uint32)(endOffset- m_nCurrOffset),
                                        m_pDataBuffer, m_nDataBufferSize,
                                        (uint32) m_pUserData) )
              {
                retError = m_eParserState;
              }
              m_nBytesRead = (uint32)(endOffset - m_nCurrOffset);
            }
          }
          //No playload here, so lets skip this TS packet
          updateOffsetToNextPacket(startOffset,m_bIsBDMVFormat,true);
        }
      }
      if(m_currTSPkt.PID == TS_NULL_PKT_PID )
      {
        //NULL packets are for padding, skip the data bytes
        updateOffsetToNextPacket(startOffset,m_bIsBDMVFormat,true);
      }
      if( (retError == MP2STREAM_DATA_UNDER_RUN) || (m_eParserState == MP2STREAM_DATA_UNDER_RUN))
      {
        m_nCurrOffset = startOffset;
        m_eParserState = MP2STREAM_READY;
      }
      else
      {
        updateOffsetToNextPacket(startOffset,m_bIsBDMVFormat,true);
      }
      if((m_bGetLastPTS) && (m_bInitialParsingPending))
      {
        updateOffsetToNextPacket(startOffset,m_bIsBDMVFormat,false);
      }
    }
    else
    {
      //Skip packet
      updateOffsetToNextPacket(startOffset,m_bIsBDMVFormat,true);
    }
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseProgAssociationTable

DESCRIPTION:
  Parses the adaption field from current MP2 transport stream.

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to adaption field

RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseProgAssociationTable(uint64 nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bOK = true;
  uint8 val = 0;
  int index = 0;
  ProgramAssociationSection tempPAT;
  uint16 bytesToRead = 0;

  memset(&tempPAT,0,sizeof(ProgramAssociationSection));

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgAssociationTable");
#endif
  if((!m_bStartOffsetSet) && (!m_bGetLastPTS))
  {
    m_nStartOffset = m_currTSPkt.noffset;
    m_bStartOffsetSet = true;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                 "parseProgAssociationTable m_nStartOffset %llu",m_nStartOffset);
  }
  if(!readMpeg2StreamData(nOffset,  TS_INIT_BYTES,
                          m_pDataBuffer, m_nDataBufferSize,
                          (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {

    if(m_currTSPkt.pyld_unit_start_indicator == 0x01 && m_pDataBuffer[index])
    {
      //First byte contains pointer field.
      tempPAT.common_sect_data.pointer_val = m_pDataBuffer[index];
      nOffset += tempPAT.common_sect_data.pointer_val;
      if(!readMpeg2StreamData(nOffset, TS_INIT_BYTES,
                      m_pDataBuffer, m_nDataBufferSize,
                      (uint32) m_pUserData) )
      {
        retError = m_eParserState;
        bOK = false;
      }
    }
    //Advance index to read the data.
    index++;
    nOffset += TS_INIT_BYTES;

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,8);
    tempPAT.common_sect_data.table_id = val;
    index++;

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,1);
    tempPAT.common_sect_data.sect_synt_indtor = val;

    if(tempPAT.common_sect_data.sect_synt_indtor != 0x01)
    {
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseProgAssociationTable sect_synt_indtor != 0x01");
      return MP2STREAM_CORRUPT_DATA;
    }

    getByteFromBitStream(&val,&m_pDataBuffer[index],1,1);
    tempPAT.common_sect_data.zero_field = val;

    getByteFromBitStream(&val,&m_pDataBuffer[index],2,2);
    tempPAT.common_sect_data.reserved = val;

    tempPAT.common_sect_data.sect_length = (m_pDataBuffer[index] & 0x0F) << 12;
    index++;

    tempPAT.common_sect_data.sect_length |= m_pDataBuffer[index];

    // m_ProgramAssociationSect.common_sect_data.sect_length is 12 bit
    if((tempPAT.common_sect_data.sect_length & 0xC00) ||
       (tempPAT.common_sect_data.sect_length > MAX_SECT_LENGTH))
    {
      //First 2 bits are not '00', stream is corrupted.
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseProgAssociationTable MP2STREAM_CORRUPT_DATA");
      return MP2STREAM_CORRUPT_DATA;
    }
    else
    {
      //Make sure sect_length is within this packet
      if( (tempPAT.common_sect_data.sect_length) <= (m_currTSPkt.noffset + TS_PKT_SIZE - nOffset) )
      {
        bytesToRead = tempPAT.common_sect_data.sect_length;
      }
      else
      {
        bytesToRead = (uint16)(m_currTSPkt.noffset + TS_PKT_SIZE - nOffset);
      }
      if(bytesToRead)
      {
        if(!readMpeg2StreamData(nOffset, bytesToRead,
                          m_pDataBuffer, m_nDataBufferSize,
                          (uint32) m_pUserData) )
        {
          retError = m_eParserState;
        }
        else
        {
          index = 0;
          //Reset number of bytes consumed before we start reading further.
          //counting number of bytes consumed in this section will help in
          //determining whether PAT is complete or not.
          tempPAT.nBytesConsumed = 0;
          tempPAT.transport_stream_id = m_pDataBuffer[index++]<<8;
          tempPAT.nBytesConsumed++;
          tempPAT.transport_stream_id |= m_pDataBuffer[index++];
          tempPAT.nBytesConsumed++;

          tempPAT.version_no = (m_pDataBuffer[index] & 0x3E)>> 1;
          tempPAT.current_next_indicator = m_pDataBuffer[index] & 0x01;
          index++;
          tempPAT.nBytesConsumed++;
          tempPAT.section_no = m_pDataBuffer[index++];
          tempPAT.nBytesConsumed++;
          tempPAT.last_sect_no = m_pDataBuffer[index++];
          tempPAT.nBytesConsumed++;
          tempPAT.isAvailable = true;
        }
      }
    }

    if(m_ProgramAssociationSect.isAvailable)
    {
      //Reset selected program pids
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"Resetting m_nVideoPIDSelected & m_nAudioPIDSelected");
      m_nStreamsSelected  = 0;
      if(m_ProgramAssociationSect.current_next_indicator)
      {
        //We are looking into current PAT
        if(tempPAT.version_no == m_ProgramAssociationSect.version_no)
        {
          if(tempPAT.section_no <= m_ProgramAssociationSect.section_no)
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"Rewriting PAT");
            //Free old buffers before alloc and copying new
            freePAT();
            memcpy(&m_ProgramAssociationSect,&tempPAT,sizeof(ProgramAssociationSection));
          }
          else
          {
            //Current PAT is continueing here
            //We will take care of reallocs while copying
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"Current PAT is continueing");
          }
        }
        else
        {
          //Found PAT with new version number, rewriting our members with new PAT
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"PAT with new version#, rewrite");
          //Free old buffers before alloc and copying new
          freePAT();
          memcpy(&m_ProgramAssociationSect,&tempPAT,sizeof(ProgramAssociationSection));
        }
      }
      else
      {
        //This is next PAT to become applicable
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"New PAT available, rewrite");
        //Free old buffers before alloc and copying new
        freePAT();
        memcpy(&m_ProgramAssociationSect,&tempPAT,sizeof(ProgramAssociationSection));
      }
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"Found PAT");
      memcpy(&m_ProgramAssociationSect,&tempPAT,sizeof(ProgramAssociationSection));
    }

    int nCount = (m_ProgramAssociationSect.common_sect_data.sect_length - SECTION_HDR_CRC_BYTES)/PROGRAM_NO_PID_BYTES;
    //We are not going to parse CRC at the end of this section,
    //so count it in bytes consumed.
    m_ProgramAssociationSect.nBytesConsumed+= CRC_BYTES;
    if(nCount > 0)
    {
      int nCurrPrograms = m_ProgramAssociationSect.nPrograms;
      if(m_ProgramAssociationSect.nPrograms == 0)
      {
        m_ProgramAssociationSect.nPrograms = nCount;

        m_ProgramAssociationSect.program_numbers = (int*)MM_Malloc(sizeof(int)* nCount);
        m_ProgramAssociationSect.ts_PID = (int*)MM_Malloc(sizeof(int)* nCount);
      }
      else
      {
        if(tempPAT.section_no > m_ProgramAssociationSect.section_no)
        {
          //program_numbers & ts_PID should be realloc. We will copy these into TEMP
          //before we do the memcpy of TEMP(latest PAT) into member so we dont lose it.
          tempPAT.program_numbers = m_ProgramAssociationSect.program_numbers;
          tempPAT.ts_PID = m_ProgramAssociationSect.ts_PID;
          //memcpy everything from latest PAT into member
          memcpy(&m_ProgramAssociationSect,&tempPAT,sizeof(ProgramAssociationSection));
          m_ProgramAssociationSect.nPrograms += nCount;

          //Now Realloc
          m_ProgramAssociationSect.program_numbers =
            (int*)MM_Realloc(m_ProgramAssociationSect.program_numbers,(sizeof(int)* m_ProgramAssociationSect.nPrograms));
          m_ProgramAssociationSect.ts_PID =
            (int*)MM_Realloc(m_ProgramAssociationSect.ts_PID,(sizeof(int)* m_ProgramAssociationSect.nPrograms));
          m_ProgMapSection =
            (ProgramMapSection*)MM_Realloc(m_ProgMapSection,(sizeof(ProgramMapSection) * m_ProgramAssociationSect.nPrograms));
        }
      }
      if( (!m_ProgramAssociationSect.program_numbers) ||
          (!m_ProgramAssociationSect.ts_PID) )
      {
        bOK = false;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                  "parseProgAssociationTable memory allocation failed, nCount %d", nCount);
        retError = MP2STREAM_OUT_OF_MEMORY;
      }
      else
      {
        for(int count = nCurrPrograms; count < m_ProgramAssociationSect.nPrograms;count++)
        {
          uint16 prog_no = m_pDataBuffer[index++] << 8;
          m_ProgramAssociationSect.nBytesConsumed++;
          prog_no |= m_pDataBuffer[index++];
          m_ProgramAssociationSect.nBytesConsumed++;
          uint16 ts_pid = (m_pDataBuffer[index++] & 0x1F);
          ts_pid = ts_pid << 8;
          m_ProgramAssociationSect.nBytesConsumed++;
          ts_pid |= m_pDataBuffer[index++];
          m_ProgramAssociationSect.nBytesConsumed++;
          m_ProgramAssociationSect.program_numbers[count] = prog_no;
          m_ProgramAssociationSect.ts_PID[count] = ts_pid;
#ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
                       "parseProgAssociationTable prog_no %d ts_pid %d",
                       prog_no, ts_pid);
#endif
        }
      }
    }

    if(bOK)
    {
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "parseProgAssociationTable sect_no %d last_sect_no %d",
                      m_ProgramAssociationSect.section_no,
                      m_ProgramAssociationSect.last_sect_no);
#endif
      if( (m_ProgramAssociationSect.nBytesConsumed == m_ProgramAssociationSect.common_sect_data.sect_length) &&
          (m_ProgramAssociationSect.section_no == m_ProgramAssociationSect.last_sect_no) )
      {
        if(m_ProgramAssociationSect.current_next_indicator == 0x01)
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "Program Association Table is complete, setting bPATComplete to TRUE");
          m_ProgramAssociationSect.bPATComplete = true;
        }
      }
      retError = MP2STREAM_SUCCESS;
    }
  }//if(bOK)
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseAdaptationField

DESCRIPTION:
  Parses the program association table from current MP2 transport stream.

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to program
                 association table.
RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseAdaptationField(uint64 nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  uint64 localOffset = 0;
  bool bOK = true;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseAdptionField");
#endif
  if(!readMpeg2StreamData (nOffset, sizeof(uint8),
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {
    nOffset += sizeof(uint8);
    uint8 val = 0;
    retError = MP2STREAM_SUCCESS;

    //Start copying data
    m_currTSPkt.adaption_field.adaption_field_length = m_pDataBuffer[localOffset];

    if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_DATA_PRSENT)
    {
      if(m_currTSPkt.adaption_field.adaption_field_length > TS_ADPT_PLYD_MAX_LEN)
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                        "adaption_field_length %d > TS_ADPT_PLYD_MAX_LEN, NON STANDARD value",
                        m_currTSPkt.adaption_field.adaption_field_length);
      }
    }
    else if(m_currTSPkt.adaption_field_control == TS_ADAPTION_FILED_PRESENT_NO_PYLD)
    {
      if(m_currTSPkt.adaption_field.adaption_field_length > TS_ADPT_NOPLYD_MAX_LEN)
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                        "adaption_field_length %d > TS_ADPT_NOPLYD_MAX_LEN, NON STANDARD value",
                        m_currTSPkt.adaption_field.adaption_field_length);
      }
    }

    localOffset = localOffset + 1;

    if(m_currTSPkt.adaption_field.adaption_field_length)
    {
      if(!readMpeg2StreamData (nOffset, m_currTSPkt.adaption_field.adaption_field_length,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
      {
        retError = m_eParserState;
      }
      else
      {
        localOffset = 0;
        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,1);
        m_currTSPkt.adaption_field.discontinuity_indicator = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],1,1);
        m_currTSPkt.adaption_field.random_access_indicator = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],2,1);
        m_currTSPkt.adaption_field.es_priority_indicator = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],3,1);
        m_currTSPkt.adaption_field.PCR_flag = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],4,1);
        m_currTSPkt.adaption_field.OPCR_flag = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],5,1);
        m_currTSPkt.adaption_field.splicing_point_flag = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],6,1);
        m_currTSPkt.adaption_field.transport_pvt_data_flag = val;

        getByteFromBitStream(&val,&m_pDataBuffer[localOffset],7,1);
        m_currTSPkt.adaption_field.adaption_field_extn_flag = val;

        localOffset = localOffset + 1;

        if(m_currTSPkt.adaption_field.PCR_flag == 1)
        {
          m_currTSPkt.adaption_field.prog_clk_ref_base = getBytesValue(sizeof(uint32),m_pDataBuffer+localOffset) << 1;
          m_currTSPkt.adaption_field.prog_clk_ref_base |= ((m_pDataBuffer[localOffset+4] & 0x01)<< 8);

          //Reserved
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],34,6);

          localOffset += 4;

          uint8 pcrextpart1     = (m_pDataBuffer[localOffset++] & 0x03)<<6;
          //get lower 7 bits out of 9 bit pcr extension
          uint8 pcrextpart2     = (m_pDataBuffer[localOffset++] & 0xFE);
          m_currTSPkt.adaption_field.prog_clk_ref_extn = make9BitValue(pcrextpart1,pcrextpart2);
          if(!m_bRefPCRSet)
          {
            m_nRefPCR = (double)( ( (m_currTSPkt.adaption_field.prog_clk_ref_base * 300) +
              m_currTSPkt.adaption_field.prog_clk_ref_extn) / 27000);
            m_bRefPCRSet = true;
          }
          localOffset = localOffset + 2;
        }
        MM_MSG_PRIO5(MM_FILE_OPS, MM_PRIO_MEDIUM,
          "pid %lu, disc flag %d, cont counter %d, pcr base %llu, pcr extn %lu",
          (uint32)m_currTSPkt.PID,
          m_currTSPkt.adaption_field.discontinuity_indicator, m_currTSPkt.continuity_counter,
          m_currTSPkt.adaption_field.prog_clk_ref_base,
          (uint32)m_currTSPkt.adaption_field.prog_clk_ref_extn);
        if(m_currTSPkt.adaption_field.OPCR_flag == 1)
        {
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,33);
          m_currTSPkt.adaption_field.orig_prog_clk_ref_base = val;
          //Reserved
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],34,6);
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],39,9);
          m_currTSPkt.adaption_field.orig_prog_clk_ref_extn = val;
          localOffset = localOffset + 6;
        }
        if(m_currTSPkt.adaption_field.splicing_point_flag == 1)
        {
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,8);
          m_currTSPkt.adaption_field.splice_countdown = val;
          localOffset = localOffset + 1;
        }
        if(m_currTSPkt.adaption_field.transport_pvt_data_flag == 1)
        {
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,8);
          m_currTSPkt.adaption_field.transport_pvt_Data_length = val;
          localOffset = localOffset + 1;
          if(m_currTSPkt.adaption_field.transport_pvt_Data_length)
          {
            //Add to localOffset
          }
        }
        if(m_currTSPkt.adaption_field.adaption_field_extn_flag == 1)
        {
          //Todo - check again
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,8);
          m_currTSPkt.adaption_field.adaption_field_extn_length = val;
          localOffset = localOffset + 1;
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,1);
          m_currTSPkt.adaption_field.ltw_flag = val;
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],1,1);
          m_currTSPkt.adaption_field.piecewise_rate_flag = val;
          getByteFromBitStream(&val,&m_pDataBuffer[localOffset],2,1);
          m_currTSPkt.adaption_field.seamless_splice_flag = val;
          // 5 bits reserved
          localOffset = localOffset + 1;
          if(m_currTSPkt.adaption_field.ltw_flag)
          {
            getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,1);
            m_currTSPkt.adaption_field.ltw_valid_flag = val;
            getByteFromBitStream(&val,&m_pDataBuffer[localOffset],1,15);
            m_currTSPkt.adaption_field.ltw_offset = val;
            localOffset = localOffset + 2;
          }
          if(m_currTSPkt.adaption_field.piecewise_rate_flag)
          {
            //Reserved
            getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,2);
            getByteFromBitStream(&val,&m_pDataBuffer[localOffset],2,22);
            m_currTSPkt.adaption_field.piecewise_rate = val;
            localOffset = localOffset + 3;
          }
          if(m_currTSPkt.adaption_field.seamless_splice_flag)
          {
            //getByteFromBitStream(&val,&m_pDataBuffer[localOffset],0,2);
            //getByteFromBitStream(&val,&m_pDataBuffer[localOffset],2,22);
            //m_currTSPkt.adaption_field.piecewise_rate = val;
            localOffset = localOffset + 5;
          }
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseAdaptationField adaption_field_extn_flag is 0");
        }
      }
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseAdaptationField adaption_field_length is 0");
    }
    if(localOffset == (nOffset + 1 + m_currTSPkt.adaption_field.adaption_field_length ))
    {
      retError = MP2STREAM_SUCCESS;
    }
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseCondAccessTable

DESCRIPTION:
  Parses the program conditional table from current MP2 transport stream.

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to program
                 conditional table table.
RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseCondAccessTable(uint64 nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bOK = true;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseCondAccessTable");
#endif
  if(!readMpeg2StreamData (nOffset,  TS_INIT_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {
    nOffset += TS_INIT_BYTES;
    uint8 val = 0;
    int index = 0;
    if(m_currTSPkt.pyld_unit_start_indicator == 0x01)
    {
      //At least one section begings in this program conditional section
      //and first byte contains pointer field. Advance index to read the data from
      //since first byte is a pointer field.
      index++;
      m_CondAccessSection.common_sect_data.pointer_val = m_pDataBuffer[0];
      index += m_CondAccessSection.common_sect_data.pointer_val;
    }
    getByteFromBitStream(&val,&m_pDataBuffer[index],0,8);
    m_CondAccessSection.common_sect_data.table_id = val;
    index++;

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,1);
    m_CondAccessSection.common_sect_data.sect_synt_indtor = val;

    if(m_CondAccessSection.common_sect_data.sect_synt_indtor != 0x01)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseCondAccessTable sect_synt_indtor != 0x01");
      //Stream is corrupted
      bOK = false;
      retError = MP2STREAM_CORRUPT_DATA;
    }

    getByteFromBitStream(&val,&m_pDataBuffer[index],1,1);
    m_CondAccessSection.common_sect_data.zero_field = val;

    getByteFromBitStream(&val,&m_pDataBuffer[index],2,2);
    m_CondAccessSection.common_sect_data.reserved = val;

    m_CondAccessSection.common_sect_data.sect_length = (m_pDataBuffer[index] & 0x0F) << 12;
    index++;
    // m_CondAccessSection.common_sect_data.sect_length is 12 bit
    if(m_CondAccessSection.common_sect_data.sect_length & 0xC00)
    {
      //First 2 bits are not '00', stream is corrupted.
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseCondAccessTable (sect_length & 0xC00) > 0x1");
      retError = MP2STREAM_CORRUPT_DATA;
    }
    else
    {
      m_CondAccessSection.common_sect_data.sect_length |= m_pDataBuffer[index];
      if(m_CondAccessSection.common_sect_data.sect_length > MAX_SECT_LENGTH)
      {
        //Stream is corrupted
        bOK = false;
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_FATAL,
                        "parseCondAccessTable sect_length %d > MAX_SECT_LENGTH %d",
                        m_CondAccessSection.common_sect_data.sect_length,MAX_SECT_LENGTH);
        retError = MP2STREAM_CORRUPT_DATA;
      }
      else
      {
        if(!readMpeg2StreamData (nOffset, m_CondAccessSection.common_sect_data.sect_length,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
        {
          retError = m_eParserState;
        }
        else
        {
          index = 0;
          m_CondAccessSection.version_no = (m_pDataBuffer[index] & 0x3E)>>1;
          m_CondAccessSection.current_next_indicator = m_pDataBuffer[index++] & 0x01;

          m_CondAccessSection.section_no = m_pDataBuffer[index++];
          m_CondAccessSection.last_sect_no = m_pDataBuffer[index++];
          int nDescriptorBytes = m_CondAccessSection.common_sect_data.sect_length -
                                 SECTION_HDR_CRC_BYTES;
          uint32 nDescriptors = parseProgDescriptors(nOffset+index,nDescriptorBytes);
          if(nDescriptors)
          {
            m_CondAccessSection.DescriptorsData =
              MM_New_Array(CADescriptor,(sizeof(CADescriptor) * nDescriptors));

            if(m_CondAccessSection.DescriptorsData)
            {
              parseProgDescriptors(nOffset+index,nDescriptorBytes,m_CondAccessSection.DescriptorsData);
            }
          }
        }
      }
    }
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseTSDescriptionTable

DESCRIPTION:
  Parses the ts description table

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to description table

RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseTSDescriptionTable(uint64 nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bOK = true;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseTSDescriptionTable");
#endif
  if(!readMpeg2StreamData (nOffset,  TS_INIT_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {
    nOffset += TS_INIT_BYTES;
    uint8 val = 0;
    int index = 0;

    if(m_currTSPkt.pyld_unit_start_indicator == 0x01)
    {
      //At least one section begings in this section
      //and first byte contains pointer field. Advance index to read the data from
      //since first byte is a pointer field.
      index++;
      m_DescSection.common_sect_data.pointer_val = m_pDataBuffer[0];
      index += m_DescSection.common_sect_data.pointer_val;
    }
    getByteFromBitStream(&val,&m_pDataBuffer[index],0,8);
    m_DescSection.common_sect_data.table_id = val;
    index++;

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,1);
    m_DescSection.common_sect_data.sect_synt_indtor = val;

    if(m_DescSection.common_sect_data.sect_synt_indtor != 0x01)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTSDescriptionTable sect_synt_indtor != 0x01");
      //Stream is corrupted
      bOK = false;
      retError = MP2STREAM_CORRUPT_DATA;
    }

    getByteFromBitStream(&val,&m_pDataBuffer[index],1,1);
    m_DescSection.common_sect_data.zero_field = val;

    getByteFromBitStream(&val,&m_pDataBuffer[index],2,2);
    m_DescSection.common_sect_data.reserved = val;

    m_DescSection.common_sect_data.sect_length = (m_pDataBuffer[index] & 0x0F) << 12;
    index++;
    // m_DescSection.common_sect_data.sect_length is 12 bit
    if(m_DescSection.common_sect_data.sect_length & 0xC00)
    {
      //First 2 bits are not '00', stream is corrupted.
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTSDescriptionTable (sect_length & 0xC00) > 0x1");
      retError = MP2STREAM_CORRUPT_DATA;
    }
    else
    {
      m_DescSection.common_sect_data.sect_length |= m_pDataBuffer[index];
      index++;
      if(m_DescSection.common_sect_data.sect_length > MAX_SECT_LENGTH)
      {
        //Stream is corrupted
        bOK = false;
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_FATAL,
                        "parseTSDescriptionTable sect_length %d > MAX_SECT_LENGTH %d",
                        m_DescSection.common_sect_data.sect_length,MAX_SECT_LENGTH);
        retError = MP2STREAM_CORRUPT_DATA;
      }
      else
      {
        if(!readMpeg2StreamData (nOffset,  m_DescSection.common_sect_data.sect_length,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
        {
          retError = m_eParserState;
        }
        else
        {
          index = 0;
          getByteFromBitStream(&val,&m_pDataBuffer[index],0,18);
          // Val is 8 bit variable
          if(val != 0x3F)
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTSDescriptionTable val != 0x3FFFF");
            //Stream is corrupted
            bOK = false;
            retError = MP2STREAM_CORRUPT_DATA;
          }
          m_DescSection.version_no = (m_pDataBuffer[index] & 0x3E)>>1;
          m_DescSection.current_next_indicator = m_pDataBuffer[index++] & 0x01;

          m_DescSection.section_no = m_pDataBuffer[index++];
          m_DescSection.last_sect_no = m_pDataBuffer[index++];
          int nDescriptorBytes = m_DescSection.common_sect_data.sect_length -
                                 SECTION_HDR_CRC_BYTES;
          uint32 nDescriptors = parseProgDescriptors(nOffset+index,nDescriptorBytes);
          if(nDescriptors)
          {
            m_DescSection.DescriptorsData =
              MM_New_Array(CADescriptor,(sizeof(CADescriptor) * nDescriptors));

            if(m_DescSection.DescriptorsData)
            {
              parseProgDescriptors(nOffset+index,nDescriptorBytes,m_DescSection.DescriptorsData);
            }
          }
        }
      }
    }
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseTSStreamSection

DESCRIPTION:
  Parses the ts description table

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to description table

RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseTSStreamSection(uint64 nOffset)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bOK = true;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseTSStreamSection");
#endif
  if(!readMpeg2StreamData (nOffset,  TS_INIT_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {
    nOffset += TS_INIT_BYTES;
    uint8 val = 0;
    int index = 0;

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,8);
    m_TSStreamSection.table_id = val;
    index++;

    if(m_TSStreamSection.table_id != 0x03)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTSStreamSection table id != 0x03");
      //Stream is corrupted
      bOK = false;
      retError = MP2STREAM_CORRUPT_DATA;
    }

    m_TSStreamSection.sect_length = (m_pDataBuffer[index] & 0x0F) << 12;
    index++;
    // m_TSStreamSection.sect_length is 12 bit
    if(m_TSStreamSection.sect_length & 0xC00)
    {
      //First 2 bits are not '00', stream is corrupted.
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseTSStreamSection (sect_length & 0xC00) > 0x1");
      retError = MP2STREAM_CORRUPT_DATA;
    }
    else
    {
      m_TSStreamSection.sect_length |= m_pDataBuffer[index];
      index++;
      if(m_TSStreamSection.sect_length > MAX_SECT_LENGTH)
      {
        //Stream is corrupted
        bOK = false;
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_FATAL,
                        "parseTSStreamSection sect_length %d > MAX_SECT_LENGTH %d",
                        m_TSStreamSection.sect_length,MAX_SECT_LENGTH);
        retError = MP2STREAM_CORRUPT_DATA;
      }
      else
      {
        if(!readMpeg2StreamData (nOffset, m_TSStreamSection.sect_length,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
        {
          retError = m_eParserState;
        }
        else
        {
          index = 0;
          m_TSStreamSection.version_no = (m_pDataBuffer[index] & 0x3E)>>1;
          m_TSStreamSection.current_next_indicator = m_pDataBuffer[index++] & 0x01;
          m_TSStreamSection.section_no = m_pDataBuffer[index++];
          m_TSStreamSection.last_sect_no = m_pDataBuffer[index++];
        }
      }
    }
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseProgMapTable

DESCRIPTION:
  Parses the program map table from current MP2 transport stream.

INPUT/OUTPUT PARAMETERS:
  nOffset[in] : Offset in transport stream which points to program
                 map table.
RETURN VALUE:
  MP2STREAM_SUCCESS if parsing is successful else returns appropriate error.

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseProgMapTable(uint64 nOffset,int nArrayIndex)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  bool bOK = true;
  bool newPMTFound = false;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable");
#endif

  if(!m_ProgMapSection)
  {
    m_ProgMapSection = (ProgramMapSection*)MM_Malloc(sizeof(ProgramMapSection) * m_ProgramAssociationSect.nPrograms);
    if(m_ProgMapSection)
    {
      memset(m_ProgMapSection,0,sizeof(ProgramMapSection) * m_ProgramAssociationSect.nPrograms);
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
          "parseProgMapTable m_ProgMapSection malloc failed!");
      retError = MP2STREAM_OUT_OF_MEMORY;
      return retError;
    }
  }
  ProgramMapSection* pCurrProgMapSection;
  if(m_ProgMapSection && (m_ProgMapSection->bProgParseComplete))
  {
    pCurrProgMapSection = (ProgramMapSection*)MM_Malloc(sizeof(ProgramMapSection)* m_ProgramAssociationSect.nPrograms);
    if(pCurrProgMapSection)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable found new PMT");
      memset(pCurrProgMapSection,0,sizeof(ProgramMapSection));
      newPMTFound = true;
      m_nVideoPIDSelected = 0;
      m_nAudioPIDSelected = 0;
      m_nStreamsSelected  = 0;
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
          "parseProgMapTable pCurrProgMapSection malloc failed!");
      retError = MP2STREAM_OUT_OF_MEMORY;
      return retError;
    }
  }
  else
  {
    pCurrProgMapSection = m_ProgMapSection;
  }

  if(!readMpeg2StreamData (nOffset,  TS_INIT_BYTES,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
  {
    retError = m_eParserState;
    bOK = false;
  }
  if(bOK)
  {
    uint8 val = 0;
    int index = 0;

    if(pCurrProgMapSection)
    {
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable PROG# %d",
                   pCurrProgMapSection->program_number);
#endif
    }
    else
    {
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"pCurrProgMapSection is NULL");
#endif
      retError = MP2STREAM_DEFAULT_ERROR;
      return retError;
    }

    if(m_currTSPkt.pyld_unit_start_indicator == 0x01 && m_pDataBuffer[0])
    {
      //At least one section begings in this program association section
      //and first byte contains pointer field. Advance index to read the data from
      //since first byte is a pointer field.
      pCurrProgMapSection->common_sect_data.pointer_val = m_pDataBuffer[0];
      nOffset += pCurrProgMapSection->common_sect_data.pointer_val;
      if(!readMpeg2StreamData(nOffset, TS_INIT_BYTES,
                              m_pDataBuffer, m_nDataBufferSize,
                              (uint32) m_pUserData) )
      {
        retError = m_eParserState;
        bOK = false;
      }
    }
    index++;
    nOffset += TS_INIT_BYTES;
    getByteFromBitStream(&val,&m_pDataBuffer[index],0,8);
    pCurrProgMapSection->common_sect_data.table_id = val;
    index++;
    if(TS_PSI_PM_TABLE_ID != pCurrProgMapSection->common_sect_data.table_id)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseProgMapTable table id != TS_PSI_PM_TABLE_ID");
      //Stream is corrupted
      bOK = false;
      retError = MP2STREAM_CORRUPT_DATA;
    }

    getByteFromBitStream(&val,&m_pDataBuffer[index],0,1);
    pCurrProgMapSection->common_sect_data.sect_synt_indtor = val;

    if(pCurrProgMapSection->common_sect_data.sect_synt_indtor != 0x01)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseProgMapTable sect_synt_indtor != 0x01");
      //Stream is corrupted
      bOK = false;
      retError = MP2STREAM_CORRUPT_DATA;
    }

    getByteFromBitStream(&val,&m_pDataBuffer[index],1,1);
    pCurrProgMapSection->common_sect_data.zero_field = val;

    getByteFromBitStream(&val,&m_pDataBuffer[index],2,2);
    pCurrProgMapSection->common_sect_data.reserved = val;

    pCurrProgMapSection->common_sect_data.sect_length = (m_pDataBuffer[index] & 0x0F) << 12;
    index++;
    // pCurrProgMapSection->common_sect_data.sect_length is 12 bit
    if(pCurrProgMapSection->common_sect_data.sect_length & 0xC00)
    {
      //First 2 bits are not '00', stream is corrupted.
      bOK = false;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"parseProgMapTable (sect_length & 0xC00) > 0x1");
      retError = MP2STREAM_CORRUPT_DATA;
    }
    else
    {
      pCurrProgMapSection->common_sect_data.sect_length |= m_pDataBuffer[index];
      index++;
      if(pCurrProgMapSection->common_sect_data.sect_length > MAX_SECT_LENGTH)
      {
        //Stream is corrupted
        bOK = false;
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_FATAL,
                        "parseProgMapTable sect_length %d > MAX_SECT_LENGTH %d",
                        pCurrProgMapSection->common_sect_data.sect_length,MAX_SECT_LENGTH);
        retError = MP2STREAM_CORRUPT_DATA;
      }
      else
      {
        if(!readMpeg2StreamData (nOffset, pCurrProgMapSection->common_sect_data.sect_length,
                           m_pDataBuffer, m_nDataBufferSize,
                           (uint32) m_pUserData) )
        {
          retError = m_eParserState;
        }
        else
        {
          index = 0;
          //Reset count of bytes consumed before we start
          //counting as we parse remaining section.
          pCurrProgMapSection->nBytesConsumed = 0;

          pCurrProgMapSection->program_number = m_pDataBuffer[index++];
          pCurrProgMapSection->program_number <<= 8;
          pCurrProgMapSection->program_number |= m_pDataBuffer[index++];
          if(!m_nProgNumSelected)
          m_nProgNumSelected = pCurrProgMapSection->program_number;

          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                          "parseProgMapTable program_number %d",pCurrProgMapSection->program_number);
          pCurrProgMapSection->nBytesConsumed += 2;

          pCurrProgMapSection->version_no = (m_pDataBuffer[index]& 0x3E)>> 1;

          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,
                          "parseProgMapTable version_number %d",pCurrProgMapSection->version_no);

          pCurrProgMapSection->current_next_indicator = (m_pDataBuffer[index++]& 0x01);
          pCurrProgMapSection->nBytesConsumed++;

          pCurrProgMapSection->section_no = m_pDataBuffer[index++];
          pCurrProgMapSection->last_sect_no = m_pDataBuffer[index++];
          pCurrProgMapSection->nBytesConsumed+= 2;

          pCurrProgMapSection->PCR_PID = (m_pDataBuffer[index++] & 0x1F);
          pCurrProgMapSection->PCR_PID <<= 8;
          pCurrProgMapSection->PCR_PID |= m_pDataBuffer[index++];
          pCurrProgMapSection->nBytesConsumed+=2;
    #ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable PCR PID %d",pCurrProgMapSection->PCR_PID);
    #endif
          pCurrProgMapSection->program_info_length = m_pDataBuffer[index++]& 0x0F;
          pCurrProgMapSection->program_info_length <<= 8;
          pCurrProgMapSection->program_info_length |= m_pDataBuffer[index++];
          pCurrProgMapSection->nBytesConsumed+=2;

          //We won't be parsing CRC at the end of this section,
          //so just count number of bytes for CRC
          pCurrProgMapSection->nBytesConsumed+=CRC_BYTES;

          int nProgramDescriptors = parseProgDescriptors(nOffset+index,
                                                         pCurrProgMapSection->program_info_length);
    #ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable program_info_length %d",pCurrProgMapSection->program_info_length);
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable nProgramDescriptors %d",nProgramDescriptors);
    #endif

          if(nProgramDescriptors)
          {
            int nCurrProgDesc = pCurrProgMapSection->nProgDesc;

            if(pCurrProgMapSection->nProgDesc == 0)
            {
              pCurrProgMapSection->nProgDesc = nProgramDescriptors;
              pCurrProgMapSection-> programDescriptorsData =
                (CADescriptor*)MM_Malloc(sizeof(CADescriptor) * nProgramDescriptors);

              if(pCurrProgMapSection-> programDescriptorsData)
              {
                memset(pCurrProgMapSection-> programDescriptorsData,
                       0,sizeof(CADescriptor) * nProgramDescriptors);
              }
            }
            else
            {
              pCurrProgMapSection->nProgDesc += nProgramDescriptors;
              pCurrProgMapSection-> programDescriptorsData =
              (CADescriptor*)MM_Realloc(pCurrProgMapSection-> programDescriptorsData,
                                     (sizeof(CADescriptor) * pCurrProgMapSection->nProgDesc) );
              if(pCurrProgMapSection-> programDescriptorsData)
              {
                memset(pCurrProgMapSection-> programDescriptorsData+nCurrProgDesc,
                       0,sizeof(CADescriptor) * nProgramDescriptors);
              }
            }
            if(pCurrProgMapSection-> programDescriptorsData)
            {
              (void)parseProgDescriptors(nOffset+index,
                                         pCurrProgMapSection->program_info_length,
                                         pCurrProgMapSection-> programDescriptorsData+nCurrProgDesc);
            }
            pCurrProgMapSection->nBytesConsumed += pCurrProgMapSection->program_info_length;
          }
          int nESDescBytes = pCurrProgMapSection->common_sect_data.sect_length -
                             PROG_MAP_SECT_HDR_BYTES -
                             pCurrProgMapSection->program_info_length - CRC_BYTES;

          int nProgESDescriptors = parseProgESDescriptors(nOffset+index+pCurrProgMapSection->program_info_length, nESDescBytes);
    #ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable nESDescBytes %d nProgESDescriptors %d",nESDescBytes,nProgESDescriptors);
    #endif

          if(nProgESDescriptors)
          {
            int nCurrESProgDesc = pCurrProgMapSection->nESDesc;
            if(pCurrProgMapSection->nESDesc == 0)
            {
              pCurrProgMapSection-> ESDescData =
                (ESDescriptor*)MM_Malloc(sizeof(ESDescriptor) * nProgESDescriptors);

              if(pCurrProgMapSection-> ESDescData)
              {
                memset(pCurrProgMapSection-> ESDescData,0,sizeof(ESDescriptor) * nProgESDescriptors);
                pCurrProgMapSection->nESDesc = nProgESDescriptors;
              }
            }
            else
            {
              pCurrProgMapSection->nESDesc += nProgESDescriptors;
              pCurrProgMapSection-> ESDescData =
              (ESDescriptor*)MM_Realloc(pCurrProgMapSection-> ESDescData,
                                     (sizeof(ESDescriptor) * pCurrProgMapSection->nESDesc) );

              if(pCurrProgMapSection-> ESDescData)
              {
                memset(pCurrProgMapSection-> ESDescData+nCurrESProgDesc,
                       0,sizeof(ESDescriptor) * nProgESDescriptors);
              }
            }
            if(pCurrProgMapSection-> ESDescData)
            {
              (void)parseProgESDescriptors(nOffset+index+pCurrProgMapSection->program_info_length,
                                           nESDescBytes,
                                           pCurrProgMapSection-> ESDescData+nCurrESProgDesc);
            }
            pCurrProgMapSection->nBytesConsumed+=nESDescBytes;
          }

          if(bOK)
          {
            if( (pCurrProgMapSection->section_no == pCurrProgMapSection->last_sect_no)&&
                (pCurrProgMapSection->common_sect_data.sect_length == pCurrProgMapSection->nBytesConsumed)
              )
            {
              pCurrProgMapSection->bProgParseComplete = true;
    #ifdef MPEG2_PARSER_DEBUG
              MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgMapTable prog# %d parsing complete",
                              pCurrProgMapSection->program_number);
    #endif
            }
            retError = MP2STREAM_SUCCESS;
            if(newPMTFound)
            {
              (void)freePMT(m_ProgMapSection);
              m_ProgMapSection = pCurrProgMapSection;
            }
          }
        }
      }
    }
  }
  if (m_ProgMapSection != pCurrProgMapSection)
  {
    // Free Memory allocated to local variable in case global
    // variable is not pointing to the same memory,
    free(pCurrProgMapSection);
    pCurrProgMapSection = NULL;
  }
  else
  {
    // We cannot free memory allocated to pCurrProgMapSection
    // as it is still being used, KW might complain.
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseProgDescriptors

DESCRIPTION:
  Returns and parses total number of descriptors found starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse
  pDescData[in/out]: Pointer to save parsed descriptors
  Note: When pDescData is NULL, function returns total number of descriptors found
        without storing any of the parsed descriptor.
RETURN VALUE:
  Total number of descriptors found

SIDE EFFECTS:
  None.
===========================================================================*/
uint32 MP2StreamParser::parseProgDescriptors(uint64 nStartOffset,
                                             const int nBytes,
                                             CADescriptor* pDescData)
{
  bool bOK = true;
  uint32 nDesc =  0;
  int nBytesConsumed = 0;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgDescriptors nBytes %d",nBytes);
#endif
  int nWriteIndex = 0;
  while(nBytesConsumed < nBytes && (bOK) && (nBytes>0))
  {
    /* 6Bytes header has to be read to count the number of descriptors or update the
       descriptor structure */
    if(!readMpeg2StreamData (nStartOffset,  TS_DESC_HEADER_LEN,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
    {
      bOK = false;
    }
    if(bOK)
    {
      int index = 0;
      uint8 desc_tag = m_pDataBuffer[index++];
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgDescriptors desc_tag %d",desc_tag);
#endif
      uint8 desc_length = m_pDataBuffer[index++];

      nStartOffset += index;
      MP2StreamStatus retStatus = MP2STREAM_DEFAULT_ERROR;
      switch(desc_tag)
      {
        case TS_REGISTRATION_DESC_TAG:
          retStatus = parseRegistrationDescriptor(nStartOffset,desc_length);
          break;
        case TS_DTS_DESC_TAG:
          retStatus = parseDTSAudioDescriptor(nStartOffset,desc_length);
          break;
        case TS_DTSHD_DESC_TAG:
          retStatus = parseDTSHDAudioDescriptor(nStartOffset,desc_length);
          break;
        case TS_AVC_DESC_TAG:
          retStatus = parseAVCDescriptor(nStartOffset,desc_length);
          break;
        case TS_CA_DESC_TAG:
          retStatus = parseCADescriptor(nStartOffset,desc_length);
          break;
        case TS_MPEG4_AUDIO_DESC_TAG:
          retStatus = parseMpeg4AudioDescriptor(nStartOffset,desc_length);
          break;
        case TS_MPEG2_AAC_AUDIO_DESC_TAG:
          retStatus = parseMp2AACAudioDescriptor(nStartOffset,desc_length);
          break;
        case TS_DVD_LPCM_AUDIO_DESC_TAG:
          retStatus = parseDVDLPCMAudioDescriptor(nStartOffset,desc_length);
          break;
        case TS_ISO_639_LANG_DESC_TAG:
          retStatus = parseLanguageDescriptor(nStartOffset,desc_length);
          break;
        case TS_AC3_AUDIO_DESC_TAG:
          retStatus = parseAC3AudioDescriptor(nStartOffset,desc_length);
          break;
#ifdef ATSC_COMPLIANCE
        case TS_EAC3_AUDIO_DESC_TAG:
          retStatus = parseEAC3AudioDescriptor(nStartOffset,desc_length);
          break;
#endif
        default:
          retStatus = MP2STREAM_SUCCESS;
          break;
      }
      if(retStatus == MP2STREAM_SUCCESS)
      {
        nBytesConsumed += (index+desc_length);
        nStartOffset += desc_length;
        nDesc++;
      }
      else
      {
        nStartOffset-= index;
        return nDesc;
      }
    }
  }
  return nDesc;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseProgESDescriptors

DESCRIPTION:
  Returns and parses total number of ES descriptors found starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse
  pESDescData[in/out]: Pointer to save parsed ES descriptors
  Note: When pESDescData is NULL, function returns total number of descriptors found
        without storing any of the parsed descriptor.
RETURN VALUE:
  Total number of descriptors found

SIDE EFFECTS:
  None.
===========================================================================*/
uint32 MP2StreamParser::parseProgESDescriptors(uint64 nStartOffset,
                                                  const int nBytes,
                                                  ESDescriptor* pESDescData)
{
  bool bOK = true;
  uint32 nDesc =  0;
  int nBytesConsumed = 0;
#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgESDescriptors nBytes %d",nBytes);
#endif
  int nWriteIndex = 0;
  while(nBytesConsumed < nBytes && (bOK) && (nBytes>0))
  {
    if(!readMpeg2StreamData (nStartOffset,  nBytes-nBytesConsumed,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
    {
      bOK = false;
    }
    if(bOK)
    {
      int index = 0;
      uint8 ucStreamType = m_pDataBuffer[index++];
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM,"parseProgESDescriptors ucStreamType %d",(int32)ucStreamType);
#endif

      uint16 elementary_pid  = m_pDataBuffer[index++] & 0x1F;
      elementary_pid <<= 8;
      elementary_pid |= m_pDataBuffer[index++];

      uint16 ES_info_length = m_pDataBuffer[index++] & 0x0F;
      ES_info_length <<= 8;
      ES_info_length |= m_pDataBuffer[index++];
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "parseProgESDescriptors elementary_pid %d ES_info_length %d",elementary_pid,ES_info_length);
#endif
      if(pESDescData)
      {
        pESDescData[nWriteIndex].stream_type = ucStreamType;
        pESDescData[nWriteIndex].elementary_pid = elementary_pid;
        MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "parseProgESDescriptors elementary_pid %d ucStreamType %d",elementary_pid,ucStreamType);

        /* zero PID is reserved for PAT and should never be assigned to m_nVideoPIDSelected */
        if((isVideoStreamType(ucStreamType)) &&
             ((!m_nVideoPIDSelected) || (m_nVideoPIDSelected > elementary_pid) ) &&
             (m_nProgNumSelected) )
        {
          if(!m_nVideoPIDSelected)
            m_nStreamsSelected++;
          m_nVideoPIDSelected = elementary_pid;

          (void)updateTotalTracks(ucStreamType, m_nVideoPIDSelected);
#ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"m_nVideoPIDSelected %d",m_nVideoPIDSelected);
#endif
        }
        else if( (isAudioStreamType(ucStreamType)) &&
                 ((!m_nAudioPIDSelected) || (m_nAudioPIDSelected > elementary_pid)) &&
                 (m_nProgNumSelected) )
        {
          if(!m_nAudioPIDSelected)
            m_nStreamsSelected++;
          m_nAudioPIDSelected = elementary_pid;

          (void)updateTotalTracks(ucStreamType, m_nAudioPIDSelected);
#ifdef MPEG2_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"m_nAudioPIDSelected %d",m_nAudioPIDSelected);
#endif
        }
        else if((ucStreamType > TS_PSI_PVT_START_ID) && (ucStreamType < TS_PSI_PVT_END_ID ))
        {
            parseRegistrationDescriptor(nStartOffset+index+2, ES_info_length);
            if(m_pRegistrationDescriptor)
            {
#ifdef ATSC_COMPLIANCE
              if (EC3_AUDIO_FORMAT_IDENTIFIER_TYPE ==
                  m_pRegistrationDescriptor->ullFormatIdentifier)
              {
                if(!m_nAudioPIDSelected)
                {
                  m_nStreamsSelected++;
                }
                m_nAudioPIDSelected = elementary_pid;
                pESDescData[nWriteIndex].stream_type =
                  EAC3_AUDIO_STREAM_TYPE;
                (void)updateTotalTracks(EAC3_AUDIO_STREAM_TYPE,
                    m_nAudioPIDSelected);
              }
#endif
              if (VC1_VIDEO_FORMAT_IDENTIFIER_TYPE ==
                  m_pRegistrationDescriptor->ullFormatIdentifier)
              {
                if(!m_nVideoPIDSelected)
                {
                  m_nStreamsSelected++;
                }
                m_nVideoPIDSelected = elementary_pid;
                (void)updateTotalTracks(VC1_VIDEO_STREAM_TYPE,
                    m_nVideoPIDSelected);
              }
              else if (LPCM_AUDIO_FORMAT_IDENTIFIER ==
                  m_pRegistrationDescriptor->ullFormatIdentifier)
              {
                if(!m_nAudioPIDSelected)
                {
                  m_nStreamsSelected++;
                }
                m_nAudioPIDSelected = elementary_pid;
                pESDescData[nWriteIndex].stream_type = LPCM_AUDIO_STREAM_TYPE;
                (void)updateTotalTracks(LPCM_AUDIO_STREAM_TYPE, m_nAudioPIDSelected);
              }
            }
        }
        pESDescData[nWriteIndex].ES_info_length = ES_info_length;
        pESDescData[nWriteIndex].nStartOffsetInFile = nStartOffset;
        int nDesc = parseProgDescriptors(nStartOffset+index,
                                         pESDescData[nWriteIndex].ES_info_length);
        if(nDesc)
        {
          int nCurrProgDesc = pESDescData[nWriteIndex].nProgDesc;
          if(pESDescData[nWriteIndex].nProgDesc == 0)
          {
            pESDescData[nWriteIndex].DescriptorData = (CADescriptor*)MM_Malloc(sizeof(CADescriptor)* nDesc);
            if(pESDescData[nWriteIndex].DescriptorData)
            {
              memset(pESDescData[nWriteIndex].DescriptorData,0,sizeof(CADescriptor)* nDesc);
            }
          }
          else
          {
            pESDescData[nWriteIndex].nProgDesc += nCurrProgDesc;
            pESDescData[nWriteIndex].DescriptorData =
            (CADescriptor*)MM_Realloc(pESDescData[nWriteIndex].DescriptorData,
                                 (sizeof(CADescriptor)* pESDescData[nWriteIndex].nProgDesc) );
            if(pESDescData[nWriteIndex].DescriptorData)
            {
              memset(pESDescData[nWriteIndex].DescriptorData+nCurrProgDesc ,
                     0,sizeof(CADescriptor)*nCurrProgDesc);
            }
          }
          if(pESDescData[nWriteIndex].DescriptorData)
          {
             (void)parseProgDescriptors(nStartOffset+index,
                                       pESDescData[nWriteIndex].ES_info_length,
                                       pESDescData[nWriteIndex].DescriptorData+nCurrProgDesc);
          }
        }
        nWriteIndex++;
      }
      nStartOffset += (5+ES_info_length);
      nDesc++;
      nBytesConsumed += (5+ES_info_length);
    }
  }
  return nDesc;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::isProgramMapPacket

DESCRIPTION:
  Returns true if given PID belongs to Program map section

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt
  index[output]: Index of the program to which PID belongs.

RETURN VALUE:
  True if PID belongs to program map section otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::isProgramMapPacket(uint16 pid,int* index)
{
  bool bRet = false;
  if(index && m_ProgramAssociationSect.bPATComplete && m_ProgramAssociationSect.ts_PID)
  {
    for (int i = 0; i < m_ProgramAssociationSect.nPrograms; i++)
    {
      //0x0 program number is for network info table.
      if( (m_ProgramAssociationSect.ts_PID[i] == pid)&&
          (m_ProgramAssociationSect.program_numbers[i] != 0x0) )
      {
        bRet = true;
        *index = i;
        break;
      }
    }
  }
  return bRet;
}
/*===========================================================================
FUNCTION:
  MakeAVCVideoConfig

DESCRIPTION:
  Returns true if given PID belongs to Program map section

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt
  index[output]: Index of the program to which PID belongs.

RETURN VALUE:
  True if PID belongs to program map section otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::makeAVCVideoConfig(uint8* pBuf,uint8* pSize)
{
  bool bRet = false;

  if(m_pAvcCodecInfo && m_pAvcCodecInfo->isValid && pSize)
  {
    *pSize = m_pAvcCodecInfo->size;

    if(pBuf)
    {
      memcpy(pBuf,m_pAvcCodecInfo->codecInfoBuf,m_pAvcCodecInfo->size);
    }
    bRet = true;
  }
  return bRet;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::GetTrackDecoderSpecificInfoContent

DESCRIPTION:
  Returns true if given PID belongs to Program map section

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt
  index[output]: Index of the program to which PID belongs.

RETURN VALUE:
  True if PID belongs to program map section otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus  MP2StreamParser::GetTrackDecoderSpecificInfoContent(uint32 id, uint8* pBuf,
                                                                      uint8* pSize)
{
  MP2StreamStatus retVal = MP2STREAM_FAIL;
  uint8 streamIndex = 0;
  track_type cType;
  media_codec_type codec_type;

  for(int i = 0; i< m_nstreams; i++)
  {
    if((m_pStream_Info) && (m_pStream_Info[i].stream_id == id))
    {
      streamIndex = i;
    }
  }

  if((GetTrackType(id,&cType,&codec_type)) == MP2STREAM_SUCCESS)
  {
    if(codec_type == AUDIO_CODEC_AAC)
    {
      if(pSize && m_pStream_Info)
      {
        uint16 samplingFreqIndex = 0;
        int tableSize = sizeof(AAC_SAMPLING_FREQUENCY_TABLE)/sizeof(unsigned long);
        for(int i = 0; i< tableSize; i++)
        {
          if(AAC_SAMPLING_FREQUENCY_TABLE[i] == m_pStream_Info[streamIndex].audio_stream_info.SamplingFrequency)
            samplingFreqIndex = i;
        }
        if(MAKE_AAC_AUDIO_CONFIG(pBuf,
                              m_pStream_Info[streamIndex].audio_stream_info.AudioObjectType,
                              samplingFreqIndex,
                              m_pStream_Info[streamIndex].audio_stream_info.NumberOfChannels,
                              pSize))
        {
          retVal = MP2STREAM_SUCCESS;
        }
      }
    }
    else if(codec_type == VIDEO_CODEC_H264)
    {
      if(makeAVCVideoConfig(pBuf,pSize))
      {
        retVal = MP2STREAM_SUCCESS;
      }
    }
  }
  return retVal;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::isPSIData

DESCRIPTION:
  Returns true if given PID has PSI data

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt

RETURN VALUE:
  True if PID has PSI data otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::isPSI(uint32 id)
{
  bool bRet = false;
  int index = 0;
  if( (id == TS_PROG_ASSOCIATION_TBL_PID) ||
      (id == TS_CONDITIONAL_ACCESS_TBL_PID) ||
      (id == TS_DESC_TBL_PID) )
  {
    bRet = true;
  }
  else if(isProgramMapPacket((uint16)id,&index))
  {
    bRet = true;
  }
  return bRet;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::freePAT

DESCRIPTION:
  Returns true if given PID has PSI data

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt

RETURN VALUE:
  True if PID has PSI data otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::freePAT(void)
{
  #ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"MP2StreamParser::freePATandPMT");
  #endif

  if(m_ProgramAssociationSect.program_numbers)
  {
    MM_Free(m_ProgramAssociationSect.program_numbers);
  }
  if(m_ProgramAssociationSect.ts_PID)
  {
    MM_Free(m_ProgramAssociationSect.ts_PID);
  }
  return true;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::freePATandPMT

DESCRIPTION:
  Returns true if given PID has PSI data

INPUT/OUTPUT PARAMETERS:
  pid[in] : PID of TS pkt

RETURN VALUE:
  True if PID has PSI data otherwise returns false;

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::freePMT(ProgramMapSection* currentProgMapPMT )
{
  #ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,"MP2StreamParser::freePMT");
  #endif

  if(currentProgMapPMT && currentProgMapPMT->ESDescData)
  {
    if(currentProgMapPMT->ESDescData->DescriptorData)
    {
      MM_Free(currentProgMapPMT->ESDescData->DescriptorData);
    }
    MM_Free(currentProgMapPMT->ESDescData);
  }
  if(currentProgMapPMT && currentProgMapPMT->programDescriptorsData)
  {
    if(currentProgMapPMT->programDescriptorsData->pvt_data_byte)
    {
      MM_Free(currentProgMapPMT->programDescriptorsData->pvt_data_byte);
    }
    MM_Free(currentProgMapPMT->programDescriptorsData);
  }
  if(currentProgMapPMT)
  {
    MM_Free(currentProgMapPMT);
  }
  currentProgMapPMT=NULL;
  return true;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::MakeAccessUnit

DESCRIPTION:
  Creates a complete Access Unit from incoming TS Packets

INPUT/OUTPUT PARAMETERS:
  trackId[in]: TrackID for which we are making access unit
  dataBuffer[in]: Buffer in which access unit is constructed
  bytesCollected[in]: Bytes so far in dataBuffer

RETURN VALUE:
  Returns MP2StreamStatus

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::MakeAccessUnit(uint32 trackId, uint8* dataBuffer, uint32 bytesCollected)
{
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;
  bool bRet = false;
  uint16 pidNeeded = 0;

  bRet = getPidForTrackId(trackId, &pidNeeded);
  if(dataBuffer && bRet)
  {
    if(m_currTSPkt.PID == pidNeeded)
    {
      //Check if we have partialFrameData before this
      if(m_pPartialFrameData && (m_pPartialFrameData->haveFrameData))
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_LOW,"MP2StreamParser::MakeAccessUnit m_pPartialFrameData");
        if(bytesCollected == 0)
        {
          m_nCurrSampleOffset = m_pPartialFrameData->dataTSPkt.noffset;
          m_nPrevCC = m_pPartialFrameData->dataTSPkt.continuity_counter;
          memcpy(dataBuffer,m_pPartialFrameData->frameBuf,m_pPartialFrameData->len);
          bytesCollected = m_pPartialFrameData->len;
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MP2StreamParser::MakeAccessUnit bytesCollected not ZERO");
        }
      }
      if( (!m_pPartialFrameData) || (!m_pPartialFrameData->haveFrameData) )
      {
        /* Check for TS continuity */
        int counterJump = getContinuityCounterJump(m_currTSPkt.continuity_counter);
        if(counterJump == 0)
        {
          if(m_currTSPkt.pyld_unit_start_indicator)
          {
            if(bytesCollected == 0)
            {
              m_nCurrSampleOffset = m_currTSPkt.noffset;
              memcpy(dataBuffer,m_pDataBuffer,m_nBytesRead);
            }
            else
            {
              memcpy(dataBuffer+bytesCollected,m_pDataBuffer,m_nBytesRead);
            }
            m_nPrevCC = m_currTSPkt.continuity_counter;
          }
          else
          {
            memcpy(dataBuffer+bytesCollected,m_pDataBuffer,m_nBytesRead);
            m_nPrevCC = m_currTSPkt.continuity_counter;
          }
        }
        else
        {
          /* There is packet loss, we can try to calculate approx bytes lost and store it.
             We will however copy data into sample buffer. */

          memcpy(dataBuffer+bytesCollected,m_pDataBuffer,m_nBytesRead);
          m_nPrevCC = m_currTSPkt.continuity_counter;

          m_nBytesLost += (uint32)(counterJump * TS_PKT_SIZE);
          MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,"Packet loss at TS packet# %llu for track %d",(m_currTSPkt.noffset/188),(int)trackId);
        }
      }
    }
  }
  return retStatus;
}
/*! ======================================================================
@brief  Scans through each transport stream packet.

@detail    Starts parsing transport stream packets from current byte offset.

@param[in] N/A

@return    MP2STREAM_SUCCESS is parsing is successful otherwise returns appropriate error.
@note      None.
========================================================================== */

MP2StreamStatus MP2StreamParser::scanTSPacketToSeek(uint64* pcr, bool* bPCRFound, bool bForward)
{
  MP2StreamStatus retError = MP2STREAM_DEFAULT_ERROR;
  uint16 ulPidFound = 0;
  uint8 ucAdaptationFieldFlag = 0;
  uint64 ullLocalOffset = 0;
  uint64 ullStartOffset = m_nCurrOffset;
  uint8 ucVal = 0;

  if(pcr && bPCRFound)
  {
    if(!readMpeg2StreamData (m_nCurrOffset,  TS_PKT_HDR_BYTES,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
    {
      retError = m_eParserState;
    }
    else
    {
      if(memcmp(m_pDataBuffer,&TS_PKT_SYNC_BYTE,sizeof(TS_PKT_SYNC_BYTE)) )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"scanTSPacketToSeek Sync byte(0x47) not found!!");
        return MP2STREAM_CORRUPT_DATA;
      }

      retError = MP2STREAM_SUCCESS;
      ulPidFound = (m_pDataBuffer[1] & 0x1F)<< 8;
      ulPidFound |=  m_pDataBuffer[2];

      getByteFromBitStream(&ucVal,&m_pDataBuffer[3],2,2);
      ucAdaptationFieldFlag = ucVal;

      if(ulPidFound == m_ProgMapSection->PCR_PID)
      {
        if( (ucAdaptationFieldFlag == TS_ADAPTION_FILED_PRESENT_NO_PYLD)||
          (ucAdaptationFieldFlag == TS_ADAPTION_FILED_DATA_PRSENT) )
        {
          m_nCurrOffset += TS_PKT_HDR_BYTES;
          if(!readMpeg2StreamData (m_nCurrOffset, sizeof(uint8),
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
          {
            retError = m_eParserState;
          }
          uint8 ucLength = m_pDataBuffer[ullLocalOffset];
          m_nCurrOffset += sizeof(uint8);

          if(ucLength)
          {
            if(!readMpeg2StreamData (m_nCurrOffset, ucLength,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
            {
              retError = m_eParserState;
            }
            getByteFromBitStream(&ucVal,&m_pDataBuffer[ullLocalOffset],3,1);
            uint8 ucPCRFlag = ucVal;
            ullLocalOffset = ullLocalOffset + 1;

            if(ucPCRFlag == 1)
            {
              uint64 ullPCRBase = getBytesValue(4,&m_pDataBuffer[ullLocalOffset]) << 1;
              ullPCRBase |= ((m_pDataBuffer[ullLocalOffset+4] & 0x01)<< 8);
              ullPCRBase = ullPCRBase * 300; /* Converting to 27MHZ clock */

              ullLocalOffset += 4;
              getByteFromBitStream(&ucVal,&m_pDataBuffer[ullLocalOffset],34,6);
              uint8 pcrextpart1 = (m_pDataBuffer[ullLocalOffset++] & 0x03)<<6;
              uint8 pcrextpart2 = (m_pDataBuffer[ullLocalOffset++] & 0xFE);
              uint64 ullPCRExtn = make9BitValue(pcrextpart1,pcrextpart2);

              uint64 ullTotalPCR = ((ullPCRBase + ullPCRExtn)/27000); /*27 MHZ clock*/
              if(ullTotalPCR > m_nRefPCR)
              {
                *pcr = ullTotalPCR - (uint64)m_nRefPCR;
                *bPCRFound = true;
              }
            }
          }
        }
      }
    }
    updateOffsetToNextPacket(ullStartOffset,m_bIsBDMVFormat,bForward);
  }
  return retError;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::isAudioStreamType

DESCRIPTION:
  Checks if streamType passed is one of the supported ones

INPUT/OUTPUT PARAMETERS:
  streamType[in] : streamType to check

RETURN VALUE:
  true if supported

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::isAudioStreamType(uint8 ucStreamType)
{
  bool bRet = false;
  switch(ucStreamType)
  {
    case MPEG1_AUDIO_STREAM_TYPE:
    case MPEG2_AUDIO_STREAM_TYPE:
    case AAC_ADTS_STREAM_TYPE:
    case PES_PVT_STREAM_TYPE:
    case AC3_AUDIO_STREAM_TYPE:
    case USER_PVT_STREAM_TYPE:
    case LPCM_AUDIO_STREAM_TYPE:
    case DTS_HD_STREAM_TYPE:
    case HDMV_DTS_STREAM_TYPE:
#ifdef ATSC_COMPLIANCE
    case EAC3_AUDIO_STREAM_TYPE:
#endif
      bRet = true;
      break;
    default:
      break;
  }
  return bRet;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::isVideoStreamType

DESCRIPTION:
  Checks if streamType passed is one of the supported ones

INPUT/OUTPUT PARAMETERS:
  streamType[in] : streamType to check

RETURN VALUE:
  true if supported

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP2StreamParser::isVideoStreamType(uint8 ucStreamType)
{
  bool bRet = false;
  switch(ucStreamType)
  {
    case VC1_VIDEO_STREAM_TYPE:
    case MPEG2_VIDEO_STREAM_TYPE:
    case AVC_VIDEO_STREAM_TYPE:
      bRet = true;
      break;
    default:
      break;
  }
  return bRet;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseCADescriptor

DESCRIPTION:
  Parses CA descriptor found starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse
  pAVCDescData[in/out]: Pointer to save parsed descriptor

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseCADescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  int nIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;
  if(!m_pCADescriptor)
  {
    m_pCADescriptor = (CADescriptor*) MM_Malloc(sizeof(CADescriptor));
    if(m_pCADescriptor)
    {
      memset(m_pCADescriptor,0,sizeof(CADescriptor));
      if(!readMpeg2StreamData (ullStartOffset,  ucBytes,
                              m_pDataBuffer, m_nDataBufferSize,
                              (uint32) m_pUserData) )
      {
        m_pCADescriptor->descriptor_tag = TS_CA_DESC_TAG;
        m_pCADescriptor->descriptor_length = ucBytes;

        uint32 ca_sys_iid = m_pDataBuffer[nIndex++];
        ca_sys_iid <<= 16;
        ca_sys_iid |= m_pDataBuffer[nIndex++];

        m_pCADescriptor->ca_system_iid = (uint16)ca_sys_iid;

        uint16 ca_pid  = m_pDataBuffer[nIndex++] & 0x1F;
        ca_pid <<= 8;
        ca_pid |= m_pDataBuffer[nIndex++];
        m_pCADescriptor->ca_pid = ca_pid;

        uint16 n_pvt_data_bytes = ucBytes - nIndex;
        m_pCADescriptor->nPvtDataBytes = n_pvt_data_bytes;
      }
      else
      {
        retStatus = m_eParserState;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseAVCDescriptor

DESCRIPTION:
  Parses AVC descriptor found starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse
  pAVCDescData[in/out]: Pointer to save parsed descriptor

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseAVCDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  int nIndex = 0;
  uint8 ucVal = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

#ifdef MPEG2_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseAVCDescriptor");
#endif

  if(m_pAVCDescriptor == NULL)
  {
    m_pAVCDescriptor = (AVCDescriptor*)MM_Malloc(sizeof(AVCDescriptor));
    if(m_pAVCDescriptor)
    {
      memset(m_pAVCDescriptor,0,sizeof(AVCDescriptor));
      if(!readMpeg2StreamData (ullStartOffset,  ucBytes,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
      {
        retStatus = m_eParserState;
      }
      else
      {
        m_pAVCDescriptor->descriptor_tag = TS_AVC_DESC_TAG;
        m_pAVCDescriptor->descriptor_length = ucBytes;

        m_pAVCDescriptor->profile_idc = m_pDataBuffer[nIndex];
        nIndex++;

        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],0,1);
        m_pAVCDescriptor->constraint_set0_flag = ucVal;

        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],1,1);
        m_pAVCDescriptor->constraint_set1_flag = ucVal;

        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],2,1);
        m_pAVCDescriptor->constraint_set2_flag = ucVal;

        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],3,5);
        m_pAVCDescriptor->AVC_compatible_flags = ucVal;

        nIndex++;
        m_pAVCDescriptor->level_idc = m_pDataBuffer[nIndex];
        nIndex++;

        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],0,1);
        m_pAVCDescriptor->AVC_still_present = ucVal;
        getByteFromBitStream(&ucVal,&m_pDataBuffer[nIndex],1,1);
        m_pAVCDescriptor->AVC_24_hour_picture_flag = ucVal;
        //Skip next 6 bits as they are reserved
        nIndex++;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseRegistrationDescriptor

DESCRIPTION:
  Parses registration descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseRegistrationDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;
  uint8 ucIndex = 0;
  if(m_pRegistrationDescriptor == NULL)
  {
    m_pRegistrationDescriptor = (RegistrationDescriptor*)MM_Malloc(sizeof(RegistrationDescriptor));
  }
    if(m_pRegistrationDescriptor)
    {
      memset(m_pRegistrationDescriptor,0,sizeof(RegistrationDescriptor));
      m_pRegistrationDescriptor->ucDescriptorTag = TS_REGISTRATION_DESC_TAG;
      m_pRegistrationDescriptor->ucDescriptorLength = ucBytes;

      if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                              m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
      {
        m_pRegistrationDescriptor->ullFormatIdentifier = getBytesValue(sizeof(uint32),m_pDataBuffer);
        ucIndex += sizeof(uint32);
        if(ucBytes > ucIndex)
        {
          if(NULL == m_pRegistrationDescriptor->pucAdditionalInfo)
          {
            m_pRegistrationDescriptor->pucAdditionalInfo = (uint8*)MM_Malloc(sizeof(uint8));
          }
          if(m_pRegistrationDescriptor->pucAdditionalInfo)
          {
            memcpy(m_pRegistrationDescriptor->pucAdditionalInfo,m_pDataBuffer+ucIndex,(ucBytes-ucIndex));
          }
          else
          {
            retStatus = MP2STREAM_OUT_OF_MEMORY;
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseRegistrationDescriptor Malloc failed!");
          }
        }
      }
      else
      {
        retStatus = m_eParserState;
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseRegistrationDescriptor read failed!");
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseRegistrationDescriptor Malloc failed!");
    }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseDTSAudioDescriptor

DESCRIPTION:
  Parses DTS audio descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseDTSAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  if(m_pDTSAudioDescriptor == NULL)
  {
    m_pDTSAudioDescriptor = (DTSAudioDescriptor*)MM_Malloc(sizeof(DTSAudioDescriptor));
    if(m_pDTSAudioDescriptor)
    {
      memset(m_pDTSAudioDescriptor,0,sizeof(DTSAudioDescriptor));
      m_pDTSAudioDescriptor->ucDescriptorTag = TS_DTS_DESC_TAG;
      m_pDTSAudioDescriptor->ucDescriptorLength = ucBytes;
      if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
      {
        m_pDTSAudioDescriptor->ucSampleRateCode = (m_pDataBuffer[ucIndex] & 0xf0) >> 4;
        m_pDTSAudioDescriptor->ucBitrateCode = (m_pDataBuffer[ucIndex] & 0x0f) | (m_pDataBuffer[ucIndex+1] & 0xc0);
        ucIndex++;
        m_pDTSAudioDescriptor->ucNBlks =  (m_pDataBuffer[ucIndex] & 0x3f) | (m_pDataBuffer[ucIndex+1] & 0x80);
        if((m_pDTSAudioDescriptor->ucNBlks < 5) || (m_pDTSAudioDescriptor->ucNBlks > 127))
        {
          retStatus = MP2STREAM_CORRUPT_DATA;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseDTSAudioDescriptor nblks not within range!!");
        }
        ucIndex++;
        m_pDTSAudioDescriptor->ulFSize = (m_pDataBuffer[ucIndex] & 0x7f) | (m_pDataBuffer[ucIndex+1] & 0xfe);
        if((m_pDTSAudioDescriptor->ulFSize < 95) || (m_pDTSAudioDescriptor->ulFSize > 8192))
        {
          retStatus = MP2STREAM_CORRUPT_DATA;
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseDTSAudioDescriptor fsize not within range!!");
        }
        ucIndex++;
        m_pDTSAudioDescriptor->ucSurroundMode = (m_pDataBuffer[ucIndex] & 0x01) | (m_pDataBuffer[ucIndex+1] & 0xf8);
        ucIndex++;
        m_pDTSAudioDescriptor->ucLFEFlag = (m_pDataBuffer[ucIndex] & 0x04);
        m_pDTSAudioDescriptor->ucExtendedSurroundFlag = (m_pDataBuffer[ucIndex] & 0x03);
        ucIndex++;
        m_pDTSAudioDescriptor->ucComponentType = m_pDataBuffer[ucIndex];
        ucIndex++;
      }
      else
      {
        retStatus = m_eParserState;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseDTSAudioDescriptor Malloc failed!");
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseDTSHDAudioDescriptor

DESCRIPTION:
  Parses DTS-HD descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseDTSHDAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  if(m_pDTSHDAudioDescriptor == NULL)
  {
    m_pDTSHDAudioDescriptor = (DTSHDAudioDescriptor*)MM_Malloc(sizeof(DTSHDAudioDescriptor));
    if(m_pDTSHDAudioDescriptor)
    {
      memset(m_pDTSHDAudioDescriptor,0,sizeof(DTSHDAudioDescriptor));
      m_pDTSHDAudioDescriptor->ucDescriptorTag = TS_DTSHD_DESC_TAG;
      m_pDTSHDAudioDescriptor->ucDescriptorLength = ucBytes;
      if(readMpeg2StreamData (ullStartOffset,  sizeof(uint16),
                             m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
      {
        m_pDTSHDAudioDescriptor->ucSubstreamCoreFlag = (m_pDataBuffer[ucIndex] & 0x80);
        m_pDTSHDAudioDescriptor->ucSubstream0Flag = (m_pDataBuffer[ucIndex] & 0x40);
        m_pDTSHDAudioDescriptor->ucSubstream1Flag = (m_pDataBuffer[ucIndex] & 0x20);
        m_pDTSHDAudioDescriptor->ucSubstream2Flag = (m_pDataBuffer[ucIndex] & 0x10);
        m_pDTSHDAudioDescriptor->ucSubstream3Flag = (m_pDataBuffer[ucIndex] & 0x08);
        ucIndex++;
        if(m_pDTSHDAudioDescriptor->ucSubstreamCoreFlag)
        {
          if(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct == NULL)
          {
            m_pDTSHDAudioDescriptor->pSubstreamCoreStruct = (DTSHDSubstreamStruct*)MM_Malloc(sizeof(DTSHDSubstreamStruct));
            if(m_pDTSHDAudioDescriptor->pSubstreamCoreStruct)
            {
              m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->ucSubstreamLength = m_pDataBuffer[ucIndex];
              ucIndex++;
              retStatus = parseDTSHDSubstreamInfo(ullStartOffset + ucIndex,m_pDTSHDAudioDescriptor->pSubstreamCoreStruct,
                                            m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->ucSubstreamLength);
              ullStartOffset += m_pDTSHDAudioDescriptor->pSubstreamCoreStruct->ucSubstreamLength;
            }
            else
            {
              return MP2STREAM_OUT_OF_MEMORY;
            }
          }
        }
        if(m_pDTSHDAudioDescriptor->ucSubstream0Flag)
        {
          if(m_pDTSHDAudioDescriptor->pSubstream0Struct == NULL)
          {
            m_pDTSHDAudioDescriptor->pSubstream0Struct = (DTSHDSubstreamStruct*)MM_Malloc(sizeof(DTSHDSubstreamStruct));
            if(m_pDTSHDAudioDescriptor->pSubstream0Struct)
            {
              m_pDTSHDAudioDescriptor->pSubstream0Struct->ucSubstreamLength = m_pDataBuffer[ucIndex];
              ucIndex++;
              retStatus = parseDTSHDSubstreamInfo(ullStartOffset + ucIndex,m_pDTSHDAudioDescriptor->pSubstream0Struct,
                                            m_pDTSHDAudioDescriptor->pSubstream0Struct->ucSubstreamLength);
              ullStartOffset += m_pDTSHDAudioDescriptor->pSubstream0Struct->ucSubstreamLength;
            }
            else
            {
              return MP2STREAM_OUT_OF_MEMORY;
            }
          }
        }
        if(m_pDTSHDAudioDescriptor->ucSubstream1Flag)
        {
          if(m_pDTSHDAudioDescriptor->pSubstream1Struct == NULL)
          {
            m_pDTSHDAudioDescriptor->pSubstream1Struct = (DTSHDSubstreamStruct*)MM_Malloc(sizeof(DTSHDSubstreamStruct));
            if(m_pDTSHDAudioDescriptor->pSubstream1Struct)
            {
              m_pDTSHDAudioDescriptor->pSubstream1Struct->ucSubstreamLength = m_pDataBuffer[ucIndex];
              ucIndex++;
              retStatus = parseDTSHDSubstreamInfo(ullStartOffset + ucIndex,m_pDTSHDAudioDescriptor->pSubstream1Struct,
                                            m_pDTSHDAudioDescriptor->pSubstream1Struct->ucSubstreamLength);
              ullStartOffset += m_pDTSHDAudioDescriptor->pSubstream1Struct->ucSubstreamLength;
            }
            else
            {
              return MP2STREAM_OUT_OF_MEMORY;
            }
          }
        }
        if(m_pDTSHDAudioDescriptor->ucSubstream2Flag)
        {
          if(m_pDTSHDAudioDescriptor->pSubstream2Struct == NULL)
          {
            m_pDTSHDAudioDescriptor->pSubstream2Struct = (DTSHDSubstreamStruct*)MM_Malloc(sizeof(DTSHDSubstreamStruct));
            if(m_pDTSHDAudioDescriptor->pSubstream2Struct)
            {
              m_pDTSHDAudioDescriptor->pSubstream2Struct->ucSubstreamLength = m_pDataBuffer[ucIndex];
              ucIndex++;
              retStatus = parseDTSHDSubstreamInfo(ullStartOffset + ucIndex,m_pDTSHDAudioDescriptor->pSubstream2Struct,
                                            m_pDTSHDAudioDescriptor->pSubstream2Struct->ucSubstreamLength);
              ullStartOffset += m_pDTSHDAudioDescriptor->pSubstream2Struct->ucSubstreamLength;
            }
            else
            {
              return MP2STREAM_OUT_OF_MEMORY;
            }
          }
        }
        if(m_pDTSHDAudioDescriptor->ucSubstream3Flag)
        {
          if(m_pDTSHDAudioDescriptor->pSubstream3Struct == NULL)
          {
            m_pDTSHDAudioDescriptor->pSubstream3Struct = (DTSHDSubstreamStruct*)MM_Malloc(sizeof(DTSHDSubstreamStruct));
            if(m_pDTSHDAudioDescriptor->pSubstream3Struct)
            {
              m_pDTSHDAudioDescriptor->pSubstream3Struct->ucSubstreamLength = m_pDataBuffer[ucIndex];
              ucIndex++;
              retStatus = parseDTSHDSubstreamInfo(ullStartOffset + ucIndex,m_pDTSHDAudioDescriptor->pSubstream3Struct,
                                            m_pDTSHDAudioDescriptor->pSubstream3Struct->ucSubstreamLength);
              ullStartOffset += m_pDTSHDAudioDescriptor->pSubstream3Struct->ucSubstreamLength;
            }
            else
            {
              return MP2STREAM_OUT_OF_MEMORY;
            }
          }
        }
      }
      else
      {
        retStatus = m_eParserState;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"parseDTSHDAudioDescriptor Malloc failed!");
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseDTSHDSubstreamInfo

DESCRIPTION:
  Parses DTSHD Substream info starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseDTSHDSubstreamInfo(uint64 ullStartOffset,
                                              DTSHDSubstreamStruct* pSubstreamStruct,
                                              uint8 ucLen)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  if(pSubstreamStruct)
  {
    if(readMpeg2StreamData (ullStartOffset,  ucLen,
                            m_pDataBuffer, m_nDataBufferSize,
                            (uint32) m_pUserData) )
    {
      pSubstreamStruct->ucNumAssets = (m_pDataBuffer[ucIndex] & 0xe0);
      pSubstreamStruct->ucChannelCount = (m_pDataBuffer[ucIndex] & 0x1f);
      ucIndex++;
      pSubstreamStruct->ucLFEFlag = (m_pDataBuffer[ucIndex] & 0x80);
      pSubstreamStruct->ucSamplingFrequency = (m_pDataBuffer[ucIndex] & 0x78);
      pSubstreamStruct->ucSampleResolution = (m_pDataBuffer[ucIndex] & 0x04);
      ucIndex++;
      if(pSubstreamStruct->ucNumAssets)
      {
        if(!pSubstreamStruct->pAssetStruct)
        {
          pSubstreamStruct->pAssetStruct = (DTSHDSubstreamAssetStruct*)
            MM_Malloc(pSubstreamStruct->ucNumAssets * sizeof(DTSHDSubstreamAssetStruct));
          if(pSubstreamStruct->pAssetStruct)
          {
            for(int i=0;i< pSubstreamStruct->ucNumAssets;i++)
            {
              pSubstreamStruct->pAssetStruct[i].ucAssetConstruction = (m_pDataBuffer[ucIndex] & 0xf8);
              pSubstreamStruct->pAssetStruct[i].ucVBRFlag = (m_pDataBuffer[ucIndex] & 0x04);
              pSubstreamStruct->pAssetStruct[i].ucPostEncodeBRScalingFlag = (m_pDataBuffer[ucIndex] & 0x02);
              pSubstreamStruct->pAssetStruct[i].ucComponentTypeFlag = (m_pDataBuffer[ucIndex] & 0x01);
              ucIndex++;
              pSubstreamStruct->pAssetStruct[i].ucLanguageCodeFlag = (m_pDataBuffer[ucIndex] & 0xf8);
              if(pSubstreamStruct->pAssetStruct[i].ucPostEncodeBRScalingFlag)
              {
                pSubstreamStruct->pAssetStruct[i].ulBitrateScaled =
                          (m_pDataBuffer[ucIndex] & 0x7f) | (m_pDataBuffer[ucIndex+1] & 0xfc);
              }
              else
              {
                pSubstreamStruct->pAssetStruct[i].ulBitrate =
                          (m_pDataBuffer[ucIndex] & 0x7f) | (m_pDataBuffer[ucIndex+1] & 0xfc);
              }
              ucIndex+=2;
              if(pSubstreamStruct->pAssetStruct[i].ucComponentTypeFlag)
              {
                pSubstreamStruct->pAssetStruct[i].ucComponentType = m_pDataBuffer[ucIndex];
                ucIndex++;
              }
              if(pSubstreamStruct->pAssetStruct[i].ucLanguageCodeFlag)
              {
                pSubstreamStruct->pAssetStruct[i].ullISO639LanguageCode = getBytesValue((3*sizeof(uint8)),m_pDataBuffer+ucIndex);
                ucIndex+= 3* sizeof(uint8);
              }
            }
          }
          else
          {
            retStatus = MP2STREAM_OUT_OF_MEMORY;
          }
        }
      }
    }
    else
    {
      retStatus = m_eParserState;
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseMpeg4AudioDescriptor

DESCRIPTION:
  Parses Mpeg4 audio descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseMpeg4AudioDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Size of descriptor as defined in the spec */
  const uint8 ucSizeDefined = 1;

  if(m_pMpeg4AudioDescriptor == NULL)
  {
    m_pMpeg4AudioDescriptor = (Mpeg4AudioDescriptor*)MM_Malloc(sizeof(Mpeg4AudioDescriptor));
    if(m_pMpeg4AudioDescriptor)
    {
      memset(m_pMpeg4AudioDescriptor,0,sizeof(Mpeg4AudioDescriptor));
      m_pMpeg4AudioDescriptor->ucDescriptorTag = TS_MPEG4_AUDIO_DESC_TAG;
      m_pMpeg4AudioDescriptor->ucDescriptorLength = ucBytes;

      /* If not equal, return corrupt data */
      if(ucSizeDefined == ucBytes)
      {
        if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          m_pMpeg4AudioDescriptor->ucMpeg4AudioProfileLevel = m_pDataBuffer[ucIndex];
          /* Fill audioInfo structure for assigned trackid(similar to parseAudioMetaData(), mark
             bParsed = true */
        }
        else
        {
          retStatus = m_eParserState;
        }
      }
      else
      {
        retStatus = MP2STREAM_CORRUPT_DATA;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseMp2AACAudioDescriptor

DESCRIPTION:
  Parses Mpeg2 AAC descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseMp2AACAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Size of descriptor as defined in the spec */
  const uint8 ucSizeDefined = 3;

  if(m_pMp2AACAudioDescriptor == NULL)
  {
    m_pMp2AACAudioDescriptor = (Mp2AACAudioDescriptor*)MM_Malloc(sizeof(Mp2AACAudioDescriptor));
    if(m_pMp2AACAudioDescriptor)
    {
      memset(m_pMp2AACAudioDescriptor,0,sizeof(Mp2AACAudioDescriptor));
      m_pMp2AACAudioDescriptor->ucDescriptorTag = TS_MPEG4_AUDIO_DESC_TAG;
      m_pMp2AACAudioDescriptor->ucDescriptorLength = ucBytes;

      /* If not equal, return corrupt data */
      if(ucSizeDefined == ucBytes)
      {
        if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          m_pMp2AACAudioDescriptor->ucMpeg2AACProfile = m_pDataBuffer[ucIndex++];
          m_pMp2AACAudioDescriptor->ucMpeg2AACChannelConfig = m_pDataBuffer[ucIndex++];
          m_pMp2AACAudioDescriptor->ucMpeg2AACAdditionalInfo = m_pDataBuffer[ucIndex++];
          for(int i = 0; i < m_nstreams; i++)
          {
            /* Update info from descriptor into track info */
            if( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
                (m_pStream_Info[i].audio_stream_info.Audio_Codec == AUDIO_CODEC_AAC) &&
                (m_pMp2AACAudioDescriptor) )
            {
              m_pStream_Info[i].audio_stream_info.AudioObjectType = m_pMp2AACAudioDescriptor->ucMpeg2AACProfile;
              m_pStream_Info[i].audio_stream_info.NumberOfChannels = m_pMp2AACAudioDescriptor->ucMpeg2AACChannelConfig;
              break;
            }
          }
        }
        else
        {
          retStatus = m_eParserState;
        }
      }
      else
      {
        retStatus = MP2STREAM_CORRUPT_DATA;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseDVDLPCMAudioDescriptor

DESCRIPTION:
  Parses DVD LPCM audio descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseDVDLPCMAudioDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Size of descriptor as defined in the spec */
  const uint8 ucSizeDefined = 2;

  if(m_pDVDLPCMAudioDescriptor == NULL)
  {
    m_pDVDLPCMAudioDescriptor = (DVDLPCMAudioDescriptor*)MM_Malloc(sizeof(DVDLPCMAudioDescriptor));
    if(m_pDVDLPCMAudioDescriptor)
    {
      memset(m_pDVDLPCMAudioDescriptor,0,sizeof(DVDLPCMAudioDescriptor));
      m_pDVDLPCMAudioDescriptor->ucDescriptorTag = TS_DVD_LPCM_AUDIO_DESC_TAG;
      m_pDVDLPCMAudioDescriptor->ucDescriptorLength = ucBytes;

      /* If not equal, return corrupt data */
      if(ucSizeDefined == ucBytes)
      {
        if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          uint8 ucSamplingFreq = (m_pDataBuffer[ucIndex] & 0xE0)>> 5;
          if(ucSamplingFreq == 1)
          {
            m_pDVDLPCMAudioDescriptor->ucSamplingFreq = (uint8)44100;
          }
          else if(ucSamplingFreq == 2)
          {
            m_pDVDLPCMAudioDescriptor->ucSamplingFreq = (uint8)48000;
          }
          uint8 ucBitsPerSample = (m_pDataBuffer[ucIndex] & 0x18) >> 3;
          if(ucBitsPerSample == 0)
          {
            m_pDVDLPCMAudioDescriptor->ucBitsPerSample = 16;
          }
          m_pDVDLPCMAudioDescriptor->ucEmphasisFlag = m_pDataBuffer[ucIndex] & 0x01;

          ucIndex++;
          uint8 ucChannels = (m_pDataBuffer[ucIndex] & 0xE0) >> 5;
          if((ucChannels == 0) || (ucChannels == 1))
          {
            m_pDVDLPCMAudioDescriptor->ucChannels = 2;
          }
          for(int i = 0; i < m_nstreams; i++)
          {
            /* Update info from descriptor into track info */
            if( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
                (m_pStream_Info[i].audio_stream_info.Audio_Codec == AUDIO_CODEC_LPCM) &&
                (m_pDVDLPCMAudioDescriptor) )
            {
              m_pStream_Info[i].audio_stream_info.SamplingFrequency = m_pDVDLPCMAudioDescriptor->ucSamplingFreq;
              m_pStream_Info[i].audio_stream_info.NumberOfChannels = m_pDVDLPCMAudioDescriptor->ucChannels;
              break;
            }
          }
        }
        else
        {
          retStatus = m_eParserState;
        }
      }
      else
      {
        retStatus = MP2STREAM_CORRUPT_DATA;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}/*===========================================================================
FUNCTION:
  MP2StreamParser::updateTotalTracks

DESCRIPTION:
  Updates default information for the streamType

INPUT/OUTPUT PARAMETERS:
  ulStreamType[in] : stream type of track

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::updateTotalTracks(uint32 ulStreamType, uint16 pid)
{
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  if(!isTrackIdInIdStore(pid))
  {
    if(isAudioStreamType((uint8)ulStreamType))
    {
      m_nstreams++;
      m_nAudioStreams++;
      if(m_pAudioStreamIds == NULL)
      {
        m_pAudioStreamIds = (uint16*)MM_Malloc(m_nAudioStreams * sizeof(uint16));
      }
      else
      {
        uint16* tmp = 0;
        tmp = (uint16*)MM_Realloc(m_pAudioStreamIds,m_nAudioStreams * sizeof(uint16));
        memset(tmp+(m_nAudioStreams-1),0,sizeof(uint16));
        m_pAudioStreamIds =  tmp;
      }
      if(m_pAudioStreamIds)
      {
        m_pAudioStreamIds[m_nAudioStreams-1] = (uint16)pid;
      }
    }
    else if(isVideoStreamType((uint8)ulStreamType))
    {
      m_nstreams++;
      m_nVideoStreams++;
      if(m_pVideoStreamIds == NULL)
      {
        m_pVideoStreamIds = (uint16*)MM_Malloc(m_nVideoStreams * sizeof(uint16));
      }
      else
      {
        uint16* tmp = 0;
        tmp = (uint16*)MM_Realloc(m_pVideoStreamIds,m_nVideoStreams * sizeof(uint16));
        memset(tmp+(m_nVideoStreams-1),0,sizeof(uint16));
        m_pVideoStreamIds =  tmp;
      }
      if(m_pVideoStreamIds)
      {
        m_pVideoStreamIds[m_nVideoStreams-1] = (uint16)pid;
      }
    }
    if((m_nstreams) && (!m_bProgramStream))
    {
      reAllocStreamInfo(m_nstreams);
    }
    retStatus = updateTrackInfoFromPSI(pid, ulStreamType);
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseLanguageDescriptor

DESCRIPTION:
  Parses ISO 639 Language descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  nStartOffset[in] : Offset to start parsing
  nBytes[in]: Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseLanguageDescriptor(uint64 ullStartOffset, uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Minimum size of descriptor as defined in the spec */
  const uint8 ucSizeDefined = 4;

  if(m_pLanguageDescriptor == NULL)
  {
    m_pLanguageDescriptor = (LanguageDescriptor*)MM_Malloc(sizeof(LanguageDescriptor));
    if(m_pLanguageDescriptor)
    {
      memset(m_pLanguageDescriptor,0,sizeof(LanguageDescriptor));
      m_pLanguageDescriptor->ucDescriptorTag = TS_ISO_639_LANG_DESC_TAG;
      m_pLanguageDescriptor->ucDescriptorLength = ucBytes;

      if(ucBytes >= ucSizeDefined)
      {
        if(readMpeg2StreamData (ullStartOffset,  ucBytes,
                               m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          uint8 ucBytesConsumed = ucBytes;
          while(ucBytesConsumed < ucBytes)
          {
            m_pLanguageDescriptor->ullISO639LanguageCode = getBytesValue(3*sizeof(uint8),m_pDataBuffer);
            ucIndex += 3*sizeof(uint8);
            m_pLanguageDescriptor->ucAudioType = m_pDataBuffer[ucIndex++];
            ucBytesConsumed += ucIndex;
          }
        }
      }
    }
  }
  return retStatus;
}
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseAC3AudioDescriptor

DESCRIPTION:
  Parses AC3 audio descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  ullStartOffset[in] : Offset to start parsing
  ucBytes[in]        : Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseAC3AudioDescriptor(uint64 ullStartOffset,
                                                         uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Minimum size of AC3 descriptor is 4bytes. */
  const uint8 ucSizeDefined = 4;

  if(NULL == m_pAC3AudioDescriptor)
  {
    m_pAC3AudioDescriptor = (AC3AudioDescriptor*)
                             MM_Malloc(sizeof(AC3AudioDescriptor));
    if(m_pAC3AudioDescriptor)
    {
      memset(m_pAC3AudioDescriptor,0,sizeof(AC3AudioDescriptor));
      m_pAC3AudioDescriptor->ucDescriptorTag = TS_AC3_AUDIO_DESC_TAG;
      m_pAC3AudioDescriptor->ucDescriptorLength = ucBytes;

      /* AC3 descriptor minimum size is 4bytes.  */
      if(ucSizeDefined <= ucBytes)
      {
        if(readMpeg2StreamData (ullStartOffset, ucBytes,
                                m_pDataBuffer, m_nDataBufferSize,
                               (uint32) m_pUserData) )
        {
          uint32 ulTotalBits    = ucBytes * 8;
          uint32 ulCurBitOffset = 0;
          m_pAC3AudioDescriptor->ucSamplingRateCode = (uint8)
            GetBitsFromBuffer(5, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 5;
          m_pAC3AudioDescriptor->ucBSID = (uint8)
            GetBitsFromBuffer(3, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 3;
          m_pAC3AudioDescriptor->ucBitRateCode = (uint8)
            GetBitsFromBuffer(6, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 6;
          m_pAC3AudioDescriptor->ucSurrondMode = (uint8)
            GetBitsFromBuffer(2, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 2;
          m_pAC3AudioDescriptor->ucBSMod = (uint8)
            GetBitsFromBuffer(3, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 3;
          m_pAC3AudioDescriptor->ucNumChannels = (uint8)
            GetBitsFromBuffer(4, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 4;
          m_pAC3AudioDescriptor->ucFullSvc = (uint8)
            GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 1;

          for(int i = 0; i < m_nstreams; i++)
          {
            /* Update info from descriptor into track info */
            if( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
                (m_pStream_Info[i].audio_stream_info.Audio_Codec ==
                 AUDIO_CODEC_AC3) )
            {
              /* As per specification, below values are recommended.
                 In case of multiple values, we use first mentioned value in
                 specifications.
                  "000" 48
                  "001" 44,1
                  "010" 32
                  "011" Reserved
                  "100" 48 or 44,1
                  "101" 48 or 32
                  "110" 44,1 or 32
                  "111" 48 or 44,1 or 32
              */
              m_pStream_Info[i].audio_stream_info.SamplingFrequency =
                AC3_SAMPLE_RATE[m_pAC3AudioDescriptor->ucSamplingRateCode];

              /* Bit rate requires 5bits only. Upper bit is used to indicate
                 whether bit rate available is Nominal or Max Bitrate.
                 However we use same value as bit rate in both cases. */
              if((m_pAC3AudioDescriptor->ucBitRateCode < 19) &&
                 (m_pAC3AudioDescriptor->ucBitRateCode >= 0))
              {
                 m_pStream_Info[i].audio_stream_info.Bitrate = 1000 *
                   AC3_BITRATE_CODE[m_pAC3AudioDescriptor->ucBitRateCode];
              }
              else if((m_pAC3AudioDescriptor->ucBitRateCode >= 32 ) &&
                      (m_pAC3AudioDescriptor->ucBitRateCode <= 50))
              {
                m_pStream_Info[i].audio_stream_info.Bitrate =
               AC3_BITRATE_CODE[m_pAC3AudioDescriptor->ucBitRateCode - 32];
              }
              else
              {
                m_pStream_Info[i].audio_stream_info.Bitrate = 0;
              }
              m_pStream_Info[i].audio_stream_info.NumberOfChannels =
                                m_pAC3AudioDescriptor->ucNumChannels;
              break;
            }
          }
        }
        else
        {
          retStatus = m_eParserState;
        }
      }
      else
      {
        retStatus = MP2STREAM_CORRUPT_DATA;
      }
    }
    else
    {
      retStatus = MP2STREAM_OUT_OF_MEMORY;
    }
  }
  return retStatus;
}
// This code is only for ATSC Standard
// Other standards may or may not use EAC3 descriptor
#ifdef ATSC_COMPLIANCE
/*===========================================================================
FUNCTION:
  MP2StreamParser::parseEAC3AudioDescriptor

DESCRIPTION:
  Parses AC3 audio descriptor starting from given offset and bytes

INPUT/OUTPUT PARAMETERS:
  ullStartOffset[in] : Offset to start parsing
  ucBytes[in]        : Total number of bytes to parse

RETURN VALUE:
  None

SIDE EFFECTS:
  None.
===========================================================================*/
MP2StreamStatus MP2StreamParser::parseEAC3AudioDescriptor(uint64 ullStartOffset,
                                                         uint8 ucBytes)
{
  uint8 ucIndex = 0;
  MP2StreamStatus retStatus = MP2STREAM_SUCCESS;

  /* Minimum size of EAC3 descriptor is 2bytes. */
  const uint8 ucSizeDefined = 2; //replace with #define

  if(NULL == m_pEAC3AudioDescriptor)
  {
    m_pEAC3AudioDescriptor = (EAC3AudioDescriptor*)
                             MM_Malloc(sizeof(EAC3AudioDescriptor));
  }
  if(m_pEAC3AudioDescriptor)
  {
    memset(m_pEAC3AudioDescriptor,0,sizeof(EAC3AudioDescriptor));
    m_pEAC3AudioDescriptor->ucDescriptorTag = TS_EAC3_AUDIO_DESC_TAG;
    m_pEAC3AudioDescriptor->ucDescriptorLength = ucBytes;

    /* EAC3 descriptor minimum size is 2bytes.  */
    if(ucSizeDefined <= ucBytes)
    {
      if(readMpeg2StreamData (ullStartOffset, ucBytes,
                              m_pDataBuffer, m_nDataBufferSize,
                             (uint32) m_pUserData) )
      {
        uint32 ulTotalBits    = ucBytes * 8;
        uint32 ulCurBitOffset = 0;
        m_pEAC3AudioDescriptor->ucReserved = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucBSIDFlag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucMainIdFlag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucAsvcFlag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucMixInfoExists = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucSubStream1Flag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucSubStream2Flag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucSubStream3Flag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucReserved = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucFullServiceFlag = (uint8)
          GetBitsFromBuffer(1, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 1;
        m_pEAC3AudioDescriptor->ucAudioServiceType = (uint8)
          GetBitsFromBuffer(3, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 3;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,
           "m_pEAC3AudioDescriptor->ucAudioServiceType=%lu",
           (uint32)m_pEAC3AudioDescriptor->ucAudioServiceType);
        m_pEAC3AudioDescriptor->ucNumOfChannels = (uint8)
          GetBitsFromBuffer(3, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
        ulCurBitOffset += 3;
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,
          "m_pEAC3AudioDescriptor->ucNumOfChannels=%lu",
           (uint32)m_pEAC3AudioDescriptor->ucNumOfChannels);
        if(m_pEAC3AudioDescriptor->ucBSIDFlag)
        {
          m_pEAC3AudioDescriptor->ucBSID = (uint8)
          GetBitsFromBuffer(5, ulCurBitOffset, m_pDataBuffer, ulTotalBits);
          ulCurBitOffset += 5;
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,
            "m_pEAC3AudioDescriptor->ucBSID=%lu",
            (uint32)m_pEAC3AudioDescriptor->ucBSID);
        }
        else
        {
          m_pEAC3AudioDescriptor->ucZeroBits = 0;
          ulCurBitOffset += 5;
        }

        for(int i = 0; i < m_nstreams; i++)
        {
          /* Update info from descriptor into track info */
          if( (m_pStream_Info[i].stream_media_type == TRACK_TYPE_AUDIO) &&
              (m_pStream_Info[i].audio_stream_info.Audio_Codec ==
               AUDIO_CODEC_EAC3) )
          {
            m_pStream_Info[i].audio_stream_info.NumberOfChannels =
                              m_pEAC3AudioDescriptor->ucNumOfChannels;
            MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_LOW,
              "m_pEAC3AudioDescriptor->ucNumOfChannels=%lu",
              (uint32)m_pEAC3AudioDescriptor->ucNumOfChannels);
            break;
          }
        }
      }
      else
      {
        retStatus = m_eParserState;
      }
    }
    else
    {
      retStatus = MP2STREAM_CORRUPT_DATA;
    }
  }
  else
  {
    retStatus = MP2STREAM_OUT_OF_MEMORY;
  }

  return retStatus;
}
#endif
